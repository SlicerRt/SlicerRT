import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorDilateEffect(AbstractScriptedSegmentEditorMorphologyEffect):
  """ DilateEffect is an MorphologyEffect to
      dilate a layer of pixels from a segment
  """

  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'Dilate'
    AbstractScriptedSegmentEditorMorphologyEffect.__init__(self, scriptedEffect)

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedMorphologyEffect(None)
    clonedEffect.setPythonSource(SegmentEditorDilateEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Dilate.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()

  def helpText(self):
    return "Add pixels to the boundary of selected segment."

  def setupOptionsFrame(self):
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Dilate selected segment")
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

    # Perform dilation
    # (use erode filter to dilate by eroding background)
    #eroder = slicer.vtkImageErode()
    #eroder.SetInputData(selectedSegmentLabelmap)
    #eroder.SetForeground(0) # Erode becomes dilate by switching the labels
    #eroder.SetBackground(1)
    #if neighborMode == 8:
    #  eroder.SetNeighborTo8()
    #elif neighborMode == 4:
    #  eroder.SetNeighborTo4()
    #else:
    #  logging.error("Invalid neighbor mode!")
    #for i in xrange(iterations):
    #  eroder.Update()
    #editedLabelmap.DeepCopy(eroder.GetOutput())

    eroder = vtk.vtkImageDilateErode3D()
    eroder.SetInputData(selectedSegmentLabelmap)
    eroder.SetDilateValue(1)
    eroder.SetErodeValue(0)
    eroder.SetKernelSize(5,5,5)
    eroder.Update()
    editedLabelmap.DeepCopy(eroder.GetOutput())

    #import vtkSegmentationCorePython as vtkSegmentationCore
    #maskLabelmapCopy = vtkSegmentationCore.vtkOrientedImageData()
    #maskLabelmapCopy.DeepCopy(self.scriptedEffect.maskLabelmap())
    #labelmapVolumeNode = slicer.vtkMRMLLabelMapVolumeNode()
    #slicer.mrmlScene.AddNode(labelmapVolumeNode)
    #slicer.vtkSlicerSegmentationsModuleLogic.CreateLabelmapVolumeFromOrientedImageData(
    #  maskLabelmapCopy, labelmapVolumeNode)
    #labelmapVolumeNode.CreateDefaultDisplayNodes()

    self.scriptedEffect.setEditedLabelmapApplyModeToSet()
    self.scriptedEffect.setEditedLabelmapApplyExtentToWholeExtent()
    self.scriptedEffect.apply()
