import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *

#
# DvhComparison
# 
class DvhComparison(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "DVH Comparison"
    self.parent.categories = ["Radiotherapy"]
    self.parent.dependencies = []
    self.parent.contributors = ["Kyle Sunderland (Queen's University), Csaba Pinter (Queen's University)"]
    self.parent.helpText = """This module compares two Dose Volume Histograms from corresponding Double Array Nodes."""
    self.parent.acknowledgementText = """ """
  # end __init__

#
# qDvhComparisonWidget
#
class DvhComparisonWidget(ScriptedLoadableModuleWidget):

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
    chartCollapsibleButton = ctk.ctkCollapsibleButton()
    chartCollapsibleButton.text = "Visualize"
    self.layout.addWidget(chartCollapsibleButton)

    # Layout within the dummy collapsible button
    chartFormLayout = qt.QFormLayout(chartCollapsibleButton)

    #
    # Input the chart node
    #
    self.chartNodeSelector = slicer.qMRMLNodeComboBox()
    self.chartNodeSelector.nodeTypes = ["vtkMRMLChartNode"]
    self.chartNodeSelector.setMRMLScene( slicer.mrmlScene )
    chartFormLayout.addRow(self.chartNodeSelector)

    self.checkboxList = []
    self.structureList = []

    #
    # Dvh List
    #
    self.dvhTable = qt.QTableWidget();
    chartFormLayout.addWidget(self.dvhTable)


    # Add vertical spacer
    self.layout.addStretch(1)

    # Connections
    self.parameterSelector.connect('nodeAddedByUser(vtkMRMLNode*)', self.parameterNodeCreated)
    self.parameterSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.updateFromParameter)

    self.showDoseVolumeOnlyCheckbox.connect('stateChanged(int)', self.showDoseVolumesOnlyCheckboxChanged)
    
    self.dvh1Selector.connect("currentNodeChanged(vtkMRMLNode*)", self.dvh1SelectorChanged)
    self.dvh2Selector.connect("currentNodeChanged(vtkMRMLNode*)", self.dvh2SelectorChanged)
    self.doseVolumeSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.doseVolumeSelectorChanged)
    
    self.volumeDifferenceSpinbox.connect("valueChanged(double)", self.volumeDifferenceSpinboxChanged)
    self.doseToAgreementSpinbox.connect("valueChanged(double)", self.doseToAgreementSpinboxChanged)

    self.computeButton.connect('clicked(bool)', self.onComputeButton)

    self.chartNodeSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.updateChecksFromChart)

    self.updatingStatus = False

    self.nodeCheck()

    self.updateFromParameter(self.parameterSelector.currentNode())

    self.updateTable()    
    currentChart = self.chartNodeSelector.currentNode()
    if (currentChart != None):
      self.updateChecksFromChart(currentChart)
  # end setup

  def enter(self):
    """Runs whenever the module is reopened
    """

    self.nodeCheck()
    self.updateFromParameter(self.parameterSelector.currentNode())

    self.updateTable()    
    currentChart = self.chartNodeSelector.currentNode()

    if (currentChart != None):
      self.updateChecksFromChart(currentChart)
  # end enter

  def parameterNodeCreated(self, node):
    """Rename the newly created parameter node
    """
    numberOfNodes = slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLScriptedModuleNode')
    name = 'DvhComparison'
    if (numberOfNodes != 1):
      node.SetName(name + '_' + str(numberOfNodes-1))
    else:
      node.SetName(name)
  # end parameterNodeCreated

  def nodeCheck(self):
    """If there is no parameter node, create one and add it to the scene
    """
    if(self.parameterSelector.currentNode() == None):
      node = slicer.vtkMRMLScriptedModuleNode()
      slicer.mrmlScene.AddNode(node)
      self.parameterNodeCreated(node)
      self.parameterSelector.setCurrentNode(node)
  # end nodeCheck

  def updateTable(self):
    """Construct and fill in the table.
    """

    self.checkboxList = []
    self.structureList = []

    numberOfMetrics = 7
    self.dvhTable.setColumnCount(0)
    self.dvhTable.setRowCount(0)

    self.dvhTable.clearContents()

    dvhNodes = slicer.mrmlScene.GetNodesByClass("vtkMRMLDoubleArrayNode")
    dvhNodes.UnRegister(slicer.mrmlScene)

    self.dvhTable.setColumnCount(numberOfMetrics)
    headerLabels = [
      "",
      "Structure",
      "Volume name",
      "Volume (cc)",
      "Mean intensity",
      "Min intensity",
      "Max intensity"]
    self.dvhTable.setHorizontalHeaderLabels(headerLabels)
    self.dvhTable.setColumnWidth(0, 24)
    for i in range(1, numberOfMetrics):
      self.dvhTable.setColumnWidth(i,80)

    currentChart = self.chartNodeSelector.currentNode()

    numberOfLegalStructures = 0
    for i in range(0, dvhNodes.GetNumberOfItems()):
      structure = dvhNodes.GetItemAsObject(i)
      structureName = (structure.GetAttribute("DoseVolumeHistogram.StructureName"))

      if (structureName != None):
        self.structureList.append(structure)
        self.dvhTable.insertRow(i)
        self.dvhTable.setCellWidget(i, 1, qt.QLabel(" " + structureName + " "))
        volumeNode = (structure.GetNodeReference("DoseVolumeHistogram.doseVolumeRef"))
        if (volumeNode != None):
          volumeName = (volumeNode.GetName())
          check = qt.QCheckBox()
          check.connect("stateChanged(int)", self.checkBoxChanged)
          self.checkboxList.append(check)
          self.dvhTable.setCellWidget(i, 0, check)
          self.dvhTable.setCellWidget(i, 2, qt.QLabel(" " + volumeName + " "))

          # stop the checkboxes from being used if there is no chart node
          if (currentChart == None):
            check.setCheckable(False)
          else:
            check.setCheckable(True)

        # Add the attributes to the chart
        totalVolumeCCs = structure.GetAttribute("DoseVolumeHistogram.DvhMetric_Volume (cc)")
        if (totalVolumeCCs != None):
          self.dvhTable.setCellWidget(i, 3, qt.QLabel(" " + totalVolumeCCs + " "))

        meanVolumeCCs = structure.GetAttribute("DoseVolumeHistogram.DvhMetric_Mean intensity")
        if (meanVolumeCCs != None):
          self.dvhTable.setCellWidget(i, 4, qt.QLabel(" " + meanVolumeCCs + " "))

        minVolumeCCs = structure.GetAttribute("DoseVolumeHistogram.DvhMetric_Min intensity")
        if (minVolumeCCs != None):
          self.dvhTable.setCellWidget(i, 5, qt.QLabel(" " + minVolumeCCs + " "))

        maxVolumeCCs = structure.GetAttribute("DoseVolumeHistogram.DvhMetric_Max intensity")
        if (maxVolumeCCs != None):
          self.dvhTable.setCellWidget(i, 6, qt.QLabel(" " + maxVolumeCCs + " "))
  # end updateTable

  def checkBoxChanged(self, int):
    """If one of the chart checkboxes has been changed, then update the chart
    """

    if (self.updatingStatus == False):
      currentChart = self.chartNodeSelector.currentNode()
      if (currentChart != None):
        currentChart.ClearArrays()

        import vtkSlicerDoseVolumeHistogramModuleLogic
        chartLogic = vtkSlicerDoseVolumeHistogramModuleLogic.vtkSlicerDoseVolumeHistogramModuleLogic()
        chartLogic.SetMRMLScene(slicer.mrmlScene)

        for i in range(0, len(self.checkboxList)):
          currentCheckbox = self.checkboxList[i]
          if (currentCheckbox.checkState()):
            currentDoubleArray =  self.structureList[i]
            chartLogic.AddDvhToChart(currentDoubleArray.GetID(), currentChart.GetID())
  # end checkBoxChanged

  def updateFromParameter(self, node):
    """Changes the displayed inputs to match the current parameter node
    If an attribute has not been set, it is set from the currently
    displayed attribute
    """

    self.updatingStatus = True

    if (node != None):
      # DVH Double Array 1
      dvh1Node = node.GetNodeReferenceID('dvh1')
      if (dvh1Node != None):
        self.dvh1Selector.setCurrentNodeID(dvh1Node)
      else :
        dvh1Node = self.dvh1Selector.currentNode()
        if (dvh1Node != None):
          node.SetNodeReferenceID('dvh1', dvh1Node.GetID())

      # DVH Double Array 2
      dvh2Node = node.GetNodeReferenceID('dvh2')
      if (dvh2Node != None):
        self.dvh2Selector.setCurrentNodeID(dvh2Node)
      else :
        dvh2Node = self.dvh2Selector.currentNode()
        if (dvh2Node != None):
          node.SetNodeReferenceID('dvh2', dvh2Node.GetID())

      # Dose Volume
      doseVolumeNode = node.GetNodeReferenceID('DoseVolumeHistogram.doseVolume')
      if (doseVolumeNode != None):
        self.doseVolumeSelector.setCurrentNodeID(doseVolumeNode)
      else :
        doseVolumeNode = self.doseVolumeSelector.currentNode()
        if (doseVolumeNode != None):
          node.SetNodeReferenceID('DoseVolumeHistogram.doseVolume', doseVolumeNode.GetID())

      # Dose Volume Only Checkbox
      doseVolumeOnlyChecked = node.GetAttribute('DoseVolumeHistogram.doseVolumeOnlyChecked')
      if (doseVolumeOnlyChecked != None):
        checkedState = int(doseVolumeOnlyChecked)
        if(checkedState):
          self.showDoseVolumeOnlyCheckbox.setCheckState(2)
        else:
          self.showDoseVolumeOnlyCheckbox.setCheckState(0)
      else:
        self.parameterSelector.currentNode().SetAttribute('DoseVolumeHistogram.doseVolumeOnlyChecked', str(self.showDoseVolumeOnlyCheckbox.checkState()))

      # Dose To Agreement Criterion
      doseToAgreementCriterion = node.GetAttribute('DoseVolumeHistogram.doseToAgreementCriterion')
      if (doseToAgreementCriterion != None):
        self.doseToAgreementSpinbox.setValue(float(doseToAgreementCriterion))
      else:
        doseToAgreementCriterion = str(self.doseToAgreementSpinbox.value)
        node.SetAttribute('DoseVolumeHistogram.doseToAgreementCriterion', doseToAgreementCriterion)

      # Volume Difference Criterion
      volumeDifferenceCriterion = node.GetAttribute('DoseVolumeHistogram.volumeDifferenceCriterion')
      if (volumeDifferenceCriterion != None):
        self.volumeDifferenceSpinbox.setValue(float(volumeDifferenceCriterion))
      else:
        volumeDifferenceCriterion = str(self.volumeDifferenceSpinbox.value)
        node.SetAttribute('DoseVolumeHistogram.volumeDifferenceCriterion', volumeDifferenceCriterion)

      # Agreement Acceptance % (use previously stored one if availiable)
      agreementAcceptancePercentage = node.GetAttribute('DoseVolumeHistogram.agreementAcceptance')
      if (agreementAcceptancePercentage != None):
        self.agreementAcceptanceOutput.text = agreementAcceptancePercentage
      else:
        agreementAcceptancePercentage = self.agreementAcceptanceOutput.text
      self.parameterSelector.currentNode().SetAttribute('DoseVolumeHistogram.agreementAcceptance', agreementAcceptancePercentage)

      self.updatingStatus = False
  # end updatingStatus


  def displayChange(self):
    """If a node has changed, make sure that the compute button is enabled/disabled as necessary
    and set clear the output to avoid an illegal state.
    """

    self.nodeCheck()
    if (self.dvh1Selector.currentNode() != None and self.dvh2Selector.currentNode() != None and self.doseVolumeSelector.currentNode() != None):
      self.computeButton.setDisabled(False)
    else:
      self.computeButton.setDisabled(True)
    self.parameterSelector.currentNode().SetAttribute('AgreementAcceptance', "")
    self.agreementAcceptanceOutput.setText("")
  # end displayChange

  def onComputeButton(self):
    import vtkSlicerDoseVolumeHistogramModuleLogic
    logic = vtkSlicerDoseVolumeHistogramModuleLogic.vtkSlicerDoseVolumeHistogramComparisonLogic()
    logic.SetDvh1DoubleArrayNode(self.dvh1Selector.currentNode())
    logic.SetDvh2DoubleArrayNode(self.dvh2Selector.currentNode())
    logic.SetDoseVolumeNode(self.doseVolumeSelector.currentNode())
    logic.SetVolumeDifferenceCriterion(self.volumeDifferenceSpinbox.value)
    logic.SetDoseToAgreementCriterion(self.doseToAgreementSpinbox.value)

    agreementAcceptancePercentage = logic.CompareDvhTables()
    self.agreementAcceptanceOutput.setText(agreementAcceptancePercentage)
    self.parameterSelector.currentNode().SetAttribute('DoseVolumeHistogram.agreementAcceptance', str(agreementAcceptancePercentage))
  # end onComputeButton

  def checkChart(self, dvhNodeId, chartNode):
    """Check the specified chart to see if a given DVH is being displayed.
    """
    arrayIds = chartNode.GetArrays()
    for arrayIndex in range(0, arrayIds.GetNumberOfValues()):
      currentArrayId = arrayIds.GetValue(arrayIndex)
      if (currentArrayId == dvhNodeId):
        return True
    return False
  # end checkChart

  def updateChecksFromChart(self, chartNode):
    """Set the check boxes in the table to match the graph display.
    """

    self.updatingStatus = True
    for structureIndex in range(0, len(self.structureList)):
      currentCheckbox = self.checkboxList[structureIndex]
      if (chartNode != None):
        currentStructure = self.structureList[structureIndex]
        if (self.checkChart(currentStructure.GetID(), chartNode)):
          currentCheckbox.setCheckState(2)
        else:
          currentCheckbox.setCheckState(0)
        currentCheckbox.setCheckable(True)
      else:
        currentCheckbox.setCheckState(0)
        currentCheckbox.setCheckable(False)
        currentCheckbox.update()

    self.updatingStatus = False
  # end updateChecksFromChart

  def dvh1SelectorChanged(self, dvh1Node):
    if (self.updatingStatus == False):
      self.displayChange()
      if (dvh1Node != None):
        self.parameterSelector.currentNode().SetNodeReferenceID('dvh1', dvh1Node.GetID())
  # end dvh1SelectorChanged

  def dvh2SelectorChanged(self, dvh2Node):
    if (self.updatingStatus == False):
      self.displayChange()
      if (dvh2Node != None):
        self.parameterSelector.currentNode().SetNodeReferenceID('dvh2', dvh2Node.GetID())
  # end dvh2SelectorChanged

  def doseVolumeSelectorChanged(self, doseVolumeNode):
    if (self.updatingStatus == False):
      self.displayChange()
      if (doseVolumeNode != None):
        self.parameterSelector.currentNode().SetNodeReferenceID('doseVolume', doseVolumeNode.GetID())
  # end doseVolumeSelectorChanged

  def volumeDifferenceSpinboxChanged(self, value):
    if (self.updatingStatus == False):
      self.displayChange()
      volumeDifferenceCriterion = str(value)
      self.parameterSelector.currentNode().SetAttribute('DoseVolumeHistogram.VolumeDifferenceCriterion', volumeDifferenceCriterion)
  # end volumeDifferenceSpinboxChanged

  def doseToAgreementSpinboxChanged(self, value):
    if (self.updatingStatus == False):
      self.displayChange()
      doseToAgreementCriterion = str(value)
      self.parameterSelector.currentNode().SetAttribute('DoseVolumeHistogram.DoseToAgreementCriterion', doseToAgreementCriterion)
  # end doseToAgreementSpinboxChanged

  def showDoseVolumesOnlyCheckboxChanged(self, checked):
    if (self.updatingStatus == False):
      self.displayChange()
      self.parameterSelector.currentNode().SetAttribute('DoseVolumeOnlyChecked', str(checked))
      if (checked):
        self.doseVolumeSelector.addAttribute("vtkMRMLScalarVolumeNode", "DicomRtImport.DoseVolume")
      else:
        self.doseVolumeSelector.removeAttribute("vtkMRMLScalarVolumeNode", "DicomRtImport.DoseVolume")
  # end showDoseVolumesOnlyCheckboxChanged

  def cleanup(self):
    pass
  # end cleanup

class DvhComparisonTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)
  # end setUp

  def runTest(self):
    """Currently no testing functionality.
    """
    self.setUp()
    self.test_DvhComparison1()
  # end runTest

  def test_DvhComparison1(self):
    """Add test here later.
    """
    self.delayDisplay("Starting the test")
    self.delayDisplay('Test passed!')
  # end test_DvhComparison1
