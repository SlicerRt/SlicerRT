import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
from scipy.sparse import csc_matrix
import vtk.util.numpy_support as numpy_support
from PlanOptimizers import *
from Python.prepareRTDataset import prepareCt, prepareCst, preparePln

class pyRadPlanPlanOptimizer(AbstractScriptedPlanOptimizer):
    """ pyRadPlan Optimizer for SlicerRT External Beam Planning Module.
    """

    def __init__(self, scriptedEngine):
        scriptedEngine.name = 'pyRadPlan'
        AbstractScriptedPlanOptimizer.__init__(self, scriptedEngine)

        temp_path = slicer.app.temporaryPath + '/pyRadPlan/'
        temp_path = os.path.normpath(temp_path)

        isExist = os.path.exists(temp_path)

        if not isExist:
            os.makedirs(temp_path)

        self.temp_path = temp_path


    def optimizePlanUsingEngine(self, beamNode, resultDoseVolumeNode):
        print('pyRadPlan Optimizer is called')
        return "Not implemented yet!"


    def optimizePlanUsingOptimizer(self, planNode, objectives, resultOptimizationVolumeNode):

        import SimpleITK as sitk
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
            'parameters': {name: objective_node.GetAttribute(name) for name in objective_node.GetAttributeNames()}
            }
            if segmentation not in objectives_dict:
                objectives_dict[segmentation] = [objective_info]
            else:
                objectives_dict[segmentation].append(objective_info)


        # get reference volume node
        referenceVolumeNode = planNode.GetReferenceVolumeNode()
        numberOfVoxels = referenceVolumeNode.GetImageData().GetNumberOfPoints()

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
            data = np.array(fieldData.GetArray('Data'))
            indices = np.array(fieldData.GetArray('Indices'))
            indptr = np.array(fieldData.GetArray('Indptr'))

            numOfCols = fieldData.GetArray('Indptr').GetSize()-1

            dose_influence_matrix = csc_matrix((data, indices, indptr), shape=(numberOfVoxels, numOfCols)) # shape important to include zeros in last indices (rows must be as long as number of voxels)

            #################### temp #######################
            dose_grid = ct.grid
            dose_grid.resolution = {"x": 5, "y": 5, "z": 5}
            #################### temp #######################


            dij = Dij(
                dose_grid=ct.grid,
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


        print(dij.physical_dose.flat[0].nnz)
        print(dij.physical_dose.flat[0].data.shape)
        print(dij.physical_dose.flat[0].max())

        # Optimize
        for voi in cst.vois:
            if voi.name in objectives_dict:
                voi.objectives = []
                for i in range(len(objectives_dict[voi.name])):
                    objective_name = objectives_dict[voi.name][i]['objectiveName']
                    print('objective_name: ', objective_name)
                    voi.objectives.append(get_objective(objective_name))#, objective['parameters']))
    
        # VOIS
        # fluence = fluence_optimization(ct, cst, stf, dij, pln)
        fluence = np.ones((dij.total_num_of_bixels,),dtype=np.float32)

        # Result
        result = dij.compute_result_ct_grid(fluence)
            
        totalDose = result["physical_dose"]


        # # insert total dose into volume node
        # flat_data_array = totalDose.swapaxes(0,2).swapaxes(2,1).flatten()
        # vtk_data = numpy_support.numpy_to_vtk(num_array=flat_data_array, deep=True, array_type=vtk.VTK_FLOAT)

        # imageData = vtk.vtkImageData()
        # imageData.DeepCopy(referenceVolumeNode.GetImageData())
        # imageData.GetPointData().SetScalars(vtk_data)
            
        # resultOptimizationVolumeNode.SetOrigin(referenceVolumeNode.GetOrigin())
        # resultOptimizationVolumeNode.SetSpacing(referenceVolumeNode.GetSpacing())
        # ijkToRASDirections = np.eye(3)
        # referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
        # resultOptimizationVolumeNode.SetIJKToRASDirections(ijkToRASDirections)
        # resultOptimizationVolumeNode.SetAndObserveImageData(imageData)
        resultNode = sitkUtils.PushVolumeToSlicer(totalDose, targetNode = resultOptimizationVolumeNode, className="vtkMRMLScalarVolumeNode")

        # Set name
        OptimizedDoseNodeName = str(planNode.GetName())+"_pyRadOptimzedDose"
        resultOptimizationVolumeNode.SetName(OptimizedDoseNodeName)

        
        slicer.util.setSliceViewerLayers(background=referenceVolumeNode, foreground=resultNode)
        slicer.util.setSliceViewerLayers(foregroundOpacity=1)

        return str()
    
