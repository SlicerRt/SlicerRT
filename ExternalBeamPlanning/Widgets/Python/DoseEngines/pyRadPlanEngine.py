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

        ##################################### PYRAD: import libraries ##########################################
        from importlib import resources
        import logging
        import numpy as np
        import SimpleITK as sitk
        import sitkUtils

        from pyRadPlan import (
            generate_stf,
            fluence_optimization,
        )
        from pyRadPlan.dij import Dij, compose_beam_dijs
        from pyRadPlan.optimization.objectives import get_objective


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


        #resultNode = sitkUtils.PushVolumeToSlicer(dij.physical_dose, targetNode = resultDoseVolumeNode, className="vtkMRMLScalarVolumeNode")
        
        return str() #return empty string to indicate success
    

    
    def calculateDoseInfluenceMatrixUsingEngine(self, beamNode):

        ##################################### PYRAD: import libraries ##########################################
        import logging
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
    