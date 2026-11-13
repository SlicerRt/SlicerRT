import os
from contextlib import contextmanager
import vtk, qt, ctk, slicer, logging
from Python.LogForwarding import forwardLogging

#
# Abstract class of python scripted dose engines
#

class AbstractScriptedDoseEngine():
  """ Abstract scripted dose engines implemented in python

      USAGE:
      1. Instantiation and registration
        Instantiate dose engine adaptor class from
        module (e.g. from setup function), and set python source:
        > import qSlicerExternalBeamPlanningModuleWidgetsPythonQt as engines
        > scriptedEngine = engines.qSlicerScriptedDoseEngine(None)
        > scriptedEngine.setPythonSource(MyEngine.filePath)
        > scriptedEngine.self().register()
        If engine name is added to slicer.modules.doseEngineNames
        list then the above instantiation and registration steps are not necessary,
        as the ExternalBeamPlanning module do all these.

      2. Call host C++ implementation using
        > self.scriptedEngine.functionName()

      2.a. Most frequently used such methods are:
        Parameter get/set: parameter, integerParameter, doubleParameter, setParameter
        Add beam parameters: addBeamParameterSpinBox, addBeamParameterSlider, addBeamParameterComboBox, addBeamParameterCheckBox

      2.b. Always call API functions (the ones that are defined in the adaptor
        class qSlicerScriptedDoseEngine) using the adaptor accessor:
        > self.scriptedEngine.addResultDose()

      3. Write messages to the dose calculation log of the module
        > self.log("message")
        Python logging output of the engine module (and optionally of further loggers,
        e.g. pyRadPlanEngine) can be forwarded too:
        > with self.forwardLogging():
        >   logging.getLogger(__name__).info("message")

      An example for a generic engine is the MockPythonDoseEngine

  """

  def __init__(self, scriptedEngine):
    self.scriptedEngine = scriptedEngine

  def log(self, message):
    self.scriptedEngine.logMessageAdded(str(message))

  @contextmanager
  def forwardLogging(self, loggerNames=(), level=logging.INFO):
    with forwardLogging(self.log, [self.__class__.__module__] + list(loggerNames), level) as handler:
      yield handler

  def register(self):
    import qSlicerExternalBeamPlanningModuleWidgetsPythonQt
    #TODO: For some reason the instance() function cannot be called as a class function although it's static
    handler = qSlicerExternalBeamPlanningModuleWidgetsPythonQt.qSlicerDoseEnginePluginHandler()
    engineHandlerSingleton = handler.instance()
    engineHandlerSingleton.registerDoseEngine(self.scriptedEngine)
