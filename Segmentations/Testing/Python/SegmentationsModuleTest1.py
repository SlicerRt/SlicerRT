import os
import unittest
from __main__ import vtk, qt, ctk, slicer
import logging

import vtkSegmentationCore
from vtkSlicerSegmentationsModuleMRML import *
from vtkSlicerSegmentationsModuleLogic import *

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
    self.inputSegmentationNode = None
    self.secondSegmentationNode = None
    self.sphereSegment = None
    self.sphereSegmentName = 'Sphere'
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
    self.inputSegmentationNode = slicer.util.getNode('vtkMRMLSegmentationNode1')
    self.assertIsNotNone(self.inputSegmentationNode)
    
    # Change master representation to closed surface (so that conversion is possible when adding segment)
    self.inputSegmentationNode.GetSegmentation().SetMasterRepresentationName(self.closedSurfaceReprName)

  #------------------------------------------------------------------------------
  def TestSection_1_AddRemoveSegment(self):
    # Add/remove segment from segmentation (check display properties, color table, etc.)
    logging.info('Test section 1: Add/remove segment')

    # Get baseline values
    displayNode = self.inputSegmentationNode.GetDisplayNode()
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
    self.sphereSegment.SetName(self.sphereSegmentName)
    self.sphereSegment.SetDefaultColor(0.0,0.0,1.0)
    self.sphereSegment.AddRepresentation(self.closedSurfaceReprName, spherePolyData)

    # Add segment to segmentation
    self.inputSegmentationNode.GetSegmentation().AddSegment(self.sphereSegment)
    self.assertEqual(self.inputSegmentationNode.GetSegmentation().GetNumberOfSegments(), 3)
    self.assertEqual(colorTableNode.GetNumberOfColors(), 5)
    sphereColor = displayNode.GetSegmentColor(self.sphereSegmentName)
    self.assertTrue(sphereColor[0] == 0.0 and sphereColor[1] == 0.0 and sphereColor[2] == 1.0)
    
    # Check merged labelmap
    imageStat = vtk.vtkImageAccumulate()
    imageStat.SetInputData(self.inputSegmentationNode.GetImageData())
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

    # Remove segment from segmentation
    self.inputSegmentationNode.GetSegmentation().RemoveSegment(self.sphereSegmentName)
    self.assertEqual(self.inputSegmentationNode.GetSegmentation().GetNumberOfSegments(), 2)
    self.assertEqual(colorTableNode.GetNumberOfColors(), 5)
    sphereColorArray = [0]*4
    colorTableNode.GetColor(4,sphereColorArray)
    self.assertTrue(int(sphereColorArray[0]*100) == 50 and int(sphereColorArray[1]*100) == 50 and int(sphereColorArray[2]*100) == 50)
    sphereColor = displayNode.GetSegmentColor(self.sphereSegmentName)
    self.assertTrue(sphereColor[0] == 0.5 and sphereColor[1] == 0.5 and sphereColor[2] == 0.5)

  #------------------------------------------------------------------------------
  def TestSection_2_MergeLabelmapWithDifferentGeometries(self):
    # Merge labelmap when segments containing labelmaps with different geometries (both same directions, different directions)
    logging.info('Test section 2: Merge labelmap with different geometries')

    self.assertIsNotNone(self.sphereSegment)
    self.sphereSegment.RemoveRepresentation(self.binaryLabelmapReprName)
    self.assertIsNone(self.sphereSegment.GetRepresentation(self.binaryLabelmapReprName))

    # Create new segmentation with sphere segment
    self.secondSegmentationNode = vtkMRMLSegmentationNode()
    self.secondSegmentationNode.SetName('Second')
    self.secondSegmentationNode.GetSegmentation().SetMasterRepresentationName(self.binaryLabelmapReprName)
    slicer.mrmlScene.AddNode(self.secondSegmentationNode)

    self.secondSegmentationNode.GetSegmentation().AddSegment(self.sphereSegment)

    # Check automatically converted labelmap. It is supposed to have the default geometry
    # (which is different than the one in the input segmentation)
    sphereLabelmap = self.sphereSegment.GetRepresentation(self.binaryLabelmapReprName)
    self.assertIsNotNone(sphereLabelmap)
    sphereLabelmapSpacing = sphereLabelmap.GetSpacing()
    self.assertTrue(sphereLabelmapSpacing[0] == 1.0 and sphereLabelmapSpacing[1] == 1.0 and sphereLabelmapSpacing[2] == 1.0)

    # Copy segment to input segmentation
    self.inputSegmentationNode.GetSegmentation().CopySegmentFromSegmentation(self.secondSegmentationNode.GetSegmentation(), self.sphereSegmentName)
    self.assertEqual(self.inputSegmentationNode.GetSegmentation().GetNumberOfSegments(), 3)

    # Check merged labelmap
    mergedLabelmap = self.inputSegmentationNode.GetImageData()
    self.assertIsNotNone(mergedLabelmap)
    mergedLabelmapSpacing = sphereLabelmap.GetSpacing()
    self.assertTrue(mergedLabelmapSpacing[0] == 1.0 and mergedLabelmapSpacing[1] == 1.0 and mergedLabelmapSpacing[2] == 1.0)

    imageStat = vtk.vtkImageAccumulate()
    imageStat.SetInputData(mergedLabelmap)
    imageStat.SetComponentExtent(0,5,0,0,0,0)
    imageStat.SetComponentOrigin(0,0,0)
    imageStat.SetComponentSpacing(1,1,1)
    imageStat.Update()
    self.assertEqual(imageStat.GetVoxelCount(), 54872000)
    imageStatResult = imageStat.GetOutput()
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(0,0,0,0), 46678738)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(1,0,0,0), 0)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(2,0,0,0), 7618805)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(3,0,0,0), 128968)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(4,0,0,0), 0) # Built from color table and color four is removed in previous test section
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(5,0,0,0), 445489)

  #------------------------------------------------------------------------------
  def TestSection_3_ImportExportSegment(self):
    # Import/export, both one label and all labels
    logging.info('Test section 3: Import/export segment')
    
    # Export single segment to model node
    bodyModelNode = slicer.vtkMRMLModelNode()
    bodyModelNode.SetName('BodyModel')
    slicer.mrmlScene.AddNode(bodyModelNode)
    
    bodySegment = self.inputSegmentationNode.GetSegmentation().GetSegment('Body_Contour')
    result = vtkSlicerSegmentationsModuleLogic.ExportSegmentToRepresentationNode(bodySegment, bodyModelNode)
    self.assertTrue(result)
    self.assertIsNotNone(bodyModelNode.GetPolyData())
    self.assertEqual(bodyModelNode.GetPolyData().GetNumberOfPoints(), 302)
    #TODO: On Linux and Windows it is 588, on Mac it is 580. Need to investigate
    # self.assertEqual(bodyModelNode.GetPolyData().GetNumberOfCells(), 588)
    self.assertTrue(bodyModelNode.GetPolyData().GetNumberOfCells() == 588 or bodyModelNode.GetPolyData().GetNumberOfCells() == 580)

    # Export single segment to volume node
    bodyLabelmapNode = slicer.vtkMRMLLabelMapVolumeNode()
    bodyLabelmapNode.SetName('BodyLabelmap')
    slicer.mrmlScene.AddNode(bodyLabelmapNode)
    result = vtkSlicerSegmentationsModuleLogic.ExportSegmentToRepresentationNode(bodySegment, bodyLabelmapNode)
    self.assertTrue(result)
    bodyImageData = bodyLabelmapNode.GetImageData()
    self.assertIsNotNone(bodyImageData)
    imageStat = vtk.vtkImageAccumulate()
    imageStat.SetInputData(bodyImageData)
    imageStat.Update()
    self.assertEqual(imageStat.GetVoxelCount(), 648)
    self.assertEqual(imageStat.GetMin()[0], 0)
    self.assertEqual(imageStat.GetMax()[0], 1)

    # Export multiple segments to volume node
    allSegmentsLabelmapNode = slicer.vtkMRMLLabelMapVolumeNode()
    allSegmentsLabelmapNode.SetName('AllSegmentsLabelmap')
    slicer.mrmlScene.AddNode(allSegmentsLabelmapNode)
    result = vtkSlicerSegmentationsModuleLogic.ExportAllSegmentsToLabelmapNode(self.inputSegmentationNode, allSegmentsLabelmapNode)
    self.assertTrue(result)
    allSegmentsImageData = allSegmentsLabelmapNode.GetImageData()
    self.assertIsNotNone(allSegmentsImageData)
    imageStat = vtk.vtkImageAccumulate()
    imageStat.SetInputData(allSegmentsImageData)
    imageStat.SetComponentExtent(0,5,0,0,0,0)
    imageStat.SetComponentOrigin(0,0,0)
    imageStat.SetComponentSpacing(1,1,1)
    imageStat.Update()
    self.assertEqual(imageStat.GetVoxelCount(), 54872000)
    imageStatResult = imageStat.GetOutput()
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(0,0,0,0), 46678738)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(1,0,0,0), 0)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(2,0,0,0), 7618805)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(3,0,0,0), 128968)
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(4,0,0,0), 0) # Built from color table and color four is removed in previous test section
    self.assertEqual(imageStatResult.GetScalarComponentAsDouble(5,0,0,0), 445489)

    # Import model to segment
    modelImportSegmentationNode = vtkMRMLSegmentationNode()
    modelImportSegmentationNode.SetName('ModelImport')
    modelImportSegmentationNode.GetSegmentation().SetMasterRepresentationName(self.closedSurfaceReprName)
    slicer.mrmlScene.AddNode(modelImportSegmentationNode)
    modelSegment = vtkSlicerSegmentationsModuleLogic.CreateSegmentFromModelNode(bodyModelNode)
    modelSegment.UnRegister(None) # Need to release ownership
    self.assertIsNotNone(modelSegment)
    self.assertIsNotNone(modelSegment.GetRepresentation(self.closedSurfaceReprName))

    # Import multi-label labelmap to segmentation
    multiLabelImportSegmentationNode = vtkMRMLSegmentationNode()
    multiLabelImportSegmentationNode.SetName('MultiLabelImport')
    multiLabelImportSegmentationNode.GetSegmentation().SetMasterRepresentationName(self.binaryLabelmapReprName)
    slicer.mrmlScene.AddNode(multiLabelImportSegmentationNode)
    result = vtkSlicerSegmentationsModuleLogic.ImportLabelmapToSegmentationNode(allSegmentsLabelmapNode, multiLabelImportSegmentationNode)
    self.assertTrue(result)
    self.assertEqual(multiLabelImportSegmentationNode.GetSegmentation().GetNumberOfSegments(), 3)

    # Import labelmap into single segment
    singleLabelImportSegmentationNode = vtkMRMLSegmentationNode()
    singleLabelImportSegmentationNode.SetName('SingleLabelImport')
    singleLabelImportSegmentationNode.GetSegmentation().SetMasterRepresentationName(self.binaryLabelmapReprName)
    slicer.mrmlScene.AddNode(singleLabelImportSegmentationNode)
    # Should not import multi-label labelmap to segment
    nullSegment = vtkSlicerSegmentationsModuleLogic.CreateSegmentFromLabelmapVolumeNode(allSegmentsLabelmapNode)
    self.assertIsNone(nullSegment)
    logging.info('(This error message tests an impossible scenario, it is supposed to appear)')
    # Make labelmap single-label and import again
    threshold = vtk.vtkImageThreshold()
    threshold.ThresholdByUpper(0.5)
    threshold.SetInValue(1)
    threshold.SetOutValue(0)
    threshold.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
    if vtk.VTK_MAJOR_VERSION <= 5:
      threshold.SetInput(allSegmentsLabelmapNode.GetImageData())
    else:
      threshold.SetInputData(allSegmentsLabelmapNode.GetImageData())
    threshold.SetOutput(allSegmentsLabelmapNode.GetImageData())
    threshold.Update()
    labelSegment = vtkSlicerSegmentationsModuleLogic.CreateSegmentFromLabelmapVolumeNode(allSegmentsLabelmapNode)
    labelSegment.UnRegister(None) # Need to release ownership
    self.assertIsNotNone(labelSegment)
    self.assertIsNotNone(labelSegment.GetRepresentation(self.binaryLabelmapReprName))

    # Import/export with transforms
    logging.info('Test section 4/2: Import/export with transforms')

    # Create transform node that will be used to transform the tested nodes
    bodyModelTransformNode = slicer.vtkMRMLLinearTransformNode()
    slicer.mrmlScene.AddNode(bodyModelTransformNode)
    bodyModelTransform = vtk.vtkTransform()
    bodyModelTransform.Translate(1000.0, 0.0, 0.0)
    bodyModelTransformNode.ApplyTransformMatrix(bodyModelTransform.GetMatrix())
    
    # Set transform as parent to input segmentation node
    self.inputSegmentationNode.SetAndObserveTransformNodeID(bodyModelTransformNode.GetID())

    # Export single segment to model node from transformed segmentation
    bodyModelNodeTransformed = slicer.vtkMRMLModelNode()
    bodyModelNodeTransformed.SetName('BodyModelTransformed')
    slicer.mrmlScene.AddNode(bodyModelNodeTransformed)
    bodySegment = self.inputSegmentationNode.GetSegmentation().GetSegment('Body_Contour')
    result = vtkSlicerSegmentationsModuleLogic.ExportSegmentToRepresentationNode(bodySegment, bodyModelNodeTransformed)
    self.assertTrue(result)
    self.assertIsNotNone(bodyModelNodeTransformed.GetParentTransformNode())

    # Export single segment to volume node from transformed segmentation
    bodyLabelmapNodeTransformed = slicer.vtkMRMLLabelMapVolumeNode()
    bodyLabelmapNodeTransformed.SetName('BodyLabelmapTransformed')
    slicer.mrmlScene.AddNode(bodyLabelmapNodeTransformed)
    result = vtkSlicerSegmentationsModuleLogic.ExportSegmentToRepresentationNode(bodySegment, bodyLabelmapNodeTransformed)
    self.assertTrue(result)
    self.assertIsNotNone(bodyLabelmapNodeTransformed.GetParentTransformNode())

    # Create transform node that will be used to transform the tested nodes
    modelTransformedImportSegmentationTransformNode = slicer.vtkMRMLLinearTransformNode()
    slicer.mrmlScene.AddNode(modelTransformedImportSegmentationTransformNode)
    modelTransformedImportSegmentationTransform = vtk.vtkTransform()
    modelTransformedImportSegmentationTransform.Translate(-500.0, 0.0, 0.0)
    modelTransformedImportSegmentationTransformNode.ApplyTransformMatrix(modelTransformedImportSegmentationTransform.GetMatrix())

    # Import transformed model to segment in transformed segmentation
    modelTransformedImportSegmentationNode = vtkMRMLSegmentationNode()
    modelTransformedImportSegmentationNode.SetName('ModelImportTransformed')
    modelTransformedImportSegmentationNode.GetSegmentation().SetMasterRepresentationName(self.closedSurfaceReprName)
    slicer.mrmlScene.AddNode(modelTransformedImportSegmentationNode)
    modelTransformedImportSegmentationNode.SetAndObserveTransformNodeID(modelTransformedImportSegmentationTransformNode.GetID())
    modelSegmentTranformed = vtkSlicerSegmentationsModuleLogic.CreateSegmentFromModelNode(bodyModelNodeTransformed, modelTransformedImportSegmentationNode)
    modelSegmentTranformed.UnRegister(None) # Need to release ownership
    self.assertIsNotNone(modelSegmentTranformed)
    modelSegmentTransformedPolyData = modelSegmentTranformed.GetRepresentation(self.closedSurfaceReprName)
    self.assertIsNotNone(modelSegmentTransformedPolyData)
    self.assertEqual(int(modelSegmentTransformedPolyData.GetBounds()[0]), 1332)
    self.assertEqual(int(modelSegmentTransformedPolyData.GetBounds()[1]), 1675)

    # Clean up temporary nodes
    slicer.mrmlScene.RemoveNode(bodyModelNode)
    slicer.mrmlScene.RemoveNode(bodyLabelmapNode)
    slicer.mrmlScene.RemoveNode(allSegmentsLabelmapNode)
    slicer.mrmlScene.RemoveNode(modelImportSegmentationNode)
    slicer.mrmlScene.RemoveNode(multiLabelImportSegmentationNode)
    slicer.mrmlScene.RemoveNode(singleLabelImportSegmentationNode)
    slicer.mrmlScene.RemoveNode(bodyModelTransformNode)
    slicer.mrmlScene.RemoveNode(bodyModelNodeTransformed)
    slicer.mrmlScene.RemoveNode(bodyLabelmapNodeTransformed)
    slicer.mrmlScene.RemoveNode(modelTransformedImportSegmentationNode)

  #------------------------------------------------------------------------------
  def TestSection_Z_ClearDatabase(self):
    # Clear temporary database and restore original one
    slicer.dicomDatabase.initializeDatabase()
    slicer.dicomDatabase.closeDatabase()
    self.assertFalse(slicer.dicomDatabase.isOpen)
    if self.originalDatabaseDirectory:
      self.dicomWidget.onDatabaseDirectoryChanged(self.originalDatabaseDirectory)

    logging.info('Test finished')
