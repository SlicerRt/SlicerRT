import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
from scipy.sparse import coo_matrix
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

        # get number of voxels
        numberOfVoxels = referenceVolumeNode.GetImageData().GetNumberOfPoints()

        totalDose = 0

        for beamNumber in range(1,planNode.GetNumberOfBeams()+1):
            
            beamNode = planNode.GetBeamByNumber(beamNumber)
            print('current beam: ', beamNode.GetName())

            # Get the dose influence matrix
            triplets = beamNode.GetDoseInfluenceMatrixTriplets()

            # Convert the triplets to a scipy.sparse.coo_matrix
            data = np.array([triplets.GetTuple(i) for i in range(triplets.GetNumberOfTuples())])
            rows = data[:, 0].astype(int)
            cols = data[:, 1].astype(int)
            values = data[:, 2]
            dose_influence_matrix = coo_matrix((values, (rows, cols)), shape=(numberOfVoxels, max(cols)+1)) # shape important to include zeros in last indices (rows must be as long as number of voxels)

            # multipy dose influence matrix with weights
            weights = np.ones(numberOfVoxels) #len(cols) = number of rows
            dose = np.dot(dose_influence_matrix, weights)

            totalDose += dose

        print('Total Dose Size: ', totalDose.size)

        import vtk.util.numpy_support as numpy_support

        flat_data_array = totalDose.flatten()
        vtk_data = numpy_support.numpy_to_vtk(num_array=flat_data_array, deep=True, array_type=vtk.VTK_FLOAT)

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