import os
import unittest
import util.DicomRtImportSelfTestPaths
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# DicomRtImportSelfTest
#

class DicomRtImportSelfTest(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "DICOM-RT Import Self Test"
    self.parent.categories = ["Testing.SlicerRT Tests"]
    self.parent.dependencies = ["DicomRtImportExport", "Segmentations"]
    self.parent.contributors = ["Csaba Pinter (Queen's)"]
    self.parent.helpText = """
    This is a self test for the DicomRtImportExport DICOM plugin module.
    """
    self.parent.acknowledgementText = """This file was originally developed by Csaba Pinter, PerkLab, Queen's University and was supported through the Applied Cancer Research Unit program of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care""" # replace with organization, grant and thanks.

#
# DicomRtImportSelfTestWidget
#

class DicomRtImportSelfTestWidget(ScriptedLoadableModuleWidget):

  def setup(self):
    self.developerMode = True
    ScriptedLoadableModuleWidget.setup(self)

    # Add vertical spacer
    self.layout.addStretch(1)

#
# DicomRtImportSelfTestLogic
#

class DicomRtImportSelfTestLogic(ScriptedLoadableModuleLogic):
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
      logging.error('no volume node')
      return False
    if volumeNode.GetImageData() == None:
      logging.error('no image data')
      return False
    return True


class DicomRtImportSelfTestTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

    self.delayMs = 700

    #TODO: Comment out
    #logFile = open('d:/pyTestLog.txt', 'a')
    #logFile.write(repr(slicer.modules.dicomrtimportselftest) + '\n')
    #logFile.write(repr(slicer.modules.dicomrtimportexport) + '\n')
    #logFile.close()

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()

    self.test_DicomRtImportSelfTest_FullTest1()

  def test_DicomRtImportSelfTest_FullTest1(self):
    # Check for modules
    self.assertTrue( slicer.modules.dicomrtimportexport )
    self.assertTrue( slicer.modules.segmentations )

    self.TestSection_0RetrieveInputData()
    self.TestSection_1OpenDatabase()
    self.TestSection_2ImportStudy()
    self.TestSection_3SelectLoadables()
    self.TestSection_4LoadIntoSlicer()
    self.TestSection_5SaveScene()
    self.TestSection_6ClearDatabase()


  def TestSection_0RetrieveInputData(self):
    if 'util' in globals() and hasattr(util, 'DicomRtImportSelfTestPaths') and os.access(util.DicomRtImportSelfTestPaths.dataDir, os.F_OK):
      self.dataDir = util.DicomRtImportSelfTestPaths.dataDir
      self.dicomDatabaseDir = util.DicomRtImportSelfTestPaths.dicomDatabaseDir
      self.tempDir = util.DicomRtImportSelfTestPaths.tempDir
      self.delayDisplay('Test data found locally: %s' % (util.DicomRtImportSelfTestPaths.dataDir),self.delayMs)
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
          ('http://slicer.kitware.com/midas3/download?items=10615', 'RS.1.2.246.352.71.4.2088656855.2404649.20110920153449.dcm'),
          ('http://slicer.kitware.com/midas3/download/item/119940', 'RI.1.2.246.352.71.3.2088656855.2381134.20110921150951.dcm')
          )

      for url,name in downloads:
        filePath = self.dataDir + '/' + name
        if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
          logging.info('Requesting download %s from %s...\n' % (name, url))
          urllib.urlretrieve(url, filePath)
      self.delayDisplay('Finished with download test data',self.delayMs)

  def TestSection_1OpenDatabase(self):
    self.delayDisplay("1: Open database",self.delayMs)

    # Open test database and empty it
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
    patient = slicer.dicomDatabase.patients()[0]
    studies = slicer.dicomDatabase.studiesForPatient(patient)
    series = [slicer.dicomDatabase.seriesForStudy(study) for study in studies]
    seriesUIDs = [uid for uidList in series for uid in uidList]
    detailsPopup.offerLoadables(seriesUIDs, 'SeriesUIDList')
    detailsPopup.examineForLoading()

    loadables = detailsPopup.loadableTable.loadables
    self.assertTrue( len(loadables) == 8 )

    # Make sure the loadables are good (RT is assigned to 4 out of 8 and they are selected)
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
    self.assertTrue( loadablesForRt == 4 )

  def TestSection_4LoadIntoSlicer(self):
    self.delayDisplay("4: Load into Slicer",self.delayMs)

    detailsPopup = slicer.modules.dicom.widgetRepresentation().self().detailsPopup
    detailsPopup.loadCheckedLoadables()

    # Verify that the correct number of objects were loaded
    # Volumes: Dose, RT image, RT image texture
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == 3 )
    # Model hierarchies: Beam models (parent + individual beams)
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') ) == 6 )
    # Subject hierarchy nodes: Patient, Study, Dose, RT image (plus SH nodes automatically created for texture
    # image and displayed model), structure set segmentation and segment virtual nodes, beam models.
    # If subject hierarchy auto creation is off, then 2 less nodes are created (the RT image plane model and texture volume)
    autoCreateSh = slicer.modules.subjecthierarchy.widgetRepresentation().pluginLogic().autoCreateSubjectHierarchy
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLSubjectHierarchyNode*') ) == 23 + 2*autoCreateSh )
    # Segmentation: The loaded structures
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLSegmentationNode*') ) == 1 )
    # Markups: the isocenters and their derived sources (in the same markup node as the isocenter)
    self.assertTrue( len( slicer.util.getNodes('vtkMRMLMarkupsFiducialNode*') ) == 5 )

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
