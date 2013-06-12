import os
import unittest
from __main__ import vtk, qt, ctk, slicer

#
# NAMIC_Tutorial_2013June_SelfTest
#
# Steps:
#
# I. Evaluate isocenter shifting effect on the dose
#   1. Load planning DICOM-RT data and day 2 volumes
#   2. Add day 2 volumes in Patient Hierarchy
#   3. Compute isodose lines for both dose distributions
#     3A. Show them one after the other using Patient Hierarchy
#   4. Register day 2 CT to planning CT using rigid registration
#   5. Resample day 2 dose volumes using the transform
#     5A. Add the resampled dose to Patient Hierarchy
#   6. Compute difference dose using gamma comparison for
#     6A. Planning dose and unregistered day 2 dose
#     6B. Planning dose and registered day 2 dose
#   7. Accumulate
#     7A. Planning dose and unregistered day 2 dose
#     7B. Planning dose and registered day 2 dose
#   8. DVH for accumulated registered and unregistered dose volumes
#     for targets and some other structures
#
# II. Evaluate deformable registration
#   1. Load planning DICOM-RT data and day 2 structures
#   2. Register day 2 CT to planning CT using deformable registration
#   3. Visualize the result deformation field
#   3. Resample day 2 structures using the result transform
#   4. Compare the planning and the resampled contours
#
# III. Add margin to target structure
#   1. Load planning DICOM-RT data
#   2. Expand GTV structure
#

class NAMIC_Tutorial_2013June_SelfTest:
  def __init__(self, parent):
    parent.title = "SlicerRT NA-MIC Tutorial 2013June Self Test"
    parent.categories = ["Testing.SlicerRT Tests"]
    parent.dependencies = ["DicomRtImport", "PatientHierarchy", "Contours", "Isodose", "BRAINSFit", "BRAINSResample", "DoseComparison", "DoseAccumulation", "DoseVolumeHistogram", "DeformationFieldVisualizer", "ContourComparison", "ContourMorphology"]
    parent.contributors = ["Csaba Pinter (Queen's)"]
    parent.helpText = """
    This is a self test that automatically runs the demo/tutorial prepared for the 2013 Summer NAMIC week tutorial contest.
    """
    parent.acknowledgementText = """This file was originally developed by Csaba Pinter, PerkLab, Queen's University and was supported through the Applied Cancer Research Unit program of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care""" # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['NAMIC_Tutorial_2013June_SelfTest'] = self.runTest

  def runTest(self):
    tester = NAMIC_Tutorial_2013June_SelfTestTest()
    tester.runTest()

#
# qNAMIC_Tutorial_2013June_SelfTest_Widget
#

