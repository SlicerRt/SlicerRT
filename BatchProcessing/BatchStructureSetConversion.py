from __future__ import absolute_import, division, print_function # Makes moving python2 to python3 much easier and ensures that nasty bugs involving integer division don't creep in
import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import argparse
import sys
import logging
from DICOMLib import DICOMUtils

slicer.exit_when_finished = True

# ------------------------------------------------------------------------------
# BatchStructureSetConversion
#   Convert structures in structure set to labelmaps and save them to disk
#
class BatchStructureSetConversion(ScriptedLoadableModule):
    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        parent.title = "Batch Structure Set Conversion"
        parent.categories = ["Testing.SlicerRT Tests"]
        parent.dependencies = ["DicomRtImportExport", "Segmentations"]
        parent.contributors = ["Csaba Pinter (Queen's)"]
        parent.helpText = """
    This is a module for converting DICOM structure set to labelmaps and saving them to disk.
    """
        parent.acknowledgementText = """This file was originally developed by Csaba Pinter, PerkLab, Queen's University and was supported through the Applied Cancer Research Unit program of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care"""  # replace with organization, grant and thanks.
        self.parent = parent

        # Add this test to the SelfTest module's list for discovery when the module
        # is created.  Since this module may be discovered before SelfTests itself,
        # create the list if it doesn't already exist.
        try:
            slicer.selfTests
        except AttributeError:
            slicer.selfTests = {}
        slicer.selfTests['BatchStructureSetConversion'] = self.runTest

    def runTest(self, msec=100, **kwargs):
        tester = BatchStructureSetConversionTest()
        tester.runTest()


# ------------------------------------------------------------------------------
# BatchStructureSetConversionWidget
#
class BatchStructureSetConversionWidget(ScriptedLoadableModuleWidget):
    def setup(self):
        self.developerMode = True
        ScriptedLoadableModuleWidget.setup(self)


