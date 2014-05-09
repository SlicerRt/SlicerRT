import os
import unittest
from __main__ import vtk, qt, ctk, slicer

#
# DvhComparison
#

class DvhComparison:
  def __init__(self, parent):
    parent.title = "DVH Comparison"
    parent.categories = ["Radiotherapy"]
    parent.dependencies = []
    parent.contributors = ["Kyle Sunderland (Queen's University), Csaba Pinter (Queen's University)"]
    parent.helpText = """
    """
    parent.acknowledgementText = """
""" # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['DvhComparison'] = self.runTest

  def runTest(self):
    tester = DvhComparisonTest()
    tester.runTest()

#
# qDvhComparisonWidget
#
class DvhComparisonWidget:
  def __init__(self, parent = None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.layout = self.parent.layout()
    if not parent:
      self.setup()
      self.parent.show()

  def setup(self):
    #
    # Reload and Test area
    #
    reloadCollapsibleButton = ctk.ctkCollapsibleButton()
    reloadCollapsibleButton.text = "Reload && Test"
    self.layout.addWidget(reloadCollapsibleButton)
    reloadFormLayout = qt.QFormLayout(reloadCollapsibleButton)

    # reload button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadButton = qt.QPushButton("Reload")
    self.reloadButton.toolTip = "Reload this module."
    self.reloadButton.name = "DvhComparison Reload"
    reloadFormLayout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    reloadFormLayout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)
	
    #
    # input Area
    #
    inputCollapsibleButton = ctk.ctkCollapsibleButton()
    inputCollapsibleButton.text = "Input"
    self.layout.addWidget(inputCollapsibleButton)

    # Layout within the dummy collapsible button
    inputFormLayout = qt.QFormLayout(inputCollapsibleButton)

    #
    # input first DVH selector
    #
    self.dvh1Selector = slicer.qMRMLNodeComboBox()
    self.dvh1Selector.nodeTypes = ( ("vtkMRMLDoubleArrayNode"), "" )
    self.dvh1Selector.selectNodeUponCreation = False
    self.dvh1Selector.removeEnabled = False
    self.dvh1Selector.noneEnabled = False
    self.dvh1Selector.showHidden = False
    self.dvh1Selector.showChildNodeTypes = False
    self.dvh1Selector.setMRMLScene( slicer.mrmlScene )
    self.dvh1Selector.setToolTip( "Pick the first DVH for comparison." )
    inputFormLayout.addRow("DVH 1: ", self.dvh1Selector)

    #
    # input second DVH selector
    #
    self.dvh2Selector = slicer.qMRMLNodeComboBox()
    self.dvh2Selector.nodeTypes = ( ("vtkMRMLDoubleArrayNode"), "" )
    self.dvh2Selector.selectNodeUponCreation = False
    self.dvh2Selector.removeEnabled = False
    self.dvh2Selector.noneEnabled = False
    self.dvh2Selector.showHidden = False
    self.dvh2Selector.showChildNodeTypes = False
    self.dvh2Selector.setMRMLScene( slicer.mrmlScene )
    self.dvh2Selector.setToolTip( "Pick the second DVH for comparison." )
    inputFormLayout.addRow("DVH 2: ", self.dvh2Selector)

    #
    # input the dose volume
    #
    self.doseVolumeSelector = slicer.qMRMLNodeComboBox()
    self.doseVolumeSelector.nodeTypes = ( ("vtkMRMLScalarVolumeNode"), "" )
    self.doseVolumeSelector.selectNodeUponCreation = False
    self.doseVolumeSelector.removeEnabled = False
    self.doseVolumeSelector.noneEnabled = False
    self.doseVolumeSelector.showHidden = False
    self.doseVolumeSelector.showChildNodeTypes = False
    self.doseVolumeSelector.setMRMLScene( slicer.mrmlScene )
    self.doseVolumeSelector.setToolTip( "Pick the dose volume." )
    inputFormLayout.addRow("Dose Volume ", self.doseVolumeSelector)

    #
    # dose volume only check box #### not correct format
    #
    self.doseVolumeOnlyCheckbox = qt.QCheckBox()
    self.doseVolumeOnlyCheckbox.setToolTip( "Pick the dose volume." )
    self.doseVolumeOnlyLayout = qt.QHBoxLayout()
    inputFormLayout.addRow(self.doseVolumeOnlyLayout)
    self.doseVolumeOnlyLayout.setAlignment(0)
    self.doseVolumeOnlyLayout.addWidget(self.doseVolumeOnlyCheckbox)	
    self.doseVolumeOnlyLayout.addSpacing(10)
    self.doseVolumeOnlyLayout.addWidget(qt.QLabel("Show dose volume only"), 1)
    	
	#
    # volume difference criterion spin box
    #
    self.volumeDifferenceSpinbox = qt.QDoubleSpinBox()
    self.volumeDifferenceSpinbox.setValue(1.0)
    self.volumeDifferenceSpinbox.setDecimals(1)
    self.volumeDifferenceSpinbox.setSingleStep(0.1)
    self.volumeDifferenceSpinbox.setToolTip( "Select the volume difference criterion." )
    inputFormLayout.addRow("Volume difference criterion: ", self.volumeDifferenceSpinbox)		
	
	#
    # dose to agreement criterion spin box
    #
    self.doseToAgreementSpinbox = qt.QDoubleSpinBox()
    self.doseToAgreementSpinbox.setValue(1.0)
    self.doseToAgreementSpinbox.setDecimals(1)
    self.doseToAgreementSpinbox.setSingleStep(0.1)
    self.doseToAgreementSpinbox.setToolTip( "Select the dose to agreement criterion." )
    inputFormLayout.addRow("Dose to agreement criterion: ", self.doseToAgreementSpinbox)	
	
	#
    # Output Area
    #
    outputCollapsibleButton = ctk.ctkCollapsibleButton()
    outputCollapsibleButton.text = "Output"
    self.layout.addWidget(outputCollapsibleButton)

    # Layout within the dummy collapsible button
    outputFormLayout = qt.QFormLayout(outputCollapsibleButton)

    self.agreementAcceptance = qt.QLineEdit()
    self.agreementAcceptance.setReadOnly(True)
    self.agreementAcceptance.setToolTip( "Agreement acceptance." )
    outputFormLayout.addRow("Agreement acceptance %: ", self.agreementAcceptance)	

    # Add vertical spacer
    self.layout.addStretch(1)
	
  def cleanup(self):
    pass

  def onReload(self,moduleName="DvhComparison"):
    """Generic reload method for any scripted module.
    ModuleWizard will substitute correct default moduleName.
    """
    globals()[moduleName] = slicer.util.reloadScriptedModule(moduleName)

  def onReloadAndTest(self,moduleName="DvhComparison"):
    try:
      self.onReload()
      evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
      tester = eval(evalString)
      tester.runTest()
    except Exception, e:
      import traceback
      traceback.print_exc()
      qt.QMessageBox.warning(slicer.util.mainWindow(),
          "Reload and Test", 'Exception!\n\n' + str(e) + "\n\nSee Python Console for Stack Trace")


