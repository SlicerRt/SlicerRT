import numpy as np
import logging
import slicer
import sitkUtils

#------------------------------------------------------------------------------
def prepareCt(beamNode):
  from pyRadPlan.ct import create_ct
  logging.basicConfig(level=logging.INFO)

  # Get sitk image from Slicer
  planNode = beamNode.GetParentPlanNode()
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  image = sitkUtils.PullVolumeFromSlicer(referenceVolumeNode)

  # Create ct object from sitk image
  return create_ct(cube_hu=image)

#------------------------------------------------------------------------------
def prepareCst(beamNode, ct):
  from pyRadPlan.cst import StructureSet, create_voi
  logging.basicConfig(level=logging.INFO)

  # Get segmentations from Slicer
  planNode = beamNode.GetParentPlanNode()
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  segmentationNode = planNode.GetSegmentationNode()
  segmentation = segmentationNode.GetSegmentation()

  # Create VOI objects from segments
  vois = []
  for id in segmentation.GetSegmentIDs():
    segmentArray = slicer.util.arrayFromSegmentBinaryLabelmap(segmentationNode, id, referenceVolumeNode)
    segmentArray = np.uint8(segmentArray)
    segmentName = segmentation.GetSegment(id).GetName()
    # Set VOI type as 'TARGET' if segment is selected as target, otherwise 'OAR'
    voi_type = 'TARGET' if id==planNode.GetTargetSegmentID() else 'OAR'
    voi = create_voi(voi_type=voi_type, name=segmentName, ct_image=ct, mask=segmentArray)
    vois.append(voi)

  # Create cst object from VOIs and ct
  cst = StructureSet(vois=vois, ct_image=ct)
  return cst

#------------------------------------------------------------------------------
def preparePln(beamNode, ct, dose_grid_from_beamNode=False):
  from pyRadPlan.plan import create_pln
  logging.basicConfig(level=logging.INFO)
  
  # Get isocenter position in RAS coordinates from Slicer
  planNode = beamNode.GetParentPlanNode()
  referenceVolumeNode = planNode.GetReferenceVolumeNode()
  ijkToRASDirections = np.eye(3)
  referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
  isocenter = [0]*3
  planNode.GetIsocenterPosition(isocenter)
  isocenter = ijkToRASDirections @ np.array(isocenter)

  # Get dose grid by resampling ct grid to resolution defined in beamNode or planNode
  if dose_grid_from_beamNode:
    # Get dose grid specs saved in beamNode (corresponding to saved dose influence matrix)
    doseGridSpacing = beamNode.GetDoseGridSpacing()
    doseGridDimensions = beamNode.GetDoseGridDim()
    # Check if dose influence matrix is available in beamNode
    if beamNode.GetDoseInfluenceMatrixFieldData() is None:
      print("Warning: No dose influence matrix found in beamNode. Using planNode's dose grid spacing.")
      doseGridSpacing = planNode.GetDoseGridSpacing()
    # Check if dose grid spacing was set in beamNode (default: {-1, -1, -1})
    if doseGridSpacing[0] < 0 or doseGridSpacing[1] < 0 or doseGridSpacing[2] < 0 :
      print("Warning: Dose grid spacing in beamNode is invalid. Using planNode's dose grid spacing.")
      doseGridSpacing = planNode.GetDoseGridSpacing()
    dose_grid = ct.grid.copy().resample(target_resolution=doseGridSpacing)
    # Set new dose grid spacing in planNode if necessary
    if doseGridSpacing != planNode.GetDoseGridSpacing():
      planNode.SetDoseGridSpacing(doseGridSpacing)
    # Check if dose grid dimensions match for doseGrid dimensions in beamNode
    if (dose_grid.dimensions != doseGridDimensions):
      print("Warning: Dose grid dimensions in beamNode do not match the calculated dose grid dimensions.")
  else:
    dose_grid = ct.grid.copy().resample(target_resolution=planNode.GetDoseGridSpacing())

  # Get radiation mode from beamNode
  if planNode.GetIonPlanFlag():
    available_radiation_modes = ['protons', 'carbon']
  else:
    available_radiation_modes = ['photons', 'protons', 'carbon']
  radiation_mode = slicer.pyRadPlanEngine.scriptedEngine.integerParameter(beamNode, 'radiationMode')


  # Create plan object from dictionary
  pln = {
    "radiation_mode": available_radiation_modes[radiation_mode],
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
    "prop_dose_calc": {"dose_grid": dose_grid},

    # Optimization settings
    "prop_opt": {},
  }

  # Use scipy solver for protons
  if pln["radiation_mode"] == "protons":
    pln["prop_opt"] = {"solver": "scipy"}

  return create_pln(pln)
