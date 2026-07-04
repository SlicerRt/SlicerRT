import os
import vtk
import slicer
from DoseEngines import AbstractScriptedDoseEngine
from Python.TopasDoseEngineUtil import TopasDoseEngineUtil
import logging

#------------------------------------------------------------------------------
#
# TopasDoseEngine
#
#------------------------------------------------------------------------------
class TopasDoseEngine(AbstractScriptedDoseEngine):
  """ Topas python dose engine
  """

  #------------------------------------------------------------------------------
  def __init__(self, scriptedEngine):
    scriptedEngine.name = 'Topas'
    AbstractScriptedDoseEngine.__init__(self, scriptedEngine)

    # Define initial defaults for parameters that are stored in application settings
    self.topasDirectoryPath = ''    # Path to TOPAS installation directory
    self.topasBinaryPath = ''       # Path to TOPAS binary executable
    self.g4dataPath = ''            # Path to Geant4 data directory
    self.rtIonPlanFilePath = ''     # Optional: Path to RT Ion Plan DICOM file for TsRTIonSource
    self.HUtoMaterialFilePath = ''  # Optional: Path to a custom HU-to-material table
    self.machineDescriptionFilePath = ''  # Optional: Path to a custom TOPAS dicom-interface PBS machine description (.table) file

    # Load paths from application settings
    self.loadPathsFromApplicationSettings()

    # Tracks which beam has already "claimed" the dose for a given (plan, RT Ion Plan file)
    # pair. An RT Ion Plan file's simulation already covers every beam inside it, so if more
    # than one Slicer beam node references the same file, only the first one to calculate
    # actually runs TOPAS; the rest get assigned zero dose, so the plan-level dose sum doesn't
    # multiply the same RT Ion Plan dose several times.
    self._rtIonPlanDoseClaimedBy = {}  # (planNodeID, rtIonPlanFilePath) -> beamNodeID

  #------------------------------------------------------------------------------
  def defineBeamParameters(self):
    """Define beam parameters that can be configured"""
    tabName = "Topas parameters"

    # Radiation mode (beam particle)
    self.scriptedEngine.addBeamParameterComboBox(
      tabName, 'radiationMode', 'Radiation mode',
      'Particle type used for the simulation',
      ['proton', 'neutron', 'gamma', 'e-', 'e+'], 0)

    # Energy parameter
    self.scriptedEngine.addBeamParameterSpinBox(
      tabName, 'energy', 'Beam energy',
      'Energy of the beam in MeV',
      1.0, 250.0, 100.0, 1.0, 1)

    # Number of histories parameter
    self.scriptedEngine.addBeamParameterSpinBox(
      tabName, 'numberOfHistories', 'Number of particles',
      'Number of particles to simulate (more particles = better statistics but slower)',
      100000, 100000000, 1000000, 500000, 0)

    # ParticlesPerHistory: downsampling factor — histories/spot = spot_MU / ParticlesPerHistory
    # Values < 1 increase histories (better statistics); values > 1 decrease them (faster)
    self.scriptedEngine.addBeamParameterSpinBox(
      tabName, 'particlesPerHistory', 'Particles per history',
      'Downsampling factor for TsRTIonSource. Values < 1 increase histories per spot for better statistics.',
      0.0001, 10000000.0, 1.0, 0.1, 4)

    self.scriptedEngine.addBeamParameterSpinBox(
      tabName, 'numberOfThreads', 'Number of threads',
      'Number of CPU threads for the TOPAS simulation',
      1, 64, os.cpu_count() or 1, 1, 0)

    # Path parameters
    self.scriptedEngine.addBeamParameterLineEdit(
      tabName, 'topasDirectory', 'TOPAS directory:',
      'Path to the TOPAS installation directory', self.topasDirectoryPath)

    self.scriptedEngine.addBeamParameterLineEdit(
      tabName, 'topasBinary', 'TOPAS binary:',
      'Path to the TOPAS executable', self.topasBinaryPath)

    self.scriptedEngine.addBeamParameterLineEdit(
      tabName, 'g4DataDirectory', 'Geant4 data directory:',
      'Path to the Geant4 data directory (G4DATAFILES)', self.g4dataPath)

    self.scriptedEngine.addBeamParameterLineEdit(
      tabName, 'rtIonPlanFile', 'RT Ion Plan file (optional):',
      'Path to a DICOM RT Ion Plan file for TsRTIonSource. Leave empty to use the simple beam source.',
      self.rtIonPlanFilePath)

    self.scriptedEngine.addBeamParameterLineEdit(
      tabName, 'HUtoMaterialFile', 'HU to material file (optional):',
      'Path to a custom HU-to-material table. Leave empty to use the bundled default '
      '(historical Schneider et al. 2000, https://doi.org/10.1088/0031-9155/45/2/314).',
      self.HUtoMaterialFilePath)

    self.scriptedEngine.addBeamParameterLineEdit(
      tabName, 'machineDescriptionFile', 'Machine description file (.table, optional):',
      'Path to a custom TOPAS dicom-interface PBS machine description (.table) file '
      '(see https://github.com/topasmc/dicom-interface/wiki/using-machine-description-file). '
      'Overrides the treatment machine read from the RT Ion Plan DICOM. Only generic PBS '
      '.table files are supported here, not compiled-in machine models. Leave empty to use '
      'the machine read from DICOM.',
      self.machineDescriptionFilePath)

  #------------------------------------------------------------------------------
  def loadPathsFromApplicationSettings(self):
    """Load TOPAS paths from application settings"""
    settings = slicer.app.userSettings()
    self.topasDirectoryPath = settings.value('TopasDoseEngine/TopasDirectory', self.topasDirectoryPath)
    self.topasBinaryPath = settings.value('TopasDoseEngine/TopasBinary', self.topasBinaryPath)
    self.g4dataPath = settings.value('TopasDoseEngine/G4DataDirectory', self.g4dataPath)
    self.rtIonPlanFilePath = settings.value('TopasDoseEngine/RTIonPlanFile', self.rtIonPlanFilePath)
    self.HUtoMaterialFilePath = settings.value('TopasDoseEngine/HUtoMaterialFile', self.HUtoMaterialFilePath)
    self.machineDescriptionFilePath = settings.value('TopasDoseEngine/MachineDescriptionFile', self.machineDescriptionFilePath)

  #------------------------------------------------------------------------------
  def savePathsInApplicationSettings(self, beamNode=None):
    """Save TOPAS paths to application settings"""
    settings = slicer.app.userSettings()
    settings.setValue('TopasDoseEngine/TopasDirectory', self.topasDirectoryPath)
    settings.setValue('TopasDoseEngine/TopasBinary', self.topasBinaryPath)
    settings.setValue('TopasDoseEngine/G4DataDirectory', self.g4dataPath)
    settings.setValue('TopasDoseEngine/RTIonPlanFile', self.rtIonPlanFilePath)
    settings.setValue('TopasDoseEngine/HUtoMaterialFile', self.HUtoMaterialFilePath)
    settings.setValue('TopasDoseEngine/MachineDescriptionFile', self.machineDescriptionFilePath)

  #------------------------------------------------------------------------------
  def resolveHUtoMaterialFile(self, planNode, beamNode):
    """Resolve the HU to material file to use for this beam.

    If the current beam doesn't have one set, fall back to any other beam in the same plan
    that does — a custom HU-to-material file selected on one beam should apply to all beams,
    not just the one it was typed into.
    """
    ownValue = self.scriptedEngine.parameter(beamNode, 'HUtoMaterialFile')
    if ownValue:
      return ownValue
    beams = vtk.vtkCollection()
    planNode.GetBeams(beams)
    for i in range(beams.GetNumberOfItems()):
      otherBeamNode = beams.GetItemAsObject(i)
      if otherBeamNode.GetID() == beamNode.GetID():
        continue
      otherValue = self.scriptedEngine.parameter(otherBeamNode, 'HUtoMaterialFile')
      if otherValue:
        return otherValue
    return ownValue

  #------------------------------------------------------------------------------
  def resolveMachineDescriptionFile(self, planNode, beamNode):
    """Resolve the machine description file to use for this beam.

    Mirrors resolveHUtoMaterialFile: a machine description file selected on one
    beam should apply to all beams in the plan, not just the one it was typed into.
    """
    ownValue = self.scriptedEngine.parameter(beamNode, 'machineDescriptionFile')
    if ownValue:
      return ownValue
    beams = vtk.vtkCollection()
    planNode.GetBeams(beams)
    for i in range(beams.GetNumberOfItems()):
      otherBeamNode = beams.GetItemAsObject(i)
      if otherBeamNode.GetID() == beamNode.GetID():
        continue
      otherValue = self.scriptedEngine.parameter(otherBeamNode, 'machineDescriptionFile')
      if otherValue:
        return otherValue
    return ownValue

  #------------------------------------------------------------------------------
  def calculateDoseUsingEngine(self, beamNode, resultDoseVolumeNode):
    """Main method to calculate dose using TOPAS.

    Uses TsRTIonSource from TOPAS dicom-interface if rtIonPlanFilePath is set,
    otherwise falls back to a simple proton beam source.
    """
    try:
      logging.info("Starting TOPAS dose calculation...")

      # Get plan node from beam
      planNode = beamNode.GetParentPlanNode()
      if not planNode:
        raise RuntimeError("No plan node found for beam")

      # Extract CT data using static utility
      logging.info("Extracting CT data...")
      ctData = TopasDoseEngineUtil.extractCTData(planNode)

      # Extract beam properties using static utility
      logging.info("Extracting beam properties...")
      beamProperties = TopasDoseEngineUtil.extractBeamProperties(beamNode)

      # Override with custom beam parameters from UI
      beamProperties['energy'] = self.scriptedEngine.doubleParameter(beamNode, 'energy')
      beamProperties['numberOfHistories'] = int(self.scriptedEngine.doubleParameter(beamNode, 'numberOfHistories'))
      beamProperties['particlesPerHistory'] = self.scriptedEngine.doubleParameter(beamNode, 'particlesPerHistory')
      beamProperties['numberOfThreads'] = int(self.scriptedEngine.doubleParameter(beamNode, 'numberOfThreads'))

      _radiationModes = ['proton', 'neutron', 'gamma', 'e-', 'e+']
      radiationModeIdx = int(self.scriptedEngine.parameter(beamNode, 'radiationMode') or 0)
      beamProperties['radiationMode'] = _radiationModes[radiationModeIdx]

      topasDirectoryPath = self.scriptedEngine.parameter(beamNode, 'topasDirectory')
      topasBinaryPath = self.scriptedEngine.parameter(beamNode, 'topasBinary')
      g4dataPath = self.scriptedEngine.parameter(beamNode, 'g4DataDirectory')
      rtIonPlanFilePath = self.scriptedEngine.parameter(beamNode, 'rtIonPlanFile')
      HUtoMaterialFilePath = self.resolveHUtoMaterialFile(planNode, beamNode)
      machineDescriptionFilePath = self.resolveMachineDescriptionFile(planNode, beamNode)

      # Persist paths to application settings so they survive session restarts
      self.topasDirectoryPath = topasDirectoryPath
      self.topasBinaryPath = topasBinaryPath
      self.g4dataPath = g4dataPath
      self.rtIonPlanFilePath = rtIonPlanFilePath
      self.HUtoMaterialFilePath = HUtoMaterialFilePath
      self.machineDescriptionFilePath = machineDescriptionFilePath
      self.savePathsInApplicationSettings()

      beamProperties['HUtoMaterialFile'] = HUtoMaterialFilePath
      beamProperties['machineDescriptionFile'] = machineDescriptionFilePath

      logging.info(f"Custom beam parameters: energy={beamProperties['energy']} MeV, "
                   f"histories={beamProperties['numberOfHistories']}")

      # Determine RT Ion Plan file path (if available)
      planFilePath = None
      if rtIonPlanFilePath and os.path.exists(rtIonPlanFilePath):
        planFilePath = rtIonPlanFilePath
        logging.info(f"Using RT Ion Plan file: {planFilePath}")

      # An RT Ion Plan file's own IonBeamSequence already defines every beam to simulate, so
      # the number of Slicer beam placeholders pointing at it is irrelevant: if more than one
      # beam in this plan references the same file, only the first one to get here actually
      # runs the simulation; any other beam referencing the same file gets assigned zero dose,
      # so the plan-level dose sum doesn't multiply the same RT Ion Plan dose several times.
      cacheKey = (planNode.GetID(), planFilePath) if planFilePath else None

      if cacheKey and cacheKey in self._rtIonPlanDoseClaimedBy and self._rtIonPlanDoseClaimedBy[cacheKey] != beamNode.GetID():
        logging.info(f"RT Ion Plan {planFilePath} dose for this plan was already assigned to another beam; "
                      "assigning zero dose to this beam to avoid double-counting.")
        TopasDoseEngineUtil.assignZeroDoseVolume(resultDoseVolumeNode, ctData.get('volumeNode'))
        return str()

      # Claim this (plan, RT Ion Plan file) pair before running, so a concurrent/later call
      # for another beam referencing the same file is short-circuited above even if this run
      # fails partway through.
      if cacheKey:
        self._rtIonPlanDoseClaimedBy[cacheKey] = beamNode.GetID()

      # Create and run TOPAS simulation using static utility
      logging.info("Running TOPAS simulation...")
      doseFilePath, workingDirectory = TopasDoseEngineUtil.runTopasSimulation(
        ctData=ctData,
        beamProperties=beamProperties,
        topasBinaryPath=topasBinaryPath,
        g4dataPath=g4dataPath,
        topasDirectoryPath=topasDirectoryPath,
        planFilePath=planFilePath
      )

      # Load result as volume in Slicer using standard DICOM loading
      logging.info("Loading dose result...")
      TopasDoseEngineUtil.loadDoseResultAsVolume(resultDoseVolumeNode, doseFilePath, workingDirectory,
                                                  particlesPerHistory=beamProperties.get('particlesPerHistory', 1.0),
                                                  ctVolumeNode=ctData.get('volumeNode'))

      logging.info("TOPAS dose calculation completed successfully!")
      return str()

    except Exception as e:
      logging.error(f"Error in TOPAS dose calculation: {str(e)}")
      raise e
