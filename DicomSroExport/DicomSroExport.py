import os
import unittest
import sys
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *

#
# DicomSroExport
#

class DicomSroExport(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "DICOM Registration Export"
    self.parent.categories = ["Plastimatch"]
    self.parent.dependencies = []
    self.parent.contributors = ["Gregory Sharp (MGH), Kevin Wang (Princess Margaret Cancer Centre)"]
    self.parent.helpText = """
    This is an example of scripted loadable module bundled in an extension.
    """
    self.parent.acknowledgementText = """
    This file was originally developed by Greg Sharp, Massachusetts General Hospital, and was partially funded by NIH grant 2-U54-EB005149.
    """ # replace with organization, grant and thanks.

#
# qDicomSroExportWidget
#

class DicomSroExportWidget(ScriptedLoadableModuleWidget):

  def setup(self):

    ScriptedLoadableModuleWidget.setup(self)

    # Instantiate and connect widgets ...

    ### Reload and Test area
    reloadCollapsibleButton = ctk.ctkCollapsibleButton()
    reloadCollapsibleButton.text = "Reload && Test"
    self.layout.addWidget(reloadCollapsibleButton)
    reloadFormLayout = qt.QFormLayout(reloadCollapsibleButton)

    # reload button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadButton = qt.QPushButton("Reload")
    self.reloadButton.toolTip = "Reload this module."
    self.reloadButton.name = "DicomSroExport Reload"
    reloadFormLayout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    reloadFormLayout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)

    ### Parameters Area
    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton)

    # fixed image (mrml input)
    self.fixedMRMLSelector = slicer.qMRMLNodeComboBox()
    self.fixedMRMLSelector.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.fixedMRMLSelector.selectNodeUponCreation = True
    self.fixedMRMLSelector.addEnabled = False
    self.fixedMRMLSelector.removeEnabled = False
    self.fixedMRMLSelector.noneEnabled = False
    self.fixedMRMLSelector.showHidden = False
    self.fixedMRMLSelector.showChildNodeTypes = False
    self.fixedMRMLSelector.setMRMLScene( slicer.mrmlScene )
    self.fixedMRMLSelector.setToolTip ("Choose either an image within the MRML scene, or a directory containing a DICOM image")
    parametersFormLayout.addRow("Fixed image: ", self.fixedMRMLSelector)

    # fixed image (directory input)
    self.fixedInputDirectory = ctk.ctkDirectoryButton()
    self.fixedInputDirectory.directory = qt.QDir.homePath()
    parametersFormLayout.addRow("", self.fixedInputDirectory)

    # moving image (mrml input)
    self.movingMRMLSelector = slicer.qMRMLNodeComboBox()
    self.movingMRMLSelector.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.movingMRMLSelector.selectNodeUponCreation = True
    self.movingMRMLSelector.addEnabled = False
    self.movingMRMLSelector.removeEnabled = False
    self.movingMRMLSelector.noneEnabled = False
    self.movingMRMLSelector.showHidden = False
    self.movingMRMLSelector.showChildNodeTypes = False
    self.movingMRMLSelector.setMRMLScene( slicer.mrmlScene )
    self.fixedMRMLSelector.setToolTip ("Choose either an image within the MRML scene, or a directory containing a DICOM image")
    parametersFormLayout.addRow("Moving image: ", self.movingMRMLSelector)

    # moving image (directory input)
    self.movingInputDirectory = ctk.ctkDirectoryButton()
    self.movingInputDirectory.directory = qt.QDir.homePath()
    parametersFormLayout.addRow("", self.movingInputDirectory)

    # transform (mrml input)
    self.xformMRMLSelector = slicer.qMRMLNodeComboBox()
    self.xformMRMLSelector.nodeTypes = ["vtkMRMLLinearTransformNode"]
    self.xformMRMLSelector.selectNodeUponCreation = True
    self.xformMRMLSelector.addEnabled = False
    self.xformMRMLSelector.removeEnabled = False
    self.xformMRMLSelector.noneEnabled = False
    self.xformMRMLSelector.showHidden = False
    self.xformMRMLSelector.showChildNodeTypes = False
    self.xformMRMLSelector.setMRMLScene( slicer.mrmlScene )
    #self.xformMRMLSelector.setToolTip( "Pick the input to the algorithm." )
    parametersFormLayout.addRow("Transform: ", self.xformMRMLSelector)

    # output directory selector
    self.outputDirectory = ctk.ctkDirectoryButton()
    self.outputDirectory.directory = qt.QDir.homePath()
    parametersFormLayout.addRow("Output Directory: ", self.outputDirectory)
    
    # check box to trigger taking screen shots for later use in tutorials
    self.enableScreenshotsFlagCheckBox = qt.QCheckBox()
    self.enableScreenshotsFlagCheckBox.checked = 0
    self.enableScreenshotsFlagCheckBox.setToolTip("If checked, take screen shots for tutorials. Use Save Data to write them to disk.")
    #    parametersFormLayout.addRow("Enable Screenshots", self.enableScreenshotsFlagCheckBox)

    # scale factor for screen shots
    self.screenshotScaleFactorSliderWidget = ctk.ctkSliderWidget()
    self.screenshotScaleFactorSliderWidget.singleStep = 1.0
    self.screenshotScaleFactorSliderWidget.minimum = 1.0
    self.screenshotScaleFactorSliderWidget.maximum = 50.0
    self.screenshotScaleFactorSliderWidget.value = 1.0
    self.screenshotScaleFactorSliderWidget.setToolTip("Set scale factor for the screen shots.")
    #    parametersFormLayout.addRow("Screenshot scale factor", self.screenshotScaleFactorSliderWidget)

    # Apply Button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the algorithm."
    self.applyButton.enabled = False
    if (self.fixedMRMLSelector.currentNode()
        and self.movingMRMLSelector.currentNode()
        and self.xformMRMLSelector.currentNode()):
      self.applyButton.enabled = True
    parametersFormLayout.addRow(self.applyButton)

    # connections
    self.applyButton.connect('clicked(bool)', self.onApplyButton)
    self.fixedMRMLSelector.connect(
      "currentNodeChanged(vtkMRMLNode*)", self.onSelect)
    self.movingMRMLSelector.connect(
      "currentNodeChanged(vtkMRMLNode*)", self.onSelect)
    self.xformMRMLSelector.connect(
      "currentNodeChanged(vtkMRMLNode*)", self.onSelect)

    # Add vertical spacer
    self.layout.addStretch(1)

  def cleanup(self):
    pass

  def onSelect(self):
    if (self.fixedMRMLSelector.currentNode()
        and self.movingMRMLSelector.currentNode()
        and self.xformMRMLSelector.currentNode()):
      self.applyButton.enabled = True
    else:
      self.applyButton.enabled = False

  def onApplyButton(self):
    logic = DicomSroExportLogic()
    enableScreenshotsFlag = self.enableScreenshotsFlagCheckBox.checked
    screenshotScaleFactor = int(self.screenshotScaleFactorSliderWidget.value)
    print("Run the algorithm")
    logic.run(
      self.fixedMRMLSelector.currentNode(),
      self.movingMRMLSelector.currentNode(),
      self.xformMRMLSelector.currentNode(),
      self.outputDirectory.directory)

