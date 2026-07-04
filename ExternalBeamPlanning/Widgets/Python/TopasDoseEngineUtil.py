import os
import math
import shutil
import tempfile
import subprocess
import vtk
import slicer
import logging
import uuid
import glob

#------------------------------------------------------------------------------
#
# TopasDoseEngineUtil
#
#------------------------------------------------------------------------------
class TopasDoseEngineUtil:
  """Static utility class for TOPAS dose engine operations"""

  #------------------------------------------------------------------------------
  @staticmethod
  def exportCTAsDicomSeries(ctData, dicomDirectory):
    """Export CT volume as DICOM series in LPS orientation to dicomDirectory"""
    volumeNode = ctData['volumeNode']
    # Prepare CLI parameters
    cliModule = slicer.modules.createdicomseries
    outputDir = dicomDirectory
    os.makedirs(outputDir, exist_ok=True)
    params = {
        'inputVolume': volumeNode.GetID(),
        'dicomDirectory': outputDir,
        'dicomPrefix': 'IMG',
        'dicomNumberFormat': '%04d',
        'type': 'Short',
        'patientSex': '[unknown]',
        'studyID': 'SLICER10001',
        'modality': 'CT',
        'manufacturer': 'Unknown manufacturer',
        'model': 'Unknown model',
        'seriesNumber': 301,
        'seriesDescription': 'No series description',
        'patientPosition': 'HFS',
        'windowCenter': 40.0,
        'windowWidth': 400.0,
        'rescaleIntercept': 0.0,
        'rescaleSlope': 1.0,
        'studyInstanceUID': str(uuid.uuid4()),
        'seriesInstanceUID': str(uuid.uuid4()),
        'frameOfReferenceUID': str(uuid.uuid4()),
    }
    # Run CLI synchronously
    cliNode = slicer.cli.runSync(cliModule, None, params)
    if cliNode.GetStatusString() != 'Completed':
        raise RuntimeError(f"CreateDICOMSeries CLI failed: {cliNode.GetStatusString()} - {cliNode.GetErrorText()}")
    # Check for DICOM files
    dicomFiles = glob.glob(os.path.join(outputDir, '**', '*.dcm'), recursive=True)
    if not dicomFiles:
        details = f"\nExport directory exists: {os.path.exists(outputDir)}\n"
        details += f"Export directory writable: {os.access(outputDir, os.W_OK)}\n"
        details += f"volumeNode: {volumeNode.GetName()} (ID: {volumeNode.GetID()})\n"
        raise RuntimeError(f"Failed to export CT as DICOM to {outputDir}.{details}")
    logging.info(f"Successfully exported CT as DICOM to {outputDir}, found {len(dicomFiles)} DICOM files.")
    return outputDir

  #------------------------------------------------------------------------------
  @staticmethod
  def extractCTData(planNode):
    """Extract CT volume data and properties"""
    # Get CT volume from plan
    ctVolumeNode = planNode.GetReferenceVolumeNode()
    if not ctVolumeNode:
      raise RuntimeError("No CT volume found in plan")

    # Get image data
    imageData = ctVolumeNode.GetImageData()
    if not imageData:
      raise RuntimeError("No image data in CT volume")

    # Get spacing and origin
    spacing = ctVolumeNode.GetSpacing()
    origin = ctVolumeNode.GetOrigin()
    dimensions = imageData.GetDimensions()

    # Convert to numpy array
    pointData = imageData.GetPointData()
    arrayData = pointData.GetScalars()
    numpyArray = vtk.util.numpy_support.vtk_to_numpy(arrayData)
    numpyArray = numpyArray.reshape(dimensions[2], dimensions[1], dimensions[0])

    return {
      'data': numpyArray,
      'spacing': spacing,
      'origin': origin,
      'dimensions': dimensions,
      'volumeNode': ctVolumeNode
    }

  #------------------------------------------------------------------------------
  @staticmethod
  def extractBeamProperties(beamNode):
    """Extract beam properties from beam node"""
    beamProperties = {}

    # Get beam geometry
    isocenter = [0.0, 0.0, 0.0]
    beamNode.GetPlanIsocenterPosition(isocenter)
    beamProperties['isocenter'] = isocenter

    beamProperties['gantryAngle'] = beamNode.GetGantryAngle()
    beamProperties['couchAngle'] = beamNode.GetCouchAngle()
    beamProperties['collimatorAngle'] = beamNode.GetCollimatorAngle()

    # Get beam parameters
    beamProperties['energy'] = beamNode.GetBeamEnergy()
    beamProperties['sourceAxisDistance'] = beamNode.GetSAD()

    # Get jaw positions (mm at isocenter).  X1/Y1 are positive distances on the
    # negative side; X2/Y2 are positive distances on the positive side.
    beamProperties['x1Jaw'] = beamNode.GetX1Jaw()  # signed mm; negative → on -X side
    beamProperties['x2Jaw'] = beamNode.GetX2Jaw()  # signed mm; positive → on +X side
    beamProperties['y1Jaw'] = beamNode.GetY1Jaw()  # signed mm; negative → on -Y side
    beamProperties['y2Jaw'] = beamNode.GetY2Jaw()  # signed mm; positive → on +Y side
    logging.info(f"Jaw positions: X1={beamProperties['x1Jaw']}, X2={beamProperties['x2Jaw']}, "
                 f"Y1={beamProperties['y1Jaw']}, Y2={beamProperties['y2Jaw']} mm")
    beamProperties['fieldSizeX'] = beamProperties['x2Jaw'] - beamProperties['x1Jaw']
    beamProperties['fieldSizeY'] = beamProperties['y2Jaw'] - beamProperties['y1Jaw']
    logging.info(f"Field size: X={beamProperties['fieldSizeX']} mm, Y={beamProperties['fieldSizeY']} mm")

    # Default number of histories
    beamProperties['numberOfHistories'] = 1000000

    return beamProperties

  #------------------------------------------------------------------------------
  @staticmethod
  def createTopasInputFileDicom(dicomDirectory, beamProperties, workingDirectory, topasDirectory, ctData=None, planFilePath=None, doseFilePath=None, beamNumber=1):
    """Create TOPAS input file for dose calculation using DICOM RT Ion interface.

    Args:
      dicomDirectory: Path to directory containing CT DICOM series
      beamProperties: Dictionary containing beam parameters (from extractBeamProperties)
      workingDirectory: Working directory for TOPAS simulation
      topasDirectory: TOPAS installation directory
      planFilePath: Path to RT Ion Plan DICOM file (optional)
      doseFilePath: Path for dose output (optional, will use default if not specified)
    """
    inputFilePath = os.path.join(workingDirectory, 'topas_input.txt')

    # Set default dose output path if not provided
    if not doseFilePath:
      doseFilePath = os.path.join(workingDirectory, 'dose_output')

    # TROTS-compatible Schneider HU table resource: adds SchneiderUseVariableDensityMaterials
    # (continuous per-voxel density instead of 25 discrete bins), 0.4 mm particle cuts,
    # and the DensityCorrection calibration factor from the TROTS pipeline.
    # A user-supplied file (beamProperties['HUtoMaterialFile']) overrides this bundled
    # default, since the bundled table is a reasonable but somewhat dated starting point.
    _HUtoMaterialFilePath = beamProperties.get('HUtoMaterialFile') or ''
    if not _HUtoMaterialFilePath or not os.path.exists(_HUtoMaterialFilePath):
      _HUtoMaterialFilePath = os.path.join(os.path.dirname(__file__), 'Resources', 'HUtoMaterialSchneider.txt')
    includeFilePathTopas = TopasDoseEngineUtil.winToWslPath(_HUtoMaterialFilePath)

    # A user-supplied machine description file overrides the machine name normally read by
    # TsRTIonSource/TsRTIonComponents from the RT Ion Plan DICOM (InstitutionName:TreatmentMachineName).
    # The dicom-interface "MachineName" parameter is parsed as "<site>:<model>"; "pbs:<path>"
    # selects the generic, runtime-configurable PBS beam model and loads it from that .table file
    # (see https://github.com/topasmc/dicom-interface/wiki/using-machine-description-file).
    _machineDescriptionFilePath = beamProperties.get('machineDescriptionFile') or ''
    _machineNameTopas = None
    if _machineDescriptionFilePath and os.path.exists(_machineDescriptionFilePath):
      _machineNameTopas = 'pbs:' + TopasDoseEngineUtil.winToWslPath(_machineDescriptionFilePath)

    # Detect TPS reference dose, used to clone the dose grid.
    _rtdoseFilePath = None
    _searchDirs = [dicomDirectory]
    if planFilePath:
      _searchDirs.append(os.path.dirname(planFilePath))
    for _searchDir in _searchDirs:
      for _dcmFile in glob.glob(os.path.join(_searchDir, '*.dcm')):
        try:
          import pydicom as _pd
          _ds = _pd.dcmread(_dcmFile, stop_before_pixels=True, specific_tags=['Modality'])
          if getattr(_ds, 'Modality', None) == 'RTDOSE':
            _rtdoseFilePath = _dcmFile
            break
        except Exception:
          continue
      if _rtdoseFilePath:
        break
    _hasDoseGrid = _rtdoseFilePath is not None

    # Convert Windows paths to WSL format if on Windows (TOPAS runs in WSL)
    if TopasDoseEngineUtil.isWindows():
      dicomDirectoryTopas = TopasDoseEngineUtil.winToWslPath(dicomDirectory)
      doseFilePathTopas = TopasDoseEngineUtil.winToWslPath(doseFilePath)
      planFilePathTopas = TopasDoseEngineUtil.winToWslPath(planFilePath) if planFilePath else None
      _rtdoseFilePathTopas = TopasDoseEngineUtil.winToWslPath(_rtdoseFilePath) if _rtdoseFilePath else None
    else:
      dicomDirectoryTopas = dicomDirectory
      doseFilePathTopas = doseFilePath
      planFilePathTopas = planFilePath
      _rtdoseFilePathTopas = _rtdoseFilePath

    # Compute CT center in LPS (= TOPAS ImgCenter).
    # TsRTIonComponents/TsRTIonSource use Trans = IsoCenter - ImgCenter to position
    # the beam in TOPAS world, where the CT center is at (0,0,0). If ImgCenter is
    # wrong the source ends up hundreds of mm from the patient.
    img_center_L = 0.0
    img_center_P = 0.0
    img_center_S = 0.0
    if ctData and ctData.get('volumeNode'):
      _ctNode = ctData['volumeNode']
      _mat = vtk.vtkMatrix4x4()
      _ctNode.GetIJKToRASMatrix(_mat)
      _dims = _ctNode.GetImageData().GetDimensions()
      # RAS position of the center voxel
      _ci = [(_dims[d] - 1) / 2.0 for d in range(3)]
      _cr = [sum(_mat.GetElement(r, c) * (_ci[c] if c < 3 else 1.0) for c in range(4)) for r in range(3)]
      # RAS → LPS: L = -R, P = -A, S = S
      img_center_L = -_cr[0]
      img_center_P = -_cr[1]
      img_center_S =  _cr[2]
      logging.info(f"CT center in LPS: L={img_center_L:.2f}, P={img_center_P:.2f}, S={img_center_S:.2f} mm")

    # Extract beam properties
    gantryAngle = beamProperties.get('gantryAngle', 0.0)
    couchAngle = beamProperties.get('couchAngle', 0.0)
    collimatorAngle = beamProperties.get('collimatorAngle', 0.0)
    isocenter = beamProperties.get('isocenter', [0.0, 0.0, 0.0])
    energy = beamProperties.get('energy', 100.0)  # MeV
    sad = beamProperties.get('sourceAxisDistance', 1000.0)  # mm
    # For RT Ion plans, read the virtual source axis distance (X direction) from the DICOM.
    if planFilePath:
      try:
        import pydicom as _pd
        _ds = _pd.dcmread(planFilePath, stop_before_pixels=True)
        _vsad = getattr(_ds.IonBeamSequence[0], 'VirtualSourceAxisDistances', None)
        if _vsad and len(_vsad) >= 1:
          sad = float(_vsad[0])
          logging.info(f"Using VirtualSourceAxisDistances[0] as SID: {sad} mm")
      except Exception as _e:
        logging.warning(f"Could not read VirtualSourceAxisDistances: {_e}")
    numberOfHistories = beamProperties.get('numberOfHistories', 1000000)
    particlesPerHistory = beamProperties.get('particlesPerHistory', 1.0)
    radiationMode = beamProperties.get('radiationMode', 'proton')

    # Display names match TOPAS BeamParticle strings directly.
    beamParticle = radiationMode

    fieldSizeX = beamProperties.get('fieldSizeX', 100.0)  # mm (total: x1+x2)
    fieldSizeY = beamProperties.get('fieldSizeY', 100.0)  # mm (total: y1+y2)
    x1Jaw = beamProperties.get('x1Jaw', fieldSizeX / 2.0)  # mm, positive distance on -X side
    x2Jaw = beamProperties.get('x2Jaw', fieldSizeX / 2.0)  # mm, positive distance on +X side
    y1Jaw = beamProperties.get('y1Jaw', fieldSizeY / 2.0)  # mm, positive distance on -Y side
    y2Jaw = beamProperties.get('y2Jaw', fieldSizeY / 2.0)  # mm, positive distance on +Y side

    logging.info(f"Beam properties extracted: energy={energy} MeV, SAD={sad} mm, histories={numberOfHistories}, field size={fieldSizeX}x{fieldSizeY} mm")
    logging.info(f"Angles: gantry={gantryAngle}°, couch={couchAngle}°, collimator={collimatorAngle}°")
    logging.info(f"Isocenter: {isocenter}")

    with open(inputFilePath, 'w') as f:
      f.write("# TOPAS Input File - RT Ion Dose Calculation\n")
      f.write("# Generated by SlicerRT TopasDoseEngine\n\n")

      # Include HU to material conversion
      f.write(f'includeFile = {includeFilePathTopas}\n\n')

      # Physics settings
      f.write("# Physics\n")
      f.write('sv:Ph/Default/Modules = 7 "g4em-standard_opt3" "g4h-phy_QGSP_BIC" "g4decay" "g4ion-binarycascade" "g4h-elastic_HP" "g4stopping" "g4radioactivedecay"\n')
      f.write('i:Ts/MaxInterruptedHistories = 1000\n')
      f.write('b:Ge/QuitIfOverlapDetected = "T"\n')
      f.write('\n')

      # World geometry — must contain the CT volume *and* the beam source.
      # World must contain the full beam geometry (source at SAD, jaws between source and isocenter).
      # Worst case: jaw at SAD from isocenter + isocenter offset from CT center (~300 mm margin).
      # For the RTION path SAD is not known here so 1.1 m suffices (TsRTIonComponents self-contained).
      # For the simple beam path, sad is known and may be up to 2000 mm, so scale accordingly.
      _worldSadMm = sad if not planFilePathTopas else 1100.0
      worldHalfLengthM = max(1.1, (_worldSadMm + 300.0) / 1000.0)
      f.write("# World\n")
      f.write('s:Ge/World/Type     = "TsBox"\n')
      f.write('s:Ge/World/Material = "G4_AIR"\n')
      f.write(f'd:Ge/World/HLX      = {worldHalfLengthM:.2f} m\n')
      f.write(f'd:Ge/World/HLY      = {worldHalfLengthM:.2f} m\n')
      f.write(f'd:Ge/World/HLZ      = {worldHalfLengthM:.2f} m\n\n')

      if planFilePathTopas:
        # === DICOM RT Ion Plan path ===

        # IEC Fixed coordinate frame (parent for patient and source)
        f.write("# IEC Fixed coordinate frame\n")
        f.write('s:Ge/IEC_F/Type   = "Group"\n')
        f.write('s:Ge/IEC_F/Parent = "World"\n')
        f.write('d:Ge/IEC_F/TransX = 0. mm\n')
        f.write('d:Ge/IEC_F/TransY = 0. mm\n')
        f.write('d:Ge/IEC_F/TransZ = 0. mm\n\n')

        # RT Ion Components (hardware geometry: range shifters, apertures, etc.)
        f.write("# RT Ion Components (TsRTIonComponents)\n")
        f.write('s:Ge/RTION/Type        = "TsRTIonComponents"\n')
        f.write('s:Ge/RTION/Parent      = "IEC_F"\n')
        f.write(f's:Ge/RTION/File        = "{planFilePathTopas}"\n')
        f.write(f'i:Ge/RTION/BeamNumber  = {beamNumber}\n')
        f.write(f's:Ge/RTION/ImgDirectory = "{dicomDirectoryTopas}"\n')
        if _machineNameTopas:
          f.write(f's:Ge/RTION/MachineName  = "{_machineNameTopas}"\n')
        # else: machinename read from DICOM (e.g. RBE:1.1)
        f.write('b:Ge/RTION/IsParallel  = "F"\n')
        f.write('b:Ge/RTION/Include     = "T"\n')
        f.write('b:Ge/RTION/IncludeSnoutIfExist        = "F"\n')
        f.write('b:Ge/RTION/IncludeRangeshifterIfExist = "F"\n')
        f.write('b:Ge/RTION/IncludeBlockIfExist        = "F"\n')
        f.write('b:Ge/RTION/IncludeCompensatorIfExist  = "F"\n')
        _doseGridFlag = '"T"' if _hasDoseGrid else '"F"'
        f.write(f'b:Ge/RTION/IncludeDoseGridIfExist     = {_doseGridFlag}\n')
        if _hasDoseGrid:
          f.write(f's:Ge/RTION/RTDoseFile                 = "{_rtdoseFilePathTopas}"\n')
        # CT center in LPS — must match TsDicomPatient's world placement so that
        # Trans = IsoCenter - ImgCenter correctly positions the beam at the isocenter.
        f.write(f'dc:Ge/RTION/ImgCenterX       = {img_center_L:.4f} mm\n')
        f.write(f'dc:Ge/RTION/ImgCenterY       = {img_center_P:.4f} mm\n')
        f.write(f'dc:Ge/RTION/ImgCenterZ       = {img_center_S:.4f} mm\n')
        f.write('dc:Ge/RTION/IsoCenterX       = 0 mm\n')
        f.write('dc:Ge/RTION/IsoCenterY       = 0 mm\n')
        f.write('dc:Ge/RTION/IsoCenterZ       = 0 mm\n')
        f.write('dc:Ge/RTION/CollimatorAngle     = 0 deg\n')
        f.write('dc:Ge/RTION/GantryAngle         = 0 deg\n')
        f.write('dc:Ge/RTION/PatientSupportAngle = 0 deg\n')
        f.write('dc:Ge/RTION/Iec2DicomAngle      = 0 deg\n')
        f.write('d:Ge/RTION/TransX = Ge/RTION/IsoCenterX - Ge/RTION/ImgCenterX mm\n')
        f.write('d:Ge/RTION/TransY = Ge/RTION/IsoCenterY - Ge/RTION/ImgCenterY mm\n')
        f.write('d:Ge/RTION/TransZ = Ge/RTION/IsoCenterZ - Ge/RTION/ImgCenterZ mm\n')
        f.write('d:Ge/RTION/RotCollimator      = Ge/RTION/CollimatorAngle deg\n')
        f.write('d:Ge/RTION/RotGantry          = Ge/RTION/GantryAngle deg\n')
        f.write('d:Ge/RTION/RotPatientSupport  = -1.0 * Ge/RTION/PatientSupportAngle deg\n')
        f.write('d:Ge/RTION/RotIEC2DICOM       = 90 deg\n\n')

        # Patient from DICOM CT — centered at IEC_F origin (TsDicomPatient
        # places the CT bounding-box center at Trans=0 of its parent).
        f.write("# Patient from DICOM CT\n")
        f.write('s:Ge/Patient/Type               = "TsDicomPatient"\n')
        f.write('s:Ge/Patient/Parent             = "IEC_F"\n')
        f.write('s:Ge/Patient/Material           = "G4_WATER"\n')
        f.write(f's:Ge/Patient/DicomDirectory    = "{dicomDirectoryTopas}"\n')
        f.write('sv:Ge/Patient/DicomModalityTags = 1 "CT"\n')
        if _hasDoseGrid:
          f.write(f's:Ge/Patient/CloneRTDoseGridFrom = "{_rtdoseFilePathTopas}"\n')
        f.write('d:Ge/Patient/TransX = 0. mm\n')
        f.write('d:Ge/Patient/TransY = 0. mm\n')
        f.write('d:Ge/Patient/TransZ = 0. mm\n')
        f.write('d:Ge/Patient/RotX   = 0. deg\n')
        f.write('d:Ge/Patient/RotY   = 0. deg\n')
        f.write('d:Ge/Patient/RotZ   = 0. deg\n')
        f.write('b:Ge/Patient/IgnoreInconsistentFrameOfReferenceUID = "True"\n\n')

  
        # RT Ion Beam Source — fires from IEC_F frame.
        # imgdirectory auto-computes ImgCenter and IsoCenter from the CT/plan DICOM.
        f.write("# RT Ion Beam Source (TsRTIonSource)\n")
        f.write('s:So/RTION/Type                   = "TsRTIonSource"\n')
        f.write('s:So/RTION/Component              = "IEC_F"\n')
        f.write(f's:So/RTION/File                  = "{planFilePathTopas}"\n')
        f.write(f's:So/RTION/ImgDirectory          = "{dicomDirectoryTopas}"\n')
        if _machineNameTopas:
          f.write(f's:So/RTION/MachineName            = "{_machineNameTopas}"\n')
        # else: machinename read from DICOM (e.g. RBE:1.1)
        f.write(f'i:So/RTION/BeamNumber             = {beamNumber}\n')
        f.write(f'd:So/RTION/SID                   = 390.0 mm\n')
        f.write(f'u:So/RTION/ParticlesPerHistory    = {particlesPerHistory}\n')
        f.write(f'dc:So/RTION/ImgCenterX       = {img_center_L:.4f} mm\n')
        f.write(f'dc:So/RTION/ImgCenterY       = {img_center_P:.4f} mm\n')
        f.write(f'dc:So/RTION/ImgCenterZ       = {img_center_S:.4f} mm\n')
        f.write('dc:So/RTION/IsoCenterX       = 0 mm\n')
        f.write('dc:So/RTION/IsoCenterY       = 0 mm\n')
        f.write('dc:So/RTION/IsoCenterZ       = 0 mm\n')
        f.write('dc:So/RTION/CollimatorAngle     = 0 deg\n')
        f.write('dc:So/RTION/GantryAngle         = 0 deg\n')
        f.write('dc:So/RTION/PatientSupportAngle = 0 deg\n')
        f.write('dc:So/RTION/Iec2DicomAngle      = 0 deg\n')
        # Shift source spots from CT center to isocenter
        f.write('d:So/RTION/ShiftX = So/RTION/IsoCenterX - So/RTION/ImgCenterX mm\n')
        f.write('d:So/RTION/ShiftY = So/RTION/IsoCenterY - So/RTION/ImgCenterY mm\n')
        f.write('d:So/RTION/ShiftZ = So/RTION/IsoCenterZ - So/RTION/ImgCenterZ mm\n')
        # Rotations: same four as on the geometry component
        f.write('d:So/RTION/RotCollimator      = So/RTION/CollimatorAngle deg\n')
        f.write('d:So/RTION/RotGantry          = So/RTION/GantryAngle deg\n')
        f.write('d:So/RTION/RotPatientSupport  = -1.0 * So/RTION/PatientSupportAngle deg\n')
        f.write('d:So/RTION/RotIEC2DICOM       = 90 deg\n\n')

      else:
        # === Simple beam path (no RT Ion Plan file) ===

        # Isocenter in LPS (scanner coords) — Slicer RAS -> DICOM/LPS: flip X and Y
        isoCenterLpsX = -isocenter[0]
        isoCenterLpsY = -isocenter[1]
        isoCenterLpsZ =  isocenter[2]

        # TsDicomPatient with Trans=0 centers the CT bounding box at World(0,0,0).
        # TOPAS World coords = scanner_LPS - ctCenter_LPS.
        # Beam anchor in TOPAS World = iso_LPS - ctCenter_LPS  (matches RTI: IsoCenter - ImgCenter).
        if ctData is not None:
          volumeNode = ctData['volumeNode']
          dims = ctData['dimensions']
          ijkToRas = vtk.vtkMatrix4x4()
          volumeNode.GetIJKToRASMatrix(ijkToRas)
          centerIjk = [(dims[0]-1)/2.0, (dims[1]-1)/2.0, (dims[2]-1)/2.0, 1.0]
          centerRas = [0.0, 0.0, 0.0, 1.0]
          ijkToRas.MultiplyPoint(centerIjk, centerRas)
          ctCenterLpsX = -centerRas[0]
          ctCenterLpsY = -centerRas[1]
          ctCenterLpsZ =  centerRas[2]
        else:
          ctCenterLpsX = isoCenterLpsX
          ctCenterLpsY = isoCenterLpsY
          ctCenterLpsZ = isoCenterLpsZ

        beamTransX = isoCenterLpsX - ctCenterLpsX
        beamTransY = isoCenterLpsY - ctCenterLpsY
        beamTransZ = isoCenterLpsZ - ctCenterLpsZ

        logging.info(f"Isocenter  (LPS): [{isoCenterLpsX:.2f}, {isoCenterLpsY:.2f}, {isoCenterLpsZ:.2f}]")
        logging.info(f"CT center  (LPS): [{ctCenterLpsX:.2f}, {ctCenterLpsY:.2f}, {ctCenterLpsZ:.2f}]")
        logging.info(f"Beam trans (TOPAS World): [{beamTransX:.2f}, {beamTransY:.2f}, {beamTransZ:.2f}]")

        # Patient at World origin (CT center = World(0,0,0))
        f.write("# Patient from DICOM CT\n")
        f.write('s:Ge/Patient/Type               = "TsDicomPatient"\n')
        f.write('s:Ge/Patient/Parent             = "World"\n')
        f.write(f's:Ge/Patient/DicomDirectory    = "{dicomDirectoryTopas}"\n')
        f.write('sv:Ge/Patient/DicomModalityTags = 1 "CT"\n')
        f.write('d:Ge/Patient/TransX = 0. mm\n')
        f.write('d:Ge/Patient/TransY = 0. mm\n')
        f.write('d:Ge/Patient/TransZ = 0. mm\n')
        f.write('d:Ge/Patient/RotX    = 0. deg\n')
        f.write('d:Ge/Patient/RotY    = 0. deg\n')
        f.write('d:Ge/Patient/RotZ    = 0. deg\n\n')

        # IEC_F: fixed frame translated to isocenter + IEC-to-DICOM correction (RotX=90°).
        #   TransX/Y/Z = iso_LPS - ctCenter_LPS  (same formula as TsRTIonComponents).
        f.write("# IEC_F: Fixed frame (isocenter translation + IEC-to-DICOM correction)\n")
        f.write('s:Ge/IEC_F/Type   = "Group"\n')
        f.write('s:Ge/IEC_F/Parent = "World"\n')
        f.write(f'd:Ge/IEC_F/TransX = {beamTransX} mm\n')
        f.write(f'd:Ge/IEC_F/TransY = {beamTransY} mm\n')
        f.write(f'd:Ge/IEC_F/TransZ = {beamTransZ} mm\n')
        f.write('d:Ge/IEC_F/RotX   = 90. deg\n\n')

        # IEC_G: gantry rotates about Y
        f.write("# IEC_G: Gantry rotation\n")
        f.write('s:Ge/IEC_G/Type   = "Group"\n')
        f.write('s:Ge/IEC_G/Parent = "IEC_F"\n')
        f.write('d:Ge/IEC_G/TransX = 0. mm\n')
        f.write('d:Ge/IEC_G/TransY = 0. mm\n')
        f.write('d:Ge/IEC_G/TransZ = 0. mm\n')
        f.write(f'd:Ge/IEC_G/RotY   = {gantryAngle} deg\n\n')

        # IEC_B: beam limiting device frame. Collimator rotates about Z
        f.write("# IEC_B: Beam limiting device frame\n")
        f.write('s:Ge/IEC_B/Type   = "Group"\n')
        f.write('s:Ge/IEC_B/Parent = "IEC_G"\n')
        f.write('d:Ge/IEC_B/TransX = 0. mm\n')
        f.write('d:Ge/IEC_B/TransY = 0. mm\n')
        f.write('d:Ge/IEC_B/TransZ = 0. mm\n')
        f.write(f'd:Ge/IEC_B/RotZ   = {collimatorAngle} deg\n\n')

        logging.info(f"Jaw values used for TOPAS: x1={x1Jaw} x2={x2Jaw} y1={y1Jaw} y2={y2Jaw} mm")

        # Jaw geometry constants
        jawLZ_mm  = 78.0   # jaw block thickness along beam axis
        stoUSD_mm = 400.0  # source-to-upstream-surface distance for both jaws

        # Unit conversions for TOPAS (uses cm)
        sadCm     = sad / 10.0
        x1JawCm   = x1Jaw / 10.0
        x2JawCm   = x2Jaw / 10.0
        y1JawCm   = y1Jaw / 10.0
        y2JawCm   = y2Jaw / 10.0

        # TsJaws always places its blocks at Z=0 of its parent — SourceToUpstreamSurfaceDistance
        # is only used for the jaw-opening projection formula, not for positioning.
        # We use one intermediate Group per jaw (JawXFrame / JawYFrame) translated to the
        # correct depth so the blocks end up at the right position along the beam axis.
        # Jaw center Z in IEC_B = -(SAD - StoUSD - LZ/2), since source is at Z=-SAD.
        jawXFrameZ = -(sad - stoUSD_mm - jawLZ_mm / 2.0)
        jawYFrameZ = jawXFrameZ - jawLZ_mm  # downstream of JawX by LZ

        # Intermediate frames — one per jaw, each at its correct depth in IEC_B.
        f.write("# JawXFrame / JawYFrame: position each jaw at the correct depth along the beam.\n")
        f.write("# TsJaws places its blocks at Z=0 of the parent, so we shift the parent.\n")
        f.write('s:Ge/JawXFrame/Type   = "Group"\n')
        f.write('s:Ge/JawXFrame/Parent = "IEC_B"\n')
        f.write('d:Ge/JawXFrame/TransX = 0. mm\n')
        f.write('d:Ge/JawXFrame/TransY = 0. mm\n')
        f.write(f'd:Ge/JawXFrame/TransZ = {jawXFrameZ} mm\n')
        # RotX=180° flips local Z so it points toward the source (IEC_B -Z).
        # TsJaws assumes source is at local +Z; without this the jaw trapezoid is inverted.
        f.write('d:Ge/JawXFrame/RotX   = 180. deg\n\n')

        f.write('s:Ge/JawYFrame/Type   = "Group"\n')
        f.write('s:Ge/JawYFrame/Parent = "IEC_B"\n')
        f.write('d:Ge/JawYFrame/TransX = 0. mm\n')
        f.write('d:Ge/JawYFrame/TransY = 0. mm\n')
        f.write(f'd:Ge/JawYFrame/TransZ = {jawYFrameZ} mm\n')
        # RotX=180° fixes jaw orientation (source at local +Z).
        # RotZ=90° maps local X onto IEC_B Y so TsJaws (which always travels along local X)
        # collimates in the Y direction.
        f.write('d:Ge/JawYFrame/RotX   = 180. deg\n')
        f.write('d:Ge/JawYFrame/RotZ   = 90. deg\n\n')

        # X jaw pair (upstream).  JawTravelAxis="X": jaws move in X to define field width.
        f.write("# X Jaw pair\n")
        f.write('s:Ge/JawX/Type                                  = "TsJaws"\n')
        f.write('s:Ge/JawX/Parent                                = "JawXFrame"\n')
        f.write('s:Ge/JawX/Material                              = "G4_W"\n')
        f.write('s:Ge/JawX/JawTravelAxis                         = "X"\n')
        f.write(f'dc:Ge/JawX/PositiveFieldSetting                 = {x2JawCm} cm\n')
        f.write(f'dc:Ge/JawX/NegativeFieldSetting                 = {x1JawCm} cm\n')
        f.write('d:Ge/JawX/LX                                    = 20. cm\n')  # travel direction — jaw block thickness
        f.write('d:Ge/JawX/LY                                    = 42. cm\n')  # perpendicular — covers full ±200 mm beam extent
        f.write('d:Ge/JawX/LZ                                    = 7.80 cm\n')
        f.write(f'dc:Ge/JawX/SourceToUpstreamSurfaceDistance       = {stoUSD_mm / 10.0} cm\n')
        f.write(f'd:Ge/JawX/SAD                                   = {sadCm} cm\n')
        f.write('s:Ge/JawX/DrawingStyle                          = "Solid"\n\n')

        # Y jaw pair (downstream).  JawTravelAxis="Y": jaws move in Y to define field height.
        f.write("# Y Jaw pair\n")
        f.write('s:Ge/JawY/Type                                  = "TsJaws"\n')
        f.write('s:Ge/JawY/Parent                                = "JawYFrame"\n')
        f.write('s:Ge/JawY/Material                              = "G4_W"\n')
        f.write('s:Ge/JawY/JawTravelAxis                         = "Y"\n')
        f.write(f'dc:Ge/JawY/PositiveFieldSetting                 = {y2JawCm} cm\n')
        f.write(f'dc:Ge/JawY/NegativeFieldSetting                 = {y1JawCm} cm\n')
        f.write('d:Ge/JawY/LX                                    = 20. cm\n')  # along JawTravelAxis (Y) — jaw block thickness
        f.write('d:Ge/JawY/LY                                    = 42. cm\n')  # perpendicular (X) — covers full ±200 mm beam extent
        f.write('d:Ge/JawY/LZ                                    = 7.80 cm\n')
        f.write(f'dc:Ge/JawY/SourceToUpstreamSurfaceDistance       = {stoUSD_mm / 10.0} cm\n')
        f.write(f'd:Ge/JawY/SAD                                   = {sadCm} cm\n')
        f.write('s:Ge/JawY/DrawingStyle                          = "Solid"\n\n')

        f.write("# Proton Beam Source\n")
        f.write('s:So/Beam/Type                    = "Beam"\n')
        f.write("# BeamSource: upstream at -SAD along Z\n")
        f.write('s:Ge/BeamSource/Type   = "Group"\n')
        f.write('s:Ge/BeamSource/Parent = "IEC_B"\n')
        f.write('d:Ge/BeamSource/TransX = 0. mm\n')
        f.write('d:Ge/BeamSource/TransY = 0. mm\n')
        f.write(f'd:Ge/BeamSource/TransZ = {-sad} mm\n\n')

        f.write('s:So/Beam/Component               = "BeamSource"\n')
        f.write(f's:So/Beam/BeamParticle           = "{beamParticle}"\n')
        f.write(f'd:So/Beam/BeamEnergy             = {energy} MeV\n')
        f.write('u:So/Beam/BeamEnergySpread        = 0.01\n')
        # Diverging point source with Gaussian angular distribution.
        # Cutoff = jaw opening + small margin so jaws define the field edge.
        # Jaws do all hard collimation; the wide spread just ensures uniform beam fill.
        margin_mm = 15.0
        cutoffX = max(abs(x1Jaw), abs(x2Jaw)) + margin_mm
        cutoffY = max(abs(y1Jaw), abs(y2Jaw)) + margin_mm
        angCutoffX = math.degrees(math.atan(cutoffX / sad))
        angCutoffY = math.degrees(math.atan(cutoffY / sad))
        f.write('s:So/Beam/BeamPositionDistribution  = "None"\n')
        f.write('s:So/Beam/BeamAngularDistribution   = "Gaussian"\n')
        f.write('d:So/Beam/BeamAngularCutoffX        = {:.6f} deg\n'.format(angCutoffX))
        f.write('d:So/Beam/BeamAngularCutoffY        = {:.6f} deg\n'.format(angCutoffY))
        f.write('d:So/Beam/BeamAngularSpreadX        = {:.6f} deg\n'.format(angCutoffX))
        f.write('d:So/Beam/BeamAngularSpreadY        = {:.6f} deg\n'.format(angCutoffY))

        f.write(f'i:So/Beam/NumberOfHistoriesInRun  = {numberOfHistories}\n\n')

      # Dose scoring - Output as DICOM RT Dose
      f.write("# Dose Scoring (DICOM RT Dose Output)\n")
      scorerComponent = "Patient/RTDoseGrid" if _hasDoseGrid else "Patient"
      f.write('s:Sc/DoseScorer/Quantity                  = "DoseToWater"\n')
      f.write(f's:Sc/DoseScorer/Component                 = "{scorerComponent}"\n')
      f.write('b:Sc/DoseScorer/PreCalculateStoppingPowerRatios = "True"\n')
      f.write('s:Sc/DoseScorer/IfOutputFileAlreadyExists = "Overwrite"\n')
      f.write(f's:Sc/DoseScorer/OutputFile               = "{doseFilePathTopas}"\n')
      f.write('s:Sc/DoseScorer/OutputType                = "DICOM"\n')
      f.write(f's:Sc/DoseScorer/DicomPatientDirectory    = "{dicomDirectoryTopas}"\n')
      f.write('s:Sc/DoseScorer/DoseUnits                 = "GY"\n')
      f.write('s:Sc/DoseScorer/DoseType                  = "PHYSICAL"\n')
      f.write('s:Sc/DoseScorer/DoseSummationType         = "PLAN"\n\n')

      # Visualization (optional, disabled by default)
      f.write("# Graphics (disabled for batch mode)\n")
      f.write('b:Gr/Enable = "False"\n\n')

      # Run settings
      f.write("# Run Settings\n")
      f.write(f'i:Ts/NumberOfThreads = {beamProperties.get("numberOfThreads", 1)}\n')
      f.write('i:Ts/ShowHistoryCountAtInterval = 100000\n')
      f.write('b:Ts/PauseBeforeQuit = "False"\n')

    logging.info(f"Created TOPAS input file: {inputFilePath}")
    return inputFilePath

  #------------------------------------------------------------------------------
  @staticmethod
  def executeTopasSimulation(inputFilePath, topasBinaryPath, workingDirectory, g4dataPath=None, timeout=None):
    """Execute TOPAS simulation

    Args:
      timeout: Maximum time in seconds to wait for the simulation (None = no limit)
    """
    # Check for TOPAS binary existence using command line (cross-platform)
    if TopasDoseEngineUtil.isWindows():
      checkCmd = ['wsl', '--', 'test', '-x', TopasDoseEngineUtil.winToWslPath(topasBinaryPath)]
    else:
      checkCmd = ['test', '-x', topasBinaryPath]
    result = subprocess.run(checkCmd)
    if result.returncode != 0:
      raise RuntimeError(f"TOPAS binary not found or not executable at: {topasBinaryPath}")

    # Change to working directory for simulation
    originalDir = os.getcwd()
    os.chdir(workingDirectory)

    try:
      # Run TOPAS using cross-platform utility
      cmd = [topasBinaryPath, inputFilePath]
      env_vars = {}
      if g4dataPath:
        env_vars["TOPAS_G4_DATA_DIR"] = g4dataPath
      # Prepend TOPAS's lib/ so its bundled Qt libs win over the parent process's Qt (e.g. Slicer's), avoiding mixed-version symbol errors on Linux.
      if not TopasDoseEngineUtil.isWindows():
        topasLibDir = os.path.realpath(os.path.join(os.path.dirname(topasBinaryPath), '..', 'lib'))
        existing = os.environ.get('LD_LIBRARY_PATH', '')
        env_vars['LD_LIBRARY_PATH'] = f"{topasLibDir}:{existing}" if existing else topasLibDir
      result = TopasDoseEngineUtil.runCommandCrossPlatform(cmd, cwd=workingDirectory, timeout=timeout, env_vars=env_vars)

      if result.returncode != 0:
        logging.error("TOPAS simulation failed.")
        logging.error(f"STDOUT: {result.stdout}")
        logging.error(f"STDERR: {result.stderr}")
        raise RuntimeError(f"TOPAS simulation failed.\nSTDOUT:\n{result.stdout}\nSTDERR:\n{result.stderr}")

      logging.info("TOPAS simulation completed successfully")
      logging.info(f"Output: {result.stdout}")

    finally:
      os.chdir(originalDir)

    return True

  #------------------------------------------------------------------------------
  @staticmethod
  def findDoseOutputFile(workingDirectory):
    """Find the TOPAS DICOM RT Dose output file.

    Args:
      workingDirectory: Directory containing the dose output file

    Returns:
      Path to the dose DICOM file
    """
    # TOPAS outputs DICOM with the base name we specified
    doseFilePath = os.path.join(workingDirectory, 'dose_output.dcm')

    # Also check for alternative naming patterns TOPAS might use
    if not os.path.exists(doseFilePath):
      doseFiles = glob.glob(os.path.join(workingDirectory, 'dose_output*.dcm'))
      if not doseFiles:
        doseFiles = glob.glob(os.path.join(workingDirectory, '*RTDOSE*.dcm'))
      if doseFiles:
        doseFilePath = doseFiles[0]
      else:
        raise RuntimeError(f"Dose output DICOM file not found in: {workingDirectory}")

    logging.info(f"Found DICOM RT Dose file: {doseFilePath}")
    return doseFilePath

  #------------------------------------------------------------------------------
  @staticmethod
  def findOriginalDicomFiles(volumeNode):
    """Return the list of original DICOM file paths for a CT volume node loaded
    from Slicer's DICOM database, or None if not available.

    Using the original files preserves the Frame of Reference UID that matches
    any associated RT Plan, avoiding the need for IgnoreInconsistentFrameOfReferenceUID.

    Args:
      volumeNode: vtkMRMLScalarVolumeNode loaded from DICOM

    Returns:
      List of file paths if found, None otherwise
    """
    try:
      if not slicer.dicomDatabase.isOpen:
        return None
      shNode = slicer.mrmlScene.GetSubjectHierarchyNode()
      itemID = shNode.GetItemByDataNode(volumeNode)
      frameOfRefUID = shNode.GetItemAttribute(itemID, 'DICOM.FrameOfReferenceUID')
      modality = shNode.GetItemAttribute(itemID, 'DICOM.Modality')
      if not frameOfRefUID or not modality:
        return None
      for patient in slicer.dicomDatabase.patients():
        for study in slicer.dicomDatabase.studiesForPatient(patient):
          for series in slicer.dicomDatabase.seriesForStudy(study):
            files = slicer.dicomDatabase.filesForSeries(series)
            if not files:
              continue
            if (slicer.dicomDatabase.fileValue(files[0], '0020,0052') == frameOfRefUID and
                slicer.dicomDatabase.fileValue(files[0], '0008,0060') == modality):
              logging.info(f"Found {len(files)} original DICOM CT files in Slicer database")
              return list(files)
      return None
    except Exception as e:
      logging.warning(f"Could not retrieve original DICOM files: {e}")
      return None

  #------------------------------------------------------------------------------
  @staticmethod
  def runTopasSimulation(ctData, beamProperties, topasBinaryPath, g4dataPath=None, topasDirectoryPath=None, planFilePath=None, timeout=None):
    """Complete TOPAS simulation workflow using DICOM input.

    Args:
      ctData: CT data dictionary from extractCTData()
      beamProperties: Beam properties dictionary from extractBeamProperties()
      topasBinaryPath: Path to TOPAS executable
      g4dataPath: Path to Geant4 data directory (optional)
      topasDirectoryPath: Path to TOPAS installation directory
      planFilePath: Path to RT Ion Plan DICOM file (optional, enables TsRTIonSource)
      timeout: Maximum time in seconds to wait for the simulation (None = no limit)

    Returns:
      Tuple of (doseFilePath, workingDirectory) where doseFilePath is the path
      to the DICOM RT Dose file and workingDirectory is the temp directory to
      clean up after loading.
    """
    workingDirectory = tempfile.mkdtemp(prefix='topas_dose_')

    # Try to reuse the original DICOM files from Slicer's database.
    # This preserves the Frame of Reference UID so it matches any RT Plan.
    # Copy only the CT files into a clean subdirectory even when originals are found —
    # the source directory may contain mixed DICOM modalities (CT + RT Struct/Plan/Dose)
    # which causes TOPAS TsDicomPatient to fail with "Failed to sort files".
    dicomDirectory = os.path.join(workingDirectory, 'dicom_ct')
    os.makedirs(dicomDirectory, exist_ok=True)
    originalFiles = TopasDoseEngineUtil.findOriginalDicomFiles(ctData['volumeNode'])
    if originalFiles:
      logging.info(f"Copying {len(originalFiles)} original CT DICOM files to clean temp directory...")
      for f in originalFiles:
        shutil.copy2(f, dicomDirectory)
      logging.info("Done copying — skipping CT export step.")
    else:
      logging.info("No original DICOM found — exporting CT as DICOM series for TOPAS...")
      TopasDoseEngineUtil.exportCTAsDicomSeries(ctData, dicomDirectory)

    # Determine number of beams to simulate
    nBeams = 1
    if planFilePath:
      try:
        import pydicom as _pd
        _ds = _pd.dcmread(planFilePath, stop_before_pixels=True)
        nBeams = len(_ds.IonBeamSequence)
        logging.info(f"RT Ion Plan has {nBeams} beam(s)")
      except Exception as _e:
        logging.warning(f"Could not read beam count from plan: {_e}")

    beamDoseFiles = []
    for beamIdx in range(1, nBeams + 1):
      beamWorkDir = os.path.join(workingDirectory, f'beam{beamIdx}')
      os.makedirs(beamWorkDir, exist_ok=True)

      logging.info(f"Creating TOPAS input file for beam {beamIdx}/{nBeams}...")
      inputFilePath = TopasDoseEngineUtil.createTopasInputFileDicom(
        dicomDirectory=dicomDirectory,
        beamProperties=beamProperties,
        workingDirectory=beamWorkDir,
        topasDirectory=topasDirectoryPath,
        ctData=ctData,
        planFilePath=planFilePath,
        beamNumber=beamIdx
      )

      logging.info(f"Executing TOPAS simulation for beam {beamIdx}/{nBeams}...")
      TopasDoseEngineUtil.executeTopasSimulation(inputFilePath, topasBinaryPath, beamWorkDir, g4dataPath, timeout)

      logging.info(f"Locating dose output for beam {beamIdx}...")
      beamDoseFiles.append(TopasDoseEngineUtil.findDoseOutputFile(beamWorkDir))

    # Sum doses from all beams into a single RT Dose file
    if nBeams == 1:
      doseFilePath = beamDoseFiles[0]
    else:
      logging.info(f"Summing doses from {nBeams} beams...")
      doseFilePath = TopasDoseEngineUtil._sumDoseDicomFiles(beamDoseFiles, workingDirectory)

    return doseFilePath, workingDirectory

  #------------------------------------------------------------------------------
  @staticmethod
  def _sumDoseDicomFiles(doseFilePaths, workingDirectory):
    """Sum multiple DICOM RT Dose files into one.

    Each file's pixel values are converted to physical dose (Gy) via its
    DoseGridScaling, summed, then stored back with a new scaling factor.
    The first file's metadata (geometry, SOP UIDs, etc.) is used as the base.
    """
    import pydicom, numpy as np
    datasets = [pydicom.dcmread(p) for p in doseFilePaths]

    # Convert each to float dose array in Gy
    dose_sum = None
    for ds in datasets:
      scaling = float(ds.DoseGridScaling)
      arr = ds.pixel_array.astype(np.float64) * scaling
      dose_sum = arr if dose_sum is None else dose_sum + arr

    # Store back into the first dataset with a fresh SOP Instance UID
    out = datasets[0]
    out.SOPInstanceUID = pydicom.uid.generate_uid()
    max_dose = float(dose_sum.max())
    new_scaling = max_dose / (2**32 - 1) if max_dose > 0 else 1e-6
    new_scaling = max(new_scaling, 1e-9)
    pixel_data = (dose_sum / new_scaling).astype(np.uint32)

    out.DoseGridScaling = new_scaling
    out.BitsAllocated = 32
    out.BitsStored = 32
    out.HighBit = 31
    out.PixelRepresentation = 0
    out.PixelData = pixel_data.tobytes()
    out.NumberOfFrames = pixel_data.shape[0]

    sumPath = os.path.join(workingDirectory, 'dose_sum.dcm')
    out.save_as(sumPath)
    logging.info(f"Summed dose written to {sumPath}")
    return sumPath

  #------------------------------------------------------------------------------
  @staticmethod
  def loadDoseResultAsVolume(resultDoseVolumeNode, doseFilePath, workingDirectory, particlesPerHistory=1.0, ctVolumeNode=None):
    """
    Load TOPAS dose result into Slicer volume node using SlicerRT's DICOM RT import.

    Uses the same DicomRtImportExport logic that runs when importing through the
    DICOM module, so DoseGridScaling and all RT-specific metadata are handled properly.

    Args:
      resultDoseVolumeNode: Slicer volume node to store the dose
      doseFilePath: Path to the DICOM RT Dose file
      workingDirectory: Temporary working directory to clean up after loading
    """
    try:
      # Use SlicerRT's DicomRtImportExport logic (same path as manual DICOM import)
      vtkFileList = vtk.vtkStringArray()
      vtkFileList.InsertNextValue(doseFilePath)

      loadablesCollection = vtk.vtkCollection()
      slicer.modules.dicomrtimportexport.logic().ExamineForLoad(vtkFileList, loadablesCollection)

      if loadablesCollection.GetNumberOfItems() == 0:
        raise RuntimeError(f"DicomRtImportExport could not recognize dose file: {doseFilePath}")

      # Track existing volume nodes to identify the newly loaded one
      existingNodeIDs = set()
      for i in range(slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLScalarVolumeNode')):
        existingNodeIDs.add(slicer.mrmlScene.GetNthNodeByClass(i, 'vtkMRMLScalarVolumeNode').GetID())

      # Load through the RT import logic
      vtkLoadable = loadablesCollection.GetItemAsObject(0)
      success = slicer.modules.dicomrtimportexport.logic().LoadDicomRT(vtkLoadable)
      if not success:
        raise RuntimeError(f"Failed to load dose DICOM file via DicomRtImportExport: {doseFilePath}")

      # Find the newly created dose volume node
      tempDoseNode = None
      for i in range(slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLScalarVolumeNode')):
        node = slicer.mrmlScene.GetNthNodeByClass(i, 'vtkMRMLScalarVolumeNode')
        if node.GetID() not in existingNodeIDs:
          tempDoseNode = node
          break

      if not tempDoseNode:
        raise RuntimeError("Could not find loaded dose volume node after DicomRtImportExport")

      # Copy geometry and voxel data
      doseImageDataCopy = vtk.vtkImageData()
      doseImageDataCopy.DeepCopy(tempDoseNode.GetImageData())
      resultDoseVolumeNode.SetAndObserveImageData(doseImageDataCopy)

      # Use CT volume geometry (direction + origin) instead of TOPAS DICOM coordinates.
      # TOPAS writes IPP/orientation in its internal world frame, not DICOM LPS — this
      # causes a 90° X rotation and wrong position. The dose grid matches the CT extent,
      # so copying CT geometry and computing origin from the reference RTDose is correct.
      if ctVolumeNode is not None:
        # Copy direction cosines from CT
        directions = vtk.vtkMatrix4x4()
        ctVolumeNode.GetIJKToRASMatrix(directions)
        resultDoseVolumeNode.SetIJKToRASMatrix(directions)
        resultDoseVolumeNode.SetSpacing(ctVolumeNode.GetSpacing())

        # Compute correct origin: use CT origin (dose grid starts at same corner as CT
        # when CloneRTDoseGridFrom uses the full-CT-extent rtdose)
        resultDoseVolumeNode.SetOrigin(ctVolumeNode.GetOrigin())
        logging.info(f"Dose geometry copied from CT: origin={ctVolumeNode.GetOrigin()}, spacing={ctVolumeNode.GetSpacing()}")
      else:
        resultDoseVolumeNode.CopyOrientation(tempDoseNode)

      # Scale by PPH to convert from dose-per-simulated-primary to clinical dose
      if particlesPerHistory != 1.0:
        import numpy as np
        arr = slicer.util.arrayFromVolume(resultDoseVolumeNode).astype(np.float32)
        arr *= float(particlesPerHistory)
        slicer.util.updateVolumeFromArray(resultDoseVolumeNode, arr)
        logging.info(f"Dose scaled by PPH={particlesPerHistory}, new max={arr.max():.4f} Gy")

      # Transfer the display node from tempDoseNode to resultDoseVolumeNode.
      # DicomRtImportExport fully configures it (window/level, colormap, dose settings).
      # Display nodes are independent scene objects — removing tempDoseNode does NOT
      # remove its display node, so the transfer is safe.
      tempDisplayNode = tempDoseNode.GetDisplayNode()
      if tempDisplayNode:
        resultDoseVolumeNode.AddAndObserveDisplayNodeID(tempDisplayNode.GetID())
      else:
        resultDoseVolumeNode.CreateDefaultDisplayNodes()

      # Diagnostic logging
      doseRange = doseImageDataCopy.GetScalarRange()
      logging.info(f"Dose scalar type: {doseImageDataCopy.GetScalarTypeAsString()}, dims: {doseImageDataCopy.GetDimensions()}")
      logging.info(f"Dose range: min={doseRange[0]:.6g}, max={doseRange[1]:.6g}, total voxels={doseImageDataCopy.GetNumberOfPoints()}")
      logging.info(f"Result node origin: {resultDoseVolumeNode.GetOrigin()}")
      logging.info(f"Result node spacing: {resultDoseVolumeNode.GetSpacing()}")
      logging.info(f"Dose volume loaded successfully from: {doseFilePath}")

      # Remove the temporary node — its display node stays in the scene,
      # now referenced by resultDoseVolumeNode
      slicer.mrmlScene.RemoveNode(tempDoseNode)

    finally:
      if os.path.exists(workingDirectory):
        shutil.rmtree(workingDirectory)
        logging.info(f"Cleaned up temporary directory: {workingDirectory}")

  #------------------------------------------------------------------------------
  @staticmethod
  def assignZeroDoseVolume(resultDoseVolumeNode, ctVolumeNode):
    """
    Fill resultDoseVolumeNode with an all-zero dose matching the CT geometry.

    Used when an RT Ion Plan file's combined dose has already been computed and assigned
    to another beam in the same plan (the DICOM file's own beam count already covers every
    beam, so additional Slicer beam placeholders pointing at the same file must contribute
    nothing, or the plan-level dose sum would double-count it).
    """
    directions = vtk.vtkMatrix4x4()
    ctVolumeNode.GetIJKToRASMatrix(directions)
    resultDoseVolumeNode.SetIJKToRASMatrix(directions)
    resultDoseVolumeNode.SetSpacing(ctVolumeNode.GetSpacing())
    resultDoseVolumeNode.SetOrigin(ctVolumeNode.GetOrigin())

    dims = ctVolumeNode.GetImageData().GetDimensions()
    imageData = vtk.vtkImageData()
    imageData.SetDimensions(dims)
    imageData.AllocateScalars(vtk.VTK_FLOAT, 1)
    imageData.GetPointData().GetScalars().Fill(0)
    resultDoseVolumeNode.SetAndObserveImageData(imageData)
    resultDoseVolumeNode.CreateDefaultDisplayNodes()
    logging.info("Assigned zero dose (RT Ion Plan dose already accounted for by another beam in this plan)")

  #------------------------------------------------------------------------------
  @staticmethod
  def isWindows():
    """Detect if running on Windows."""
    return os.name == 'nt'

  #------------------------------------------------------------------------------
  @staticmethod
  def winToWslPath(winPath):
    """Convert a Windows path to a WSL path."""
    # Example: C:\Users\User\file.txt -> /mnt/c/Users/User/file.txt
    drive, path = os.path.splitdrive(winPath)
    if not drive:
      return winPath.replace('\\', '/')
    driveLetter = drive[0].lower()
    wslPath = f"/mnt/{driveLetter}{path.replace('\\', '/')}"
    return wslPath

  #------------------------------------------------------------------------------
  @staticmethod
  def runCommandCrossPlatform(cmd, cwd=None, captureOutput=True, text=True, timeout=None, env_vars=None):
    """Run a command in WSL if on Windows, or natively otherwise. Optionally export env_vars in WSL."""
    # Ensure cwd is a valid directory if provided
    if cwd and not os.path.isdir(cwd):
      raise NotADirectoryError(f"The working directory '{cwd}' does not exist or is not a directory.")
    if TopasDoseEngineUtil.isWindows():
      # Convert all paths in cmd to WSL format if they are absolute Windows paths
      def convertArg(arg):
        if isinstance(arg, str) and (os.path.isabs(arg) and ':' in arg):
          return TopasDoseEngineUtil.winToWslPath(arg)
        return arg
      cmdWsl = [convertArg(a) for a in cmd]
      # Prepare export string if env_vars provided
      export_str = ''
      if env_vars:
        export_str = ' '.join([f"export {k}='{v}';" for k, v in env_vars.items()])
      if cwd:
        wslCwd = TopasDoseEngineUtil.winToWslPath(cwd)
        cmdWsl = ['wsl', '--', 'bash', '-c', f"cd '{wslCwd}' && {export_str} {' '.join(cmdWsl)}"]
        return subprocess.run(cmdWsl, capture_output=captureOutput, text=text, timeout=timeout)
      else:
        cmdWsl = ['wsl', '--', 'bash', '-c', f"{export_str} {' '.join(cmdWsl)}"]
        return subprocess.run(cmdWsl, capture_output=captureOutput, text=text, timeout=timeout)
    else:
      env = {**os.environ, **env_vars} if env_vars else None
      return subprocess.run(cmd, cwd=cwd, capture_output=captureOutput, text=text, timeout=timeout, env=env)
