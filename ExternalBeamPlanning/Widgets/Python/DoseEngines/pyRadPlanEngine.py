import os
import sys
sys.path.append('C:/l868r/pyRadPlan')

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

        ############################### SLICER: change os path to pyRadPlan ####################################
        # TODO: Avoid Hardcoded Information here
        os.chdir('C:/l868r/pyRadPlan')
        sys.path.append('C:/l868r/pyRadPlan')

        ##################################### PYRAD: import libraries ##########################################
        from pyRadPlan import stf

        from pyRadPlan.plan import create_pln

        import pyRadPlan.io.matRad as matRadIO
        import pyRadPlan.matRad as matRad
        import pyRadPlan.dose as dose
        from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.patients._patient_loader import PatientLoader
        from pyRadPlan.stf import StfGeneratorIMPT

        # Ignore deprication warnings
        # np.warnings.filterwarnings('ignore', category=np.VisibleDeprecationWarning)


        ###################################### SLICER: load nodes ##############################################
        parentPlanNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()        


        ##################################### PYRAD: prepare engine ############################################
        # matRad needs to be installed on the PC, and the engine initialization has to point to it.
        # Choose between MatRadEngineOctave or MatRadEngineMatlab
        eng = matRad.MatRadEngineMatlab("matRad")
        # eng = matRad.MatRadEngineOctave('matRad',
        #     octave_exec = "C:/Program Files/GNU Octave/Octave-8.4.0/mingw64/bin/octave-cli.exe")
        matRad.setEngine(eng)


        ##################################### SLICER: prepare data ##############################################
        # Prepare the ct
        ct = prepareCt(beamNode, self.temp_path)

        # Prepare the cst (segmentations)
        cst = prepareCst(beamNode, ct, self.temp_path)

        # Prepare the plan configuration
        pln = preparePln(beamNode, self.temp_path)


        ##################################### PYRAD: generate stf ###############################################

        # Creates a validated pln pydantic dataclass from the dictionary
        pln = create_pln(pln)

        # For now we dump the model again as we ahve not implemented plan management down the road
        pln = pln.to_matrad_dict()


        # MatRad functions ending in py which are part of the functions seen below have been slightly
        # edited to allow for calling them with the matlab engine. A specific description of the
        # changes can be found in the respective matRad functions.

        # Used for the class-based implementation
        stfgen = StfGeneratorIMPT(pln)
        stf = stfgen.generate(ct, cst)

        # doseInit = dose.calcDoseInit(ct, cst, stf, pln)  # Testing native dose engine

        # Calculate the photon dose using a matRad function called matRad_calcPhotonDose
        # Only pencil beam implemented so far
        # This can also be moved behind the curtains
        if pln["radiationMode"] == "photons":
            dose.calcPhotonDose(ct, stf, pln, cst)
            # dose.calcPhotonDoseMC(ct, stf, pln, cst, 10)
        elif pln["radiationMode"] == "protons" or pln["radiationMode"] == "carbon":
            dose.calcParticleDose(ct, stf, pln, cst)

        optimizer = FluenceOptimizer(cst, ct, pln)
        optimizer.solve()

        matRadIO.save(os.path.join(self.temp_path, "physicalDose.mat"), {"physicalDose": optimizer.dOpt})

        # Once the run is finished, loading all the .mat files generated (cst, ct, pln, resultGUI, stf)
        # in Matlab will allow the user to run the function matRadGUI and visualize the plan.


        ############################## SLICER: load optimized dose to Slicer ####################################

        data = optimizer.dOpt

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

        ########################## SLICER: change os path to pyRadPlan & imports #############ä#################
        # TODO: Avoid Hardcoded Information here
        os.chdir('C:/l868r/pyRadPlan')
        sys.path.append('C:/l868r/pyRadPlan')

        from scipy.sparse import coo_matrix
        from scipy.io import loadmat as read_mat


        ##################################### PYRAD: import libraries ##########################################
        from pyRadPlan import stf

        from pyRadPlan.plan import create_pln

        import pyRadPlan.io.matRad as matRadIO
        import pyRadPlan.matRad as matRad
        import pyRadPlan.dose as dose
        from pyRadPlan.optimization._fluenceOptimizer import FluenceOptimizer
        from pyRadPlan.patients._patient_loader import PatientLoader
        from pyRadPlan.stf import StfGeneratorIMPT

        # Ignore deprication warnings
        # np.warnings.filterwarnings('ignore', category=np.VisibleDeprecationWarning)


        ##################################### PYRAD: prepare engine ############################################
        # matRad needs to be installed on the PC, and the engine initialization has to point to it.
        # Choose between MatRadEngineOctave or MatRadEngineMatlab
        eng = matRad.MatRadEngineMatlab("matRad")
        # eng = matRad.MatRadEngineOctave('matRad',
        #     octave_exec = "C:/Program Files/GNU Octave/Octave-8.4.0/mingw64/bin/octave-cli.exe")
        matRad.setEngine(eng)


        # Prepare the ct
        ct = prepareCt(beamNode, self.temp_path)

        # Prepare the cst (segmentations)
        cst = prepareCst(beamNode, ct,  self.temp_path)

        # Prepare the plan configuration
        pln = preparePln(beamNode, self.temp_path)


        ############################ PYRAD: validate ct, cst , pln & generate stf ################################

        # Creates a validated pln pydantic dataclass from the dictionary
        pln = create_pln(pln)

        # For now we dump the model again as we ahve not implemented plan management down the road
        pln = pln.to_matrad_dict()


        # MatRad functions ending in py which are part of the functions seen below have been slightly
        # edited to allow for calling them with the matlab engine. A specific description of the
        # changes can be found in the respective matRad functions.

        # Used for the class-based implementation
        stfgen = StfGeneratorIMPT(pln)
        stf = stfgen.generate(ct, cst)

        # doseInit = dose.calcDoseInit(ct, cst, stf, pln)  # Testing native dose engine

        # Calculate the photon dose using a matRad function called matRad_calcPhotonDose
        # Only pencil beam implemented so far
        # This can also be moved behind the curtains
        if pln["radiationMode"] == "photons":
            dose.calcPhotonDose(ct, stf, pln, cst)
            # dose.calcPhotonDoseMC(ct, stf, pln, cst, 10)
        elif pln["radiationMode"] == "protons" or pln["radiationMode"] == "carbon":
            dose.calcParticleDose(ct, stf, pln, cst)


        ################################ SLICER: load dose to beamNode #######################################

        dose_path = os.path.join(self.temp_path,'dij.mat')
        dij_mat = read_mat(dose_path)  # keeping read_mat here for now

        # optimize storage such that we don't have multiple instances in memory
        # we use a coo matrix here as it is the most efficient way to get the matrix into slicer
        dose_matrix = coo_matrix(dij_mat['dij']['physicalDose'])

        beamNode.SetDoseInfluenceMatrixFromTriplets(dose_matrix.shape[0], dose_matrix.shape[1],dose_matrix.row, dose_matrix.col, dose_matrix.data)

        return str() #return empty string to indicate success
    