#
# DicomSroExportLogic
#

class DicomSroExportLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual 
  computation done by your module.  The interface 
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """

  def hasImageData(self,volumeNode):
    """This is a dummy logic method that 
    returns true if the passed in volume
    node has valid image data
    """
    if not volumeNode:
      print('no volume node')
      return False
    if volumeNode.GetImageData() == None:
      print('no image data')
      return False
    return True

  def run (self,fixedNode,movingNode,xformNode,outputDir):
    """
    Run the actual algorithm
    """
    import sys
    import vtkSlicerPlastimatchPyModuleLogic
    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    sro = vtkSlicerPlastimatchPyModuleLogic.vtkPlmpyDicomSroExport()
    sro.SetMRMLScene(slicer.mrmlScene)
    sro.SetFixedImageID(fixedNode.GetID())
    sro.SetMovingImageID(movingNode.GetID())
    sro.SetXformID(xformNode.GetID())
    sro.SetOutputDirectory(outputDir)
    sro.DoExport()

    return True


class DicomSroExportTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_DicomSroExport1()

  def test_DicomSroExport1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests sould exercise the functionality of the logic with different inputs
    (both valid and invalid).  At higher levels your tests should emulate the
    way the user would interact with your code and confirm that it still works
    the way you intended.
    One of the most important features of the tests is that it should alert other
    developers when their changes will have an impact on the behavior of your
    module.  For example, if a developer removes a feature that you depend on,
    your test should break so they know that the feature is needed.
    """

    self.delayDisplay("Starting the test")
    #
    # first, get some data
    #
    import urllib
    downloads = (
        ('http://slicer.kitware.com/midas3/download?items=5767', 'FA.nrrd', slicer.util.loadVolume),
        )

    for url,name,loader in downloads:
      filePath = slicer.app.temporaryPath + '/' + name
      if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
        print('Requesting download %s from %s...\n' % (name, url))
        urllib.urlretrieve(url, filePath)
      if loader:
        print('Loading %s...\n' % (name,))
        loader(filePath)
    self.delayDisplay('Finished with download and loading\n')

    volumeNode = slicer.util.getNode(pattern="FA")
    logic = DicomSroExportLogic()
    self.assertTrue( logic.hasImageData(volumeNode) )
    self.delayDisplay('Test passed!')
