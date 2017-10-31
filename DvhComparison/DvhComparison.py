import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *

#
# ------------------------------------------------------------------------------
# DvhComparison
# ------------------------------------------------------------------------------
# 
class DvhComparison(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "DVH Comparison"
    self.parent.categories = ["Radiotherapy"]
    self.parent.dependencies = ["DoseVolumeHistogram"]
    self.parent.contributors = ["Kyle Sunderland (Queen's University), Csaba Pinter (Queen's University)"]
    self.parent.helpText = """This module compares two Dose Volume Histograms from corresponding Double Array Nodes."""
    self.parent.acknowledgementText = """ """

    
#
# ------------------------------------------------------------------------------
# qDvhComparisonWidget
# ------------------------------------------------------------------------------
#
class DvhComparisonWidget(ScriptedLoadableModuleWidget):
  def __init__(self, parent):
    ScriptedLoadableModuleWidget.__init__(self, parent)
    
    self.volumeDifferenceCriterionAttrName = 'DvhComparison.VolumeDifferenceCriterion'
    self.doseToAgreementCriterionAttrName = 'DvhComparison.DoseToAgreementCriterion'
    self.doseVolumeOnlyCheckedAttrName = 'DvhComparison.DoseVolumeOnlyChecked'
    self.agreementAcceptanceAttrName = 'DvhComparison.AgreementAcceptance'
    self.dvh1NodeReference = 'dvh1Ref'
    self.dvh2NodeReference = 'dvh2Ref'
    self.doseVolumeNodeReference = 'doseVolumeRef'

  #------------------------------------------------------------------------------
  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    #
    # Parameter Combobox
    #
    self.parameterSelector = slicer.qMRMLNodeComboBox()
    self.parameterLabel = qt.QLabel("  Parameter set: ")
    self.parameterSelector.nodeTypes = ["vtkMRMLScriptedModuleNode"]
    self.parameterSelector.removeEnabled = False
    self.parameterSelector.showHidden = True
    self.parameterSelector.setMRMLScene( slicer.mrmlScene )
    self.parameterLayout = qt.QHBoxLayout()
    self.parameterLayout.addWidget(self.parameterLabel)
    self.parameterLayout.addWidget(self.parameterSelector)
    self.layout.addLayout(self.parameterLayout)

    #
    # Input Area
    #
    inputCollapsibleButton = ctk.ctkCollapsibleButton()
    inputCollapsibleButton.text = "Input"
    self.layout.addWidget(inputCollapsibleButton)

    # Layout within the dummy collapsible button
    inputFormLayout = qt.QFormLayout(inputCollapsibleButton)

    #
    # Input first DVH selector
    #
    self.dvh1Selector = slicer.qMRMLNodeComboBox()
    self.dvh1Selector.nodeTypes = ["vtkMRMLDoubleArrayNode"]
    self.dvh1Selector.removeEnabled = False
    self.dvh1Selector.setMRMLScene( slicer.mrmlScene )
    inputFormLayout.addRow("DVH 1: ", self.dvh1Selector)

    #
    # Input second DVH selector
    #
    self.dvh2Selector = slicer.qMRMLNodeComboBox()
    self.dvh2Selector.nodeTypes = ["vtkMRMLDoubleArrayNode"]
    self.dvh2Selector.removeEnabled = False
    self.dvh2Selector.setMRMLScene( slicer.mrmlScene )
    inputFormLayout.addRow("DVH 2: ", self.dvh2Selector)

    #
    # Input the dose volume
    #
    self.doseVolumeSelector = slicer.qMRMLNodeComboBox()
    self.doseVolumeSelector.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.doseVolumeSelector.addAttribute("vtkMRMLScalarVolumeNode", "DicomRtImport.DoseVolume")
    self.doseVolumeSelector.removeEnabled = False
    self.doseVolumeSelector.setMRMLScene( slicer.mrmlScene )
    inputFormLayout.addRow("Dose Volume: ", self.doseVolumeSelector)

    #
    # Dose volume only check box
    #
    self.showDoseVolumeOnlyCheckbox = qt.QCheckBox("Show dose volume only")
    self.showDoseVolumeOnlyCheckbox.setChecked(2)
    inputFormLayout.addWidget(self.showDoseVolumeOnlyCheckbox)

    #
    # Volume difference criterion spin box
    #
    self.volumeDifferenceSpinbox = qt.QDoubleSpinBox()
    self.volumeDifferenceSpinbox.setValue(1.0)
    self.volumeDifferenceSpinbox.setDecimals(1)
    self.volumeDifferenceSpinbox.setSingleStep(0.1)
    inputFormLayout.addRow("Volume difference criterion: ", self.volumeDifferenceSpinbox)

    #
    # Dose to agreement criterion spin box
    #
    self.doseToAgreementSpinbox = qt.QDoubleSpinBox()
    self.doseToAgreementSpinbox.setValue(1.0)
    self.doseToAgreementSpinbox.setDecimals(1)
    self.doseToAgreementSpinbox.setSingleStep(0.1)
    inputFormLayout.addRow("Dose to agreement criterion: ", self.doseToAgreementSpinbox)

    #
    # Compute button
    #
    self.computeButton = qt.QPushButton("Compute")
    self.computeButtonLayout = qt.QVBoxLayout()
    self.computeButtonLayout.addStrut(100)
    self.computeButtonLayout.setAlignment(2)
    self.computeButtonLayout.addWidget(self.computeButton)
    self.computeButtonFont = qt.QFont()
    self.computeButtonFont.setBold(True)
    self.computeButton.setFont(self.computeButtonFont)
    inputFormLayout.addRow(self.computeButtonLayout)

    #
    # Output Area
    #
    outputCollapsibleButton = ctk.ctkCollapsibleButton()
    outputCollapsibleButton.text = "Output"
    self.layout.addWidget(outputCollapsibleButton)

    # Layout within the dummy collapsible button
    outputFormLayout = qt.QFormLayout(outputCollapsibleButton)

    self.agreementAcceptanceOutput = qt.QLineEdit()
    self.agreementAcceptanceOutput.setReadOnly(True)
    outputFormLayout.addRow("Agreement acceptance %: ", self.agreementAcceptanceOutput)

    #
    # Visualize Area
    #
    visualizeCollapsibleButton = ctk.ctkCollapsibleButton()
    visualizeCollapsibleButton.text = "Visualize"
    sizePolicy = qt.QSizePolicy()
    sizePolicy.setHorizontalPolicy(qt.QSizePolicy.Preferred)
    sizePolicy.setVerticalPolicy(qt.QSizePolicy.Expanding)
    visualizeCollapsibleButton.setSizePolicy(sizePolicy)
    self.layout.addWidget(visualizeCollapsibleButton)

    # Layout within the dummy collapsible button
    visualizeLayout = qt.QVBoxLayout(visualizeCollapsibleButton)

    #
    # DVH Table
    #
    self.dvhTable = slicer.qMRMLTableView()
    self.dvhTable.setMRMLScene(slicer.mrmlScene)
    self.dvhTable.setSelectionMode(qt.QAbstractItemView.NoSelection)
    self.dvhTable.setSizePolicy(sizePolicy)
    visualizeLayout.addWidget(self.dvhTable)

    # Connections
    self.parameterSelector.connect('nodeAddedByUser(vtkMRMLNode*)', self.parameterNodeCreated)
    self.parameterSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.updateWidgetFromMRML)

    self.showDoseVolumeOnlyCheckbox.connect('stateChanged(int)', self.showDoseVolumesOnlyCheckboxChanged)
    
    self.dvh1Selector.connect("currentNodeChanged(vtkMRMLNode*)", self.dvh1SelectorChanged)
    self.dvh2Selector.connect("currentNodeChanged(vtkMRMLNode*)", self.dvh2SelectorChanged)
    self.doseVolumeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.doseVolumeSelectorChanged)
    
    self.volumeDifferenceSpinbox.connect("valueChanged(double)", self.volumeDifferenceSpinboxChanged)
    self.doseToAgreementSpinbox.connect("valueChanged(double)", self.doseToAgreementSpinboxChanged)

    self.computeButton.connect('clicked(bool)', self.onComputeButton)

    self.updateWidgetFromMRML()

  #------------------------------------------------------------------------------
  def enter(self):
    self.updateWidgetFromMRML()

  #------------------------------------------------------------------------------
  def parameterNodeCreated(self, node):
    # Rename the newly created parameter node
    numberOfNodes = slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLScriptedModuleNode')
    name = 'DvhComparison'
    if numberOfNodes != 1:
      node.SetName(name + '_' + str(numberOfNodes-1))
    else:
      node.SetName(name)

  #------------------------------------------------------------------------------
  def updateWidgetFromMRML(self):
    """Changes the displayed inputs to match the current parameter node
       If an attribute has not been set, it is set from the currently
       displayed attribute
    """

    # If there is no parameter node, create one and add it to the scene
    if self.parameterSelector.currentNode() is None:
      node = slicer.vtkMRMLScriptedModuleNode()
      slicer.mrmlScene.AddNode(node)
      self.parameterNodeCreated(node)
      self.parameterSelector.setCurrentNode(node)
    paramNode = self.parameterSelector.currentNode()

    if paramNode is not None:
      # DVH Array 1
      dvh1Node = paramNode.GetNodeReferenceID(self.dvh1NodeReference)
      self.dvh1Selector.blockSignals(True)
      self.dvh1Selector.setCurrentNodeID(dvh1Node)
      self.dvh1Selector.blockSignals(False)

      # DVH Array 2
      dvh2Node = paramNode.GetNodeReferenceID(self.dvh2NodeReference)
      self.dvh2Selector.blockSignals(True)
      self.dvh2Selector.setCurrentNodeID(dvh2Node)
      self.dvh2Selector.blockSignals(False)

      # Dose Volume
      doseVolumeNode = paramNode.GetNodeReferenceID(self.doseVolumeNodeReference)
      self.doseVolumeSelector.blockSignals(True)
      self.doseVolumeSelector.setCurrentNodeID(doseVolumeNode)
      self.doseVolumeSelector.blockSignals(False)

      # Dose Volume Only Checkbox
      doseVolumeOnlyChecked = paramNode.GetAttribute(self.doseVolumeOnlyCheckedAttrName)
      if doseVolumeOnlyChecked is not None:
        checkedState = int(doseVolumeOnlyChecked)
        self.showDoseVolumeOnlyCheckbox.blockSignals(True)
        if checkedState:
          self.showDoseVolumeOnlyCheckbox.setCheckState(2)
        else:
          self.showDoseVolumeOnlyCheckbox.setCheckState(0)
        self.showDoseVolumeOnlyCheckbox.blockSignals(False)
      else:
        self.parameterSelector.currentNode().SetAttribute(self.doseVolumeOnlyCheckedAttrName, str(self.showDoseVolumeOnlyCheckbox.checkState()))

      # Dose To Agreement Criterion
      doseToAgreementCriterion = paramNode.GetAttribute(self.doseToAgreementCriterionAttrName)
      if doseToAgreementCriterion is not None:
        self.doseToAgreementSpinbox.blockSignals(True)
        self.doseToAgreementSpinbox.setValue(float(doseToAgreementCriterion))
        self.doseToAgreementSpinbox.blockSignals(False)
      else:
        paramNode.SetAttribute(self.doseToAgreementCriterionAttrName, str(self.doseToAgreementSpinbox.value))

      # Volume Difference Criterion
      volumeDifferenceCriterion = paramNode.GetAttribute(self.volumeDifferenceCriterionAttrName)
      if volumeDifferenceCriterion is not None:
        self.volumeDifferenceSpinbox.blockSignals(True)
        self.volumeDifferenceSpinbox.setValue(float(volumeDifferenceCriterion))
        self.volumeDifferenceSpinbox.blockSignals(False)
      else:
        paramNode.SetAttribute(self.volumeDifferenceCriterionAttrName, str(self.volumeDifferenceSpinbox.value))

      # Agreement Acceptance % (use previously stored one if availiable)
      agreementAcceptancePercentage = paramNode.GetAttribute(self.agreementAcceptanceAttrName)
      if agreementAcceptancePercentage is not None:
        self.agreementAcceptanceOutput.text = agreementAcceptancePercentage
      else:
        self.agreementAcceptanceOutput.text = ''

  #------------------------------------------------------------------------------
  def updateWidgetForSelection(self):
    # Make sure that the compute button is enabled/disabled as necessary
    dvh1Node = self.dvh1Selector.currentNode()
    dvh2Node = self.dvh2Selector.currentNode()
    if dvh1Node is not None and dvh2Node is not None and self.doseVolumeSelector.currentNode() is not None:
      self.computeButton.setEnabled(True)
    else:
      self.computeButton.setEnabled(False)

    # Clear the output
    self.parameterSelector.currentNode().SetAttribute(self.agreementAcceptanceAttrName, '')
    self.agreementAcceptanceOutput.setText('')

    # Set table to show in visualize section. It will be the metrics table for DVH 1
    dvh1MetricsTable = dvh1Node.GetNodeReference('dvhMetricsTableRef')
    self.dvhTable.setMRMLTableNode(dvh1MetricsTable)    
    self.dvhTable.setFirstRowLocked(True)
    self.dvhTable.resizeColumnsToContents()
    self.dvhTable.setColumnWidth(slicer.vtkMRMLDoseVolumeHistogramNode.MetricColumnVisible, 36)

  #------------------------------------------------------------------------------
  def onComputeButton(self):
    paramNode = self.parameterSelector.currentNode()
    dvh1Node = paramNode.GetNodeReference(self.dvh1NodeReference)
    dvh2Node = paramNode.GetNodeReference(self.dvh2NodeReference)
    doseVolumeNode = paramNode.GetNodeReference(self.doseVolumeNodeReference)
    doseToAgreementCriterion = float(paramNode.GetAttribute(self.doseToAgreementCriterionAttrName))
    volumeDifferenceCriterion = float(paramNode.GetAttribute(self.volumeDifferenceCriterionAttrName))

    agreementAcceptancePercentage = slicer.vtkSlicerDoseVolumeHistogramComparisonLogic.CompareDvhTables(dvh1Node, dvh2Node, doseVolumeNode, volumeDifferenceCriterion, doseToAgreementCriterion)

    self.agreementAcceptanceOutput.setText(agreementAcceptancePercentage)
    self.parameterSelector.currentNode().SetAttribute(self.agreementAcceptanceAttrName, str(agreementAcceptancePercentage))

  #------------------------------------------------------------------------------
  def dvh1SelectorChanged(self, dvh1Node):
    if dvh1Node is not None:
      self.parameterSelector.currentNode().SetNodeReferenceID(self.dvh1NodeReference, dvh1Node.GetID())
    self.updateWidgetForSelection()

  #------------------------------------------------------------------------------
  def dvh2SelectorChanged(self, dvh2Node):
    if dvh2Node is not None:
      self.parameterSelector.currentNode().SetNodeReferenceID(self.dvh2NodeReference, dvh2Node.GetID())
    self.updateWidgetForSelection()

  #------------------------------------------------------------------------------
  def doseVolumeSelectorChanged(self, doseVolumeNode):
    if doseVolumeNode is not None:
      self.parameterSelector.currentNode().SetNodeReferenceID(self.doseVolumeNodeReference, doseVolumeNode.GetID())
    self.updateWidgetForSelection()

  #------------------------------------------------------------------------------
  def volumeDifferenceSpinboxChanged(self, value):
    self.parameterSelector.currentNode().SetAttribute(self.volumeDifferenceCriterionAttrName, str(value))
    self.updateWidgetForSelection()

  #------------------------------------------------------------------------------
  def doseToAgreementSpinboxChanged(self, value):
    self.parameterSelector.currentNode().SetAttribute(self.doseToAgreementCriterionAttrName, str(value))
    self.updateWidgetForSelection()

  #------------------------------------------------------------------------------
  def showDoseVolumesOnlyCheckboxChanged(self, checked):
    self.parameterSelector.currentNode().SetAttribute(self.doseVolumeOnlyCheckedAttrName, str(checked))
    if checked:
      self.doseVolumeSelector.addAttribute("vtkMRMLScalarVolumeNode", "DicomRtImport.DoseVolume")
    else:
      self.doseVolumeSelector.removeAttribute("vtkMRMLScalarVolumeNode", "DicomRtImport.DoseVolume")
    self.updateWidgetForSelection()

  #------------------------------------------------------------------------------
  def cleanup(self):
    pass


#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
class DvhComparisonTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Currently no testing functionality.
    """
    self.setUp()
    self.test_DvhComparison1()

  def test_DvhComparison1(self):
    """Add test here later.
    """
    self.delayDisplay("Starting the test")
    self.delayDisplay('Test passed!')
