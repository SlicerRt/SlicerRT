import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

class DicomRtImportTest(unittest.TestCase):
  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

    #TODO: Comment out
    #logFile = open('d:/pyTestLog.txt', 'a')
    #logFile.write(repr(slicer.modules.DicomRtImportTest) + '\n')
    #logFile.write(repr(slicer.modules.dicomrtimportexport) + '\n')
    #logFile.close()

  #------------------------------------------------------------------------------
  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_DicomRtImportTest_FullTest1()

  #------------------------------------------------------------------------------
  def test_DicomRtImportTest_FullTest1(self):
    # Check for modules
    self.assertIsNotNone( slicer.modules.dicomrtimportexport )
    self.assertIsNotNone( slicer.modules.segmentations )
    self.assertIsNotNone( slicer.modules.dicom )

    self.dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
    self.assertIsNotNone( self.dicomWidget )

    self.TestSection_RetrieveInputData()
    self.TestSection_OpenTempDatabase()
    self.TestSection_ImportStudy()
    self.TestSection_SelectLoadables()
    self.TestSection_LoadIntoSlicer()
    self.TestSection_SaveScene()
    self.TestSection_ClearDatabase()

    logging.info("Test finished")

  #------------------------------------------------------------------------------
  def TestSection_RetrieveInputData(self):
    import urllib.request, urllib.parse, urllib.error

    dicomRtImportTestDir = slicer.app.temporaryPath + '/DicomRtImportTest'
    if not os.access(dicomRtImportTestDir, os.F_OK):
      os.mkdir(dicomRtImportTestDir)
    self.dataDir = dicomRtImportTestDir + '/EclipseProstatePhantomRtData'
    if not os.access(self.dataDir, os.F_OK):
      os.mkdir(self.dataDir)
    self.dicomDatabaseDir = dicomRtImportTestDir + '/CtkDicomDatabase'
    self.tempDir = dicomRtImportTestDir + '/Temp'

    downloads = (
        ('http://slicer.kitware.com/midas3/download?items=10613', 'RD.1.2.246.352.71.7.2088656855.452083.20110920153746.dcm'),
        ('http://slicer.kitware.com/midas3/download?items=10614', 'RP.1.2.246.352.71.5.2088656855.377401.20110920153647.dcm'),
        ('http://slicer.kitware.com/midas3/download?items=10615', 'RS.1.2.246.352.71.4.2088656855.2404649.20110920153449.dcm'),
        ('http://slicer.kitware.com/midas3/download/item/119940', 'RI.1.2.246.352.71.3.2088656855.2381134.20110921150951.dcm')
        )

    for url,name in downloads:
      filePath = self.dataDir + '/' + name
      if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
        logging.info('Requesting download %s from %s...\n' % (name, url))
        urllib.request.urlretrieve(url, filePath)

    logging.info("Finished with download test data")

  #------------------------------------------------------------------------------
  def TestSection_OpenTempDatabase(self):
    # Open test database and empty it
    if not os.access(self.dicomDatabaseDir, os.F_OK):
      os.mkdir(self.dicomDatabaseDir)

    if slicer.dicomDatabase:
      self.originalDatabaseDirectory = os.path.split(slicer.dicomDatabase.databaseFilename)[0]
    else:
      self.originalDatabaseDirectory = None
      settings = qt.QSettings()
      settings.setValue('DatabaseDirectory', self.dicomDatabaseDir)

    slicer.dicomDatabase.openDatabase(self.dicomDatabaseDir + "/ctkDICOM.sql")
    self.assertTrue( slicer.dicomDatabase.isOpen )

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

  #------------------------------------------------------------------------------
  def TestSection_ImportStudy(self):
    # slicer.util.delayDisplay("Import study",self.delayMs)
    logging.info("Import study")

    indexer = ctk.ctkDICOMIndexer()
    self.assertIsNotNone( indexer )

    # Import study to database
    indexer.addDirectory( slicer.dicomDatabase, self.dataDir )
    indexer.waitForImportFinished()

    self.assertEqual( len(slicer.dicomDatabase.patients()), 1 )
    self.assertIsNotNone( slicer.dicomDatabase.patients()[0] )

  #------------------------------------------------------------------------------
  def TestSection_SelectLoadables(self):
    # slicer.util.delayDisplay("Select loadables",self.delayMs)
    logging.info("Select loadables")

    # Choose first patient from the patient list
    patient = slicer.dicomDatabase.patients()[0]
    studies = slicer.dicomDatabase.studiesForPatient(patient)
    series = [slicer.dicomDatabase.seriesForStudy(study) for study in studies]
    seriesUIDs = [uid for uidList in series for uid in uidList]
    self.dicomWidget.browserWidget.onSeriesSelected(seriesUIDs)
    self.dicomWidget.browserWidget.examineForLoading()

    loadables = self.dicomWidget.browserWidget.loadableTable.loadables

    # Make sure the loadables are good (RT is assigned to 4 out of 8 and they are selected)
    loadablesByPlugin = self.dicomWidget.browserWidget.loadablesByPlugin
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
    self.assertEqual( loadablesForRt, 4 )

  #------------------------------------------------------------------------------
  def TestSection_LoadIntoSlicer(self):
    # slicer.util.delayDisplay("Load into Slicer",self.delayMs)
    logging.info("Load into Slicer")

    self.dicomWidget.browserWidget.loadCheckedLoadables()

    # Verify that the correct number of objects were loaded
    # Volumes: Dose, RT image
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ), 2 )
    # RT Beam nodes (static beams and dynamic beam sequences)
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLRTBeamNode*') ), 5 )
    # Segmentation: The loaded structures
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLSegmentationNode*') ), 1 )
    # Markups: the RT plan POI
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLMarkupsFiducialNode*') ), 1 )
    # Subject hierarchy items
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    self.assertEqual( shNode.GetNumberOfItems(), 28 )

  #------------------------------------------------------------------------------
  def TestSection_SaveScene(self):
    # slicer.util.delayDisplay("Save scene",self.delayMs)
    logging.info("Save scene")

    if not os.access(self.tempDir, os.F_OK):
      os.mkdir(self.tempDir)

    sceneFileName = self.tempDir + '/DicomRtImportTestScene.mrml'
    if os.access(sceneFileName, os.F_OK):
      os.remove(sceneFileName)

    # Save MRML scene into file
    slicer.mrmlScene.Commit(sceneFileName)

    readable = os.access(sceneFileName, os.R_OK)
    self.assertTrue( readable )

  #------------------------------------------------------------------------------
  def TestSection_ClearDatabase(self):
    # slicer.util.delayDisplay("Clear database",self.delayMs)
    logging.info("Clear database")

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )

    # slicer.util.delayDisplay("Restoring original database directory",self.delayMs)
    logging.info("Restoring original database directory")
    if self.originalDatabaseDirectory:
      slicer.dicomDatabase.openDatabase(self.originalDatabaseDirectory)
