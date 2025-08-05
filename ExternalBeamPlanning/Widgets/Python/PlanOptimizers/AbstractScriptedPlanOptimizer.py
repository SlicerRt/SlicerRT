import os
import vtk, qt, ctk, slicer, logging

#
# Abstract class of python scripted dose PlanOptimizers
#

class AbstractScriptedPlanOptimizer():
  """ Abstract scripted dose PlanOptimizers implemented in python

      USAGE:
      1. Instantiation and registration
        Instantiate dose PlanOptimizer adaptor class from
        module (e.g. from setup function), and set python source:
        > import qSlicerExternalBeamPlanningModuleWidgetsPythonQt as PlanOptimizers
        > scriptedPlanOptimizer = PlanOptimizers.qSlicerScriptedDosePlanOptimizer(None)
        > scriptedPlanOptimizer.setPythonSource(MyPlanOptimizer.filePath)
        > scriptedPlanOptimizer.self().register()
        If PlanOptimizer name is added to slicer.modules.dosePlanOptimizernames
        list then the above instantiation and registration steps are not necessary,
        as the ExternalBeamPlanning module do all these.

      2. Call host C++ implementation using
        > self.scriptedPlanOptimizer.functionName()

      2.a. Most frequently used such methods are:
        Parameter get/set: parameter, integerParameter, doubleParameter, setParameter
        Add beam parameters: addBeamParameterSpinBox, addBeamParameterSlider, addBeamParameterComboBox, addBeamParameterCheckBox

      2.b. Always call API functions (the ones that are defined in the adaptor
        class qSlicerScriptedDosePlanOptimizer) using the adaptor accessor:
        > self.scriptedPlanOptimizer.addResultDose()

      An example for a generic PlanOptimizer is the MockPythonDosePlanOptimizer

  """

  def __init__(self, scriptedPlanOptimizer):
    self.scriptedPlanOptimizer = scriptedPlanOptimizer

  def register(self):
    import qSlicerExternalBeamPlanningModuleWidgetsPythonQt
    #TODO: For some reason the instance() function cannot be called as a class function although it's static
    handler = qSlicerExternalBeamPlanningModuleWidgetsPythonQt.qSlicerPlanOptimizerPluginHandler()
    planOptimizerHandlerSingleton = handler.instance()
    planOptimizerHandlerSingleton.registerPlanOptimizer(self.scriptedPlanOptimizer)
