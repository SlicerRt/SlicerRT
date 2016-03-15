import os
import vtk, qt, ctk, slicer, logging
from AbstractScriptedSegmentEditorEffect import *

__all__ = ['AbstractScriptedSegmentEditorMorphologyEffect']

#
# Abstract class of python scripted segment editor morphology effects
#
# Morphology effects are a subtype of general effects that implement math morphology
# operations (erode, dilate).
#

class AbstractScriptedSegmentEditorMorphologyEffect(AbstractScriptedSegmentEditorEffect):
  """ Abstract scripted segment editor morphology effects for effects implemented in python

      USAGE:
      1. Instantiation and registration
        Instantiate segment editor morphology effect adaptor class from
        module (e.g. from setup function), and set python source:
        > import qSlicerSegmentationsEditorEffectsPythonQt as effects
        > scriptedEffect = effects.qSlicerSegmentEditorScriptedMorphologyEffect(None)
        > scriptedEffect.setPythonSource(MyMorphologyEffect.filePath)
        Registration is automatic

      2. Call host C++ implementation using
        > self.scriptedEffect.functionName()
        
      2.a. Most frequently used such methods are:
        Parameter get/set: parameter, integerParameter, doubleParameter, setParameter
        Add options widget: addOptionsWidget 
        Coordinate transforms: rasToXy, xyzToRas, xyToRas, xyzToIjk, xyToIjk
        Convenience getters: renderWindow, renderer, viewNode

      2.b. Always call API functions (the ones that are defined in the adaptor
        class qSlicerSegmentEditorScriptedMorphologyEffect) using the adaptor accessor:
        > self.scriptedEffect.updateGUIFromMRML()

      An example for a generic effect is the DilateEffect

  """

  def __init__(self, scriptedEffect):
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)
