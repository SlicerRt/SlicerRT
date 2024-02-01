import os
import sys
import vtk, qt, ctk, slicer
import numpy as np
import logging
import random
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
        println('pyRadPlan Optimizer is called')
        return "Not implemented yet!"