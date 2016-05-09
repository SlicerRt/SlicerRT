import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorErodeEffect(AbstractScriptedSegmentEditorMorphologyEffect):
  """ ErodeEffect is an MorphologyEffect to
      erode a layer of pixels from a segment
  """

  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'Erode'
    AbstractScriptedSegmentEditorMorphologyEffect.__init__(self, scriptedEffect)

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedMorphologyEffect(None)
    clonedEffect.setPythonSource(SegmentEditorErodeEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Erode.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()

  def helpText(self):
    return "Use this tool to remove pixels from the boundary of the current label."

  def setupOptionsFrame(self):
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Erode selected segment")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.applyButton.connect('clicked()', self.onApply)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def onApply(self):
    # Get parameters
    neighborMode = self.scriptedEffect.integerParameter("NeighborMode")
    iterations = self.scriptedEffect.integerParameter("Iterations")

    # Get edited labelmap
    editedLabelmap = self.scriptedEffect.editedLabelmap()
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()    

    # Perform erosion
    eroder = slicer.vtkImageErode()
    eroder.SetInputData(selectedSegmentLabelmap)
    eroder.SetForeground(1)
    eroder.SetBackground(0)
    if neighborMode == 8:
      eroder.SetNeighborTo8()
    elif neighborMode == 4:
      eroder.SetNeighborTo4()
    else:
      logging.error("Invalid neighbor mode!")
    for i in xrange(iterations):
      eroder.Update()
    editedLabelmap.DeepCopy(eroder.GetOutput())

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.apply()
