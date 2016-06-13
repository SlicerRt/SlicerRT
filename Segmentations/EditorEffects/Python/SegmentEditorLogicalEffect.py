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

    self.modifierSegmentSelector = slicer.qMRMLSegmentsTableView()
    self.modifierSegmentSelector.selectionMode = qt.QAbstractItemView.SingleSelection
    self.modifierSegmentSelector.headerVisible = False
    self.modifierSegmentSelector.visibilityColumnVisible = False
    self.modifierSegmentSelector.opacityColumnVisible = False

    self.modifierSegmentSelector.setMRMLScene(slicer.mrmlScene)
    self.modifierSegmentSelector.setToolTip('Contents of this segment will be used for modifying the selected segment. This segment itself will not be changed.')
    self.scriptedEffect.addOptionsWidget(self.modifierSegmentSelector)

    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Logical selected segment")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.applyButton.connect('clicked()', self.onApply)
    self.methodSelectorComboBox.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
    self.modifierSegmentSelector.connect("selectionChanged(QItemSelection, QItemSelection)", self.updateMRMLFromGUI)

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
    self.modifierSegmentSelector.setVisible(modifierSegmentRequired)

    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    selectedSegmentIDs = self.scriptedEffect.parameter("SelectedSegmentID").split(';')
    wasBlocked = self.modifierSegmentSelector.blockSignals(True)
    self.modifierSegmentSelector.setSegmentationNode(segmentationNode)
    self.modifierSegmentSelector.setSelectedSegmentIDs(selectedSegmentIDs)
    self.modifierSegmentSelector.blockSignals(wasBlocked)

  def updateMRMLFromGUI(self):
    operationIndex = self.methodSelectorComboBox.currentIndex
    operation = self.methodSelectorComboBox.itemData(operationIndex)
    self.scriptedEffect.setParameter("Operation", operation)

    selectedSegmentIDs = ';'.join(self.modifierSegmentSelector.selectedSegmentIDs()) # semicolon-separated list of segment IDs
    self.scriptedEffect.setParameter("SelectedSegmentID", selectedSegmentIDs)

  def onApply(self):

    import vtkSegmentationCorePython as vtkSegmentationCore

    # Get edited labelmap and parameters
    editedLabelmap = self.scriptedEffect.editedLabelmap()
    selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()

    operation = self.scriptedEffect.parameter("Operation")

    # Get modifier segment
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    segmentation = segmentationNode.GetSegmentation()
    selectedSegmentIDs = self.scriptedEffect.parameter("SelectedSegmentID").split(';')
    modifierSegment = None
    if selectedSegmentIDs:
      firstSelectedSegmentID = selectedSegmentIDs[0]
      modifierSegment = segmentation.GetSegment(firstSelectedSegmentID)
      modifierSegmentLabelmap = modifierSegment.GetRepresentation(vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName())

    if operation == LOGICAL_COPY:
      editedLabelmap.DeepCopy(modifierSegmentLabelmap)
      self.scriptedEffect.setEditedLabelmapApplyModeToSet()
      self.scriptedEffect.setEditedLabelmapApplyExtentToWholeExtent()
    else:
      return

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.apply()

LOGICAL_INVERT = 'INVERT'
LOGICAL_CLEAR = 'CLEAR'
LOGICAL_FILL = 'FILL'
LOGICAL_COPY = 'COPY'
LOGICAL_UNION = 'UNION'
LOGICAL_INTERSECTION = 'INTERSECTION'
LOGICAL_SUBTRACT = 'SUBTRACT'
