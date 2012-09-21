import unittest

import slicer
import ctk

import DICOMLib
import DicomRtImportPlugin

class DicomRtImportTest(unittest.TestCase):
  def setUp(self):
    self.logFile = open('d:/pyTestLog.txt', 'w')
    self.logFile.write('01-setUp\n\n')
    self.logFile.write('sajt\n')

  def tearDown(self):
    self.logFile.close()

  def test_ImportAndLoadRtDataset(self):
    #self.assertEqual(scene.GetNumberOfNodesByClass('vtkMRMLScalarVolumeNode'), 1)

    self.logFile.write('02-test_ImportAndLoadRtDataset\n')

    # Create new DICOM database
    dicomDatabase = ctk.ctkDICOMDatabase()

    # Set database precache tags
    self.logFile.write('a')
    dicomRtImportPlugin = DicomRtImportPlugin.DicomRtImportPlugin()
    self.logFile.write('b')
    tagsToPrecache = list(dicomDatabase.tagsToPrecache)
    self.logFile.write(repr(dicomRtImportPlugin))
    #dicomRtImportDicomPlugin = slicer.modules.dicomPlugins['DicomRtImportPlugin']
    self.logFile.write('.')
    tagsToPrecache += dicomRtImportDicomPlugin.tags.values()
    self.logFile.write('.')
    tagsToPrecache.sort()
    self.logFile.write('.')
    dicomDatabase.tagsToPrecache = tagsToPrecache
    self.logFile.write('.')

    if not dicomDatabase.isOpen:
      self.messageBox('The database file path "%s" cannot be opened.' % 'sajt')
    else:
      self.messageBox('Open')

    self.logFile.write('02-end\n\n')