# ------------------------------------------------------------------------------
# BatchStructureSetConversionLogic
#
class BatchStructureSetConversionLogic(ScriptedLoadableModuleLogic):
    """This class should implement all the actual
    computation done by your module.  The interface
    should be such that other python code can import
    this class and make use of the functionality without
    requiring an instance of the Widget
    """

    def __init__(self):
      ScriptedLoadableModuleLogic.__init__(self)

      self.dataDir = slicer.app.temporaryPath + '/BatchStructureSetConversion'
      if not os.access(self.dataDir, os.F_OK):
        os.mkdir(self.dataDir)

    def LoadFirstPatientIntoSlicer(self):
      # Choose first non-empty patient from the patient list
      all_patients = slicer.dicomDatabase.patients()
      for patient in all_patients:
        try:
          DICOMUtils.loadPatientByUID(patient)
          return
        except OSError as e:
          # Failed to load patient, probably no studies are associated with it
          pass

    def ConvertStructureSetToLabelmap(self, use_ref_image, ref_image_node_id=None):
      import vtkSegmentationCorePython as vtkSegmentationCore

      labelmapsToSave = []

      # Get reference image volume node
      referenceVolume = None
      if ref_image_node_id is not None:
        try:
          referenceVolume = slicer.util.getNode(ref_image_node_id)
        except slicer.util.MRMLNodeNotFoundException:
          logging.error('Failed to get reference image with ID ' + str(ref_image_node_id) + '. Using image referenced by DICOM')

      # Get all segmentation nodes from the scene
      segmentationNodes = slicer.util.getNodesByClass('vtkMRMLSegmentationNode')

      for segmentationNode in segmentationNodes:
        logging.info(f'  Converting structure set {segmentationNode.GetName()} to labelmap')
        # Set referenced volume as rasterization reference from DICOM if not explicitly specified
        if referenceVolume is None and use_ref_image == True:
          referenceVolume = slicer.vtkSlicerDicomRtImportExportModuleLogic.GetReferencedVolumeByDicomForSegmentation(
            segmentationNode)
        if referenceVolume is None and use_ref_image == True:
          logging.error('No reference volume found for segmentation ' + segmentationNode.GetName())
          continue

        # Use reference image for closed surface to binary labelmap conversion
        if use_ref_image:
          segmentationNode.SetReferenceImageGeometryParameterFromVolumeNode(referenceVolume)

        # Perform conversion
        if not segmentationNode.CreateBinaryLabelmapRepresentation():
          logging.error('Failed to create binary labelmap representation for segmentation ' + segmentationNode.GetName())
          continue

        # Create labelmap volume nodes from binary labelmaps
        allSegmentIDs = vtk.vtkStringArray()
        segmentationNode.GetSegmentation().GetSegmentIDs(allSegmentIDs)
        for segmentIndex in range(allSegmentIDs.GetNumberOfValues()):
          segmentID = allSegmentIDs.GetValue(segmentIndex)

          # Create output labelmap volume
          labelmapNode = slicer.vtkMRMLLabelMapVolumeNode()
          slicer.mrmlScene.AddNode(labelmapNode)
          labelmapName = segmentationNode.GetName() + "_" + segmentID
          labelmapNode.SetName(labelmapName)

          # Export single segment to labelmap
          singleSegmentIDArray = vtk.vtkStringArray()
          singleSegmentIDArray.InsertNextValue(segmentID)
          if not slicer.vtkSlicerSegmentationsModuleLogic.ExportSegmentsToLabelmapNode(
              segmentationNode, singleSegmentIDArray, labelmapNode, referenceVolume):
            logging.error('Failed to create labelmap from segment ' + segmentID + ' in segmentation ' + segmentationNode.GetName())
            continue

          # Append volume to list
          labelmapsToSave.append(labelmapNode)

      return labelmapsToSave

    def SaveLabelmaps(self, labelmapsToSave, outputDir):
      for labelmapNode in labelmapsToSave:
        # Clean up file name and set path
        fileName = labelmapNode.GetName() + '.nrrd'
        table = str.maketrans(dict.fromkeys('!?:;'))
        fileName = fileName.translate(table)
        filePath = outputDir + '/' + fileName
        logging.info('  Saving structure ' + labelmapNode.GetName() + '\n    to file ' + fileName)

        # Save to file
        success = slicer.util.saveNode(labelmapNode, filePath)
        if not success:
          logging.error('Failed to save labelmap: ' + filePath)

    def SaveImages(self, outputDir, node_key='vtkMRMLScalarVolumeNode*'):
      # Save all of the ScalarVolumes (or whatever is in node_key) to NRRD files
      sv_nodes = slicer.util.getNodes(node_key)
      logging.info("Save image volumes nodes to directory %s: %s" % (outputDir, ','.join(sv_nodes.keys())))
      for imageNode in sv_nodes.values():
        # Clean up file name and set path
        fileName = imageNode.GetName() + '.nrrd'
        table = str.maketrans(dict.fromkeys('!?:;'))
        fileName = fileName.translate(table)
        filePath = outputDir + '/' + fileName
        logging.info('  Saving image ' + imageNode.GetName() + '\n    to file ' + fileName)

        # Save to file
        success = slicer.util.saveNode(imageNode, filePath)
        if not success:
          logging.error('Failed to save image volume: ' + filePath)

    def SaveModels(self, outputDir):
      import vtkSegmentationCorePython as vtkSegmentationCore
      success = True
      segmentationNodes = slicer.util.getNodesByClass('vtkMRMLSegmentationNode')
      for segmentationNode in segmentationNodes:
        logging.info(f'  Exporting structure set {segmentationNode.GetName()} to model files')
        if not slicer.vtkSlicerSegmentationsModuleLogic.ExportSegmentsClosedSurfaceRepresentationToFiles(
          outputDir, segmentationNode, None, "STL"):
          logging.error(f'Error occurred while exporting structure set {segmentationNode.GetName()} to model files')
          success = False
      return success

