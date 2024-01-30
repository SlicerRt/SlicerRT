import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
from DoseEngines import *

class pyRadPlanEngine(AbstractScriptedDoseEngine):
    """ Mock python dose engine to test python interface for External Beam Planning
    """

    def __init__(self, scriptedEngine):
        scriptedEngine.name = 'pyRadPlan'
        AbstractScriptedDoseEngine.__init__(self, scriptedEngine)

        temp_path = slicer.app.temporaryPath + '/pyRadPlan/'
        temp_path = os.path.normpath(temp_path)

        isExist = os.path.exists(temp_path)

        if not isExist:
            os.makedirs(temp_path)

        self.temp_path = temp_path



    def defineBeamParameters(self):


        self.scriptedEngine.addBeamParameterSpinBox(
        "pyRadPlan parameters", "numOfFractions", "number of fractions", "Range of noise added to the prescription dose (+- half of the percentage of the Rx dose)",
        0.0, 99.99, 30.0, 1.0, 2 )

        self.scriptedEngine.addBeamParameterComboBox(
        "pyRadPlan parameters", "radiationMode", "radiation mode",
        "comment", ["photons", "protons", "carbons"], 0)

        self.scriptedEngine.addBeamParameterComboBox('pyRadPlan parameters','machine','machine','comment',['generic'],0)      

    def calculateDoseUsingEngine(self, beamNode, resultDoseVolumeNode):
        # TODO: Avoid Hardcoded Information here
        os.chdir('D:/Python/pyRadPlan')
        sys.path.append('D:/Python/pyRadPlan')

        import pyRadPlan.io.matRad as matRadIO
        import pyRadPlan.matRad.engine as engine
        #from pyRadPlan.GUI import matRadGUI
        #from pyRadPlan.stf.generate_stf import generateStf
        from pyRadPlan.dose.calcPhotonDose import calcPhotonDose, calcPhotonDoseMC
        from pyRadPlan.dose.calcParticleDose import calcParticleDose, calcParticleDoseMC
        # from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.stf.StfGenerator_temporary import StfGenerator
        from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.patients._patient_loader import PatientLoader
        from pymatreader import read_mat
        from scipy.sparse import csr_matrix
        import matplotlib.pyplot as plt

        from pyRadPlan.optimization.components.objectives import (SquaredDeviation, SquaredOverdosing)

        from pyRadPlan.optimization import fluenceOptimization
        import pymedphys as pymed

        # path2pyRadPlan = os.getcwd()
        # temp_path = path2pyRadPlan + '/pyRadPlan/temp_files/'

        parentPlanNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()        


        '''
        matRad needs to be installed on the PC, and the engine initialization has to point to it
        choose between MatRadEngineOctave or MatRadEngineMatlab
        '''
        eng = engine.MatRadEngineMatlab('matRad')
        matRadV = eng.engine.matRad_version()
        print('pyRadPlan uses matRad version: ' + matRadV)

        # Prepare the ct
        ct = self.prepareCt(beamNode)

        # Prepare the cst (segmentations)
        cst = self.prepareCst(beamNode)

        # Prepare the plan configuration
        pln = self.preparePln(beamNode)

        # Generate the stf (inverse planning irradiation geommetry like beamlet positions, etc.)
        '''
        matRad functions ending in py which are part of the functions seen below have been slightly edited to allow for calling 
        them with the matlab engine. A specific description of the changes can be found in the respective matRad functions.
        '''
        stf = StfGenerator(ct, cst, pln)

        # An elaborate way to save all the dictionaries into structs that can be loaded by matlab. This will not be necessary
        # once the dose calculation and optimization functions are set. It is there as a "place-holder" and a check if the
        # stf generation works
        # https://stackoverflow.com/questions/61542500/convert-multiple-python-dictionaries-to-matlab-structure-array-with-scipy-io-sav
        matRadIO.save(self.temp_path, 'stf_with_separate_rays.mat', {'stf': np.array([stf[i] for i in range(len(stf))], dtype=object),
                                                            'rays': np.array([[stf[j]['ray'][i] for i in range(len(stf[j]['ray']))]
                                                                            for j in range(len(stf))], dtype=object)})#'ray':[stf[0]['ray'][i] for i in range(len(stf[0]['ray']))]})

        # This is required to properly load all the structs that are inside stf_with_separate_rays.mat
        eng.engine.IO_stf(self.temp_path)


        # Calculate the photon dose using a matRad function called matRad_calcPhotonDose
        # Only pencil beam implemented so far
        if pln['radiationMode']=='photons':
            calcPhotonDose('ct.mat', 'stf.mat', 'pln.mat', 'cst.mat', in_path=self.temp_path, out_path=self.temp_path)
            #calcPhotonDoseMC('ct.mat', 'stf.mat', 'pln.mat', 'cst.mat', in_path=temp_path, out_path=temp_path)
        elif pln['radiationMode']=='protons' or pln['radiationMode']=='carbon':
            calcParticleDose('ct.mat', 'stf.mat', 'pln.mat', 'cst.mat', in_path=self.temp_path, out_path=self.temp_path)

        # Setting up the plan configuration for the optimization algorithm. Only the RBE is necessary. We will get it from the
        # previously defined pln dictionary

        if pln['radiationMode'] == 'photons':
            pln_optim = {'RBE': 1.0}
        elif pln['radiationMode'] == 'protons':
            pln_optim = {'RBE': 1.1}

        ''' Preparing dose information for optimization '''

        dose_path = os.path.join(self.temp_path,'dij.mat')
        dij_mat = read_mat(dose_path)  # keeping read_mat here for now
        dose_matrix = csr_matrix(dij_mat['dij']['physicalDose'])

        dose_information = {
            "resolution": {"x": 3.0,
                        "y": 3.0,
                        "z": 3.0},

            'cubeDim': tuple(dij_mat['dij']['doseGrid']['dimensions']),

            'numOfVoxels': np.prod(tuple(dij_mat['dij']['doseGrid']['dimensions'])),

            'numOfFractions': pln['numOfFractions'],

            'physicalDose': dose_matrix,  # dose influence matrix

            'totalNumOfBixels': dose_matrix.shape[1]  # dof
        }

        for dimension in ('x', 'y', 'z'):
            dose_information[dimension] = np.arange(
                ct[dimension][0],
                ct[dimension][-1]
                + dose_information['resolution'][dimension],
                dose_information['resolution'][dimension])

        ''' Preparing the constraints and objectives as they are not read with loadmat '''

        # Loading objectives and constraints from the patient cst
        for i in cst:
            cst[i]['doseObjective'] = locals()[cst[i]['doseObjective']](cst, dose_information,
                                                                        cst[i]['doseConstraint'][0],
                                                                        cst[i]['doseConstraint'][1])
            cst[i]['doseConstraint'] = None

        a = FluenceOptimizer(cst, ct, pln_optim, dose_information)
        a.solve()

        matRadIO.save(self.temp_path, 'physicalDose.mat', {'physicalDose': a.dOpt})

        ''' load to slicer'''

        data = a.dOpt

        import vtk.util.numpy_support as numpy_support

        flat_data_array = data.swapaxes(0,2).swapaxes(2,1).flatten()
        vtk_data = numpy_support.numpy_to_vtk(num_array=flat_data_array, deep=True, array_type=vtk.VTK_FLOAT)

        imageData = vtk.vtkImageData()
        imageData.DeepCopy(referenceVolumeNode.GetImageData())
        imageData.GetPointData().SetScalars(vtk_data)

    
        resultDoseVolumeNode.SetOrigin(referenceVolumeNode.GetOrigin())
        resultDoseVolumeNode.SetSpacing(referenceVolumeNode.GetSpacing())
        ijkToRASDirections = np.eye(3)
        referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
        resultDoseVolumeNode.SetIJKToRASDirections(ijkToRASDirections)
        resultDoseVolumeNode.SetAndObserveImageData(imageData)


        # Set name
        # DoseNodeName = str(beamNode.GetName())+"_pyRadDose"
        # resultDoseVolumeNode.SetName(DoseNodeName)

    def prepareCt(self, beamNode):
        import pyRadPlan.io.matRad as matRadIO

        # accessing nodes in Slicer
        # beamNode = slicer.mrmlScene.GetNodeByID('vtkMRMLRTBeamNode1')
        parentPlanNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()

        # CT

        # MATLAB format (needed at the moment to run Matlab functions)
        cubeHU = np.empty((1, 1), dtype=object)
        cubeHU[0,0] = np.double(np.transpose(slicer.util.arrayFromVolume(referenceVolumeNode),(1,2,0)))

        ct = {
            'resolution':{
                'x': referenceVolumeNode.GetSpacing()[0],
                'y': referenceVolumeNode.GetSpacing()[1],
                'z': referenceVolumeNode.GetSpacing()[2]
                },
            'cubeDim': referenceVolumeNode.GetImageData().GetDimensions(),
            'numOfCtScen': 1,
            'cubeHU': cubeHU
        }

        origin=referenceVolumeNode.GetOrigin()

        ijkToRASDirections = np.eye(3)
        referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
        origin = ijkToRASDirections @ origin

        
        ct['x'] =  ct['resolution']['x']*np.arange(0,ct['cubeDim'][1]).astype(np.double) - ct['resolution']['x']/2.0 - abs(origin[0])
        ct['y'] =  ct['resolution']['y']*np.arange(0,ct['cubeDim'][0]).astype(np.double) - ct['resolution']['y']/2.0 - abs(origin[1])
        ct['z'] =  ct['resolution']['z']*np.arange(0,ct['cubeDim'][2]).astype(np.double) - ct['resolution']['z']/2.0 - abs(origin[2])

        #ct['SliceThickness'] = np.ones(ct['cubeDim'][2])*ct['resolution']['z'] # ???? data.SliceThickness = ct['resolution']['z']
        ct['number_of_voxels'] = np.prod(ct['cubeDim'])

        #Saving the ct dictionary as a .mat file
        matRadIO.save(self.temp_path, 'ct.mat', {'ct': ct})

        # convert ct back to PYTHON format
        ct['cubeHU'] = np.transpose(slicer.util.arrayFromVolume(referenceVolumeNode),(1,2,0))

        return ct
    
    def prepareCst(self, beamNode):
        import pyRadPlan.io.matRad as matRadIO

        # CST
        parentPlanNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()
        node = parentPlanNode.GetSegmentationNode()
        segNode = node.GetSegmentation()

        cst ={}
        index = 0

        for id in segNode.GetSegmentIDs():
            
            print('Converting segmentation: ' + id)
            segmentArray = slicer.util.arrayFromSegmentBinaryLabelmap(node, id, referenceVolumeNode)#.swapaxes(0,1)
            segmentArray = np.transpose(segmentArray,(1,2,0))
            segmentArray = np.asarray(segmentArray).ravel(order='F')
            nonZeroIndices=np.where([segmentArray>0])[1]

            print('Number of voxels in Segmentation: ' + str(len(nonZeroIndices)))

            cst[id] = {
                'index': index,
                'raw_indices': nonZeroIndices,
                'prioritized_indices': nonZeroIndices,
                'resized_indices': nonZeroIndices, #same indices (raw,prioritized,resized); like in patientLoader
            }

            if id==parentPlanNode.GetTargetSegmentID():
                cst[id]['type']='TARGET'
                cst[id]['doseObjective'] = 'SquaredDeviation'
                cst[id]['doseConstraint'] = [50.0,1000.0]
                cst[id]['parameters'] = {'priority' : 1.0,'alphaX' : 0.1,'betaX' : 0.05}
            else:
                cst[id]['type']='OAR'
                cst[id]['doseObjective'] = 'SquaredOverdosing'
                cst[id]['doseConstraint'] = [25.0,300.0]
                cst[id]['parameters'] = {'priority' : 2.0,'alphaX' : 0.1,'betaX' : 0.05}

            currentSegmentation = segNode.GetSegment(id)
            
            cst[id]['parameters']['visibleColor'] = currentSegmentation.GetColor()
            cst[id]['parameters']['Visible'] = True

            index += 1


        # MATLAB format (needed at the moment to run Matlab functions)
        cstForMat = np.empty((len(cst),6), dtype=object)
        

        index = 0
        for id in cst:
            cellArrayIndices = np.empty((1,1), dtype=object)
            cellArrayDose = np.empty((1,1), dtype=object)
            cellArrayDoseParameters = np.empty((1,1), dtype=object)
            print('Converting segmentation: ' + id + ' into matRad/pyRadPlan cst format')
            cellArrayIndices[0][0] = np.double(cst[id]['raw_indices']).reshape(-1,1) #???

            print('Number of voxels in Segmentation: ' + str(len(cellArrayIndices[0][0])))
            
            cellArrayDoseParameters[0][0] = cst[id]['doseConstraint'][0]
            cellArrayDose[0][0] = {'className' : 'DoseObjectives.matRad_'+cst[id]['doseObjective'],
                                'parameters' : cellArrayDoseParameters,
                                'penalty' : cst[id]['doseConstraint'][1]
                                }

            cstForMat[index][0] = cst[id]['index']
            cstForMat[index][1] = id
            cstForMat[index][2] = cst[id]['type']
            cstForMat[index][3] = cellArrayIndices
            cstForMat[index][4] = cst[id]['parameters']
            cstForMat[index][5] = cellArrayDose


            index += 1

        #Saving the cst dictionary as a .mat file
        matRadIO.save(self.temp_path, 'cst.mat', {'cst': cstForMat})

        return cst
    
    def preparePln(self, beamNode):
        import pyRadPlan.io.matRad as matRadIO

        '''
        All values in the pln dictionary need to be floats. INCLUDING the numOfFractions.
        '''
        parentPlanNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()

        origin=referenceVolumeNode.GetOrigin()
        ijkToRASDirections = np.eye(3)
        referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
        origin = ijkToRASDirections @ origin

        isocenter = [0]*3
        parentPlanNode.GetIsocenterPosition(isocenter)
        isocenter = ijkToRASDirections @ np.array(isocenter) - np.array(origin)

        pln = {
            "radiationMode": ['photons','proton','carbon'][int(self.scriptedEngine.doubleParameter(beamNode,'radiationMode'))],
            "machine": ['Generic'][int(self.scriptedEngine.doubleParameter(beamNode,'machine'))],
            "numOfFractions": self.scriptedEngine.doubleParameter(beamNode,'numOfFractions'),
            "propStf": {

                # beam geometry settings
                "bixelWidth": 5.0,
                "gantryAngles": [beamNode.GetGantryAngle()], #"gantryAngles": [parentPlanNode.GetBeamByNumber(i).GetGantryAngle() for i in range(1,numOfBeams+1)],
                "couchAngles": [beamNode.GetCouchAngle()], #[parentPlanNode.GetBeamByNumber(i).GetCouchAngle() for i in range(1,numOfBeams+1)],
                "numOfBeams": 1.0,
                "isoCenter": [isocenter]
            },

            # dose calculation settings
            "propDoseCalc": {"doseGrid": {"resolution": {"x": 3.0,#parentPlanNode.GetDoseGrid()[0],
                                                        "y": 3.0,#parentPlanNode.GetDoseGrid()[1],
                                                        "z": 3.0,#parentPlanNode.GetDoseGrid()[2],
                                                        },
                                        },
                            },

            # optimization settings
            "propOpt": {"optimizer": 'IPOPT',
                        "bioOptimization": 'none',
                        "runDAO": False,
                        "runSequencing": True
                        },
        }

        #Saving the pln dictionary as a .mat file
        matRadIO.save(self.temp_path, 'pln.mat', {'pln': pln})

        return pln


        
        