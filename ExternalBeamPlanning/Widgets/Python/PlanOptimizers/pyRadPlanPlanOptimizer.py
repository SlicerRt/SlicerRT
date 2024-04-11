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


    def optimizePlanUsingOptimizer(self, planNode, resultOptimizationVolumeNode):
        from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.optimization.components.objectives import (SquaredDeviation, SquaredOverdosing)
        
        print('pyRadPlan Optimizer is called')

        referenceVolumeNode = planNode.GetReferenceVolumeNode()
        numberOfVoxels = referenceVolumeNode.GetImageData().GetNumberOfPoints()

        totalDose = 0

        for beamNumber in range(1,planNode.GetNumberOfBeams()+1):
            
            beamNode = planNode.GetBeamByNumber(beamNumber)
            print('current beam: ', beamNode.GetName())

            # Prepare the ct
            ct = prepareCt(beamNode, self.temp_path)

            # Prepare the cst (segmentations)
            cst = prepareCst(beamNode, self.temp_path)

            # Prepare the plan configuration
            pln = preparePln(beamNode, self.temp_path)

            if pln['radiationMode'] == 'photons':
                pln_optim = {'RBE': 1.0}
            elif pln['radiationMode'] == 'protons':
                pln_optim = {'RBE': 1.1}

            # Get Dose Influence Matrix
            fieldData = beamNode.GetDoseInfluenceMatrixFieldData()
            data = np.array(fieldData.GetArray('Data'))
            indices = np.array(fieldData.GetArray('Indices'))
            indptr = np.array(fieldData.GetArray('Indptr'))

            numOfCols = fieldData.GetArray('Indptr').GetSize()-1

            dose_influence_matrix = csc_matrix((data, indices, indptr), shape=(numberOfVoxels, numOfCols)) # shape important to include zeros in last indices (rows must be as long as number of voxels)

            
            dose_information = {
            "resolution": {"x": 3.0,
                        "y": 3.0,
                        "z": 3.0},

            'cubeDim': tuple(referenceVolumeNode.GetImageData().GetDimensions()),

            'numOfVoxels': np.prod(referenceVolumeNode.GetImageData().GetDimensions()),

            'numOfFractions': pln['numOfFractions'],

            'physicalDose': dose_influence_matrix,  # dose influence matrix

            'totalNumOfBixels': dose_influence_matrix.shape[1]  # dof
            }

            for dimension in ('x', 'y', 'z'):
                dose_information[dimension] = np.arange(
                    ct[dimension][0],
                    ct[dimension][-1]
                    + dose_information['resolution'][dimension],
                    dose_information['resolution'][dimension])


            for i in cst:
                cst[i]['doseObjective'] = locals()[cst[i]['doseObjective']](cst, dose_information,
                                                                            cst[i]['doseConstraint'][0],
                                                                            cst[i]['doseConstraint'][1])
                cst[i]['doseConstraint'] = None


            print('calling FluenceOptimizer')
            a = FluenceOptimizer(cst, ct, pln_optim, dose_information)
            a.solve()

            dose = a.dOpt
            totalDose += dose


        # insert total dose into volume node
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
    
