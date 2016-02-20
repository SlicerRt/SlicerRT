import os
import vtk, qt, ctk, slicer, logging
from SegmentEditorEffects import *

#
# Abstract class of python scripted segment editor label effects
#
# Label effects are a subtype of general effects that edit the currently selected
# segment (i.e. for things like paint or draw, but not for things like threshold or morphology)
#

class AbstractScriptedSegmentEditorLabelEffect(AbstractScriptedSegmentEditorEffect):
  """ Abstract scripted segment editor effects for effects implemented in python

      USAGE:
      1. Instantiation and registration
        Instantiate segment editor effect adaptor class from
        module (e.g. from setup function), and set python source:
        > import qSlicerSegmentationsEditorEffectsPythonQt
        > scriptedEffect = qSlicerSegmentationsEditorEffectsPythonQt.qSlicerSegmentEditorScriptedLabelEffect(None)
        > scriptedEffect.setPythonSource(MyEffect.filePath)
        Registration is automatic

      2. Call host C++ implementation using
        > self.scriptedEffect.functionName()
        
      2.a. Most frequently used such methods are:
        Parameter get/set: parameter, integerParameter, doubleParameter, setParameter
        Add options widget: addOptionsWidget 
        Coordinate transforms: rasToXy, xyzToRas, xyToRas, xyzToIjk, xyToIjk
        Convenience getters: renderWindow, renderer, viewNode
        Geometry getters: imageToWorldMatrix (for volume node and for oriented image data with segmentation)

      2.b. Always call API functions (the ones that are defined in the adaptor
        class qSlicerSegmentEditorScriptedLabelEffect) using the adaptor accessor:
        > self.scriptedEffect.updateGUIFromMRML()

      An example for a generic effect is the DrawEffect

  """

  def __init__(self, scriptedEffect):
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)
