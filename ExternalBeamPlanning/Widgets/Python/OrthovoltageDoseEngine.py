import os, shutil
import vtk, qt, ctk, slicer
import logging
from DoseEngines import AbstractScriptedDoseEngine
from DoseEngines import OrthovoltageDoseEngineUtil
from DoseEngines import EGSnrcUtil

#------------------------------------------------------------------------------
#
# OrthovoltageDoseEngine
#
#------------------------------------------------------------------------------
class OrthovoltageDoseEngine(AbstractScriptedDoseEngine):
  """ Orthovoltage python dose engine
  """

  #------------------------------------------------------------------------------
  def __init__(self, scriptedEngine):
    scriptedEngine.name = 'Orthovoltage'
    AbstractScriptedDoseEngine.__init__(self, scriptedEngine)

    # Define initial defaults for parameters that are stored in application settings
    self.ctcreateExecFilePathDefault = "C:/EGSnrc/HEN_HOUSE/bin/win6432/ctcreate.exe"
    self.ctcreateOutputPathDefault = "C:/d/tmp"
    self.phaseSpaceFilePathDefault = "C:/d/6MV 8x6/DOSXYZnrc_files/egsphsp/6MV_8x6asym_sum.egsphsp1"
    self.dosxyznrcFolderPathDefault = "C:/EGSnrc/egs_home/dosxyznrc"
    self.dosxyznrcExecFilePathDefault = "C:/EGSnrc/egs_home/bin/win6432/dosxyznrc.exe"
    self.pegsFilePathDefault = "C:/EGSnrc/HEN_HOUSE/pegs4/data/521icru.pegs4dat"

  #------------------------------------------------------------------------------
  def defineBeamParameters(self):
    # Define session defaults for parameters that are stored in application settings
    settings = qt.QSettings()
    ctcreateExecFilePath = self.ctcreateExecFilePathDefault
    if settings.contains('OrthovoltageDoseEngine/CtcreateExecFilePath'):
       ctcreateExecFilePath = str(settings.value('OrthovoltageDoseEngine/CtcreateExecFilePath'))
    ctcreateOutputPath = self.ctcreateOutputPathDefault
    if settings.contains('OrthovoltageDoseEngine/CtcreateOutputPath'):
       ctcreateOutputPath = str(settings.value('OrthovoltageDoseEngine/CtcreateOutputPath'))
    phaseSpaceFilePath = self.phaseSpaceFilePathDefault
    if settings.contains('OrthovoltageDoseEngine/PhaseSpaceFilePath'):
       phaseSpaceFilePath = str(settings.value('OrthovoltageDoseEngine/PhaseSpaceFilePath'))
    dosxyznrcFolderPath = self.dosxyznrcFolderPathDefault
    if settings.contains('OrthovoltageDoseEngine/DosxyznrcFolderPath'):
       dosxyznrcFolderPath = str(settings.value('OrthovoltageDoseEngine/DosxyznrcFolderPath'))
    dosxyznrcExecFilePath = self.dosxyznrcExecFilePathDefault
    if settings.contains('OrthovoltageDoseEngine/DosxyznrcExecFilePath'):
       dosxyznrcExecFilePath = str(settings.value('OrthovoltageDoseEngine/DosxyznrcExecFilePath'))
    pegsFilePath = self.pegsFilePathDefault
    if settings.contains('OrthovoltageDoseEngine/PegsFilePath'):
       pegsFilePath = str(settings.value('OrthovoltageDoseEngine/PegsFilePath'))

    ##########################################
    # Generate ctcrate phantom parameters tab
    ##########################################

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "CtcreateExecFilePath", "Ctcreate executable file path:",
    "Enter file path of the ctcreate executable", ctcreateExecFilePath)

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "CtcreateOutputPath", "Ctcreate output file path:",
    "Enter file path to store ctcreate phantom and related files", ctcreateOutputPath)

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "ROIName", "ROI node name for cropping image (optional):",
    "Enter name of ROI, if you wish to crop CT image which will be converted to ctcreate phantom format. \
    If no ROI entered, will use image bounds from original CT image volume", "")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "SliceThicknessX", "Slice thickness (mm) in X direction (optional):",
    "Enter desired slice thickness in X direction for output ctcreate phantom. If not x,y,z slice \
    thickness not provided, will use same thickness from original CT image volume", "")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "SliceThicknessY", "Slice thickness (mm) in Y direction (optional):",
    "Enter desired slice thickness in Y direction for output ctcreate phantom. If not x,y,z slice \
    thickness not provided, will use same thickness from original CT image volume", "")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Generate ctcreate phantom", "SliceThicknessZ", "Slice thickness (mm) in Z direction (optional):",
    "Enter desired slice thickness in Z direction for output ctcreate phantom. If not x,y,z slice \
    thickness not provided, will use same thickness from original CT image volume", "")

    ##########################################
    # Orthovoltage dose parameters tab
    ##########################################

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "SimulationTitle", "Simulation title:",
    "Enter a title for the DOSXYZNrc simulation", "DOSXYZnrc simulation")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "PhaseSpaceFilePath", "Phase space file path:",
    "Enter full path to phase space file", phaseSpaceFilePath)

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "ECut", "Global electron cutoff energy - ECUT (MeV):",
    "Select global electron cutoff energy (MeV)", "0.512")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "PCut", "Global photon cutoff energy - PCUT (MeV):",
    "Select global electron cutoff energy (MeV)", "0.001")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "IncidentBeamSize", "Incident beam size (cm):",
    "SBEAM SIZE is the side of a square field in cm. The default value for \
    BEAM SIZE is 100 cm. When phase-space particles are read from a data file \
    or reconstructed by a multiple-source model, DOSXYZnrcwill check their \
    positions and discard those that fall outside of the specified field. \
    Use with caution.", "100.0")

    self.scriptedEngine.addBeamParameterLineEdit(
    "Orthovoltage dose", "DistanceSourceToIsocenter", "Distance from source to isocenter (cm):",
    "Enter absolute distance from source to isocenter (cm):", "0")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "NumHistories", "Number of histories:",
      "Number of histories to use for simulation", "129796480")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "Inseed1", "Random number generator seed 1:",
      "The random number generator used requires 2 seeds between 1 and \
      31328 and 1 and 30081 respectively.  For each different pair of \
      seeds, an independent random number sequence is generated.  Once \
      the seeds are used to establish the state of the generator, the \
      \"seeds\" output in log files etc. are really just pointers between 1 and 98.",
      "33")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "Inseed2", "Random number generator seed 2:",
      "The random number generator used requires 2 seeds between 1 and \
      31328 and 1 and 30081 respectively.  For each different pair of \
      seeds, an independent random number sequence is generated.  Once \
      the seeds are used to establish the state of the generator, the \
      \"seeds\" output in log files etc. are really just pointers between 1 and 98.",
      "97")

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "DosxyznrcFolderPath", "Dosxyznrc folder path:",
      "Enter file path to dosxyznrc folder", dosxyznrcFolderPath)

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "DosxyznrcExecFilePath", "Dosxyznrc executable file path:",
      "Enter file path to dosxyznrc executable", dosxyznrcExecFilePath)

    self.scriptedEngine.addBeamParameterLineEdit(
      "Orthovoltage dose", "PegsFilePath", "Path to pegs4 data file:",
      "Enter file path to .pegs5dat file", pegsFilePath)

  #------------------------------------------------------------------------------
  #TODO: Add a path parameter type using the CTK path selector that saves the selections to Application Settings
  def savePathsInApplicationSettings(self, beamNode):
    if beamNode is None:
      return

    settings = qt.QSettings()

    ctcreateExecFilePath = self.scriptedEngine.parameter(beamNode, "CtcreateExecFilePath")
    if ctcreateExecFilePath != self.ctcreateExecFilePathDefault:
      qt.QSettings().setValue('OrthovoltageDoseEngine/CtcreateExecFilePath', ctcreateExecFilePath)

    ctcreateOutputPath = self.scriptedEngine.parameter(beamNode, "CtcreateOutputPath")
    if ctcreateOutputPath != self.ctcreateOutputPathDefault:
      qt.QSettings().setValue('OrthovoltageDoseEngine/CtcreateOutputPath', ctcreateOutputPath)

    phaseSpaceFilePath = self.scriptedEngine.parameter(beamNode, "PhaseSpaceFilePath")
    if phaseSpaceFilePath != self.phaseSpaceFilePathDefault:
      qt.QSettings().setValue('OrthovoltageDoseEngine/PhaseSpaceFilePath', phaseSpaceFilePath)

    dosxyznrcFolderPath = self.scriptedEngine.parameter(beamNode, "DosxyznrcFolderPath")
    if dosxyznrcFolderPath != self.dosxyznrcFolderPathDefault:
      qt.QSettings().setValue('OrthovoltageDoseEngine/DosxyznrcFolderPath', dosxyznrcFolderPath)

    dosxyznrcExecFilePath = self.scriptedEngine.parameter(beamNode, "DosxyznrcExecFilePath")
    if dosxyznrcExecFilePath != self.dosxyznrcExecFilePathDefault:
      qt.QSettings().setValue('OrthovoltageDoseEngine/DosxyznrcExecFilePath', dosxyznrcExecFilePath)

    pegsFilePath = self.scriptedEngine.parameter(beamNode, "PegsFilePath")
    if pegsFilePath != self.pegsFilePathDefault:
      qt.QSettings().setValue('OrthovoltageDoseEngine/PegsFilePath', pegsFilePath)

  #------------------------------------------------------------------------------
  def calculateDoseUsingEngine(self, beamNode, resultDoseVolumeNode):
    # Save path selections in application settings
    self.savePathsInApplicationSettings(beamNode)

    # Get input nodes and parameters
    parentPlan = beamNode.GetParentPlanNode()
    volumeNode = parentPlan.GetReferenceVolumeNode()

    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    refVolumeShID = shNode.GetItemByDataNode(volumeNode)
    seriesUID = shNode.GetItemUID(refVolumeShID, 'DICOM')

    # Get isocenter and prepare it for DOSXYZnrc:
    # 1. Truncate isocenter position (see https://github.com/SlicerRt/SlicerRT/issues/112)
    # 2. Convert isocenter from mm to cm
    isocenter = [0]*3
    parentPlan.GetIsocenterPosition(isocenter)
    import math
    isocenter = [math.floor(x*100) / 1000 for x in isocenter]
    logging.info("Isocenter (cm): " + str(isocenter))

    ##########################################
    # Get ctcreate parameters
    ##########################################

    ctcreateExecFilePath = self.scriptedEngine.parameter(beamNode, "CtcreateExecFilePath")
    ctcreateOutputPath = self.scriptedEngine.parameter(beamNode, "CtcreateOutputPath")
    roiNodeName = self.scriptedEngine.parameter(beamNode, "ROIName")
    volumeName = self.scriptedEngine.parameter(beamNode, "VolumeName")
    sliceThicknessX = self.scriptedEngine.parameter(beamNode, "SliceThicknessX")
    sliceThicknessY = self.scriptedEngine.parameter(beamNode, "SliceThicknessY")
    sliceThicknessZ = self.scriptedEngine.parameter(beamNode, "SliceThicknessZ")

    roiNode = None
    if roiNodeName != "":
      try:
        roiNode = slicer.util.getNode(roiNodeName)
      except:
        logging.error("Unable to get ROI by name " + roiNodeName)

    thicknesses = None
    if sliceThicknessX != "" and sliceThicknessY != "" and sliceThicknessZ != "":
      thicknesses = [float(sliceThicknessX), float(sliceThicknessY), float(sliceThicknessZ)]

    ##########################################
    # Call ctcreate
    ##########################################

    OrthovoltageDoseEngineUtil.generateCtcreateInput(volumeNode, seriesUID, ctcreateOutputPath, roiNode, thicknesses)
    EGSnrcUtil.callCtcreate(ctcreateExecFilePath, ctcreateOutputPath)

    ##########################################
    # Get DOSXYZnrc parameters
    ##########################################
    title = self.scriptedEngine.parameter(beamNode, "SimulationTitle")
    phaseSpaceFilePath = self.scriptedEngine.parameter(beamNode, "PhaseSpaceFilePath")
    ecut = self.scriptedEngine.parameter(beamNode, "ECut")
    pcut = self.scriptedEngine.parameter(beamNode, "PCut")
    incidentBeamSize = self.scriptedEngine.parameter(beamNode,"IncidentBeamSize")
    dosxyznrcFolderPath = self.scriptedEngine.parameter(beamNode, "DosxyznrcFolderPath")
    dosxyznrcExecFilePath = self.scriptedEngine.parameter(beamNode, "DosxyznrcExecFilePath")
    pegsFilePath = self.scriptedEngine.parameter(beamNode, "PegsFilePath")
    nmed = 0 # nmed is number of media
    smax = 0 # dummy input, used to be max step length
    zeroairdose = 1
    doseprint = 0
    MAX20 = 0

    # Record SC1-8
    iqin = 2              # charge of incident beam
    isource = 2 # Note: May need to change later, see code commented out below
    xiso = -isocenter[0]  # isocenter x (RAS -> LPS)
    yiso = -isocenter[1]  # isocenter y (RAS -> LPS)
    ziso =  isocenter[2]  # isocenter z

    nang = 1 # number of incident theta-phi pairs
    dsource = self.scriptedEngine.parameter(beamNode, "DistanceSourceToIsocenter") # absolute distance from isocenter to source center
    i_dbs = 1 # I=8: 0
    r_dbs = 10 # I=8: 0
    ssd_dbs = 20 # I=8: 0
    z_dbs = 20 # I=8: 0
    e_split = 0

    # Record SC1-8a
    (theta, phi, phicol) = EGSnrcUtil.dcm2dosxyz(
      beamNode.GetGantryAngle(), beamNode.GetCouchAngle(), beamNode.GetCollimatorAngle() )
    # nang1_theta = theta # incident theta angle
    # nang1_phi = phi     # incident phi angle
    # nang1_pang = 1      # probability of a particle being incident at theta(i)-phi(i)
    #                     # (probabilities are automatically normalized to 1).

    # Record SC2
    enflag = 2        # for ph-sp beam input or full BEAM sim.
    mode = 0          # default file format for ph-sp data (enflag=2)
    medsur = 0        # medium number for the region outside the phantom             # I=8: 1
    dsurround_1 = 10  # thickness (cm) of region surrounding phantom in x direction  # I=8: 30
    dflag = 0         # dsurround(1) applied to x direction only                     # I=8: 1
    dsurround_2 = 0   # thickness (cm) of region surrounding phantom in y direction  # I=8: 60
    dsurround_3 = 0   # thickness (cm) of region surrounding phantom in +z direction # I=8: 30
    dsurround_4 = 0   # thickness (cm) of region surrounding phantom in -z direction # I=8: 30

    # Record 13
    ncase = self.scriptedEngine.parameter(beamNode, "NumHistories")
    iwatch = 0        # no tracking output
    timmax = 99       # max CPU time in hours allowed for a simulation, not activated in the current version of DOSXYZnrc
    inseed1 = self.scriptedEngine.parameter(beamNode, "Inseed1")      # starting random seed number
    inseed2 = self.scriptedEngine.parameter(beamNode, "Inseed2")      # starting random seed number
    beam_size = incidentBeamSize
    ismooth = 0       # re-use the ph-sp data once run out (no redistribution)
    irestart = 0      # first run for this data set (default)
    idat = 0          # output the data file for restart at end only                                       # I=8: 2
    ireject = 0       # do not perform charged particle range rejection (default)
    esave_global = "" # energy (MeV) below which charged particle will be considered for range rejection
    nrcycl = 0        # use entire phase space file with no restarts
    iparallel = 0     # only relevalnt when manually creating/submitting paralel jobs or using unix pproccess script
    parnum = 0        # only relevalnt when manually creating/submitting paralel jobs or using unix pproccess script
    n_split = 1
    ihowfarless = 0
    i_phsp_out = 0    # no phase space output (default)

    # EGSnrc inputs
    globalEcut = ecut
    globalPcut = pcut
    globalSmax = 5    #
    estepe = 0.25     # max fractional energy loss per step (default)
    ximax = 0.5       # max first elastic scattering moment per step (default)

    import datetime
    dateTimeStr = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    dosXyznrcSessionFilePrefix = dateTimeStr + "_dosxyznrc"
    dosXyznrcInputFileName = dosXyznrcSessionFilePrefix + ".egsinp"

    with open(os.path.join(ctcreateOutputPath, dosXyznrcInputFileName), "w") as dosXyzInFile:
      dosXyzInFile.write(title + "\n")
      dosXyzInFile.write(str(nmed) + "\n")
      dosXyzInFile.write(os.path.join(ctcreateOutputPath, "slicenames.txt.egsphant") + "\n") #TODO: remove hardcode
      dosXyzInFile.write("{}, {}, {}\n".format(ecut, pcut, smax))
      dosXyzInFile.write("{}, {}, {},\n".format(zeroairdose, doseprint, MAX20))
      # Note: This was for ISOSOURCE=8
      # dosXyzInFile.write("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}\n".format(
      #   iqin, isource, xiso, yiso, ziso, nang, dsource, phicol, i_dbs, r_dbs, ssd_dbs, z_dbs, e_split))
      # ISOSOURCE=2
      dosXyzInFile.write("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}\n".format(
        iqin, isource, xiso, yiso, ziso, theta, phi, dsource, phicol, i_dbs, r_dbs, ssd_dbs, z_dbs, e_split))
      # dosXyzInFile.write("{}, {}, {}\n".format(nang1_theta, nang1_phi, nang1_pang)) # Note: Was here for ISOSOURCE=8
      dosXyzInFile.write("{}, {}, {}, {}, {}, {}, {}, {}\n".format(
        enflag, mode, medsur, dsurround_1, dflag, dsurround_2, dsurround_3, dsurround_4))
      dosXyzInFile.write(phaseSpaceFilePath + "\n")
      dosXyzInFile.write("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}\n".format(
        ncase, iwatch, timmax, inseed1, inseed2, beam_size, ismooth, irestart, idat, ireject,
        esave_global, nrcycl, iparallel, parnum, n_split, ihowfarless, i_phsp_out))
      dosXyzInFile.write(" #########################\n")
      dosXyzInFile.write(" :Start MC Transport Parameter:\n \n")
      dosXyzInFile.write(" Global ECUT= {}\n".format(globalEcut))
      dosXyzInFile.write(" Global PCUT= {}\n".format(globalPcut))
      dosXyzInFile.write(" Global SMAX= {}\n".format(globalSmax))
      dosXyzInFile.write(" ESTEPE= {}\n".format(estepe))
      dosXyzInFile.write(" XIMAX= {}\n".format(ximax))
      dosXyzInFile.write(" Boundary crossing algorithm= PRESTA-I\n") #TODO: add comments
      dosXyzInFile.write(" Skin depth for BCA= 0\n")
      dosXyzInFile.write(" Electron-step algorithm= PRESTA-II\n")
      dosXyzInFile.write(" Spin effects= On\n")
      dosXyzInFile.write(" Brems angular sampling= Simple\n")
      dosXyzInFile.write(" Brems cross sections= BH\n")
      dosXyzInFile.write(" Bound Compton scattering= Off\n")
      dosXyzInFile.write(" Compton cross sections= default\n")
      dosXyzInFile.write(" Pair angular sampling= Simple\n")
      dosXyzInFile.write(" Pair cross sections= BH\n")
      dosXyzInFile.write(" Photoelectron angular sampling= Off\n")
      dosXyzInFile.write(" Rayleigh scattering= Off\n")
      dosXyzInFile.write(" Atomic relaxations= Off\n")
      dosXyzInFile.write(" Electron impact ionization= Off\n")
      dosXyzInFile.write(" Photon cross sections= xcom\n")
      dosXyzInFile.write(" Photon cross-sections output= Off\n \n")
      dosXyzInFile.write(" :Stop MC Transport Parameter:\n")
      dosXyzInFile.write(" #########################\n")

    ##########################################
    # Call DOSXYZnrc
    ##########################################

    # Copy DOSXYZnrc input file to dosxyznrc directory
    shutil.copy2(os.path.join(ctcreateOutputPath, dosXyznrcInputFileName), dosxyznrcFolderPath)

    # Run dose calculation
    import subprocess
    proc = subprocess.Popen([dosxyznrcExecFilePath, '-i', dosXyznrcInputFileName, '-p', pegsFilePath], stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()

    # Manage output log
    outStr = str(out).replace('\\r','').replace('\\n','\n')
    logging.debug("-----------------------------\n")
    logging.debug("DOSXYZ output: \n" + outStr)
    outStr = outStr[:outStr.rfind("'")] # Strip closing single quote
    if outStr[len(outStr)-1:] == '\n':
      outStr = outStr[:len(outStr)-1] # Strip last empty line if any
    logging.info("DOSXYZ output (last paragraph): \n" + outStr[outStr.rfind('\n\n')+2:])
    if err is not None and str(err) != '':
      logging.error("DOSXYZ error: \n" + str(err))
      return "DOSXYZ error! Please check the log"

    # Read output 3ddose file into result dose volume
    dosXyznrcOutputFileName = dosXyznrcSessionFilePrefix + ".3ddose"
    dosXyznrcOutputFilePath = os.path.join(dosxyznrcFolderPath, dosXyznrcOutputFileName)
    loadedVolumeNode = slicer.util.loadNodeFromFile(dosXyznrcOutputFilePath, 'DosxyzNrc3dDoseFile', {})
    if loadedVolumeNode is None:
      logging.error("Failed to load result dose file {} for session using DOSXYZnrc input file {}".format(dosXyznrcOutputFilePath, dosXyznrcInputFileName))
      return "Failed to load result dose file! Please check the log"

    resultDoseVolumeNode.CopyOrientation(loadedVolumeNode)
    doseImageDataCopy = vtk.vtkImageData()
    doseImageDataCopy.DeepCopy(loadedVolumeNode.GetImageData())
    resultDoseVolumeNode.SetAndObserveImageData(doseImageDataCopy)
    slicer.mrmlScene.RemoveNode(loadedVolumeNode)
    accumulate = vtk.vtkImageAccumulate()
    accumulate.SetInputData(doseImageDataCopy)
    accumulate.IgnoreZeroOn()
    accumulate.Update()
    logging.info( 'Result dose volume for beam ' + beamNode.GetName() + ' successfully loaded.\n'
      + '  Dose range: ({:.4f}-{:.4f})'.format(accumulate.GetMin()[0],accumulate.GetMax()[0]) )

    # Successful execution, no error message
    return ""
