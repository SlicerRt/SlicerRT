import os
from contextlib import contextmanager
import vtk, qt, ctk, slicer, logging
from Python.LogForwarding import forwardLogging

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

      3. Show status text next to the Optimize button and write messages to the
        optimization log of the module
        > self.progressInfo("status")
        > self.log("message")
        Python logging output of the optimizer module (and optionally of further loggers,
        e.g. pyRadPlanPlanOptimizer) can be forwarded too:
        > with self.forwardLogging():
        >   logging.getLogger(__name__).info("message")

      An example for a PlanOptimizer is the pyRadPlanPlanOptimizer

  """

  def __init__(self, scriptedPlanOptimizer):
    self.scriptedPlanOptimizer = scriptedPlanOptimizer

  def progressInfo(self, info):
    self.scriptedPlanOptimizer.progressInfoUpdated(info)

  def log(self, message):
    self.scriptedPlanOptimizer.logMessageAdded(str(message))

  @contextmanager
  def forwardLogging(self, loggerNames=(), level=logging.INFO):
    with forwardLogging(self.log, [self.__class__.__module__] + list(loggerNames), level) as handler:
      yield handler

  def register(self):
    import qSlicerExternalBeamPlanningModuleWidgetsPythonQt
    #TODO: For some reason the instance() function cannot be called as a class function although it's static
    handler = qSlicerExternalBeamPlanningModuleWidgetsPythonQt.qSlicerPlanOptimizerPluginHandler()
    planOptimizerHandlerSingleton = handler.instance()
    planOptimizerHandlerSingleton.registerPlanOptimizer(self.scriptedPlanOptimizer)
