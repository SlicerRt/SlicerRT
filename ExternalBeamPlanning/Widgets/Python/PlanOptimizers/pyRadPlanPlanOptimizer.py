import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
from scipy.sparse import coo_matrix
import vtk.util.numpy_support as numpy_support
from PlanOptimizers import *

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


    def optimizePlanUsingOptimizer(self, planNode, resultOptimizationVolumeNode):
        print('pyRadPlan Optimizer is called')

        referenceVolumeNode = planNode.GetReferenceVolumeNode()
        numberOfVoxels = referenceVolumeNode.GetImageData().GetNumberOfPoints()

        totalDose = 0

        for beamNumber in range(1,planNode.GetNumberOfBeams()+1):
            
            beamNode = planNode.GetBeamByNumber(beamNumber)
            print('current beam: ', beamNode.GetName())

            # Get the dose influence matrix
            triplets = beamNode.GetDoseInfluenceMatrixTriplets()

            data = numpy_support.vtk_to_numpy(triplets)
            rows = data[:, 0].astype(int)
            cols = data[:, 1].astype(int)
            values = data[:, 2]

            dose_influence_matrix = coo_matrix((values, (rows, cols)), shape=(numberOfVoxels, max(cols)+1)) # shape important to include zeros in last indices (rows must be as long as number of voxels)


            # multipy dose influence matrix with weights
            weights = np.ones(max(cols)+1)
            dose = dose_influence_matrix.dot(weights)

            totalDose += dose


        print('Total Dose Size: ', totalDose.size)

        # insert total dose into volumee node
        vtk_data = numpy_support.numpy_to_vtk(num_array=totalDose, deep=True, array_type=vtk.VTK_FLOAT)

        imageData = vtk.vtkImageData()
        imageData.DeepCopy(referenceVolumeNode.GetImageData())
        imageData.GetPointData().SetScalars(vtk_data)
            
        resultOptimizationVolumeNode.SetOrigin(referenceVolumeNode.GetOrigin())
        resultOptimizationVolumeNode.SetSpacing(referenceVolumeNode.GetSpacing())
        ijkToRASDirections = np.eye(3)
        referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
        resultOptimizationVolumeNode.SetIJKToRASDirections(ijkToRASDirections)
        resultOptimizationVolumeNode.SetAndObserveImageData(imageData)

        # Set name
        OptimizedDoseNodeName = str(beamNode.GetName())+"_pyRadOptimzedDose"
        resultOptimizationVolumeNode.SetName(OptimizedDoseNodeName)

        return str()