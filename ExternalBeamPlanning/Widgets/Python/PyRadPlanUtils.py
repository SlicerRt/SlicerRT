import numpy as np
import logging
import slicer
import sitkUtils
import SimpleITK as sitk
import vtkSegmentationCorePython as vtkSegmentationCore

#------------------------------------------------------------------------------
def prepareCt(beamNode):
  from pyRadPlan.ct import create_ct

  # Get sitk image from Slicer
  planNode = beamNode.GetParentPlanNode()
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  image = sitkUtils.PullVolumeFromSlicer(referenceVolumeNode)

  # Create ct object from sitk image
  return create_ct(cube_hu=image)

#------------------------------------------------------------------------------
def prepareCst(beamNode, ct, needBody=False):
  from pyRadPlan.cst import StructureSet, create_voi, validate_cst

  # Get segmentations from Slicer
  planNode = beamNode.GetParentPlanNode()
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  segmentationNode = planNode.GetSegmentationNode()
  segmentation = segmentationNode.GetSegmentation()

  if planNode.GetTargetSegmentID() == "":
    raise ValueError("No target segment selected in plan node. Please select a target segment and try again.")
  if planNode.GetTargetSegmentID() == planNode.GetBodySegmentID():
    raise ValueError("Target segment and body segment are the same. Please select a different target segment or body segment and try again.")

  # Get arrays for all segments in segmentation node (convert to binary labelmap if necessary)
  segmentArrays = getSegmentArrays(segmentationNode, referenceVolumeNode)

  # Create VOI objects from segments
  vois = []
  for id in segmentation.GetSegmentIDs():
    segmentArray = segmentArrays[id]['array']
    segmentName = segmentArrays[id]['name']
    voiType = 'OAR'
    if id == planNode.GetBodySegmentID():
      voiType = 'EXTERNAL'
    if id == planNode.GetTargetSegmentID(): # overwrite if also selected as body
      voiType = 'TARGET'
    voi = create_voi(voi_type=voiType, name=segmentName, ct_image=ct, mask=segmentArray)
    vois.append(voi)

  # Create cst object from VOIs and ct
  cst = StructureSet(vois=vois, ct_image=ct)

  if needBody:
    createAndLoadBodySegment(planNode, cst)
    
  return validate_cst(cst)

#------------------------------------------------------------------------------
def preparePln(beamNode, ct, doseGridFromBeamNode=False):
  from pyRadPlan.plan import create_pln
  
  # Get isocenter position in RAS coordinates from Slicer
  planNode = beamNode.GetParentPlanNode()
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  ijkToRASDirections = np.eye(3)
  referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
  isocenter = [0]*3
  planNode.GetIsocenterPosition(isocenter)
  isocenter = ijkToRASDirections @ np.array(isocenter)

  # Get dose grid by resampling ct grid to resolution defined in beamNode or planNode
  if doseGridFromBeamNode:
    # Get dose grid specs saved in beamNode (corresponding to saved dose influence matrix)
    doseGridSpacing = beamNode.GetDoseGridSpacing()
    doseGridDimensions = beamNode.GetDoseGridDim()
    # Check if dose influence matrix is available in beamNode
    if beamNode.GetDoseInfluenceMatrixFieldData() is None:
      logging.warning("No dose influence matrix found in beamNode. Using planNode's dose grid spacing.")
      doseGridSpacing = planNode.GetDoseGridSpacing()
    # Check if dose grid spacing was set in beamNode (default: {-1, -1, -1})
    if doseGridSpacing[0] < 0 or doseGridSpacing[1] < 0 or doseGridSpacing[2] < 0 :
      logging.warning("Dose grid spacing in beamNode is invalid. Using planNode's dose grid spacing.")
      doseGridSpacing = planNode.GetDoseGridSpacing()
    doseGrid = ct.grid.model_copy().resample(target_resolution=doseGridSpacing)
    # Set new dose grid spacing in planNode if necessary
    if doseGridSpacing != planNode.GetDoseGridSpacing():
      planNode.SetDoseGridSpacing(doseGridSpacing)
    # Check if dose grid dimensions match for doseGrid dimensions in beamNode
    if (doseGrid.dimensions != doseGridDimensions):
      logging.warning("Dose grid dimensions in beamNode do not match the calculated dose grid dimensions.")
  else:
    doseGrid = ct.grid.copy().resample(target_resolution=planNode.GetDoseGridSpacing())

  # Get radiation mode from beamNode
  if planNode.GetIonPlanFlag():
    availableRadiationModes = ['protons', 'carbon']
  else:
    availableRadiationModes = ['photons', 'protons', 'carbon']
  radiationMode = slicer.pyRadPlanEngine.scriptedEngine.integerParameter(beamNode, 'radiationMode')


  # Create plan object from dictionary
  pln = {
    "radiation_mode": availableRadiationModes[radiationMode],
    "machine": ['Generic'][slicer.pyRadPlanEngine.scriptedEngine.integerParameter(beamNode, 'machine')],
    "num_of_fractions": slicer.pyRadPlanEngine.scriptedEngine.doubleParameter(beamNode, 'numOfFractions'),
    "prop_stf": {
      # Beam geometry settings
      "bixel_width": 5.0,
      "gantry_angles": [beamNode.GetGantryAngle()],
      "couch_angles": [beamNode.GetCouchAngle()],
      "iso_center": isocenter
    },

    # Dose calculation settings
    "prop_dose_calc": {"dose_grid": doseGrid},

    # Optimization settings
    "prop_opt": {},
  }

  # Use scipy solver for protons
  if pln["radiation_mode"] == "protons":
    pln["prop_opt"] = {"solver": "scipy"}

  return create_pln(pln)