class NAMIC_Tutorial_2013June_SelfTestWidget:
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
    # Instantiate and connect widgets ...

    # reload button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadButton = qt.QPushButton("Reload")
    self.reloadButton.toolTip = "Reload this module."
    self.reloadButton.name = "NAMIC_Tutorial_2013June_SelfTest Reload"
    self.layout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    self.layout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)

    # Buttons to perform parts of the test
    self.layout.addStretch(1)

    # Create groupbox for workflow I
    self.workflow1Groupbox = qt.QGroupBox("Evaluate isocenter shifting")
    self.workflow1GroupboxLayout = qt.QVBoxLayout()

    # Load data button
    self.loadDataButton = qt.QPushButton("Load data")
    self.loadDataButton.toolTip = "Download (if necessary), import and load input data."
    self.loadDataButton.name = "NAMIC_Tutorial_2013June_SelfTest_LoadData"
    self.workflow1GroupboxLayout.addWidget(self.loadDataButton)
    self.loadDataButton.connect('clicked()', self.onLoadData)

    # Generate isodose button
    self.generateIsodoseButton = qt.QPushButton("Generate isodose")
    self.generateIsodoseButton.toolTip = "Generate isodose lines for both dose volumes"
    self.generateIsodoseButton.name = "NAMIC_Tutorial_2013June_SelfTest_LoadData"
    self.workflow1GroupboxLayout.addWidget(self.generateIsodoseButton)
    self.generateIsodoseButton.connect('clicked()', self.onGenerateIsodose)

    # Register button and checkbox
    self.registerButton = qt.QPushButton("Register")
    self.registerButton.toolTip = "Registers Day 2 CT to Day 1 CT. Data needs to be loaded!"
    self.registerButton.name = "NAMIC_Tutorial_2013June_SelfTest_Register"
    self.workflow1GroupboxLayout.addWidget(self.registerButton)
    self.registerButton.connect('clicked()', self.onRegister)

    # Resample button
    self.resampleButton = qt.QPushButton("Resample")
    self.resampleButton.toolTip = "Resamples Day 2 dose volume using the resulting transformations. All previous steps are needed to be run!"
    self.resampleButton.name = "NAMIC_Tutorial_2013June_SelfTest_Resample"
    self.workflow1GroupboxLayout.addWidget(self.resampleButton)
    self.resampleButton.connect('clicked()', self.onResample)

    # Compute gamma button
    self.computeGammaButton = qt.QPushButton("Compare dose distributions")
    self.computeGammaButton.toolTip = "Computes gamma dose difference for the two dose volumes."
    self.computeGammaButton.name = "NAMIC_Tutorial_2013June_SelfTest_ComputeDvh"
    self.workflow1GroupboxLayout.addWidget(self.computeGammaButton)
    self.computeGammaButton.connect('clicked()', self.onComputeGamma)

    # Accumulate dose button
    self.accumulateDoseButton = qt.QPushButton("Accumulate dose")
    self.accumulateDoseButton.toolTip = "Accumulates doses using all the Day 2 variants. All previous steps are needed to be run!"
    self.accumulateDoseButton.name = "NAMIC_Tutorial_2013June_SelfTest_AccumulateDose"
    self.workflow1GroupboxLayout.addWidget(self.accumulateDoseButton)
    self.accumulateDoseButton.connect('clicked()', self.onAccumulateDose)
    
    # Compute DVH button
    self.computeDvhButton = qt.QPushButton("Compute DVH")
    self.computeDvhButton.toolTip = "Computes DVH on the accumulated doses. All previous steps are needed to be run!"
    self.computeDvhButton.name = "NAMIC_Tutorial_2013June_SelfTest_ComputeDvh"
    self.workflow1GroupboxLayout.addWidget(self.computeDvhButton)
    self.computeDvhButton.connect('clicked()', self.onComputeDvh)

    self.workflow1Groupbox.setLayout(self.workflow1GroupboxLayout)
    self.layout.addWidget(self.workflow1Groupbox)
    
    # Add vertical spacer
    self.layout.addStretch(4)

  def onReload(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    """Generic reload method for any scripted module.
    ModuleWizard will subsitute correct default moduleName.
    """
    import imp, sys, os, slicer

    widgetName = moduleName + "Widget"

    # reload the source code
    # - set source file path
    # - load the module to the global space
    filePath = eval('slicer.modules.%s.path' % moduleName.lower())
    p = os.path.dirname(filePath)
    if not sys.path.__contains__(p):
      sys.path.insert(0,p)
    fp = open(filePath, "r")
    globals()[moduleName] = imp.load_module(
        moduleName, fp, filePath, ('.py', 'r', imp.PY_SOURCE))
    fp.close()

    # rebuild the widget
    # - find and hide the existing widget
    # - create a new widget in the existing parent
    parent = slicer.util.findChildren(name='%s Reload' % moduleName)[0].parent()
    for child in parent.children():
      try:
        child.hide()
      except AttributeError:
        pass
    # Remove spacer items
    item = parent.layout().itemAt(0)
    while item:
      parent.layout().removeItem(item)
      item = parent.layout().itemAt(0)
    # create new widget inside existing parent
    globals()[widgetName.lower()] = eval(
        'globals()["%s"].%s(parent)' % (moduleName, widgetName))
    globals()[widgetName.lower()].setup()

  def onReloadAndTest(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.runTest()

  def onLoadData(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp()

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_01A_OpenTempDatabase()
    tester.TestSection_I_01B_DownloadDay1Data()
    tester.TestSection_I_01C_ImportDay1Study()
    tester.TestSection_I_01D_SelectLoadablesAndLoad()
    tester.TestSection_I_01E_LoadDay2Data()
    tester.TestSection_I_01F_SetDisplayOptions()
    tester.TestSection_I_02_AddDayDataToPatientHierarchy()

  def onGenerateIsodose(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_03A_ComputeIsodose()
    tester.TestSection_I_03B_ShowIsodoseLineSets()

  def onRegister(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_04_RegisterDay2CTToDay1CT()

  def onResample(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_05A_ResampleDoseVolumes()
    tester.TestSection_I_05B_AddResampledDoseVolumesToPatientHierarchy()

  def onComputeGamma(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_06_ComputeGamma()

  def onAccumulateDose(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_07_AccumulateDose()

  def onComputeDvh(self,moduleName="NAMIC_Tutorial_2013June_SelfTest"):
    performDeformableRegistration = self.deformableCheckbox.checked
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    self.deformableCheckbox.setChecked(performDeformableRegistration)
    tester.performDeformableRegistration = performDeformableRegistration
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_I_00_SetupPathsAndNames()
    tester.TestSection_I_08_ComputeDvh()

#
# NAMIC_Tutorial_2013June_SelfTestLogic
#

class NAMIC_Tutorial_2013June_SelfTestLogic:
  """This class should implement all the actual 
  computation done by your module.  The interface 
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    pass

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


class NAMIC_Tutorial_2013June_SelfTestTest(unittest.TestCase):
  """
  This is the test case for your scripted module.
  """

  def __init__(self):
    self.performDeformableRegistration = False

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

  def setUp(self, clearScene=True):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    if clearScene:
      slicer.mrmlScene.Clear(0)

    self.delayMs = 700

    #TODO: Comment out
    #logFile = open('d:/pyTestLog.txt', 'w')
    #logFile.write(repr(slicer.modules.models) + '\n')
    #logFile.write(repr(slicer.modules.NAMIC_Tutorial_2013June_SelfTest) + '\n')
    #logFile.write(repr(slicer.modules.dicomrtimport) + '\n')
    #logFile.write(repr(slicer.modules.models) + '\n')
    #logFile.close()

    self.moduleName = "NAMIC_Tutorial_2013June_SelfTest"

  def clickAndDrag(self,widget,button='Left',start=(10,10),end=(10,40),steps=20,modifiers=[]):
    """Send synthetic mouse events to the specified widget (qMRMLSliceWidget or qMRMLThreeDView)
    button : "Left", "Middle", "Right", or "None"
    start, end : window coordinates for action
    steps : number of steps to move in
    modifiers : list containing zero or more of "Shift" or "Control"
    """
    style = widget.interactorStyle()
    interator = style.GetInteractor()
    if button == 'Left':
      down = style.OnLeftButtonDown
      up = style.OnLeftButtonUp
    elif button == 'Right':
      down = style.OnRightButtonDown
      up = style.OnRightButtonUp
    elif button == 'Middle':
      down = style.OnMiddleButtonDown
      up = style.OnMiddleButtonUp
    elif button == 'None' or not button:
      down = lambda : None
      up = lambda : None
    else:
      raise Exception("Bad button - should be Left or Right, not %s" % button)
    if 'Shift' in modifiers:
      interator.SetShiftKey(1)
    if 'Control' in modifiers:
      interator.SetControlKey(1)
    interator.SetEventPosition(*start)
    down()
    for step in xrange(steps):
      frac = float(step)/steps
      x = int(start[0] + frac*(end[0]-start[0]))
      y = int(start[1] + frac*(end[1]-start[1]))
      interator.SetEventPosition(x,y)
      style.OnMouseMove()
    up()
    interator.SetShiftKey(0)
    interator.SetControlKey(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp(self.performDeformableRegistration)

    self.test_NAMIC_Tutorial_2013June_SelfTest_FullTest()

  def test_NAMIC_Tutorial_2013June_SelfTest_FullTest(self):
    self.TestSection_I_EvaluateIsocenterShifting()
    self.TestSection_II_EvaluateDeformableRegistration()
    self.TestSection_III_AddMarginToTargetStructure()

  def TestSection_I_EvaluateIsocenterShifting(self):
    try:
      # Check for modules
      self.assertTrue( slicer.modules.dicomrtimport )
      self.assertTrue( slicer.modules.patienthierarchy )
      self.assertTrue( slicer.modules.contours )
      self.assertTrue( slicer.modules.isodose )
      self.assertTrue( slicer.modules.brainsfit )
      self.assertTrue( slicer.modules.brainsresample )
      self.assertTrue( slicer.modules.dosecomparison )
      self.assertTrue( slicer.modules.doseaccumulation )
      self.assertTrue( slicer.modules.dosevolumehistogram )
      self.assertTrue( slicer.modules.deformationfieldvisualizer )
      self.assertTrue( slicer.modules.contourcomparison )
      self.assertTrue( slicer.modules.contourmorphology )

      self.TestSection_I_00_SetupPathsAndNames()
      self.TestSection_I_01A_OpenTempDatabase()
      self.TestSection_I_01B_DownloadDay1Data()
      self.TestSection_I_01C_ImportDay1Study()
      self.TestSection_I_01D_SelectLoadablesAndLoad()
      self.TestSection_I_01E_LoadDay2Data()
      self.TestSection_I_01F_SetDisplayOptions()
      self.TestSection_I_02_AddDayDataToPatientHierarchy()
      self.TestSection_I_03A_ComputeIsodose()
      self.TestSection_I_03B_ShowIsodoseLineSets()
      self.TestSection_I_04_RegisterDay2CTToDay1CT()
      self.TestSection_I_05A_ResampleDoseVolumes()
      self.TestSection_I_05B_AddResampledDoseVolumesToPatientHierarchy()
      self.TestSection_I_06_ComputeGamma()
      self.TestSection_I_07_AccumulateDose()
      self.TestSection_I_08_ComputeDvh()
      # self.TestSection_ClearDatabase()

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_00_SetupPathsAndNames(self):
    NAMIC_Tutorial_2013June_SelfTestDir = slicer.app.temporaryPath + '/NAMIC_Tutorial_2013June_SelfTest'
    if not os.access(NAMIC_Tutorial_2013June_SelfTestDir, os.F_OK):
      os.mkdir(NAMIC_Tutorial_2013June_SelfTestDir)

    self.dicomDataDir = NAMIC_Tutorial_2013June_SelfTestDir + '/EclipseEntPhantomRtData'
    if not os.access(self.dicomDataDir, os.F_OK):
      os.mkdir(self.dicomDataDir)

    self.day2DataDir = NAMIC_Tutorial_2013June_SelfTestDir + '/EclipseEntComputedDay2Data'
    if not os.access(self.day2DataDir, os.F_OK):
      os.mkdir(self.day2DataDir)

    self.dicomDatabaseDir = NAMIC_Tutorial_2013June_SelfTestDir + '/CtkDicomDatabase'
    self.dicomZipFilePath = NAMIC_Tutorial_2013June_SelfTestDir + '/EclipseEntDicomRt.zip'
    self.expectedNumOfFilesInDicomDataDir = 141
    self.tempDir = NAMIC_Tutorial_2013June_SelfTestDir + '/Temp'
    
    self.day1CTName = '2: ENT IMRT'
    self.day1DoseName = '5: RTDOSE'
    self.day2CTName = '2_ENT_IMRT_Day2'
    self.day2DoseName = '5_RTDOSE_Day2'
    self.transformDay2ToDay1RigidName = 'Transform_Day2ToDay1_Rigid'
    self.transformDay2ToDay1BSplineName = 'Transform_Day2ToDay1_BSpline'
    self.day2DoseRigidName = '5_RTDOSE_Day2Registered_Rigid'
    self.day2DoseBSplineName = '5_RTDOSE_Day2Registered_BSpline'
    self.doseUnitNameAttributeName = 'DicomRtImport.DoseUnitName'
    self.doseUnitValueAttributeName = 'DicomRtImport.DoseUnitValue'
    self.doseAccumulationDoseVolumeNameProperty = 'DoseAccumulation.DoseVolumeNodeName'
    self.accumulatedDoseUnregisteredName = '5_RTDOSE Accumulated Unregistered'
    self.accumulatedDoseRigidName = '5_RTDOSE Accumulated Rigid'
    self.accumulatedDoseBSplineName = '5_RTDOSE Accumulated BSpline'
    
    self.setupPathsAndNamesDone = True

  def TestSection_I_01A_OpenTempDatabase(self):
    # Open test database and empty it
    try:
      qt.QDir().mkpath(self.dicomDatabaseDir)

      if slicer.dicomDatabase:
        self.originalDatabaseDirectory = os.path.split(slicer.dicomDatabase.databaseFilename)[0]
      else:
        self.originalDatabaseDirectory = None
        settings = qt.QSettings()
        settings.setValue('DatabaseDirectory', self.dicomDatabaseDir)

      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      dicomWidget.onDatabaseDirectoryChanged(self.dicomDatabaseDir)
      self.assertTrue( slicer.dicomDatabase.isOpen )

      initialized = slicer.dicomDatabase.initializeDatabase()

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_01B_DownloadDay1Data(self):
    import urllib
    downloads = (
        ('http://slicer.kitware.com/midas3/download?items=10704', self.dicomZipFilePath),
        )

    downloaded = 0
    for url,filePath in downloads:
      if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
        if downloaded == 0:
          self.delayDisplay('Downloading Day 1 input data to folder\n' + self.dicomZipFilePath + '.\n\n  It may take a few minutes...',self.delayMs)
        print('Requesting download from %s...' % (url))
        urllib.urlretrieve(url, filePath)
        downloaded += 1
      else:
        self.delayDisplay('Day 1 input data has been found in folder ' + self.dicomZipFilePath, self.delayMs)
    if downloaded > 0:
      self.delayDisplay('Downloading Day 1 input data finished',self.delayMs)

    numOfFilesInDicomDataDir = len([name for name in os.listdir(self.dicomDataDir) if os.path.isfile(self.dicomDataDir + '/' + name)])
    if (numOfFilesInDicomDataDir != self.expectedNumOfFilesInDicomDataDir):
      slicer.app.applicationLogic().Unzip(self.dicomZipFilePath, self.dicomDataDir)
      self.delayDisplay("Unzipping done",self.delayMs)

    numOfFilesInDicomDataDirTest = len([name for name in os.listdir(self.dicomDataDir) if os.path.isfile(self.dicomDataDir + '/' + name)])
    self.assertTrue( numOfFilesInDicomDataDirTest == self.expectedNumOfFilesInDicomDataDir )

  def TestSection_I_01C_ImportDay1Study(self):
    self.delayDisplay("Import Day 1 study",self.delayMs)

    try:
      mainWindow = slicer.util.mainWindow()
      mainWindow.moduleSelector().selectModule('DICOM')

      # Import study to database
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      dicomWidget.dicomApp.suspendModel()
      indexer = ctk.ctkDICOMIndexer()
      self.assertTrue( indexer )

      indexer.addDirectory( slicer.dicomDatabase, self.dicomDataDir )
      dicomWidget.dicomApp.resumeModel()

      self.assertTrue( len(slicer.dicomDatabase.patients()) == 1 )
      self.assertTrue( slicer.dicomDatabase.patients()[0] )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_01D_SelectLoadablesAndLoad(self):
    self.delayDisplay("Select loadables and load data",self.delayMs)

    try:
      numOfScalarVolumeNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') )
      numOfModelHierarchyNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') )
      numOfContourNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLContourNode*') )

      # Choose first patient from the patient list
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      self.delayDisplay("Wait for DICOM browser to initialize",self.delayMs)
      index = dicomWidget.tree.indexAt(qt.QPoint(0,0))
      dicomWidget.onTreeClicked(index)

      # Make sure the loadables are good (RT is assigned to 3 out of 4 and they are selected)
      loadablesByPlugin = dicomWidget.detailsPopup.loadablesByPlugin
      rtFound = False
      loadablesForRt = 0
      for plugin in loadablesByPlugin:
        if plugin.loadType == 'RT':
          rtFound = True
        else:
          continue
        for loadable in loadablesByPlugin[plugin]:
          loadablesForRt += 1
          self.assertTrue( loadable.selected )

      self.assertTrue( rtFound )
      self.assertTrue( loadablesForRt == 2 )

      dicomWidget.detailsPopup.loadCheckedLoadables()

      # Verify that the correct number of objects were loaded
      scene = slicer.mrmlScene
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == numOfScalarVolumeNodesBeforeLoad + 2 )
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') ) == numOfModelHierarchyNodesBeforeLoad + 17 )
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLContourNode*') ) == numOfContourNodesBeforeLoad + 16 )
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_01E_LoadDay2Data(self):
    import urllib
    downloads = (
        ('http://slicer.kitware.com/midas3/download?items=10702', self.day2DataDir + '/' + self.day2CTName + '.nrrd', slicer.util.loadVolume),
        ('http://slicer.kitware.com/midas3/download?items=10703', self.day2DataDir + '/' + self.day2DoseName + '.nrrd', slicer.util.loadVolume),
        )

    numOfScalarVolumeNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') )

    downloaded = 0
    for url,filePath,loader in downloads:
      if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
        if downloaded == 0:
          self.delayDisplay('Downloading Day 2 input data to folder\n' + self.day2DataDir + '\n\n  It may take a few minutes...',self.delayMs)
        print('Requesting download from %s...\n' % (url))
        urllib.urlretrieve(url, filePath)
        downloaded += 1
      else:
        self.delayDisplay('Day 2 input data has been found in folder ' + self.day2DataDir, self.delayMs)
      if loader:
        print('Loading %s...' % (os.path.split(filePath)[1]))
        loader(filePath)
    if downloaded > 0:
      self.delayDisplay('Downloading Day 2 input data finished',self.delayMs)

    # Verify that the correct number of objects were loaded
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == numOfScalarVolumeNodesBeforeLoad + 2 )

  def TestSection_I_01F_SetDisplayOptions(self):
    self.delayDisplay('Setting display options for loaded data',self.delayMs)

    layoutManager = slicer.app.layoutManager()
    layoutManager.setLayout(3)

    # Set Day 2 dose color map
    day2Dose = slicer.util.getNode(pattern=self.day2DoseName)
    day2Dose.GetDisplayNode().SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow")

    # Set CT windows
    day1CT = slicer.util.getNode(pattern=self.day1CTName)
    day1CT.GetDisplayNode().SetAutoWindowLevel(0)
    day1CT.GetDisplayNode().SetWindowLevel(250,80)

    day2CT = slicer.util.getNode(pattern=self.day2CTName)
    day2CT.GetDisplayNode().SetAutoWindowLevel(0)
    day2CT.GetDisplayNode().SetWindowLevel(250,80)

    # Set volumes to show
    sliceWidgetNames = ['Red', 'Green', 'Yellow']
    for sliceWidgetName in sliceWidgetNames:
      slice = layoutManager.sliceWidget(sliceWidgetName)
      sliceLogic = slice.sliceLogic()
      compositeNode = sliceLogic.GetSliceCompositeNode()
      compositeNode.SetBackgroundVolumeID(day1CT.GetID())
      compositeNode.SetForegroundVolumeID(day2CT.GetID())
      sliceLogic.SetForegroundOpacity(0.5)

    layoutManager.sliceWidget(sliceWidgetNames[0]).sliceController().setSliceOffsetValue(138)
    layoutManager.sliceWidget(sliceWidgetNames[1]).sliceController().setSliceOffsetValue(-18)
    
    # Set structure visibilities/transparencies
    optBrain = slicer.util.getNode(pattern='optBRAIN_Contour_RibbonModel')
    optBrain.GetDisplayNode().SetVisibility(0)
    optOptic = slicer.util.getNode(pattern='optOptic_Contour_RibbonModel')
    optOptic.GetDisplayNode().SetVisibility(0)

    threeDView = layoutManager.threeDWidget(0).threeDView()
    self.clickAndDrag(threeDView,button='Middle',start=(10,110),end=(10,10))
    self.clickAndDrag(threeDView,button='Middle',start=(10,100),end=(10,10))
    self.clickAndDrag(threeDView,start=(10,70),end=(90,10))

  def TestSection_I_02_AddDayDataToPatientHierarchy(self):
    try:
      pass #TODO
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_03A_ComputeIsodose(self):
    try:
      pass #TODO
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_03B_ShowIsodoseLineSets(self):
    try:
      pass #TODO
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_04_RegisterDay2CTToDay1CT(self):
    try:
      scene = slicer.mrmlScene
      mainWindow = slicer.util.mainWindow()
      mainWindow.moduleSelector().selectModule('BRAINSFit')
      brainsFit = slicer.modules.brainsfit

      # Register Day 2 CT to Day 1 CT using rigid registration
      self.delayDisplay("Register Day 2 CT to Day 1 CT using rigid registration.\n  It may take a few minutes...",self.delayMs)

      parametersRigid = {}
      day1CT = slicer.util.getNode(pattern=self.day1CTName)
      parametersRigid["fixedVolume"] = day1CT.GetID()

      day2CT = slicer.util.getNode(pattern=self.day2CTName)
      parametersRigid["movingVolume"] = day2CT.GetID()
      
      linearTransform = slicer.vtkMRMLLinearTransformNode()
      linearTransform.SetName(self.transformDay2ToDay1RigidName)
      slicer.mrmlScene.AddNode( linearTransform )
      parametersRigid["linearTransform"] = linearTransform.GetID()

      parametersRigid["useRigid"] = True

      self.cliBrainsFitRigidNode = None
      self.cliBrainsFitRigidNode = slicer.cli.run(brainsFit, None, parametersRigid)
      waitCount = 0
      while self.cliBrainsFitRigidNode.GetStatusString() != 'Completed' and waitCount < 100:
        self.delayDisplay( "Register Day 2 CT to Day 1 CT using rigid registration... %d" % waitCount )
        waitCount += 1
      self.delayDisplay("Register Day 2 CT to Day 1 CT using rigid registration finished",self.delayMs)

      self.assertTrue( self.cliBrainsFitRigidNode.GetStatusString() == 'Completed' )

      # Register Day 2 CT to Day 1 CT using BSpline registration
      if self.performDeformableRegistration:
        self.delayDisplay("Register Day 2 CT to Day 1 CT using BSpline registration.\n  It may take a few minutes...",self.delayMs)

        parametersBSpline = {}
        parametersBSpline["fixedVolume"] = day1CT.GetID()
        parametersBSpline["movingVolume"] = day2CT.GetID()
        parametersBSpline["initialTransform"] = linearTransform.GetID()

        bsplineTransform = slicer.vtkMRMLBSplineTransformNode()
        bsplineTransform.SetName(self.transformDay2ToDay1BSplineName)
        slicer.mrmlScene.AddNode( bsplineTransform )
        parametersBSpline["bsplineTransform"] = bsplineTransform.GetID()

        parametersBSpline["useBSpline"] = True

        self.cliBrainsFitBSplineNode = None
        self.cliBrainsFitBSplineNode = slicer.cli.run(brainsFit, None, parametersBSpline)
        waitCount = 0
        while self.cliBrainsFitBSplineNode.GetStatusString() != 'Completed' and waitCount < 600:
          self.delayDisplay( "Register Day 2 CT to Day 1 CT using BSpline registration... %d" % waitCount )
          waitCount += 1
        self.delayDisplay("Register Day 2 CT to Day 1 CT using BSpline registration finished",self.delayMs)

        self.assertTrue( self.cliBrainsFitBSplineNode.GetStatusString() == 'Completed' )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_05A_ResampleDoseVolumes(self):
    try:
      mainWindow = slicer.util.mainWindow()
      mainWindow.moduleSelector().selectModule('BRAINSResample')
      brainsResample = slicer.modules.brainsresample

      day1Dose = slicer.util.getNode(pattern=self.day1DoseName)
      day2Dose = slicer.util.getNode(pattern=self.day2DoseName)

      # Resample Day 2 Dose using Day 2 CT to Day 1 CT rigid transform
      self.delayDisplay("Resample Day 2 Dose using Day 2 CT to Day 1 CT rigid transform",self.delayMs)

      parametersRigid = {}
      parametersRigid["inputVolume"] = day2Dose.GetID()
      parametersRigid["referenceVolume"] = day1Dose.GetID()

      day2DoseRigid = slicer.vtkMRMLScalarVolumeNode()
      day2DoseRigid.SetName(self.day2DoseRigidName)
      slicer.mrmlScene.AddNode( day2DoseRigid )
      parametersRigid["outputVolume"] = day2DoseRigid.GetID()

      parametersRigid["pixelType"] = 'float'

      transformDay2ToDay1Rigid = slicer.util.getNode(pattern=self.transformDay2ToDay1RigidName)
      parametersRigid["warpTransform"] = transformDay2ToDay1Rigid.GetID()

      self.cliBrainsResampleRigidNode = None
      self.cliBrainsResampleRigidNode = slicer.cli.run(brainsResample, None, parametersRigid)
      waitCount = 0
      while self.cliBrainsResampleRigidNode.GetStatusString() != 'Completed' and waitCount < 100:
        self.delayDisplay( "Resample Day 2 Dose using Day 2 CT to Day 1 CT rigid transform... %d" % waitCount )
        waitCount += 1
      self.delayDisplay("Resample Day 2 Dose using Day 2 CT to Day 1 CT rigid transform finished",self.delayMs)

      self.assertTrue( self.cliBrainsResampleRigidNode.GetStatusString() == 'Completed' )

      # Resample Day 2 Dose using Day 2 CT to Day 1 CT rigid transform
      if self.performDeformableRegistration:
        self.delayDisplay("Resample Day 2 Dose using Day 2 CT to Day 1 CT BSpline transform",self.delayMs)

        parametersBSpline = {}
        parametersBSpline["inputVolume"] = day2Dose.GetID()
        parametersBSpline["referenceVolume"] = day1Dose.GetID()

        day2DoseBSplineName = slicer.vtkMRMLScalarVolumeNode()
        day2DoseBSplineName.SetName(self.day2DoseBSplineName)
        slicer.mrmlScene.AddNode( day2DoseBSplineName )
        parametersBSpline["outputVolume"] = day2DoseBSplineName.GetID()

        parametersBSpline["pixelType"] = 'float'

        transformDay2ToDay1BSpline = slicer.util.getNode(pattern=self.transformDay2ToDay1BSplineName)
        parametersBSpline["warpTransform"] = transformDay2ToDay1BSpline.GetID()

        self.cliBrainsResampleBSplineNode = None
        self.cliBrainsResampleBSplineNode = slicer.cli.run(brainsResample, None, parametersBSpline)
        waitCount = 0
        while self.cliBrainsResampleBSplineNode.GetStatusString() != 'Completed' and waitCount < 100:
          self.delayDisplay( "Resample Day 2 Dose using Day 2 CT to Day 1 CT BSpline transform... %d" % waitCount )
          waitCount += 1
        self.delayDisplay("Resample Day 2 Dose using Day 2 CT to Day 1 CT BSpline transform finished",self.delayMs)

        self.assertTrue( self.cliBrainsResampleBSplineNode.GetStatusString() == 'Completed' )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_05B_AddResampledDoseVolumesToPatientHierarchy(self):
    self.delayDisplay("Setting attributes for resampled dose volumes",self.delayMs)

    try:
      day1Dose = slicer.util.getNode(pattern=self.day1DoseName)
      doseUnitName = day1Dose.GetAttribute(self.doseUnitNameAttributeName)
      doseUnitValue = day1Dose.GetAttribute(self.doseUnitValueAttributeName)

      day2Dose = slicer.util.getNode(pattern=self.day2DoseName)
      day2Dose.SetAttribute(self.doseUnitNameAttributeName,doseUnitName)
      day2Dose.SetAttribute(self.doseUnitValueAttributeName,doseUnitValue)

      day2DoseRigid = slicer.util.getNode(pattern=self.day2DoseRigidName)
      self.assertTrue(day2DoseRigid)
      day2DoseRigid.SetAttribute(self.doseUnitNameAttributeName,doseUnitName)
      day2DoseRigid.SetAttribute(self.doseUnitValueAttributeName,doseUnitValue)
      self.assertTrue(day2DoseRigid.GetDisplayNode())
      day2DoseRigid.GetDisplayNode().SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow")

      if self.performDeformableRegistration:
        day2DoseBSplineName = slicer.util.getNode(pattern=self.day2DoseBSplineName)
        self.assertTrue(day2DoseBSplineName)
        day2DoseBSplineName.SetAttribute(self.doseUnitNameAttributeName,doseUnitName)
        day2DoseBSplineName.SetAttribute(self.doseUnitValueAttributeName,doseUnitValue)
        self.assertTrue(day2DoseBSplineName.GetDisplayNode())
        day2DoseBSplineName.GetDisplayNode().SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow")

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_06_ComputeGamma(self):
    try:
      pass #TODO
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def DoseAccumulation_CheckDoseVolume(self, widget, doseVolumeName, checked):
    try:
      checkboxes = slicer.util.findChildren(widget=widget, className='QCheckBox')
      for checkbox in checkboxes:
        if checkbox.property(self.doseAccumulationDoseVolumeNameProperty) == doseVolumeName:
          checkbox.setChecked(checked)
          break

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_07_AccumulateDose(self):
    try:
      mainWindow = slicer.util.mainWindow()
      mainWindow.moduleSelector().selectModule('DoseAccumulation')
      doseAccumulationWidget = slicer.modules.doseaccumulation.widgetRepresentation()

      day1Dose = slicer.util.getNode(pattern=self.day1DoseName)
      inputFrame = slicer.util.findChildren(widget=doseAccumulationWidget, className='ctkCollapsibleButton', text='Input')[0]
      referenceVolumeCombobox = slicer.util.findChildren(widget=inputFrame, className='qMRMLNodeComboBox')[0]
      referenceVolumeCombobox.setCurrentNode(day1Dose)

      applyButton = slicer.util.findChildren(widget=doseAccumulationWidget, text='Apply')[0]
      outputFrame = slicer.util.findChildren(widget=doseAccumulationWidget, className='ctkCollapsibleButton', text='Output')[0]
      outputMrmlNodeCombobox = slicer.util.findChildren(widget=outputFrame, className='qMRMLNodeComboBox')[0]
      
      self.assertTrue( referenceVolumeCombobox != None )
      self.assertTrue( applyButton != None )
      self.assertTrue( outputMrmlNodeCombobox != None )

      self.DoseAccumulation_CheckDoseVolume(doseAccumulationWidget, self.day1DoseName, 1)
      
      # Create output volumes
      accumulatedDoseUnregistered = slicer.vtkMRMLScalarVolumeNode()
      accumulatedDoseUnregistered.SetName(self.accumulatedDoseUnregisteredName)
      slicer.mrmlScene.AddNode( accumulatedDoseUnregistered )

      accumulatedDoseRigid = slicer.vtkMRMLScalarVolumeNode()
      accumulatedDoseRigid.SetName(self.accumulatedDoseRigidName)
      slicer.mrmlScene.AddNode( accumulatedDoseRigid )

      if self.performDeformableRegistration:
        accumulatedDoseBSpline = slicer.vtkMRMLScalarVolumeNode()
        accumulatedDoseBSpline.SetName(self.accumulatedDoseBSplineName)
        slicer.mrmlScene.AddNode( accumulatedDoseBSpline )

      # Accumulate Day 1 dose and untransformed Day 2 dose
      self.delayDisplay("Accumulate Day 1 dose with unregistered Day 2 dose",self.delayMs)
      self.DoseAccumulation_CheckDoseVolume(doseAccumulationWidget, self.day2DoseName, 1)
      outputMrmlNodeCombobox.setCurrentNode(accumulatedDoseUnregistered)
      applyButton.click()
      
      self.assertTrue( accumulatedDoseUnregistered.GetImageData() )

      self.delayDisplay("Accumulate Day 1 dose with unregistered Day 2 dose finished",self.delayMs)
      self.DoseAccumulation_CheckDoseVolume(doseAccumulationWidget, self.day2DoseName, 0)

      # Accumulate Day 1 dose and Day 2 dose transformed using the rigid transform
      self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with rigid registration",self.delayMs)
      self.DoseAccumulation_CheckDoseVolume(doseAccumulationWidget, self.day2DoseRigidName, 1)
      outputMrmlNodeCombobox.setCurrentNode(accumulatedDoseRigid)
      applyButton.click()
      
      self.assertTrue( accumulatedDoseRigid.GetImageData() )

      self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with rigid registration finished",self.delayMs)
      self.DoseAccumulation_CheckDoseVolume(doseAccumulationWidget, self.day2DoseRigidName, 0)

      # Accumulate Day 1 dose and Day 2 dose transformed using the BSpline transform
      if self.performDeformableRegistration:
        self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with BSpline registration",self.delayMs)
        self.DoseAccumulation_CheckDoseVolume(doseAccumulationWidget, self.day2DoseBSplineName, 1)
        outputMrmlNodeCombobox.setCurrentNode(accumulatedDoseBSpline)
        applyButton.click()
        
        self.assertTrue( accumulatedDoseBSpline.GetImageData() )

        self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with BSpline registration finished",self.delayMs)

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_I_08_ComputeDvh(self):
    try:
      scene = slicer.mrmlScene
      mainWindow = slicer.util.mainWindow()
      mainWindow.moduleSelector().selectModule('DoseVolumeHistogram')
      dvhWidget = slicer.modules.dosevolumehistogram.widgetRepresentation()

      numOfDoubleArrayNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLDoubleArrayNode*') )

      computeDvhButton = slicer.util.findChildren(widget=dvhWidget, text='Compute DVH')[0]
      mrmlNodeComboboxes = slicer.util.findChildren(widget=dvhWidget, className='qMRMLNodeComboBox')
      for mrmlNodeCombobox in mrmlNodeComboboxes:
        if 'vtkMRMLScalarVolumeNode' in mrmlNodeCombobox.nodeTypes:
          doseVolumeNodeCombobox = mrmlNodeCombobox
        elif 'vtkMRMLContourNode' in mrmlNodeCombobox.nodeTypes:
          contourNodeCombobox = mrmlNodeCombobox
        elif 'vtkMRMLChartNode' in mrmlNodeCombobox.nodeTypes:
          chartNodeCombobox = mrmlNodeCombobox

      ptvContour = slicer.util.getNode(pattern='PTV1_Contour')
      contourNodeCombobox.setCurrentNode(ptvContour)

      # Compute DVH using untransformed accumulated dose
      self.delayDisplay("Compute DVH of accumulated dose (unregistered)",self.delayMs)
      accumulatedDoseUnregistered = slicer.util.getNode(pattern=self.accumulatedDoseUnregisteredName)
      doseVolumeNodeCombobox.setCurrentNode(accumulatedDoseUnregistered)
      computeDvhButton.click()
      
      # Compute DVH using accumulated dose volume that used Day 2 dose after rigid transform
      self.delayDisplay("Compute DVH of accumulated dose (rigid registration)",self.delayMs)
      accumulatedDoseRigid = slicer.util.getNode(pattern=self.accumulatedDoseRigidName)
      doseVolumeNodeCombobox.setCurrentNode(accumulatedDoseRigid)
      computeDvhButton.click()

      # Compute DVH using accumulated dose volume that used Day 2 dose after BSpline transform
      if self.performDeformableRegistration:
        self.delayDisplay("Compute DVH of accumulated dose (BSpline registration)",self.delayMs)
        accumulatedDoseBSpline = slicer.util.getNode(pattern=self.accumulatedDoseBSplineName)
        doseVolumeNodeCombobox.setCurrentNode(accumulatedDoseBSpline)
        computeDvhButton.click()

      self.assertTrue( len( slicer.util.getNodes('vtkMRMLDoubleArrayNode*') ) == numOfDoubleArrayNodesBeforeLoad + 2 + self.performDeformableRegistration )

      # Create chart and show plots
      chartNodeCombobox.addNode()
      self.delayDisplay("Show DVH charts",self.delayMs)
      showAllCheckbox = slicer.util.findChildren(widget=dvhWidget, text='Show/hide all', className='qCheckBox')[0]
      self.assertTrue( showAllCheckbox )
      showAllCheckbox.setChecked(1)

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_ClearDatabase(self):
    self.delayDisplay("Clear database",self.delayMs)

    initialized = slicer.dicomDatabase.initializeDatabase()

    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )

    self.delayDisplay("Restoring original database directory",self.delayMs)
    if self.originalDatabaseDirectory:
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      dicomWidget.onDatabaseDirectoryChanged(self.originalDatabaseDirectory)

  def TestSection_II_EvaluateDeformableRegistration(self):
    try:
      # Check for modules
      self.assertTrue( slicer.modules.dicomrtimport )
      self.assertTrue( slicer.modules.patienthierarchy )
      self.assertTrue( slicer.modules.contours )
      self.assertTrue( slicer.modules.brainsfit )
      self.assertTrue( slicer.modules.brainsresample )
      self.assertTrue( slicer.modules.deformationfieldvisualizer )
      self.assertTrue( slicer.modules.contourcomparison )

      # self.TestSection_II..._00_SetupPathsAndNames()

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_III_AddMarginToTargetStructure(self):
    try:
      # Check for modules
      self.assertTrue( slicer.modules.dicomrtimport )
      self.assertTrue( slicer.modules.patienthierarchy )
      self.assertTrue( slicer.modules.contours )
      self.assertTrue( slicer.modules.contourmorphology )

      # self.TestSection_III..._00_SetupPathsAndNames()

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
