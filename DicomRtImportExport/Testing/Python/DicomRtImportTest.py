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

    self.TestSection_0RetrieveInputData()
    self.TestSection_1OpenTempDatabase()
    self.TestSection_2ImportStudy()
    self.TestSection_3SelectLoadables()
    self.TestSection_4LoadIntoSlicer()
    self.TestSection_5SaveScene()
    self.TestSection_6ClearDatabase()

    logging.info("Test finished")

  #------------------------------------------------------------------------------
  def TestSection_0RetrieveInputData(self):
    import urllib

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
        urllib.urlretrieve(url, filePath)
    # slicer.util.delayDisplay("Finished with download test data",self.delayMs)
    logging.info("Finished with download test data")

  #------------------------------------------------------------------------------
  def TestSection_1OpenTempDatabase(self):
    # Open test database and empty it
    if not os.access(self.dicomDatabaseDir, os.F_OK):
      os.mkdir(self.dicomDatabaseDir)

    if slicer.dicomDatabase:
      self.originalDatabaseDirectory = os.path.split(slicer.dicomDatabase.databaseFilename)[0]
    else:
      self.originalDatabaseDirectory = None
      settings = qt.QSettings()
      settings.setValue('DatabaseDirectory', self.dicomDatabaseDir)
      
    self.dicomWidget.onDatabaseDirectoryChanged(self.dicomDatabaseDir)
    self.assertTrue( slicer.dicomDatabase.isOpen )

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

  #------------------------------------------------------------------------------
  def TestSection_2ImportStudy(self):
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
  def TestSection_3SelectLoadables(self):
    # slicer.util.delayDisplay("Select loadables",self.delayMs)
    logging.info("Select loadables")

    # Choose first patient from the patient list
    patient = slicer.dicomDatabase.patients()[0]
    studies = slicer.dicomDatabase.studiesForPatient(patient)
    series = [slicer.dicomDatabase.seriesForStudy(study) for study in studies]
    seriesUIDs = [uid for uidList in series for uid in uidList]
    self.dicomWidget.detailsPopup.offerLoadables(seriesUIDs, 'SeriesUIDList')
    self.dicomWidget.detailsPopup.examineForLoading()

    loadables = self.dicomWidget.detailsPopup.loadableTable.loadables
    self.assertEqual( len(loadables), 8 )

    # Make sure the loadables are good (RT is assigned to 4 out of 8 and they are selected)
    loadablesByPlugin = self.dicomWidget.detailsPopup.loadablesByPlugin
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
  def TestSection_4LoadIntoSlicer(self):
    # slicer.util.delayDisplay("Load into Slicer",self.delayMs)
    logging.info("Load into Slicer")

    self.dicomWidget.detailsPopup.loadCheckedLoadables()

    # Verify that the correct number of objects were loaded
    # Volumes: Dose, RT image
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ), 2 )
    # Model hierarchies: Beam models (parent + individual beams)
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') ), 6 )
    # Subject hierarchy nodes: Patient, Study, Dose, RT beams, RT image, structure set segmentation and segment virtual nodes.
    # Plus SH nodes automatically created for and displayed model and beam transforms
    # If subject hierarchy auto creation is off, then 2 less nodes are created (the RT image plane model and texture volume)
    autoCreateSh = slicer.modules.subjecthierarchy.widgetRepresentation().pluginLogic().autoCreateSubjectHierarchy
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLSubjectHierarchyNode*') ), 18 + 6*autoCreateSh )
    # Segmentation: The loaded structures
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLSegmentationNode*') ), 1 )
    # Markups: the RT plan POI
    self.assertEqual( len( slicer.util.getNodes('vtkMRMLMarkupsFiducialNode*') ), 1 )

  #------------------------------------------------------------------------------
  def TestSection_5SaveScene(self):
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
  def TestSection_6ClearDatabase(self):
    # slicer.util.delayDisplay("Clear database",self.delayMs)
    logging.info("Clear database")

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )

    # slicer.util.delayDisplay("Restoring original database directory",self.delayMs)
    logging.info("Restoring original database directory")
    if self.originalDatabaseDirectory:
      self.dicomWidget.onDatabaseDirectoryChanged(self.originalDatabaseDirectory)
