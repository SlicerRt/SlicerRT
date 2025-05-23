import slicer
import numpy as np
from scipy.sparse import csc_matrix
from PlanOptimizers import *
from Python.prepareRTDataset import prepareCt, prepareCst, preparePln

class pyRadPlanPlanOptimizer(AbstractScriptedPlanOptimizer):
    """ pyRadPlan Optimizer for SlicerRT External Beam Planning Module.
    """

    def __init__(self, scriptedEngine):
        scriptedEngine.name = 'pyRadPlan'
        AbstractScriptedPlanOptimizer.__init__(self, scriptedEngine)


    def optimizePlanUsingEngine(self, beamNode, resultDoseVolumeNode):
        print('pyRadPlan Optimizer is called')
        return "Not implemented yet!"


    def optimizePlanUsingOptimizer(self, planNode, objectives, resultOptimizationVolumeNode):
        import sitkUtils
        from pyRadPlan import (
            generate_stf,
            fluence_optimization,
        )
        from pyRadPlan.dij import Dij, compose_beam_dijs
        from pyRadPlan.optimization.objectives import get_objective

        # create dict with objectives for segmentation from Slicer obejctives table 
        objectives_dict = {} 
        for node_dict in objectives:
            objective_node = node_dict['ObjectiveNode']
            segmentation = objective_node.GetSegmentation()
            objective_info = {
            'objectiveName': objective_node.GetName(),
            'priority': objective_node.GetAttribute('Priority'),
            'parameters': {name: objective_node.GetAttribute(name) for name in objective_node.GetAttributeNames() if name != 'Priority'}
            }
            if segmentation not in objectives_dict:
                objectives_dict[segmentation] = [objective_info]
            else:
                objectives_dict[segmentation].append(objective_info)


        # get reference volume node
        referenceVolumeNode = planNode.GetReferenceVolumeNode()

        # total dose summed over all beams
        totalDose = 0

        numberOfBeams = planNode.GetNumberOfBeams()

        # get beams in plan
        beamNames = []
        tried_beam_index = 0
        while (len(beamNames) < numberOfBeams):
            try:
                beam = planNode.GetBeamByNumber(tried_beam_index)
                beamNames.append(beam.GetName())
                tried_beam_index += 1
            except:
                tried_beam_index += 1
            

        dij_list = []
        for beamName in beamNames:
            
            beamNode = planNode.GetBeamByName(beamName)
            print('current beam: ', beamName)

            # Prepare the ct
            ct = prepareCt(beamNode)

            # Prepare the cst (segmentations)
            cst = prepareCst(beamNode, ct)

            # Prepare the plan configuration
            pln = preparePln(beamNode, ct)

            # Generate Steering Geometry ("stf")
            stf = generate_stf(ct, cst, pln)

            # Get Dose Influence Matrix
            fieldData = beamNode.GetDoseInfluenceMatrixFieldData()
            data = np.array(fieldData.GetArray('Data'), dtype=np.float32)
            indices = np.array(fieldData.GetArray('Indices'), dtype=np.int32)
            indptr = np.array(fieldData.GetArray('Indptr'), dtype=np.int32)

            numOfCols = len(indptr) - 1

            numberOfVoxels = pln.prop_dose_calc['dose_grid'].num_voxels
            dose_influence_matrix = csc_matrix((data, indices, indptr), shape=(numberOfVoxels, numOfCols)) # shape important to include zeros in last indices (rows must be as long as number of voxels)

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

        if len(dij_list) > 1:
            dij = compose_beam_dijs(dij_list)
        else:
            dij = dij_list[0]

        # Optimize
        for voi in cst.vois:
            if voi.name in objectives_dict:
                voi.objectives = []
                for i in range(len(objectives_dict[voi.name])):
                    objective_name = objectives_dict[voi.name][i]['objectiveName']
                    objective_parameters = objectives_dict[voi.name][i]['parameters']
                    objective_instance = get_objective(objective_name)
                    objective_instance.priority = objectives_dict[voi.name][i]['priority']
                    for parameter in objective_instance.parameter_names:
                        if parameter in objective_parameters:
                            # TODO: type check
                            setattr(objective_instance, parameter, objective_parameters[parameter])
                        else:
                            print(f"Parameter {parameter} not found in objectives table.")
                            print(f"Using default value: {getattr(objective_instance, parameter)}")
                    voi.objectives.append(objective_instance)
    
        # VOIS
        fluence = fluence_optimization(ct, cst, stf, dij, pln)

        # Result
        result = dij.compute_result_ct_grid(fluence)
            
        totalDose = result["physical_dose"]

        # TODO: directly get dose per beam from overlay=result.beam_quantities["physical_dose"][0].distribution (once reult_gui is merged)
        # Now: manually get dose per beam and create a new volume node for each beam
        for i in range(planNode.GetNumberOfBeams()):
            mask = (dij.beam_num == i)
            fluence_for_beam = np.zeros_like(fluence)
            fluence_for_beam[mask] = fluence[mask]
            result_for_beam = dij.compute_result_ct_grid(fluence_for_beam)
            dose_per_beam = result_for_beam["physical_dose"]
            
            # create a new volume node for each beam
            beamDoseVolumeNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode", "BeamDose")
            beamDoseVolumeNode.SetName(planNode.GetName()+"_pyRadOptimzedDose_Beam"+str(i+1))
            beamDoseVolumeNode.SetSpacing(referenceVolumeNode.GetSpacing())
            beamDoseVolumeNode.SetOrigin(referenceVolumeNode.GetOrigin())

            #set colormap
            rxDose = planNode.GetRxDose()
            # displayNode = beamDoseVolumeNode.GetDisplayNode()
            # displayNode.GetColorNode().SetAndObserveColorNodeID("vtkMRMLColorTableNodeDose_ColorTable_Relative")
            # displayNode.AutoWindowLevelOn()
            # displayNode.SetLowerThreshold(0.05*rxDose)
            # displayNode.ApplyThresholdOn()

            sitkUtils.PushVolumeToSlicer(dose_per_beam, targetNode = beamDoseVolumeNode)

        sitkUtils.PushVolumeToSlicer(totalDose, targetNode = resultOptimizationVolumeNode)#, className="vtkMRMLScalarVolumeNode")

        # Set name
        OptimizedDoseNodeName = str(planNode.GetName())+"_pyRadOptimzedDose"
        resultOptimizationVolumeNode.SetName(OptimizedDoseNodeName)

        slicer.util.setSliceViewerLayers(background=referenceVolumeNode, foreground=resultOptimizationVolumeNode)
        slicer.util.setSliceViewerLayers(foregroundOpacity=1)

        return str()
    