# ------------------------------------------------------------------------------
# BatchStructureSetConversionTest
#
class BatchStructureSetConversionTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

    self.delayMs = 700

    # TODO: Comment out - sample code for debugging by writing to file
    # logFile = open('d:/pyTestLog.txt', 'a')
    # logFile.write(repr(slicer.modules.BatchStructureSetConversion) + '\n')
    # logFile.close()

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()

    self.test_BatchStructureSetConversion_FullTest1()

  def test_BatchStructureSetConversion_FullTest1(self):
    # Create logic
    self.logic = BatchStructureSetConversionLogic()

    # Check for modules
    self.assertTrue(slicer.modules.dicomrtimportexport)
    self.assertTrue(slicer.modules.segmentations)

    self.TestSection_0_SetupPathsAndNames()
    # Open test database and empty it
    with DICOMUtils.TemporaryDICOMDatabase(self.dicomDatabaseDir) as self.db:
      self.TestSection_1_LoadDicomData()
      self.TestSection_2_ConvertStructureSetToLabelmap()
      self.TestSection_3_SaveLabelmaps()
    logging.info('Test finished')

  def TestSection_0_SetupPathsAndNames(self):
    if not os.access(self.logic.dataDir, os.F_OK):
      os.mkdir(self.logic.dataDir)
    self.dicomDataDir = self.logic.dataDir + '/TinyRtStudy'
    if not os.access(self.dicomDataDir, os.F_OK):
      os.mkdir(self.dicomDataDir)

    # Define variables
    self.dataDir = slicer.app.temporaryPath + '/BatchStructureSetConversion'
    if not os.access(self.dataDir, os.F_OK):
        os.mkdir(self.dataDir)
    self.dicomDatabaseDir = self.dataDir + '/CtkDicomDatabase'
    self.dicomZipFileUrl = 'https://github.com/SlicerRt/SlicerRtData/releases/download/TestData/plastimatch_tiny-rt-study.zip'
    self.dicomZipFilePath = self.logic.dataDir + '/TinyRtStudy.zip'
    self.expectedNumOfFilesInDicomDataDir = 12
    self.db = None
    self.outputDir = self.logic.dataDir + '/Output'

  def TestSection_1_LoadDicomData(self):
    try:
      self.assertTrue( self.db.isOpen )
      self.assertEqual( slicer.dicomDatabase, self.db)

      # Download, unzip, import, and load data. Verify selected plugins and loaded nodes.
      selectedPlugins = { 'Scalar Volume':1, 'RT':2 }
      loadedNodes = { 'vtkMRMLScalarVolumeNode':3, \
                      'vtkMRMLSegmentationNode':1 }
      with DICOMUtils.LoadDICOMFilesToDatabase( \
          self.dicomZipFileUrl, self.dicomZipFilePath, \
          self.dicomDataDir, self.expectedNumOfFilesInDicomDataDir, \
          {}, loadedNodes) as success:
        self.assertTrue(success)

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_2_ConvertStructureSetToLabelmap(self):
    self.delayDisplay("Convert loaded structure set to labelmap", self.delayMs)
    qt.QApplication.setOverrideCursor(qt.QCursor(qt.Qt.BusyCursor))
    try:
      self.labelmapsToSave = self.logic.ConvertStructureSetToLabelmap(use_ref_image=True)
      self.assertTrue(len(self.labelmapsToSave) > 0)
    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e), self.delayMs * 2)
    qt.QApplication.restoreOverrideCursor()

  def TestSection_3_SaveLabelmaps(self):
    self.delayDisplay("Save labelmaps to directory\n  %s" % (self.outputDir), self.delayMs)

    self.assertTrue(len(self.labelmapsToSave) > 0)
    qt.QApplication.setOverrideCursor(qt.QCursor(qt.Qt.BusyCursor))

    self.logic.SaveLabelmaps(self.labelmapsToSave, self.outputDir)

    self.delayDisplay('  Labelmaps saved to  %s' % (self.outputDir), self.delayMs)
    qt.QApplication.restoreOverrideCursor()


