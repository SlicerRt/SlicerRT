import os
import unittest
import DicomRtImportSelfTestPaths
from __main__ import vtk, qt, ctk, slicer

#
# DicomRtImportSelfTest
#

class DicomRtImportSelfTest:
  def __init__(self, parent):
    parent.title = "DicomRtImportSelfTest" # TODO make this more human readable by adding spaces
    parent.categories = ["Testing.TestCases"]
    parent.dependencies = ["DicomRtImport"]
    parent.contributors = ["Csaba Pinter (Queen's)"]
    parent.helpText = """
    This is a self test for the DicomRtImport DICOM plugin module.
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
    slicer.selfTests['DicomRtImportSelfTest'] = self.runTest

  def runTest(self):
    tester = DicomRtImportSelfTestTest()
    tester.runTest()

#
# qDicomRtImportSelfTestWidget
#

class DicomRtImportSelfTestWidget:
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
    self.reloadButton.name = "DicomRtImportSelfTest Reload"
    self.layout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    self.layout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)

    # Add vertical spacer
    self.layout.addStretch(1)

  def onReload(self,moduleName="DicomRtImportSelfTest"):
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

  def onReloadAndTest(self,moduleName="DicomRtImportSelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.runTest()

#
# DicomRtImportSelfTestLogic
#

class DicomRtImportSelfTestLogic:
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


class DicomRtImportSelfTestTest(unittest.TestCase):
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

    self.delayMs = 700

    #TODO: Comment out
    #logFile = open('d:/pyTestLog.txt', 'w')
    #logFile.write(repr(slicer.modules.models) + '\n')
    #logFile.write(repr(slicer.modules.dicomrtimportselftest) + '\n')
    #logFile.write(repr(slicer.modules.dicomrtimport) + '\n')
    #logFile.write(repr(slicer.modules.models) + '\n')
    #logFile.close()

    self.moduleName = "DicomRtImportSelfTest"
    """ Determine data directory for the tests """
    moduleFilePath = eval('slicer.modules.%s.path' % self.moduleName.lower())
    moduleDir = os.path.dirname(moduleFilePath)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()

    self.test_DicomRtImportSelfTest_FullTest1()

  def test_DicomRtImportSelfTest_FullTest1(self):
    # Check for DicomRtImport module
    self.assertTrue( slicer.modules.dicomrtimport )

    self.TestSection_0RetrieveInputData()
    self.TestSection_1OpenDatabase()
    self.TestSection_2ImportStudy()
    self.TestSection_3SelectLoadables()
    self.TestSection_4LoadIntoSlicer()
    self.TestSection_5SaveScene()
    self.TestSection_6ClearDatabase()


  def TestSection_0RetrieveInputData(self):
    if os.access(DicomRtImportSelfTestPaths.dataDir, os.F_OK):
      self.dataDir = DicomRtImportSelfTestPaths.dataDir
      self.dicomDatabaseDir = DicomRtImportSelfTestPaths.dicomDatabaseDir
      self.tempDir = DicomRtImportSelfTestPaths.tempDir
    else:
      import urllib

      dicomRtImportSelfTestDir = slicer.app.temporaryPath + '/DicomRtImportSelfTest'
      if not os.access(dicomRtImportSelfTestDir, os.F_OK):
        os.mkdir(dicomRtImportSelfTestDir)
      self.dataDir = dicomRtImportSelfTestDir + '/EclipseProstatePhantomRtData'
      if not os.access(self.dataDir, os.F_OK):
        os.mkdir(self.dataDir)
      self.dicomDatabaseDir = dicomRtImportSelfTestDir + '/CtkDicomDatabase'
      self.tempDir = dicomRtImportSelfTestDir + '/Temp'

      downloads = (
          ('http://slicer.kitware.com/midas3/download?items=10613', 'RD.1.2.246.352.71.7.2088656855.452083.20110920153746.dcm'),
          ('http://slicer.kitware.com/midas3/download?items=10614', 'RP.1.2.246.352.71.5.2088656855.377401.20110920153647.dcm'),
          ('http://slicer.kitware.com/midas3/download?items=10615', 'RS.1.2.246.352.71.4.2088656855.2404649.20110920153449.dcm')
          )

      for url,name in downloads:
        filePath = self.dataDir + '/' + name
        if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
          print('Requesting download %s from %s...\n' % (name, url))
          urllib.urlretrieve(url, filePath)
      self.delayDisplay('Finished with download and loading\n')

  def TestSection_1OpenDatabase(self):
    self.delayDisplay("1: Open database",self.delayMs)

    # Open test database and empty it
    databaseFileName = 'ctkDICOM.sql'

    if not os.access(self.dicomDatabaseDir, os.F_OK):
      os.mkdir(self.dicomDatabaseDir)

    dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
    dicomWidget.onDatabaseDirectoryChanged(self.dicomDatabaseDir)
    self.assertTrue( slicer.dicomDatabase.isOpen )

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

  def TestSection_2ImportStudy(self):
    self.delayDisplay("2: Import study",self.delayMs)

    indexer = ctk.ctkDICOMIndexer()
    self.assertTrue( indexer )

    # Import study to database
    indexer.addDirectory( slicer.dicomDatabase, self.dataDir )
    indexer.waitForImportFinished()

    self.assertTrue( len(slicer.dicomDatabase.patients()) == 1 )
    self.assertTrue( slicer.dicomDatabase.patients()[0] )

  def TestSection_3SelectLoadables(self):
    self.delayDisplay("3: Select loadables",self.delayMs)

    # Choose first patient from the patient list
    detailsPopup = slicer.modules.dicom.widgetRepresentation().self().detailsPopup
    detailsPopup.offerLoadables( slicer.dicomDatabase.patients()[0], "Patient" )

    loadables = detailsPopup.loadableTable.loadables
    self.assertTrue( len(loadables) == 4 )

    # Make sure the loadables are good (RT is assigned to 3 out of 4 and they are selected)
    loadablesByPlugin = detailsPopup.loadablesByPlugin
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
    self.assertTrue( loadablesForRt == 3 )

  def TestSection_4LoadIntoSlicer(self):
    self.delayDisplay("4: Load into Slicer",self.delayMs)

    detailsPopup = slicer.modules.dicom.widgetRepresentation().self().detailsPopup
    detailsPopup.loadCheckedLoadables()

    # Verify that the correct number of objects were loaded
    scene = slicer.mrmlScene
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == 1 )
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') ) == 7 )
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLContourNode*') ) == 6 )
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLContourHierarchyNode*') ) == 7 )
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLAnnotationFiducialNode*') ) == 5 )

  def TestSection_5SaveScene(self):
    self.delayDisplay("5: Save scene",self.delayMs)

    if not os.access(self.tempDir, os.F_OK):
      os.mkdir(self.tempDir)

    sceneFileName = self.tempDir + '/DicomRtImportTestScene.mrml'
    if os.access(sceneFileName, os.F_OK):
      os.remove(sceneFileName)

    # Save MRML scene into file
    slicer.mrmlScene.Commit(sceneFileName)

    readable = os.access(sceneFileName, os.R_OK)
    self.assertTrue( readable )

  def TestSection_6ClearDatabase(self):
    self.delayDisplay("6: Clear database",self.delayMs)

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )
