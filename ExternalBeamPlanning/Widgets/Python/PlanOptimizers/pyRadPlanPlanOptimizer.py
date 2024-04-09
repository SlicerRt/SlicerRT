import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
from scipy.sparse import csc_matrix
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

            fieldData = beamNode.GetDoseInfluenceMatrixFieldData()
            # data = np.array(fieldData.GetArray('Data'))
            # indices = np.array(fieldData.GetArray('Indices'))
            # indptr = np.array(fieldData.GetArray('Indptr'))


            numOfCols = fieldData.GetArray('Indptr').GetSize()-1

            dose_influence_matrix = csc_matrix((np.array(fieldData.GetArray('Data')),
                                                np.array(fieldData.GetArray('Indices')),
                                                np.array(fieldData.GetArray('Indptr'))),
                                                shape=(numberOfVoxels, numOfCols)) # shape important to include zeros in last indices (rows must be as long as number of voxels)

            # dose_influence_matrix = csc_matrix((data, indices, indptr), shape=(numberOfVoxels, numOfCols))

            # multipy dose influence matrix with weights
            weights = np.ones(numOfCols) #len(cols) = number of rows
            dose = dose_influence_matrix.dot(weights)
            totalDose += dose


        print('Total Dose Size: ', totalDose.size)

        # insert total dose into volumee node
        flat_data_array = totalDose.swapaxes(0,2).swapaxes(2,1).flatten()
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