#
# DvhComparisonLogic
#
class DvhComparisonLogic:

  def __init__(self):
    pass

  def delayDisplay(self,message,msec=1000):
    #
    # logic version of delay display
    #
    print(message)
    self.info = qt.QDialog()
    self.infoLayout = qt.QVBoxLayout()
    self.info.setLayout(self.infoLayout)
    self.label = qt.QLabel(message,self.info)
    self.infoLayout.addWidget(self.label)
    qt.QTimer.singleShot(msec, self.info.close)
    self.info.exec_()

  def run(self):
    """
    Run the actual algorithm
    """

    return True


class DvhComparisonTest(unittest.TestCase):
  """
  This is the test case for your scripted module.
  """

  def delayDisplay(self,message,msec=1000):
    """This utility method displays a small dialog and waits.
    This does two things: 1) it lets the event loop catch up
    to the state of the test so that rendering and widget updates
    have all taken place before the test continues and 2) it
    shows the user/developer/tester the state of the test
    so that we'll know when it breaks.
    """
    print(message)
    self.info = qt.QDialog()
    self.infoLayout = qt.QVBoxLayout()
    self.info.setLayout(self.infoLayout)
    self.label = qt.QLabel(message,self.info)
    self.infoLayout.addWidget(self.label)
    qt.QTimer.singleShot(msec, self.info.close)
    self.info.exec_()

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_DvhComparison1()

  def test_DvhComparison1(self):

    self.delayDisplay("Starting the test")
    
    self.delayDisplay('Test passed!')
