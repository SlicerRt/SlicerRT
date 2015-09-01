import os
import unittest
from __main__ import vtk, qt, ctk, slicer
import logging
import vtkSegmentationCore

class SegmentationsModuleTest1(unittest.TestCase):
  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_SegmentationsModuleTest1()

  #------------------------------------------------------------------------------
  def test_SegmentationsModuleTest1(self):
    # Check for modules
    self.assertIsNotNone( slicer.modules.dicomrtimportexport )
    self.assertIsNotNone( slicer.modules.segmentations )

    self.TestSection_00_SetupPathsAndNames()
    self.TestSection_01_RetrieveInputData()
    self.TestSection_02_LoadInputData()
    self.TestSection_1_AddRemoveSegment()
    self.TestSection_2_MergeLabelmapWithDifferentGeometries()
    self.TestSection_3_ImportExportSegment()
    self.TestSection_4_ConversionWithTransforms()
    self.TestSection_Z_ClearDatabase()

  #------------------------------------------------------------------------------
  def TestSection_00_SetupPathsAndNames(self):
    # Set up paths used for this test
    segmentationsModuleTestDir = slicer.app.temporaryPath + '/SegmentationsModuleTest'
    if not os.access(segmentationsModuleTestDir, os.F_OK):
      os.mkdir(segmentationsModuleTestDir)

    self.dicomDataDir = segmentationsModuleTestDir + '/TinyRtStudy'
    if not os.access(self.dicomDataDir, os.F_OK):
      os.mkdir(self.dicomDataDir)
      
    self.testDicomDatabaseDir = segmentationsModuleTestDir + '/CtkDicomDatabase'
    self.dicomZipFilePath = segmentationsModuleTestDir + '/TinyRtStudy.zip'
    
    # Get slicer objects that are used throughout the test
    self.dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
    
    # Define variables
    self.originalDatabaseDirectory = None
    self.expectedNumOfFilesInDicomDataDir = 12
    self.segmentationNode = None
    self.sphereSegment = None
    self.closedSurfaceReprName = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationClosedSurfaceRepresentationName()
    self.binaryLabelmapReprName = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName()

    # Switch to temporary DICOM database
    try:
      if not os.access(self.testDicomDatabaseDir, os.F_OK):
        os.mkdir(self.testDicomDatabaseDir)
      if slicer.dicomDatabase:
        self.originalDatabaseDirectory = os.path.split(slicer.dicomDatabase.databaseFilename)[0]
      else:
        settings = qt.QSettings()
        settings.setValue('DatabaseDirectory', self.testDicomDatabaseDir)
      self.dicomWidget.onDatabaseDirectoryChanged(self.testDicomDatabaseDir)
      self.assertTrue(slicer.dicomDatabase.isOpen)
      slicer.dicomDatabase.initializeDatabase()
    except Exception, e:
      import traceback
      traceback.print_exc()
      logging.error('Failed to open temporary DICOM database')

  #------------------------------------------------------------------------------
  def TestSection_01_RetrieveInputData(self):
    try:
      import urllib
      downloads = (
          ('http://slicer.kitware.com/midas3/download/folder/2822/TinyRtStudy.zip', self.dicomZipFilePath),
          )

      downloaded = 0
      for url,filePath in downloads:
        if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
          if downloaded == 0:
            logging.info('Downloading input data to folder\n' + self.dicomZipFilePath)
          logging.info('Requesting download from %s...' % (url))
          urllib.urlretrieve(url, filePath)
          downloaded += 1
        else:
          logging.info('Input data has been found in folder ' + self.dicomZipFilePath)
      if downloaded > 0:
        logging.info('Downloading input data finished')

      numOfFilesInDicomDataDir = len([name for name in os.listdir(self.dicomDataDir) if os.path.isfile(self.dicomDataDir + '/' + name)])
      if (numOfFilesInDicomDataDir != self.expectedNumOfFilesInDicomDataDir):
        slicer.app.applicationLogic().Unzip(self.dicomZipFilePath, self.dicomDataDir)
        logging.info("Unzipping done")

      numOfFilesInDicomDataDirTest = len([name for name in os.listdir(self.dicomDataDir) if os.path.isfile(self.dicomDataDir + '/' + name)])
      self.assertEqual( numOfFilesInDicomDataDirTest, self.expectedNumOfFilesInDicomDataDir )

    except Exception, e:
      import traceback
      traceback.print_exc()
      logging.error('Test caused exception!\n' + str(e))

  #------------------------------------------------------------------------------
  def TestSection_02_LoadInputData(self):
    indexer = ctk.ctkDICOMIndexer()
    self.assertIsNotNone( indexer )

    # Import study to database
    indexer.addDirectory( slicer.dicomDatabase, self.dicomDataDir )
    indexer.waitForImportFinished()

    self.assertEqual( len(slicer.dicomDatabase.patients()), 1 )
    self.assertIsNotNone( slicer.dicomDatabase.patients()[0] )

    # Choose first patient from the patient list
    patient = slicer.dicomDatabase.patients()[0]
    studies = slicer.dicomDatabase.studiesForPatient(patient)
    series = [slicer.dicomDatabase.seriesForStudy(study) for study in studies]
    seriesUIDs = [uid for uidList in series for uid in uidList]
    self.dicomWidget.detailsPopup.offerLoadables(seriesUIDs, 'SeriesUIDList')
    self.dicomWidget.detailsPopup.examineForLoading()

    loadables = self.dicomWidget.detailsPopup.loadableTable.loadables
    self.assertTrue( len(loadables) > 0 )
    
    # Load into Slicer
    self.dicomWidget.detailsPopup.loadCheckedLoadables()
    self.segmentationNode = slicer.util.getNode('vtkMRMLSegmentationNode1')
    self.assertIsNotNone(self.segmentationNode)
    
    # Change master representation to closed surface (so that conversion is possible when adding segment)
    self.segmentationNode.GetSegmentation().SetMasterRepresentationName(self.closedSurfaceReprName)

  #------------------------------------------------------------------------------
  def TestSection_1_AddRemoveSegment(self):
    # Add/remove segment from segmentation (check display properties, color table, etc.)
    logging.info('Test section 1: Add/remove segment')

    # Get baseline values
    displayNode = self.segmentationNode.GetDisplayNode()
    self.assertIsNotNone(displayNode)
    colorTableNode = displayNode.GetColorNode()
    self.assertIsNotNone(colorTableNode)
    self.assertEqual(colorTableNode.GetNumberOfColors(), 4)
    # If segments are not found then the returned color is the pre-defined invalid color
    bodyColor = displayNode.GetSegmentColor('Body_Contour')
    self.assertTrue(int(bodyColor[0]*100) == 33 and int(bodyColor[1]*100) == 66 and bodyColor[2] == 0.0)
    tumorColor = displayNode.GetSegmentColor('Tumor_Contour')
    self.assertTrue(tumorColor[0] == 1.0 and tumorColor[1] == 0.0 and tumorColor[2] == 0.0)

    # Create new segment
    sphere = vtk.vtkSphereSource()
    sphere.SetCenter(0,50,0)
    sphere.SetRadius(50)
    sphere.Update()
    spherePolyData = vtk.vtkPolyData()
    spherePolyData.DeepCopy(sphere.GetOutput())
    
    self.sphereSegment = vtkSegmentationCore.vtkSegment()
    self.sphereSegment.SetName('sphere1')
    self.sphereSegment.SetDefaultColor(0.0,0.0,1.0)
    self.sphereSegment.AddRepresentation(self.closedSurfaceReprName, spherePolyData);

    # Add segment to segmentation
    self.segmentationNode.GetSegmentation().AddSegment(self.sphereSegment)
    self.assertEqual(self.segmentationNode.GetSegmentation().GetNumberOfSegments(), 3)
    self.assertEqual(colorTableNode.GetNumberOfColors(), 5)
    sphereColor = displayNode.GetSegmentColor('sphere1')
    self.assertTrue(sphereColor[0] == 0.0 and sphereColor[1] == 0.0 and sphereColor[2] == 1.0)
    
    # Check merged labelmap
    imageStat = vtk.vtkImageAccumulate()
    imageStat.SetInputData(self.segmentationNode.GetImageData())
    imageStat.SetComponentExtent(0,4,0,0,0,0)
    imageStat.SetComponentOrigin(0,0,0)
    imageStat.SetComponentSpacing(1,1,1)
    imageStat.Update()
    self.assertEqual(imageStat.GetVoxelCount(), 1000)
    imageStatResult = imageStat.GetOutput()
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(0,0,0,0), 814)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(1,0,0,0), 0)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(2,0,0,0), 175)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(3,0,0,0), 4)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(4,0,0,0), 7)

    self.segmentationNode.GetSegmentation().RemoveSegment('sphere1')
    self.assertEqual(self.segmentationNode.GetSegmentation().GetNumberOfSegments(), 2)
    self.assertEqual(colorTableNode.GetNumberOfColors(), 5)
    sphereColorArray = [0]*4
    colorTableNode.GetColor(4,sphereColorArray)
    print('ZZZ sphereColorArray: ' + repr(sphereColorArray))
    self.assertTrue(int(sphereColorArray[0]*100) == 50 and int(sphereColorArray[1]*100) == 50 and int(sphereColorArray[2]*100) == 50)
    sphereColor = displayNode.GetSegmentColor('sphere1')
    self.assertTrue(sphereColor[0] == 0.5 and sphereColor[1] == 0.5 and sphereColor[2] == 0.5)

  #------------------------------------------------------------------------------
  def TestSection_2_MergeLabelmapWithDifferentGeometries(self):
    # Merge labelmap when segments containing labelmaps with different geometries (both same directions, different directions)
    logging.info('Test section 2: Merge labelmap with different geometries')

  #------------------------------------------------------------------------------
  def TestSection_3_ImportExportSegment(self):
    # Import/export, both one label and all labels
    logging.info('Test section 3: Import/export segment')

  #------------------------------------------------------------------------------
  def TestSection_4_ConversionWithTransforms(self):
    # Conversion rules with all kinds of transformed scenarios
    logging.info('Test section 4: Conversion with transformed scenarios')

  #------------------------------------------------------------------------------
  def TestSection_Z_ClearDatabase(self):
    # Clear temporary database and restore original one
    slicer.dicomDatabase.initializeDatabase()
    slicer.dicomDatabase.closeDatabase()
    self.assertFalse(slicer.dicomDatabase.isOpen)
    if self.originalDatabaseDirectory:
      self.dicomWidget.onDatabaseDirectoryChanged(self.originalDatabaseDirectory)

    logging.info('Test finished')
