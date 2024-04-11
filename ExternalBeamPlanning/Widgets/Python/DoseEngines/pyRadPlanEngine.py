import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
from DoseEngines import *
from Python.prepareRTDataset import prepareCt, prepareCst, preparePln

class pyRadPlanEngine(AbstractScriptedDoseEngine):
    """ Mock python dose engine to test python interface for External Beam Planning
    """

    def __init__(self, scriptedEngine):
        scriptedEngine.name = 'pyRadPlan'
        scriptedEngine.isInverse = True #pyRadPlan has Inverse planning capabilities, i.e., it can compute a dose influence matrix
        AbstractScriptedDoseEngine.__init__(self, scriptedEngine)

        temp_path = slicer.app.temporaryPath + '/pyRadPlan/'
        #temp_path = os.path.normpath(temp_path)

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
        os.chdir('C:/l868r/pyRadPlan')
        sys.path.append('C:/l868r/pyRadPlan')

        from pyRadPlan import stf

        import pyRadPlan.io.matRad as matRadIO
        import pyRadPlan.matRad as matRad
        #from pyRadPlan.GUI import matRadGUI
        #from pyRadPlan.stf.generate_stf import generateStf
        from pyRadPlan.dose.calcPhotonDose import calcPhotonDose, calcPhotonDoseMC
        from pyRadPlan.dose.calcParticleDose import calcParticleDose, calcParticleDoseMC
        # from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.stf.StfGenerator_temporary import StfGenerator
        from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.patients._patient_loader import PatientLoader
        from pymatreader import read_mat
        from scipy.sparse import coo_matrix, csr_matrix
        import matplotlib.pyplot as plt

        from pyRadPlan.optimization.components.objectives import (SquaredDeviation, SquaredOverdosing)

        from pymatreader import read_mat
        from scipy.sparse import csr_matrix
        # import pymedphys as pymed

        # Ignore deprication warnings
        # np.warnings.filterwarnings('ignore', category=np.VisibleDeprecationWarnin


        parentPlanNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()        


        '''
        matRad needs to be installed on the PC, and the engine initialization has to point to it
        choose between MatRadEngineOctave or MatRadEngineMatlab
        '''
        eng = matRad.MatRadEngineMatlab('matRad')
        # eng = matRad.MatRadEngineOctave('matRad',octave_exec = "C:/Program Files/GNU Octave/Octave-8.4.0/mingw64/bin/octave-cli.exe")
        matRad.setEngine(eng)

        # Prepare the ct
        ct = prepareCt(beamNode, self.temp_path)

        # Prepare the cst (segmentations)
        cst = prepareCst(beamNode, self.temp_path)

        # Prepare the plan configuration
        pln = preparePln(beamNode, self.temp_path)

        # Generate the stf (inverse planning irradiation geommetry like beamlet positions, etc.)
        '''
        matRad functions ending in py which are part of the functions seen below have been slightly edited to allow for calling 
        them with the matlab engine. A specific description of the changes can be found in the respective matRad functions.
        '''

        # Used for the class-based implementation
        # s = stf.photons.StfGeneratorPhotonBixel(ct, cst, pln, engine)
        # stf = s.generate()

        stf = StfGenerator(ct, cst, pln)  # This still calls the function from StfGenerator_temporary.py until the class-based
        # stf = StfGenerator(ct,cst,pln,eng)

        # An elaborate way to save all the dictionaries into structs that can be loaded by matlab. This will not be necessary
        # once the dose calculation and optimization functions are set. It is there as a "place-holder" and a check if the
        # stf generation works
        # https://stackoverflow.com/questions/61542500/convert-multiple-python-dictionaries-to-matlab-structure-array-with-scipy-io-sav
        matRadIO.save(self.temp_path, 'stf_with_separate_rays.mat', {'stf': np.array([stf[i] for i in range(len(stf))], dtype=object),
                                                            'rays': np.array([[stf[j]['ray'][i] for i in range(len(stf[j]['ray']))]
                                                                            for j in range(len(stf))], dtype=object)})

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
        DoseNodeName = str(beamNode.GetName())+"_pyRadDose"
        resultDoseVolumeNode.SetName(DoseNodeName)
        
        return str() #return empty string to indicate success
    
    def calculateDoseInfluenceMatrixUsingEngine(self, beamNode):
        # TODO: Avoid Hardcoded Information here
        os.chdir('C:/l868r/pyRadPlan')
        sys.path.append('C:/l868r/pyRadPlan')

        from pyRadPlan import stf

        import pyRadPlan.io.matRad as matRadIO
        import pyRadPlan.matRad as matRad
        #from pyRadPlan.GUI import matRadGUI
        #from pyRadPlan.stf.generate_stf import generateStf
        from pyRadPlan.dose.calcPhotonDose import calcPhotonDose, calcPhotonDoseMC
        from pyRadPlan.dose.calcParticleDose import calcParticleDose, calcParticleDoseMC
        # from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.stf.StfGenerator_temporary import StfGenerator
        from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.patients._patient_loader import PatientLoader
        from pymatreader import read_mat
        from scipy.sparse import coo_matrix, csr_matrix
        import matplotlib.pyplot as plt

        from pyRadPlan.optimization.components.objectives import (SquaredDeviation, SquaredOverdosing)

        from pymatreader import read_mat
        from scipy.sparse import csr_matrix
        # import pymedphys as pymed

        # Ignore deprication warnings
        # np.warnings.filterwarnings('ignore', category=np.VisibleDeprecationWarnin


        '''
        matRad needs to be installed on the PC, and the engine initialization has to point to it
        choose between MatRadEngineOctave or MatRadEngineMatlab
        '''
        eng = matRad.MatRadEngineMatlab('matRad')
        # eng = matRad.MatRadEngineOctave('matRad',octave_exec = "C:/Program Files/GNU Octave/Octave-8.4.0/mingw64/bin/octave-cli.exe")
        matRad.setEngine(eng)

        # Prepare the ct
        ct = prepareCt(beamNode, self.temp_path)

        # Prepare the cst (segmentations)
        cst = prepareCst(beamNode, self.temp_path)

        # Prepare the plan configuration
        pln = preparePln(beamNode, self.temp_path)

        # Generate the stf (inverse planning irradiation geommetry like beamlet positions, etc.)
        '''
        matRad functions ending in py which are part of the functions seen below have been slightly edited to allow for calling 
        them with the matlab engine. A specific description of the changes can be found in the respective matRad functions.
        '''

        # Used for the class-based implementation
        # s = stf.photons.StfGeneratorPhotonBixel(ct, cst, pln, engine)
        # stf = s.generate()

        stf = StfGenerator(ct, cst, pln)  # This still calls the function from StfGenerator_temporary.py until the class-based
        # stf = StfGenerator(ct,cst,pln,eng)

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

        # optimize storage such that we don't have multiple instances in memory
        # we use a coo matrix here as it is the most efficient way to get the matrix into slicer
        dose_matrix = coo_matrix(dij_mat['dij']['physicalDose'])

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
            
        beamNode.SetDoseInfluenceMatrixFromTriplets(dose_matrix.shape[0], dose_matrix.shape[1],dose_matrix.row, dose_matrix.col, dose_matrix.data)

        return str() #return empty string to indicate success