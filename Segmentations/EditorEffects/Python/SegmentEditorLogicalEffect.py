import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorLogicalEffect(AbstractScriptedSegmentEditorEffect):
  """ LogicalEffect is an MorphologyEffect to erode a layer of pixels from a segment
  """

  # Necessary static member to be able to set python source to scripted segment editor effect plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'Logical operators'
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(SegmentEditorLogicalEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Logical.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()

  def helpText(self):
    return "Apply logical operators on a segment or combine it with other segments."

  def setupOptionsFrame(self):
    
    self.methodSelectorComboBox = qt.QComboBox()
    self.methodSelectorComboBox.addItem("Copy from segment", LOGICAL_COPY)
    self.methodSelectorComboBox.addItem("Union with segment", LOGICAL_UNION)
    self.methodSelectorComboBox.addItem("Intersection with segment", LOGICAL_INTERSECTION)
    self.methodSelectorComboBox.addItem("Subtract segment", LOGICAL_SUBTRACT)
    self.methodSelectorComboBox.addItem("Invert", LOGICAL_INVERT)
    self.methodSelectorComboBox.addItem("Clear", LOGICAL_CLEAR)
    self.methodSelectorComboBox.addItem("Fill", LOGICAL_FILL)
    self.methodSelectorComboBox.setToolTip('<html>Available operations:<ul style="margin: 0">'
      '<li><b>Copy from segment:</b> replace the selected segment by the modifier segment.</li>'
      '<li><b>Union with segment:</b> add modifier segment to current segment.</li>'
      '<li><b>Intersection with segment:</b> only keeps those regions in the select segment that are common with the modifier segment.</li>'
      '<li><b>Subtract segment:</b> subtract region of modifier segment from the selected segment.</li>'
      '<li><b>Invert:</b> inverts selected segment.</li>'
      '<li><b>Invert:</b> clears selected segment.</li>'
      '<li><b>Invert:</b> completely fills selected segment.</li>')
    self.scriptedEffect.addLabeledOptionsWidget("Operation:", self.methodSelectorComboBox)
    
    self.modifierSegmentSelectorLabel = qt.QLabel("Modifier segment:")
    self.scriptedEffect.addOptionsWidget(self.modifierSegmentSelectorLabel)
    
    self.modifierSegmentSelectorComboBox = slicer.qMRMLSegmentsTableView()
    self.modifierSegmentSelectorComboBox.setMRMLScene(slicer.mrmlScene)
    self.modifierSegmentSelectorComboBox.setToolTip('Contents of this segment will be used for modifying the selected segment. This segment itself will not be changed.')
    self.scriptedEffect.addOptionsWidget(self.modifierSegmentSelectorComboBox)

    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Logical selected segment")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.applyButton.connect('clicked()', self.onApply)
    self.methodSelectorComboBox.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
    
    self.applyButton.setToolTip("This effect has not been implemented yet.")
    self.applyButton.setEnabled(False)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameter("Operation", LOGICAL_COPY)
    
  def activate(self):
    # TODO: is this needed? probably it should be called for all effects on activation
    self.updateGUIFromMRML()
    
  def updateGUIFromMRML(self):
    operation = self.scriptedEffect.parameter("Operation")
    operationIndex = self.methodSelectorComboBox.findData(operation)
    wasBlocked = self.methodSelectorComboBox.blockSignals(True)
    self.methodSelectorComboBox.setCurrentIndex(operationIndex)
    self.methodSelectorComboBox.blockSignals(wasBlocked)

    modifierSegmentRequired = (operation == LOGICAL_COPY or operation == LOGICAL_UNION or operation == LOGICAL_INTERSECTION or operation == LOGICAL_SUBTRACT)
    self.modifierSegmentSelectorLabel.setVisible(modifierSegmentRequired)
    self.modifierSegmentSelectorComboBox.setVisible(modifierSegmentRequired)
    
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    self.modifierSegmentSelectorComboBox.setSegmentationNode(segmentationNode)    

  def updateMRMLFromGUI(self):
    operationIndex = self.methodSelectorComboBox.currentIndex
    operation = self.methodSelectorComboBox.itemData(operationIndex)
    self.scriptedEffect.setParameter("Operation", operation)
    
  def onApply(self):

      # Get edited labelmap and parameters
    editedLabelmap = self.scriptedEffect.editedLabelmap()
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()

    marginSizeMm = self.scriptedEffect.doubleParameter("MarginSizeMm")
    kernelSizePixel = self.getKernelSizePixel()

    # We need to know exactly the value of the segment voxels, apply threshold to make force the selected label value
    labelValue = 1
    backgroundValue = 0
    thresh = vtk.vtkImageThreshold()
    thresh.SetInputData(selectedSegmentLabelmap)
    thresh.ThresholdByLower(0)
    thresh.SetInValue(backgroundValue)
    thresh.SetOutValue(labelValue)
    thresh.SetOutputScalarType(selectedSegmentLabelmap.GetScalarType())

    erodeDilate = vtk.vtkImageDilateErode3D()
    erodeDilate.SetInputConnection(thresh.GetOutputPort())
    if marginSizeMm>0:
      # grow
      erodeDilate.SetDilateValue(labelValue)
      erodeDilate.SetErodeValue(backgroundValue)
    else:
      # shrink
      erodeDilate.SetDilateValue(backgroundValue)
      erodeDilate.SetErodeValue(labelValue)
    erodeDilate.SetKernelSize(kernelSizePixel[0],kernelSizePixel[1],kernelSizePixel[2])
    erodeDilate.Update()
    editedLabelmap.DeepCopy(erodeDilate.GetOutput())

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.setEditedLabelmapApplyModeToSet()
    self.scriptedEffect.setEditedLabelmapApplyExtentToWholeExtent()
    self.scriptedEffect.apply()

LOGICAL_INVERT = 'INVERT'
LOGICAL_CLEAR = 'CLEAR'
LOGICAL_FILL = 'FILL'
LOGICAL_COPY = 'COPY'
LOGICAL_UNION = 'UNION'
LOGICAL_INTERSECTION = 'INTERSECTION'
LOGICAL_SUBTRACT = 'SUBTRACT'