def main(argv):

  try:
    logging.info("Start batch RTSTRUCT conversion")

    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Batch Structure Set Conversion")
    parser.add_argument("-i", "--input-folder", dest="input_folder", metavar="PATH",
                        default="-", required=True,
                        help="Folder of input DICOM study (or database path to use existing)")
    parser.add_argument("-r", "--ref-dicom-folder", dest="ref_dicom_folder", metavar="PATH",
                        default="", required=False,
                        help="Folder containing reference anatomy DICOM image series, if stored outside the input study")
    parser.add_argument("-u", "--use-ref-image", dest="use_ref_image",
                        default=False, required=False, action='store_true',
                        help="Use anatomy image as reference when converting structure set to labelmap")
    parser.add_argument("-x", "--exist-db", dest="exist_db",
                        default=False, required=False, action='store_true',
                        help="Process an existing database")
    parser.add_argument("-m", "--export-images", dest="export_images",
                        default=False, required=False, action='store_true',
                        help="Export image data with labelmaps")
    parser.add_argument("-o", "--output-folder", dest="output_folder", metavar="PATH",
                        default=".",
                        help="Folder for output labelmaps")
    parser.add_argument("-s", "--export-surfaces", dest="export_surfaces",
                        default=False, required=False, action='store_true',
                        help="Export surface mesh representation")
    parser.add_argument("-c", "--show-python-console", dest="show_python_console",
                        default=False, required=False, action='store_true',
                        help="If this flag is specified then messages are displayed in an interactive Python console and the application does not quit when the script is finished.")

    args = parser.parse_args(argv)

    if args.show_python_console:
      slicer.util.pythonShell().show()
      slicer.exit_when_finished = False

    # Check if SlicerRT is installed
    try:
      slicer.modules.dicomrtimportexport
    except AttributeError:
      logging.error("Please install SlicerRT extension")
      return 1

    # Check required arguments
    if args.input_folder == "-":
      logging.warning('Please specify input DICOM study folder!')
    if args.output_folder == ".":
      logging.info('Current directory is selected as output folder (default). To change it, please specify --output-folder')

    # Convert to python path style
    input_folder = args.input_folder.replace('\\', '/')
    ref_dicom_folder = args.ref_dicom_folder.replace('\\', '/')
    output_folder = args.output_folder.replace('\\', '/')

    use_ref_image = args.use_ref_image
    exist_db = args.exist_db
    export_images = args.export_images
    export_surfaces = args.export_surfaces

    # Perform batch conversion
    logic = BatchStructureSetConversionLogic()
    def save_rtslices(output_dir, use_ref_image, ref_image_node_id=None):
      # package the saving code into a subfunction
      logging.info("Convert loaded structure set to labelmap volumes")
      labelmaps = logic.ConvertStructureSetToLabelmap(use_ref_image, ref_image_node_id)

      logging.info("Save labelmaps to directory " + output_dir)
      logic.SaveLabelmaps(labelmaps, output_dir)
      if export_surfaces:
        logic.SaveModels(output_dir)
      if export_images:
        logic.SaveImages(output_dir)
      logging.info("DONE")

    if exist_db:
      logging.info('BatchStructureSet running in existing database mode')
      DICOMUtils.openDatabase(input_folder)
      all_patients = slicer.dicomDatabase.patients()
      logging.info('Processing %d patients...' % len(all_patients))

      for patient in all_patients:
        try:
          slicer.mrmlScene.Clear(0) # clear the scene
          DICOMUtils.loadPatientByUID(patient)
          output_dir = os.path.join(output_folder,patient)
          if not os.access(output_dir, os.F_OK):
            os.mkdir(output_dir)
          save_rtslices(output_dir, use_ref_image)
        except OSError as e:
          # Failed to load data from this patient, continue with the next one
          print(e)

    else:
      logging.info('BatchStructureSet running in file mode')
      ref_volume_file_path = None
      if os.path.isdir(ref_dicom_folder):
        # If reference DICOM folder is given and valid, then import reference patient and save its ID
        logging.info("Import reference anatomy DICOM data from " + ref_dicom_folder)
        DICOMUtils.openTemporaryDatabase()
        DICOMUtils.importDicom(ref_dicom_folder)
        # Save first volume to be used as reference
        logic.LoadFirstPatientIntoSlicer()
        scalarVolumeNodes = list(slicer.util.getNodes('vtkMRMLScalarVolume*').values())
        if len(scalarVolumeNodes) > 0:
          refVolNode = scalarVolumeNodes[0]
          refVolStorageNode = refVolNode.CreateDefaultStorageNode()
          ref_volume_file_path = os.path.join(output_folder, 'refVolume.nrrd')
          refVolStorageNode.SetFileName(ref_volume_file_path)
          refVolStorageNode.WriteData(refVolNode)        

      logging.info("Import DICOM data from " + input_folder)
      DICOMUtils.openTemporaryDatabase()
      DICOMUtils.importDicom(input_folder)

      all_patients = slicer.dicomDatabase.patients()
      logging.info('Processing %d patients...' % len(all_patients))
      for patient in all_patients:
        try:
          slicer.mrmlScene.Clear(0) # clear the scene
          DICOMUtils.loadPatientByUID(patient)
          output_dir = os.path.join(output_folder,patient)
          if not os.access(output_dir, os.F_OK):
            os.mkdir(output_dir)
          ref_volume_node_id = None
          if ref_volume_file_path:
            try:
              refVolNode = slicer.util.loadVolume(ref_volume_file_path)
              ref_volume_node_id = refVolNode.GetID()
            except:
              pass
          save_rtslices(output_dir, use_ref_image, ref_volume_node_id)
        except OSError as e:
          # Failed to load data from this patient, continue with the next one
          print(e)

  except Exception as e:
      import traceback
      traceback.print_exc()
      print(e)
      return 1

  return 0


if __name__ == "__main__":
  errorCode = main(sys.argv[1:])
  if slicer.exit_when_finished:
    sys.exit(errorCode)