#------------------------------------------------------------------------------
# HELPERS
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def getSegmentArrays(segmentationNode, referenceVolumeNode):
  """
  Extract binary labelmap arrays from all segments in a segmentation node.
  Converts segments to binary labelmap representation if necessary.
  
  Args:
    segmentationNode: Slicer segmentation node containing segments
    referenceVolumeNode: Reference volume node for array orientation
    
  Returns:
    Dictionary mapping segment IDs to their arrays and names
  """
  segmentation = segmentationNode.GetSegmentation()
  
  PLANAR_CONTOUR = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationPlanarContourRepresentationName()
  RIBBON_MODEL   = "Ribbon model"
  LABELMAP       = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName()

  firstSegment = segmentation.GetSegment(segmentation.GetSegmentIDs()[0])
  if firstSegment.GetRepresentation(LABELMAP) is None:
    logging.info(f"\nBinary labelmap not found for segmentation {segmentationNode.GetName()}, converting...")

    converter = vtkSegmentationCore.vtkSegmentationConverter()
    sourceRep = segmentation.GetSourceRepresentationName()

    if sourceRep == PLANAR_CONTOUR:
      paths = vtkSegmentationCore.vtkSegmentationConversionPaths()
      converter.GetPossibleConversions(PLANAR_CONTOUR, RIBBON_MODEL, paths)
      cheapestPath = vtkSegmentationCore.vtkSegmentationConverter.GetCheapestPath(paths)
      success = segmentation.CreateRepresentation(RIBBON_MODEL, cheapestPath)
      logging.info(f"Planar contour source detected. Conversion to Ribbon model {'succeeded' if success else 'FAILED'}")
      segmentation.SetSourceRepresentationName(RIBBON_MODEL)

    paths = vtkSegmentationCore.vtkSegmentationConversionPaths()
    converter.GetPossibleConversions(segmentation.GetSourceRepresentationName(), LABELMAP, paths)
    cheapestPath = vtkSegmentationCore.vtkSegmentationConverter.GetCheapestPath(paths)
    success = segmentation.CreateRepresentation(LABELMAP, cheapestPath)
    logging.info(f"Conversion from {segmentation.GetSourceRepresentationName()} source to binary labelmap {'succeeded' if success else 'FAILED'}")
    segmentation.SetSourceRepresentationName(LABELMAP)
    logging.info("Setting source representation to binary labelmap for future conversions\n")

  segmentArrays = {}
  for id in segmentation.GetSegmentIDs():
    segmentArray = slicer.util.arrayFromSegmentBinaryLabelmap(segmentationNode, id, referenceVolumeNode)
    segmentName = segmentation.GetSegment(id).GetName()
    segmentArrays[id] = {
        'array': np.uint8(segmentArray),
        'name': segmentName
    }
  return segmentArrays


#------------------------------------------------------------------------------
def createAndLoadBodySegment(planNode, cst):
  """
  If body segment is not selected, create body segment from cst and load into segmentation node in Slicer.
  
  Args:
    planNode: Slicer plan node to access body segment ID and segmentation node
    cst: StructureSet object containing VOIs, used to create new body segment
  """
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  segmentationNode = planNode.GetSegmentationNode()
  segmentation = segmentationNode.GetSegmentation()

  # Check if body segment selected and create from cst if not
  bodySegmentID = planNode.GetBodySegmentID()
  if bodySegmentID == "" or bodySegmentID is None:
    # ensure no segment is named "BODY" to avoid confusion
    segmentNames = [segmentation.GetSegment(id).GetName() for id in segmentation.GetSegmentIDs()]
    bodySegmentName = 'BODY'
    i = 1
    while bodySegmentName in segmentNames:
      bodySegmentName += str(i)
      i += 1

    # Create body segment from cst (ask user to confirm)
    msg = ("No body segment selected in plan node.\n\n"
      "Create a body segment named '{}' from the current structure set?").format(bodySegmentName)
    if not slicer.util.confirmOkCancelDisplay(msg):
      logging.info("Body segment creation canceled by user.")
      return cst
    
    cst.create_body_seg(voi_type="EXTERNAL", name=bodySegmentName)

    # Find body mask in cst to add to segmentation
    for voi in cst.vois:
      if voi.name == bodySegmentName:
        bodyMask = voi.mask
        break
    
    # Add new segment directly as binary labelmap
    logging.info("Adding body segment to segmentation in Slicer.")
    LABELMAP = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName()
    maskArray = sitk.GetArrayFromImage(bodyMask)
    newSegmentID = segmentation.AddEmptySegment("", bodySegmentName)
    slicer.util.updateSegmentBinaryLabelmapFromArray(
        maskArray, segmentationNode, newSegmentID, referenceVolumeNode
    )
    segmentation.SetSourceRepresentationName(LABELMAP)

    # Set body segment ID in plan node
    planNode.SetBodySegmentID(newSegmentID)
    segmentationNode.GetDisplayNode().SetSegmentOpacity3D(newSegmentID, 0.2)
    logging.info(f"Body segment '{bodySegmentName}' (ID: {newSegmentID}) added successfully as binary labelmap.\n")
