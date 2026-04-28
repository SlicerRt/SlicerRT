import slicer
import numpy as np
import logging
from scipy.sparse import csc_matrix
from PlanOptimizers import *
from Python.PyRadPlanUtils import prepareCt, prepareCst, preparePln

class pyRadPlanPlanOptimizer(AbstractScriptedPlanOptimizer):
  """ pyRadPlan Optimizer for SlicerRT External Beam Planning Module.
  """

  #------------------------------------------------------------------------------
  def __init__(self, scriptedEngine):
    scriptedEngine.name = 'pyRadPlan'
    AbstractScriptedPlanOptimizer.__init__(self, scriptedEngine)

  #------------------------------------------------------------------------------
  def setAvailableObjectives(self):
    """
    Return dict: { 'Objective Name': { 'param1': {'default': value1}, 'param2': {'default': value2}, ... }, ... }
    This matches the structure your C++ expects.
    """

    from pyRadPlan.optimization.objectives import get_available_objectives
    availableObjectivesDict = {}

    for objName, objClass in get_available_objectives().items():
      objInstance = objClass()
      parameterDict = {}
      parameterNames = objInstance.parameter_names
      parameterDefaultValues = objInstance.parameters
      for i in range(len(parameterNames)):
          parameterDict[parameterNames[i]] = {
              "default": parameterDefaultValues[i],
          }
      availableObjectivesDict[objName] = parameterDict

    return availableObjectivesDict

  #------------------------------------------------------------------------------
  def optimizePlanUsingOptimizer(self, planNode, objectives, resultOptimizationVolumeNode):
    import sitkUtils
    import SimpleITK as sitk
    from pyRadPlan import (
      generate_stf,
      fluence_optimization,
    )
    from pyRadPlan.dij import Dij, compose_beam_dijs
    from pyRadPlan.optimization.objectives import get_objective


    ########################### Get objectives from Slicer objectives table ##############################
    objectivesDict = {}

    # Loop through all objectives in the Slicer objectives table
    for nodeDict in objectives:

      # Get objective node
      objectiveNode = nodeDict['ObjectiveNode']
      if not objectiveNode:
        raise ValueError("Objective node is not set. Please select an objective node in the Slicer objectives table.")

      # Get segmentation
      segmentationNode = objectiveNode.GetSegmentationNode()
      if not segmentationNode:
        raise ValueError("Segmentation node is not set for objective node. Please select a segmentation node in the Slicer objectives table.")
      segmentID = objectiveNode.GetSegmentID()
      if not segmentID:
        raise ValueError("Segment ID is not set for objective node. Please select a segment in the Slicer objectives table.")
      segmentName = segmentationNode.GetSegmentation().GetSegment(segmentID).GetName()

      # Get objective information
      objectiveInfo = {
        'objectiveName': objectiveNode.GetName(),
        'overlapPriority': objectiveNode.GetAttribute('overlapPriority'),
        'penalty': objectiveNode.GetAttribute('penalty'),
        'parameters': {name: objectiveNode.GetAttribute(name) for name in objectiveNode.GetAttributeNames() if name != 'overlapPriority' and name != 'penalty'}
      }

      # Save objectives for each segmentation
      if segmentName not in objectivesDict:
        objectivesDict[segmentName] = [objectiveInfo]
      else:
        objectivesDict[segmentName].append(objectiveInfo)

    msg = "Objectives extracted from Slicer objectives table:\n"
    for segmentName, objectivesList in objectivesDict.items():
        msg += f"  Segment: {segmentName}\n"
        for objectiveInfo in objectivesList:
            msg += f"    Objective: {objectiveInfo['objectiveName']}\n"
            msg += f"      Overlap Priority: {objectiveInfo['overlapPriority']}\n"
            msg += f"      Penalty: {objectiveInfo['penalty']}\n"
            msg += f"      Parameters: {objectiveInfo['parameters']}\n"
    logging.info(msg)

    ###################################### Get overlap priorites #########################################
    overlapPriorityDict = {}

    # Extract overlap priorities from objectivesDict (saved for each objective, but needed for each VOI)
    for voiName, objectivesList in objectivesDict.items():

      # Take first overlap priority for each VOI
      overlapPriority = objectivesList[0]['overlapPriority']
      overlapPriorityDict[voiName] = overlapPriority

      # Check if all objectives for this VOI have the same overlap priority
      for objective in objectivesList:
        if objective['overlapPriority'] != overlapPriority:
          raise ValueError(f"Overlap priorities do not match for VOI '{voiName}' in objectives table. All objectives for a VOI must have the same overlap priority.")


    ############################ Get dose influence matrices from beam nodes #############################
    self.scriptedPlanOptimizer.progressInfoUpdated("get dijs")

    # Get beams in plan
    numberOfBeams = planNode.GetNumberOfBeams()
    beamNames = []
    triedBeamIndex = 0
    while (len(beamNames) < numberOfBeams):
      try:
        beam = planNode.GetBeamByNumber(triedBeamIndex)
        beamNames.append(beam.GetName())
        triedBeamIndex += 1
      except:
        triedBeamIndex += 1

    # Get dij of each beam
    dijList = []
    beamIndex = 0
    for beamName in beamNames:

      beamIndex += 1
      self.scriptedPlanOptimizer.progressInfoUpdated("get dijs ("+ str(beamIndex) + "/" + str(numberOfBeams) + ")")

      beamNode = planNode.GetBeamByName(beamName)
      logging.info(f"Processing beam: {beamName}")

      # Prepare the ct
      ct = prepareCt(beamNode)

      # Prepare the cst
      cst = prepareCst(beamNode, ct)

      # Prepare the plan configuration
      pln = preparePln(beamNode, ct, doseGridFromBeamNode=True)

      # Generate Steering Geometry ("stf")
      stf = generate_stf(ct, cst, pln)

      # Get Dose Influence Matrix (coo_matrix)
      fieldData = beamNode.GetDoseInfluenceMatrixFieldData()
      data = np.array(fieldData.GetArray('Data'), dtype=np.float32)
      indices = np.array(fieldData.GetArray('Indices'), dtype=np.int32)
      indptr = np.array(fieldData.GetArray('Indptr'), dtype=np.int32)

      # Convert to csc_matrix
      numOfCols = len(indptr) - 1
      numberOfVoxels = pln.prop_dose_calc['dose_grid'].num_voxels
      doseInfluenceMatrix = csc_matrix((data, indices, indptr), shape=(numberOfVoxels, numOfCols))

      # Create Dij object
      dij = Dij(
        dose_grid=pln.prop_dose_calc['dose_grid'],
        ct_grid=ct.grid,
        physical_dose=doseInfluenceMatrix,
        num_of_beams = 1,
        beam_num=stf.bixel_beam_index_map,
        ray_num=stf.bixel_ray_index_per_beam_map,
        bixel_num=stf.bixel_index_per_beam_map
      )

      dijList.append(dij)

    # Compose dij from all beams
    if len(dijList) > 1:
      dij = compose_beam_dijs(dijList)
    else:
      dij = dijList[0]


    ########################### Update overlap priorities & objectives in cst ############################
    for voi in cst.vois:

      if voi.name in objectivesDict:
        voi.objectives = []

        # Set overlap priority for VOI
        voi.overlap_priority = overlapPriorityDict[voi.name]

        for i in range(len(objectivesDict[voi.name])):
          objectiveName = objectivesDict[voi.name][i]['objectiveName']
          objectiveParameters = objectivesDict[voi.name][i]['parameters']

          # Get instance of objective function from pyRadPlan
          objectiveInstance = get_objective(objectiveName)

          # Set penalty (called "priority" in pyRadPlan) for objective function
          objectiveInstance.priority = objectivesDict[voi.name][i]['penalty']

          # Set parameters for objective function
          for parameter in objectiveInstance.parameter_names:
            if parameter in objectiveParameters:
              # TODO: type check
              setattr(objectiveInstance, parameter, objectiveParameters[parameter])
            else:
              logging.warning(f"Parameter {parameter} not found in objectives table. "
                              f"Using default value: {getattr(objectiveInstance, parameter)}")
          voi.objectives.append(objectiveInstance)


    ########################################## Optimize dose #############################################
    self.scriptedPlanOptimizer.progressInfoUpdated("optimize fluence")

    fluence = fluence_optimization(ct, cst, stf, dij, pln)

    # Result
    result = dij.compute_result_ct_grid(fluence)


    ##################################### Visualize dose in Slicer #######################################
    self.scriptedPlanOptimizer.progressInfoUpdated("visualize dose")

    # Get global variables for dose volume node naming and hierarchy placement
    shNode = slicer.mrmlScene.GetSubjectHierarchyNode()
    planItemID = shNode.GetItemByDataNode(planNode)
    rxDose = planNode.GetRxDose()

    # Get dose for each and beam
    if (planNode.GetNumberOfBeams() > 1):
      for i, beamName in enumerate(beamNames):
        # Create a new volume node for each beam
        beamDoseVolumeNodeName = f'{planNode.GetName()}_pyRadPlanOptimized_physicalDose_{beamName}'
        beamDoseVolumeNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode", beamDoseVolumeNodeName)
        displayNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeDisplayNode")
        beamDoseVolumeNode.SetAndObserveDisplayNodeID(displayNode.GetID())
        sitkUtils.PushVolumeToSlicer(result['physical_dose_beam'][i], beamDoseVolumeNode)

        # Place under the plan in the hierarchy
        itemID = shNode.GetItemByDataNode(beamDoseVolumeNode)
        shNode.SetItemParent(itemID, planItemID)

        # Tag & trigger re-evaluation as dose volume
        shNode.SetItemAttribute(itemID, 'DoseUnitName', "Gy")
        shNode.SetItemAttribute(itemID, 'DoseUnitValue', "1.0")
        beamDoseVolumeNode.SetAttribute("DicomRtImport.DoseVolume", "1")
        shNode.RequestOwnerPluginSearch(itemID)
        shNode.ItemModified(itemID)

        # Set colormap
        displayNode.SetAndObserveColorNodeID("vtkMRMLColorTableNodeDose_ColorTable_Relative")
        displayNode.AutoWindowLevelOn()
        displayNode.SetLowerThreshold(0.05*rxDose)
        displayNode.ApplyThresholdOn()

    # Push total dose to volumeNode in Slicer, set name & overlay on CT
    sitkUtils.PushVolumeToSlicer(result["physical_dose"], targetNode = resultOptimizationVolumeNode)
    try:
      resultOptimizationVolumeNodeName = planNode.GetOutputTotalDoseVolumeNode().GetName()
    except AttributeError:
      resultOptimizationVolumeNodeName = str(planNode.GetName())+"_pyRadPlanOptimized_physicalDose"
    resultOptimizationVolumeNode.SetName(resultOptimizationVolumeNodeName)

    return str()
