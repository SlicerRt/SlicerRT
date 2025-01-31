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
        from pyRadPlan.stf import StfGeneratorIMPT, StfGeneratorPhotonIMRT

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
        ct = prepareCt(beamNode)

        # Prepare the cst (segmentations)
        cst = prepareCst(beamNode, ct)

        # Prepare the plan configuration
        pln = preparePln(beamNode)

        #Saving the ct, cst and pln dictionaries as a .mat file
        matRadIO.save(os.path.join(self.temp_path,'ct.mat'), {'ct': ct.to_matrad_dict()})
        matRadIO.save(os.path.join(self.temp_path,'cst.mat'), {'cst': cst.to_matrad_list()})
        matRadIO.save(os.path.join(self.temp_path,'ct.mat'), {'pln': pln.to_matrad_dict()})


        ##################################### PYRAD: generate stf ###############################################

        # MatRad functions ending in py which are part of the functions seen below have been slightly
        # edited to allow for calling them with the matlab engine. A specific description of the
        # changes can be found in the respective matRad functions.

        # Used for the class-based implementation
        if pln.radiation_mode in ['photons']:
            stfgen = StfGeneratorPhotonIMRT(pln)
        elif pln.radiation_mode in ['protons', 'carbon']:    
            stfgen = StfGeneratorIMPT(pln)
            
        stfgen.bixel_width = 5.0
        stfgen.gantry_angles = [0.0]

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

        ##################################### PYRAD: import libraries ##########################################
        from importlib import resources
        import logging
        import numpy as np
        import SimpleITK as sitk
        import pymatreader
        import matplotlib.pyplot as plt

        from scipy.sparse import coo_matrix

        import time

        # import pyRadPlan

        from pyRadPlan import (
            # PhotonPlan,
            # validate_ct,
            # validate_cst,
            generate_stf,
            calc_dose_influence
        )

        logging.basicConfig(level=logging.INFO)


        # ##################################### PYRAD: prepare data structures ##########################################
        # Prepare the ct
        t_start = time.time()
        ct = prepareCt(beamNode)
        t_end = time.time()
        print(f"Time to prepare CT: {t_end - t_start}")

        # Prepare the cst (segmentations)
        t_start = time.time()
        cst = prepareCst(beamNode, ct)
        t_end = time.time()
        print(f"Time to prepare CST: {t_end - t_start}")

        # Prepare the plan configuration
        t_start = time.time()
        pln = preparePln(beamNode, ct)
        t_end = time.time()
        print(f"Time to prepare PLN: {t_end - t_start}")

        # Generate Steering Geometry ("stf")
        t_start = time.time()
        stf = generate_stf(ct, cst, pln)
        t_end = time.time()
        print(f"Time to generate STF: {t_end - t_start}")

        # Calculate Dose Influence Matrix ("dij")
        t_start = time.time()
        dij = calc_dose_influence(ct, cst, stf, pln)
        t_end = time.time()
        print(f"Time to calculate Dij: {t_end - t_start}")


        # ################################ SLICER: load dose to beamNode #######################################

        # optimize storage such that we don't have multiple instances in memory
        # we use a coo matrix here as it is the most efficient way to get the matrix into slicer
        dose_matrix = coo_matrix(dij.physical_dose.flat[0])

        beamNode.SetDoseInfluenceMatrixFromTriplets(dose_matrix.shape[0], dose_matrix.shape[1],dose_matrix.row, dose_matrix.col, dose_matrix.data)

        return str() #return empty string to indicate success
    