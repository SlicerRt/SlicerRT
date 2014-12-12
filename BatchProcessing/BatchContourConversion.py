import os
import unittest
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import argparse
import sys

#
# BatchContourConversion
#   Convert contours in structure set to labelmaps and save them to disk
#

class BatchContourConversion(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    parent.title = "Batch Contour Conversion"
    parent.categories = ["Testing.SlicerRT Tests"]
    parent.dependencies = ["DicomRtImportExport", "Contours"]
    parent.contributors = ["Csaba Pinter (Queen's)"]
    parent.helpText = """
    This is a self test for testing batch contour conversion.
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
    slicer.selfTests['BatchContourConversion'] = self.runTest

  def runTest(self):
    tester = BatchContourConversionTest()
    tester.runTest()

#
# BatchContourConversionWidget
#

class BatchContourConversionWidget(ScriptedLoadableModuleWidget):
  def setup(self):
    self.developerMode = True
    ScriptedLoadableModuleWidget.setup(self)

#
# BatchContourConversionLogic
#

class BatchContourConversionLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual 
  computation done by your module.  The interface 
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    ScriptedLoadableModuleLogic.__init__(self)

    self.dataDir = slicer.app.temporaryPath + '/BatchContourConversion'
    if not os.access(self.dataDir, os.F_OK):
      os.mkdir(self.dataDir)

    self.dicomDatabaseDir = self.dataDir + '/CtkDicomDatabase'

  def OpenDatabase(self):
    # Open test database and empty it
    if not os.access(self.dicomDatabaseDir, os.F_OK):
      os.mkdir(self.dicomDatabaseDir)

    dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
    dicomWidget.onDatabaseDirectoryChanged(self.dicomDatabaseDir)

    slicer.dicomDatabase.initializeDatabase()

  def ImportStudy(self,dicomDataDir):
    indexer = ctk.ctkDICOMIndexer()

    # Import study to database
    indexer.addDirectory( slicer.dicomDatabase, dicomDataDir )
    indexer.waitForImportFinished()

  def LoadFirstPatientIntoSlicer(self):
    # Choose first patient from the patient list
    detailsPopup = slicer.modules.dicom.widgetRepresentation().self().detailsPopup
    patient = slicer.dicomDatabase.patients()[0]
    studies = slicer.dicomDatabase.studiesForPatient(patient)
    series = [slicer.dicomDatabase.seriesForStudy(study) for study in studies]
    seriesUIDs = [uid for uidList in series for uid in uidList]
    detailsPopup.offerLoadables(seriesUIDs, 'SeriesUIDList')
    detailsPopup.examineForLoading()

    loadables = detailsPopup.loadableTable.loadables

    # Load into Slicer
    detailsPopup = slicer.modules.dicom.widgetRepresentation().self().detailsPopup
    detailsPopup.loadCheckedLoadables()

  def ConvertContoursToLabelmap(self):
    import vtkSlicerContoursModuleLogic

    labelmapsToSave = []
    
    # Get all contour nodes from the scene
    contourNodes = slicer.mrmlScene.GetNodesByClass('vtkMRMLContourNode')
    numberOfContours = contourNodes.GetNumberOfItems()

    for contourIndex in xrange(0,numberOfContours):
      contourNode = contourNodes.GetItemAsObject(contourIndex)
      print('  Converting contour ' + contourNode.GetName())

      # Set referenced volume as rasterization reference
      referenceVolume = vtkSlicerContoursModuleLogic.vtkSlicerContoursModuleLogic.GetReferencedVolumeByDicomForContour(contourNode)
      if referenceVolume == None:
        import sys
        sys.stderr.write('No reference volume found for contour ' + contourNode.GetName())
        continue

      contourNode.SetAndObserveRasterizationReferenceVolumeNodeId(referenceVolume.GetID())

      # Perform conversion
      contourNode.GetLabelmapImageData()
      contourLabelmapNode = vtkSlicerContoursModuleLogic.vtkSlicerContoursModuleLogic.ExtractLabelmapFromContour(contourNode)

      # Append contour to list
      labelmapsToSave.append(contourLabelmapNode)
      
    return labelmapsToSave

  def SaveLabelmaps(self,labelmapsToSave,outputDir):
    for labelmapNode in labelmapsToSave:
      # Assemble file name from patient name and contour name
      fileName = labelmapNode.GetName() + '.nrrd'
      filePath = outputDir + '/' + fileName
      print('  Saving contour ' + labelmapNode.GetName() + '\n    to file ' + fileName)

      # Workaround: create storage node to be able to save the labelmap
      storageNode = labelmapNode.CreateDefaultStorageNode()
      slicer.mrmlScene.AddNode(storageNode)
      labelmapNode.SetAndObserveStorageNodeID(storageNode.GetID())

      # Save to file
      success = slicer.util.saveNode(labelmapNode, filePath)
      if not success:
        sys.stderr.write('Failed to save labelmap: ' + filePath)

class BatchContourConversionTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

    self.delayMs = 700

    #TODO: Comment out - sample code for debugging by writing to file
    #logFile = open('d:/pyTestLog.txt', 'w')
    #logFile.write(repr(slicer.modules.batchcontourconversion) + '\n')
    #logFile.close()

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    
    # Create logic
    self.logic = BatchContourConversionLogic()

    self.test_BatchContourConversion_FullTest1()

  def test_BatchContourConversion_FullTest1(self):
    # Check for modules
    self.assertTrue( slicer.modules.dicomrtimportexport )
    self.assertTrue( slicer.modules.contours )

    self.TestSection_0SetupPathsAndNames()
    self.TestSection_1RetrieveInputData()
    self.TestSection_2OpenDatabase()
    self.TestSection_3ImportStudy()
    self.TestSection_4LoadFirstPatientIntoSlicer()
    self.TestSection_5ConvertContoursToLabelmap()
    self.TestSection_6SaveLabelmaps()
    self.TestSection_0Clear()


  def TestSection_0SetupPathsAndNames(self):
    self.dicomDataDir = self.logic.dataDir + '/TinyRtStudy'
    if not os.access(self.dicomDataDir, os.F_OK):
      os.mkdir(self.dicomDataDir)

    self.dicomZipFilePath = self.logic.dataDir + '/TinyRtStudy.zip'
    self.expectedNumOfFilesInDicomDataDir = 12
    self.outputDir = self.logic.dataDir + '/Output'
    
  def TestSection_1RetrieveInputData(self):
    try:
      import urllib
      downloads = (
          ('http://slicer.kitware.com/midas3/download/folder/2478/TinyRtStudy.zip', self.dicomZipFilePath),
          )

      downloaded = 0
      for url,filePath in downloads:
        if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
          if downloaded == 0:
            self.delayDisplay('Downloading input data to folder\n' + self.dicomZipFilePath + '.\n\n  It may take a few minutes...',self.delayMs)
          print('Requesting download from %s...' % (url))
          urllib.urlretrieve(url, filePath)
          downloaded += 1
        else:
          self.delayDisplay('Input data has been found in folder ' + self.dicomZipFilePath, self.delayMs)
      if downloaded > 0:
        self.delayDisplay('Downloading input data finished',self.delayMs)

      numOfFilesInDicomDataDir = len([name for name in os.listdir(self.dicomDataDir) if os.path.isfile(self.dicomDataDir + '/' + name)])
      if (numOfFilesInDicomDataDir != self.expectedNumOfFilesInDicomDataDir):
        slicer.app.applicationLogic().Unzip(self.dicomZipFilePath, self.logic.dataDir)
        self.delayDisplay("Unzipping done",self.delayMs)

      numOfFilesInDicomDataDirTest = len([name for name in os.listdir(self.dicomDataDir) if os.path.isfile(self.dicomDataDir + '/' + name)])
      self.assertEqual( numOfFilesInDicomDataDirTest, self.expectedNumOfFilesInDicomDataDir )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  def TestSection_2OpenDatabase(self):
    self.delayDisplay("Open database",self.delayMs)
    self.logic.OpenDatabase()

    self.assertTrue( slicer.dicomDatabase.isOpen )

  def TestSection_3ImportStudy(self):
    self.delayDisplay("Import study",self.delayMs)
    self.logic.ImportStudy(self.dicomDataDir)

    self.assertTrue( len(slicer.dicomDatabase.patients()) > 0 )
    self.assertTrue( slicer.dicomDatabase.patients()[0] )

  def TestSection_4LoadFirstPatientIntoSlicer(self):
    self.delayDisplay("Load first patient into Slicer",self.delayMs)
    self.logic.LoadFirstPatientIntoSlicer()

  def TestSection_5ConvertContoursToLabelmap(self):
    self.delayDisplay("Convert loaded contours to labelmap",self.delayMs)
    qt.QApplication.setOverrideCursor(qt.QCursor(qt.Qt.BusyCursor))
    try:
      self.labelmapsToSave = self.logic.ConvertContoursToLabelmap()
    except e, Exception:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
    qt.QApplication.restoreOverrideCursor()

  def TestSection_6SaveLabelmaps(self):
    self.delayDisplay("Save labelmaps to directory\n  %s" % (self.outputDir),self.delayMs)

    self.assertTrue(len(self.labelmapsToSave) > 0)
    qt.QApplication.setOverrideCursor(qt.QCursor(qt.Qt.BusyCursor))

    self.logic.SaveLabelmaps(self.labelmapsToSave,self.outputDir)

    self.delayDisplay('  Labelmaps saved to  %s' % (self.outputDir),self.delayMs)
    qt.QApplication.restoreOverrideCursor()

  def TestSection_0Clear(self):
    self.delayDisplay("Clear database and scene",self.delayMs)

    initialized = slicer.dicomDatabase.initializeDatabase()
    self.assertTrue( initialized )

    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )

    #slicer.mrmlScene.Clear(0) #TODO


def main(argv):
  try:
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Batch Contour Conversion")
    parser.add_argument("-i", "--input-folder", dest="input_folder", metavar="PATH",
                        default="-", required=True, help="Folder of input DICOM study")
    parser.add_argument("-o", "--output-folder", dest="output_folder", metavar="PATH",
                        default=".", help="Folder for output labelmaps")

    args = parser.parse_args(argv)

    # Check required arguments
    if args.input_folder == "-":
      print('Please specify input DICOM study folder!')
    if args.output_folder == ".":
      print('Current directory is selected as output folder (default). To change it, please specify --output-folder')

    # Convert to python path style
    input_folder = args.input_folder.replace('\\','/')
    output_folder = args.output_folder.replace('\\','/')

    # Perform batch conversion
    logic = BatchContourConversionLogic()

    print("Import DICOM data from " + input_folder)
    logic.OpenDatabase()
    logic.ImportStudy(input_folder)

    print("Load first patient into Slicer")
    logic.LoadFirstPatientIntoSlicer()

    print("Convert loaded contours to labelmap volumes")
    labelmaps = logic.ConvertContoursToLabelmap()

    print("Save labelmaps to directory " + output_folder)
    logic.SaveLabelmaps(labelmaps, output_folder)
    print("DONE")

  except Exception, e:
    print e
  sys.exit()

if __name__ == "__main__":
  main(sys.argv[1:])
