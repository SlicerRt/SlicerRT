from DoseEngines import *
from Python.prepareRTDataset import prepareCt, prepareCst, preparePln

class pyRadPlanEngine(AbstractScriptedDoseEngine):
    """ Mock python dose engine to test python interface for External Beam Planning
    """

    def __init__(self, scriptedEngine):
        scriptedEngine.name = 'pyRadPlan'
        scriptedEngine.isInverse = True #pyRadPlan has Inverse planning capabilities, i.e., it can compute a dose influence matrix
        scriptedEngine.canDoIonPlan = True
        AbstractScriptedDoseEngine.__init__(self, scriptedEngine)


    def defineBeamParameters(self):

        self.scriptedEngine.addBeamParameterSpinBox(
        "pyRadPlan parameters", "numOfFractions", "number of fractions", "Range of noise added to the prescription dose (+- half of the percentage of the Rx dose)",
        0.0, 99.99, 30.0, 1.0, 2 )

        self.scriptedEngine.addBeamParameterComboBox(
        "pyRadPlan parameters", "radiationMode", "radiation mode",
        "comment", ["photons", "protons", "carbons"], 0)

        self.scriptedEngine.addBeamParameterComboBox('pyRadPlan parameters','machine','machine','comment',['generic'],0)      


    def updateBeamParametersForIonPlan(self, isIonPlanActive):
        if isIonPlanActive:
            available_radiation_modes = ["protons", "carbons"]
            parameter_label = "radiation mode (ion)"
        else:
            available_radiation_modes = ["photons", "protons", "carbons"]
            parameter_label = "radiation mode"

        self.scriptedEngine.updateBeamParameterComboBox(
        "pyRadPlan parameters", "radiationMode", parameter_label,
        "comment", available_radiation_modes, 0)


    def calculateDoseUsingEngine(self, beamNode, resultDoseVolumeNode):

        #################################### import pyRadPlan libraries ######################################
        import logging
        import time

        import numpy as np
        import sitkUtils
        import slicer

        from pyRadPlan import (
            generate_stf,
            calc_dose_influence
        )

        logging.basicConfig(level=logging.INFO)


        ##################################### prepare data structures ########################################
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


        ##################################### visualize dose in Slicer #######################################
        # TEMPORARY: use uniform fluence
        fluence = np.ones(dij.total_num_of_bixels)
        result = dij.compute_result_ct_grid(fluence)
        totalDose = result["physical_dose"]

        planNode = beamNode.GetParentPlanNode()
        referenceVolumeNode = planNode.GetReferenceVolumeNode()

        # Push total dose to volumeNode in Slicer, set name & overlay on CT
        sitkUtils.PushVolumeToSlicer(totalDose, targetNode = resultDoseVolumeNode)
        resultDoseNodeName = str(planNode.GetName())+"_pyRadDose_" + beamNode.GetName()
        resultDoseVolumeNode.SetName(resultDoseNodeName)
        slicer.util.setSliceViewerLayers(background=referenceVolumeNode, foreground=resultDoseVolumeNode)
        slicer.util.setSliceViewerLayers(foregroundOpacity=1)

        return str() #return empty string to indicate success
    

    
    def calculateDoseInfluenceMatrixUsingEngine(self, beamNode):
        
        #################################### import pyRadPlan libraries ######################################
        import logging
        from scipy.sparse import coo_matrix
        import time

        from pyRadPlan import (
            generate_stf,
            calc_dose_influence
        )

        logging.basicConfig(level=logging.INFO)

        
        ##################################### prepare data structures ########################################
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

        
        ###################################### load dose to beamNode #########################################

        # Optimize storage such that we don't have multiple instances in memory
        # we use a coo matrix here as it is the most efficient way to get the matrix into slicer
        dose_matrix = coo_matrix(dij.physical_dose.flat[0])

        beamNode.SetDoseInfluenceMatrixFromTriplets(
            dose_matrix.shape[0], dose_matrix.shape[1],
            dose_matrix.row,
            dose_matrix.col,
            dose_matrix.data,
            dij.dose_grid.dimensions, #set dimensions of dose grid in beamNode for which the dose influence matrix is defined
            dij.dose_grid.resolution_vector) #set spacing of dose grid in beamNode for which the dose influence matrix is defined

        return str() #return empty string to indicate success
    