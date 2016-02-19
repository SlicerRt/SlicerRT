import os
import vtk, qt, ctk, slicer, logging

#
# Abstract class of python scripted segment editor effects
#

class AbstractScriptedSegmentEditorEffect():
  """ Abstract scripted segment editor effects for effects implemented in python

      USAGE:
        # Instantiate segment editor effect adaptor class from
        # module (e.g. from setup function), and set python source:
        ...
        import qSlicerSegmentationsEditorEffectsPythonQt
        scriptedEffect = qSlicerSegmentationsEditorEffectsPythonQt.qSlicerSegmentEditorScriptedEffect(None)
        scriptedEffect.setPythonSource(MyEffect.filePath)
        ...
        # Call host C++ implementation using
        self.scriptedEffect.functionName()
        
        # Most frequently used such methods are:
        # Parameter get/set: parameter, integerParameter, doubleParameter, setParameter
        # Add options widget: addOptionsWidget 
        # Coordinate transforms: rasToXy, xyzToRas, xyToRas, xyzToIjk, xyToIjk
        # Convenience getters: renderWindow, renderer, viewNode

      An example for a generic effect is the ThresholdEffect
  """

  def __init__(self, scriptedEffect):
    self.scriptedEffect = scriptedEffect

    # Register plugin on initialization
    self.register()

  def register(self):
    import qSlicerSegmentationsEditorEffectsPythonQt
    #TODO: For some reason the instance() function cannot be called as a class function although it's static
    factory = qSlicerSegmentationsEditorEffectsPythonQt.qSlicerSegmentEditorEffectFactory()
    effectFactorySingleton = factory.instance()
    effectFactorySingleton.registerEffect(self.scriptedEffect)
    logging.debug('Scripted segment editor effects registered: ' + self.scriptedEffect.name)
