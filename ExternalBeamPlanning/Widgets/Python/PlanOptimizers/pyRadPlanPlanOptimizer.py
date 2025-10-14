import slicer
import numpy as np
from scipy.sparse import csc_matrix
from PlanOptimizers import *
from Python.PyRadPlanUtils import prepareCt, prepareCst, preparePln

class pyRadPlanPlanOptimizer(AbstractScriptedPlanOptimizer):
  """ pyRadPlan Optimizer for SlicerRT External Beam Planning Module.
  """

  def __init__(self, scriptedEngine):
    scriptedEngine.name = 'pyRadPlan'
    AbstractScriptedPlanOptimizer.__init__(self, scriptedEngine)

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
    objectives_dict = {}

    # Loop through all objectives in the Slicer objectives table
    for node_dict in objectives:

      # Get objective node
      objective_node = node_dict['ObjectiveNode']
      if not objective_node:
        raise ValueError("Objective node is not set. Please select an objective node in the Slicer objectives table.")

      # Get segmentation
      segmentationNode = objective_node.GetSegmentationNode()
      if not segmentationNode:
        raise ValueError("Segmentation node is not set for objective node. Please select a segmentation node in the Slicer objectives table.")
      segmentID = objective_node.GetSegmentID()
      if not segmentID:
        raise ValueError("Segment ID is not set for objective node. Please select a segment in the Slicer objectives table.")
      segmentName = segmentationNode.GetSegmentation().GetSegment(segmentID).GetName()

      # Get objective information
      objective_info = {
        'objectiveName': objective_node.GetName(),
        'overlapPriority': objective_node.GetAttribute('overlapPriority'),
        'penalty': objective_node.GetAttribute('penalty'),
        'parameters': {name: objective_node.GetAttribute(name) for name in objective_node.GetAttributeNames() if name != 'overlapPriority' and name != 'penalty'}
      }

      # Save objectives for each segmentation
      if segmentName not in objectives_dict:
        objectives_dict[segmentName] = [objective_info]
      else:
        objectives_dict[segmentName].append(objective_info)


    ###################################### Get overlap priorites #########################################
    overlap_priority_dict = {}

    # Extract overlap priorities from objectives_dict (saved for each objective, but needed for each VOI)
    for voi_name, objectives_list in objectives_dict.items():

      # Take first overlap priority for each VOI
      overlap_priority = objectives_list[0]['overlapPriority']
      overlap_priority_dict[voi_name] = overlap_priority

      # Check if all objectives for this VOI have the same overlap priority
      for objective in objectives_list:
        if objective['overlapPriority'] != overlap_priority:
          raise ValueError(f"Overlap priorities do not match for VOI '{voi_name}' in objectives table. All objectives for a VOI must have the same overlap priority.")


    ############################ Get dose influence matrices from beam nodes #############################
    self.scriptedPlanOptimizer.progressInfoUpdated("get dijs")
    referenceVolumeNode = planNode.GetReferenceVolumeNode()

    # Get beams in plan
    numberOfBeams = planNode.GetNumberOfBeams()
    beam_names = []
    tried_beam_index = 0
    while (len(beam_names) < numberOfBeams):
      try:
        beam = planNode.GetBeamByNumber(tried_beam_index)
        beam_names.append(beam.GetName())
        tried_beam_index += 1
      except:
        tried_beam_index += 1

    # Get dij of each beam
    dij_list = []
    beam_index = 0
    for beam_name in beam_names:

      beam_index += 1
      self.scriptedPlanOptimizer.progressInfoUpdated("get dijs ("+ str(beam_index) + "/" + str(numberOfBeams) + ")")

      beamNode = planNode.GetBeamByName(beam_name)
      print('current beam: ', beam_name)

      # Prepare the ct
      ct = prepareCt(beamNode)

      # Prepare the cst
      cst = prepareCst(beamNode, ct)

      # Prepare the plan configuration
      pln = preparePln(beamNode, ct, dose_grid_from_beamNode=True)

      # Generate Steering Geometry ("stf")
      stf = generate_stf(ct, cst, pln)

      # Get Dose Influence Matrix (coo_matrix)
      field_data = beamNode.GetDoseInfluenceMatrixFieldData()
      data = np.array(field_data.GetArray('Data'), dtype=np.float32)
      indices = np.array(field_data.GetArray('Indices'), dtype=np.int32)
      indptr = np.array(field_data.GetArray('Indptr'), dtype=np.int32)

      # Convert to csc_matrix
      num_of_cols = len(indptr) - 1
      number_of_voxels = pln.prop_dose_calc['dose_grid'].num_voxels
      dose_influence_matrix = csc_matrix((data, indices, indptr), shape=(number_of_voxels, num_of_cols))

      # Create Dij object
      dij = Dij(
        dose_grid=pln.prop_dose_calc['dose_grid'],
        ct_grid=ct.grid,
        physical_dose=dose_influence_matrix,
        num_of_beams = 1,
        beam_num=stf.bixel_beam_index_map,
        ray_num=stf.bixel_ray_index_per_beam_map,
        bixel_num=stf.bixel_index_per_beam_map
      )

      dij_list.append(dij)

    # Compose dij from all beams
    if len(dij_list) > 1:
      dij = compose_beam_dijs(dij_list)
    else:
      dij = dij_list[0]


    ########################### Update overlap priorities & objectives in cst ############################
    for voi in cst.vois:

      if voi.name in objectives_dict:
        voi.objectives = []

        # Set overlap priority for VOI
        voi.overlap_priority = overlap_priority_dict[voi.name]

        for i in range(len(objectives_dict[voi.name])):
          objective_name = objectives_dict[voi.name][i]['objectiveName']
          objective_parameters = objectives_dict[voi.name][i]['parameters']

          # Get instance of objective function from pyRadPlan
          objective_instance = get_objective(objective_name)

          # Set penalty (called "priority" in pyRadPlan) for objective function
          objective_instance.priority = objectives_dict[voi.name][i]['penalty']

          # Set parameters for objective function
          for parameter in objective_instance.parameter_names:
            if parameter in objective_parameters:
              # TODO: type check
              setattr(objective_instance, parameter, objective_parameters[parameter])
            else:
              print(f"Parameter {parameter} not found in objectives table.")
              print(f"Using default value: {getattr(objective_instance, parameter)}")
          voi.objectives.append(objective_instance)


    ########################################## Optimize dose #############################################
    self.scriptedPlanOptimizer.progressInfoUpdated("optimize fluence")

    fluence = fluence_optimization(ct, cst, stf, dij, pln)

    # Result
    result = dij.compute_result_ct_grid(fluence)


    ##################################### Visualize dose in Slicer #######################################
    self.scriptedPlanOptimizer.progressInfoUpdated("visualize dose")

    total_dose = result["physical_dose"]

    # TODO: directly get dose per beam from overlay=result.beam_quantities["physical_dose"][0].distribution (once result_gui is merged)
    # Now: manually get dose per beam and create a new volume node for each beam
    for i in range(planNode.GetNumberOfBeams()):
      mask = (dij.beam_num == i)
      fluence_for_beam = np.zeros_like(fluence)
      fluence_for_beam[mask] = fluence[mask]
      result_for_beam = dij.compute_result_ct_grid(fluence_for_beam)
      dose_per_beam = result_for_beam["physical_dose"]

      # Create a new volume node for each beam
      beamDoseVolumeNodeName = f'{planNode.GetName()}_pyRadOptimizedDose_Beam_{i+1}'
      beamDoseVolumeNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode", beamDoseVolumeNodeName)
      displayNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeDisplayNode")
      beamDoseVolumeNode.SetAndObserveDisplayNodeID(displayNode.GetID())

      sitkUtils.PushVolumeToSlicer(dose_per_beam, beamDoseVolumeNode)

      # Set colormap
      rxDose = planNode.GetRxDose()
      displayNode = beamDoseVolumeNode.GetDisplayNode()
      displayNode.SetAndObserveColorNodeID("vtkMRMLColorTableNodeDose_ColorTable_Relative")
      displayNode.AutoWindowLevelOn()
      displayNode.SetLowerThreshold(0.05*rxDose)
      displayNode.ApplyThresholdOn()

    # Push total dose to volumeNode in Slicer
    sitkUtils.PushVolumeToSlicer(total_dose, targetNode = resultOptimizationVolumeNode)

    # Set name of result volume node
    try:
      resultOptimizationVolumeNodeName = planNode.GetOutputTotalDoseVolumeNode().GetName()
    except AttributeError:
      resultOptimizationVolumeNodeName = str(planNode.GetName())+"_pyRadOptimizedDose"
    resultOptimizationVolumeNode.SetName(resultOptimizationVolumeNodeName)

    return str()
