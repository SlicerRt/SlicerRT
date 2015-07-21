import os
import unittest
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# IGRTWorkflow_SelfTest
#
# IGRT: Calculate isocenter shifting, and evaluate its effect on the dose
#   1. Load planning DICOM-RT data and day 2 volumes
#   2. Add day 2 volumes in Subject Hierarchy
#   3. Compute isodose lines for both dose distributions
#   4. Register day 2 CT to planning CT using rigid registration
#   5. Transform day 2 dose volumes using the result transform
#   6. Compute difference dose using gamma comparison for
#     6A. Planning dose and unregistered day 2 dose
#     6B. Planning dose and registered day 2 dose
#   7. Accumulate
#     7A. Planning dose and unregistered day 2 dose
#     7B. Planning dose and registered day 2 dose
#   8. DVH for accumulated registered and unregistered dose volumes
#     for targets and some other structures
#

class IGRTWorkflow_SelfTest(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "SlicerRT IGRT Self Test"
    self.parent.categories = ["Testing.SlicerRT Tests"]
    self.parent.dependencies = ["DicomRtImportExport", "SubjectHierarchy", "Segmentations", "Isodose", "BRAINSFit", "DoseComparison", "DoseAccumulation", "DoseVolumeHistogram", "SegmentComparison", "SegmentMorphology"]
    self.parent.contributors = ["Csaba Pinter (Queen's)"]
    self.parent.helpText = """
    This is a self test that automatically runs the IGRT workflow, for tutorial purposes and to test every step involved.
    """
    self.parent.acknowledgementText = """This file was originally developed by Csaba Pinter, PerkLab, Queen's University and was supported through the Applied Cancer Research Unit program of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care""" # replace with organization, grant and thanks.

#
# -----------------------------------------------------------------------------
# IGRTWorkflow_SelfTestTest
# -----------------------------------------------------------------------------
#

class IGRTWorkflow_SelfTestTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  #------------------------------------------------------------------------------
  def test_IGRTWorkflow_SelfTest_FullTest(self):
    try:
      #slicer.namic_selftest_instance = self #TODO: For debugging

      # Check for modules
      self.assertTrue( slicer.modules.dicomrtimportexport )
      self.assertTrue( slicer.modules.subjecthierarchy )
      self.assertTrue( slicer.modules.segmentations )
      self.assertTrue( slicer.modules.isodose )
      self.assertTrue( slicer.modules.brainsfit )
      self.assertTrue( slicer.modules.dosecomparison )
      self.assertTrue( slicer.modules.doseaccumulation )
      self.assertTrue( slicer.modules.dosevolumehistogram )

      self.TestSection_00_SetupPathsAndNames()
      self.TestSection_01A_OpenTempDatabase()
      self.TestSection_01B_DownloadDay1Data()
      self.TestSection_01C_ImportDay1Study()
      self.TestSection_01D_SelectLoadablesAndLoad()
      self.TestSection_01E_LoadDay2Data()
      self.TestSection_01F_AddDay2DataToSubjectHierarchy()
      self.TestSection_01G_SetDisplayOptions()
      self.TestSection_03A_ComputeIsodoseForDay1()
      self.TestSection_03B_ComputeIsodoseForDay2()
      self.TestSection_04_RegisterDay2CTToDay1CT()
      self.TestSection_05_TransformDay2DoseVolume()
      self.TestSection_06_ComputeGamma()
      self.TestSection_07_AccumulateDose()
      self.TestSection_08_ComputeDvh()
      self.TestUtility_ClearDatabase()

    except Exception, e:
      pass

  #------------------------------------------------------------------------------
  def TestSection_00_SetupPathsAndNames(self):
    # Make sure subject hierarchy auto-creation is on for this test
    subjectHierarchyWidget = slicer.modules.subjecthierarchy.widgetRepresentation()
    subjectHierarchyPluginLogic = subjectHierarchyWidget.pluginLogic()
    self.assertTrue( subjectHierarchyWidget != None )
    self.assertTrue( subjectHierarchyPluginLogic != None )
    subjectHierarchyPluginLogic.autoCreateSubjectHierarchy = True

    IGRTWorkflow_SelfTestDir = slicer.app.temporaryPath + '/IGRTWorkflow_SelfTest'
    if not os.access(IGRTWorkflow_SelfTestDir, os.F_OK):
      os.mkdir(IGRTWorkflow_SelfTestDir)

    self.dicomDataDir = IGRTWorkflow_SelfTestDir + '/EclipseEntPhantomRtData'
    if not os.access(self.dicomDataDir, os.F_OK):
      os.mkdir(self.dicomDataDir)

    self.day2DataDir = IGRTWorkflow_SelfTestDir + '/EclipseEntComputedDay2Data'
    if not os.access(self.day2DataDir, os.F_OK):
      os.mkdir(self.day2DataDir)

    self.dicomDatabaseDir = IGRTWorkflow_SelfTestDir + '/CtkDicomDatabase'
    self.dicomZipFilePath = IGRTWorkflow_SelfTestDir + '/EclipseEntDicomRt.zip'
    self.expectedNumOfFilesInDicomDataDir = 142
    self.tempDir = IGRTWorkflow_SelfTestDir + '/Temp'
    
    self.day1CTName = '2: ENT IMRT'
    self.day1DoseName = '5: RTDOSE: BRAI1'
    self.day1StructureSetName = '3: RTSTRUCT: ENT'
    self.day1BeamsName = '4: RTPLAN: BRAI1_BeamModels' + slicer.vtkMRMLSubjectHierarchyConstants.GetSubjectHierarchyNodeNamePostfix()
    self.day1IsodosesName = '5: RTDOSE: BRAI1_IsodoseSurfaces' + slicer.vtkMRMLSubjectHierarchyConstants.GetSubjectHierarchyNodeNamePostfix()
    self.day2CTName = '2_ENT_IMRT_Day2'
    self.day2DoseName = '5_RTDOSE_Day2'
    self.day2IsodosesName = self.day2DoseName + '_IsodoseSurfaces' + slicer.vtkMRMLSubjectHierarchyConstants.GetSubjectHierarchyNodeNamePostfix()
    self.transformDay2ToDay1RigidName = 'Transform_Day2ToDay1_Rigid'
    self.transformDay2ToDay1BSplineName = 'Transform_Day2ToDay1_BSpline'
    self.day2DoseRigidName = self.day2DoseName + '_Registered_Rigid'
    self.day2DoseBSplineName = self.day2DoseName + '_Registered_BSpline'
    self.gammaVolumeName = 'Gamma_Day1_Day2Rigid'
    self.doseVolumeIdentifierAttributeName = 'DicomRtImport.DoseVolume'
    self.doseUnitNameAttributeName = 'DicomRtImport.DoseUnitName'
    self.doseUnitValueAttributeName = 'DicomRtImport.DoseUnitValue'
    self.doseAccumulationDoseVolumeNameProperty = 'DoseAccumulation.DoseVolumeNodeName'
    self.accumulatedDoseUnregisteredName = '5_RTDOSE_Accumulated_Unregistered'
    self.accumulatedDoseRigidName = '5_RTDOSE_Accumulated_Rigid'
    # self.accumulatedDoseBSplineName = '5_RTDOSE_Accumulated_BSpline'
    
    self.setupPathsAndNamesDone = True

  #------------------------------------------------------------------------------
  def TestSection_01A_OpenTempDatabase(self):
    # Open test database and empty it
    try:
      os.mkdir(self.dicomDatabaseDir)

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
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_01B_DownloadDay1Data(self):
    try:
      import urllib
      downloads = (
          ('http://slicer.kitware.com/midas3/download/item/101019/EclipseEntPhantomRtData.zip', self.dicomZipFilePath),
          )

      downloaded = 0
      for url,filePath in downloads:
        if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
          if downloaded == 0:
            self.delayDisplay('Downloading Day 1 input data to folder\n' + self.dicomZipFilePath + '.\n\n  It may take a few minutes...',self.delayMs)
          logging.info('Requesting download from %s...' % (url))
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

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_01C_ImportDay1Study(self):
    self.delayDisplay("Import Day 1 study",self.delayMs)

    try:
      slicer.util.selectModule('DICOM')

      # Import study to database
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      indexer = ctk.ctkDICOMIndexer()
      self.assertTrue( indexer )

      indexer.addDirectory( slicer.dicomDatabase, self.dicomDataDir )

      self.assertTrue( len(slicer.dicomDatabase.patients()) == 1 )
      self.assertTrue( slicer.dicomDatabase.patients()[0] )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_01D_SelectLoadablesAndLoad(self):
    self.delayDisplay("Select loadables and load data",self.delayMs)

    try:
      numOfScalarVolumeNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') )
      numOfModelHierarchyNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') )
      numOfSegmentationNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLSegmentationNode*') )

      # Choose first patient from the patient list
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      self.delayDisplay("Wait for DICOM browser to initialize",self.delayMs)
      patient = slicer.dicomDatabase.patients()[0]
      studies = slicer.dicomDatabase.studiesForPatient(patient)
      series = [slicer.dicomDatabase.seriesForStudy(study) for study in studies]
      seriesUIDs = [uid for uidList in series for uid in uidList]
      dicomWidget.detailsPopup.offerLoadables(seriesUIDs, 'SeriesUIDList')
      dicomWidget.detailsPopup.examineForLoading()

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
      self.assertTrue( loadablesForRt == 3 )

      dicomWidget.detailsPopup.loadCheckedLoadables()

      # Verify that the correct number of objects were loaded
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == numOfScalarVolumeNodesBeforeLoad + 2 )
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLModelHierarchyNode*') ) == numOfModelHierarchyNodesBeforeLoad + 7 )
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLSegmentationNode*') ) == numOfSegmentationNodesBeforeLoad + 1 )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_01E_LoadDay2Data(self):
    try:
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
          logging.info('Requesting download from %s...\n' % (url))
          urllib.urlretrieve(url, filePath)
          # TODO Check file sizes if possible (sometimes one of them does not fully download)
          downloaded += 1
        else:
          self.delayDisplay('Day 2 input data has been found in folder ' + self.day2DataDir, self.delayMs)
        if loader:
          logging.info('Loading %s...' % (os.path.split(filePath)[1]))
          loader(filePath)
      if downloaded > 0:
        self.delayDisplay('Downloading Day 2 input data finished',self.delayMs)

      # Verify that the correct number of objects were loaded
      self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == numOfScalarVolumeNodesBeforeLoad + 2 )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_01F_AddDay2DataToSubjectHierarchy(self):
    try:
      from vtkSlicerSubjectHierarchyModuleMRML import vtkMRMLSubjectHierarchyNode

      # Get patient node
      day1CT = slicer.util.getNode(self.day1CTName)
      ct1HierarchyNode = slicer.vtkMRMLHierarchyNode.GetAssociatedHierarchyNode(slicer.mrmlScene, day1CT.GetID())
      patientNode = ct1HierarchyNode.GetParentNode().GetParentNode()
      self.assertTrue( patientNode != None )
      
      # Add new study for the day 2 data
      studyNode = vtkMRMLSubjectHierarchyNode.CreateSubjectHierarchyNode(slicer.mrmlScene, patientNode, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMLevelStudy(), 'Day2')
      studyNode.AddUID(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(),'Day2Study_UID')
      
      # Add dose unit attributes to the new study node
      studyDay1Node = ct1HierarchyNode.GetParentNode()
      doseUnitName = studyDay1Node.GetAttribute(self.doseUnitNameAttributeName)
      doseUnitValue = studyDay1Node.GetAttribute(self.doseUnitValueAttributeName)
      studyNode.SetAttribute(self.doseUnitNameAttributeName, doseUnitName)
      studyNode.SetAttribute(self.doseUnitValueAttributeName, doseUnitValue)

      # Add day 2 CT series
      day2CT = slicer.util.getNode(self.day2CTName)
      seriesNodeCT = vtkMRMLSubjectHierarchyNode.CreateSubjectHierarchyNode(slicer.mrmlScene, studyNode, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMLevelSeries(), self.day2CTName, day2CT)
      seriesNodeCT.AddUID(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(),'Day2CT_UID')

      # Set dose attributes for day 2 dose
      day2Dose = slicer.util.getNode(self.day2DoseName)
      day2Dose.SetAttribute(self.doseVolumeIdentifierAttributeName,'1')

      # Add day 2 dose series
      day2Dose = slicer.util.getNode(self.day2DoseName)
      seriesNodeDose = vtkMRMLSubjectHierarchyNode.CreateSubjectHierarchyNode(slicer.mrmlScene, studyNode, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMLevelSeries(), self.day2DoseName, day2Dose)
      seriesNodeDose.AddUID(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(),'Day2Dose_UID')

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_01G_SetDisplayOptions(self):
    self.delayDisplay('Setting display options for loaded data',self.delayMs)

    try:
      from vtkSlicerSubjectHierarchyModuleMRML import vtkMRMLSubjectHierarchyNode

      # Set default dose color map and W/L
      defaultDoseColorTable = slicer.util.getNode('Dose_ColorTable')
      day2Dose = slicer.util.getNode(self.day2DoseName)
      day2Dose.GetDisplayNode().SetAndObserveColorNodeID(defaultDoseColorTable.GetID())

      day1Dose = slicer.util.getNode(self.day1DoseName)
      day1DoseSubjectHierarchy = vtkMRMLSubjectHierarchyNode.GetAssociatedSubjectHierarchyNode(day1Dose)
      doseUnitValue = day1DoseSubjectHierarchy.GetAttributeFromAncestor(self.doseUnitValueAttributeName, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMLevelStudy())
      day2Dose.GetDisplayNode().SetAutoWindowLevel(0)
      day2Dose.GetDisplayNode().SetWindowLevel(day1Dose.GetDisplayNode().GetWindow(),day1Dose.GetDisplayNode().GetLevel())
      day2Dose.GetDisplayNode().AutoThresholdOff();
      day2Dose.GetDisplayNode().SetLowerThreshold(0.5 * float(doseUnitValue));
      day2Dose.GetDisplayNode().SetApplyThreshold(1);
      
      # Set CT windows
      day1CT = slicer.util.getNode(self.day1CTName)
      day1CT.GetDisplayNode().SetAutoWindowLevel(0)
      day1CT.GetDisplayNode().SetWindowLevel(250,80)

      day2CT = slicer.util.getNode(self.day2CTName)
      day2CT.GetDisplayNode().SetAutoWindowLevel(0)
      day2CT.GetDisplayNode().SetWindowLevel(250,80)

      # Set volumes to show
      self.TestUtility_ShowVolumes(day1CT,day1Dose)

      layoutManager = slicer.app.layoutManager()
      sliceWidgetNames = ['Red', 'Green', 'Yellow']
      layoutManager.sliceWidget(sliceWidgetNames[0]).sliceController().setSliceOffsetValue(138)
      layoutManager.sliceWidget(sliceWidgetNames[1]).sliceController().setSliceOffsetValue(-18)
      
      # Set structure visibilities/transparencies
      day1StructureSet = slicer.util.getNode(self.day1StructureSetName)
      day1StructureSet.GetDisplayNode().SetSegmentVisibility('optBRAIN', 0)
      day1StructureSet.GetDisplayNode().SetSegmentVisibility('optOptic', 0)

      threeDView = layoutManager.threeDWidget(0).threeDView()
      self.clickAndDrag(threeDView,button='Middle',start=(10,110),end=(10,10))
      self.clickAndDrag(threeDView,button='Middle',start=(10,100),end=(10,10))
      self.clickAndDrag(threeDView,start=(10,70),end=(90,20))

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_03A_ComputeIsodoseForDay1(self):
    self.delayDisplay('Computing isodose for day 1',self.delayMs)

    try:
      from vtkSlicerSubjectHierarchyModuleMRML import vtkMRMLSubjectHierarchyNode

      # Hide beams
      beamsSubjectHierarchy = slicer.util.getNode(self.day1BeamsName)
      beamsSubjectHierarchy.SetDisplayVisibilityForBranch(0)

      scene = slicer.mrmlScene
      slicer.util.selectModule('Isodose')
      numOfModelNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLModelNode*') )

      isodoseWidget = slicer.modules.isodose.widgetRepresentation()
      doseVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=isodoseWidget, className='qMRMLNodeComboBox', name='MRMLNodeComboBox_DoseVolume')[0]
      applyButton = slicer.util.findChildren(widget=isodoseWidget, className='QPushButton', name='Apply')[0]
      
      # Compute isodose for day 1 dose
      day1Dose = slicer.util.getNode(self.day1DoseName)
      doseVolumeMrmlNodeCombobox.setCurrentNodeID(day1Dose.GetID())
      applyButton.click()

      self.assertTrue( len( slicer.util.getNodes('vtkMRMLModelNode*') ) == numOfModelNodesBeforeLoad + 6 )

      # Show day 1 isodose
      day1CT = slicer.util.getNode(self.day1CTName)
      self.TestUtility_ShowVolumes(day1CT)

      day1IsodoseSubjectHierarchy = slicer.util.getNode(self.day1IsodosesName)
      day1IsodoseSubjectHierarchy.SetDisplayVisibilityForBranch(1)

      self.delayDisplay('Show day 1 isodose lines',self.delayMs)

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_03B_ComputeIsodoseForDay2(self):
    self.delayDisplay('Computing isodose for day 2',self.delayMs)

    try:
      from vtkSlicerSubjectHierarchyModuleMRML import vtkMRMLSubjectHierarchyNode

      scene = slicer.mrmlScene
      slicer.util.selectModule('Isodose')
      numOfModelNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLModelNode*') )

      isodoseWidget = slicer.modules.isodose.widgetRepresentation()
      doseVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=isodoseWidget, className='qMRMLNodeComboBox', name='MRMLNodeComboBox_DoseVolume')[0]
      applyButton = slicer.util.findChildren(widget=isodoseWidget, className='QPushButton', name='Apply')[0]

      # Compute isodose for day 2 dose
      day2Dose = slicer.util.getNode(self.day2DoseName)
      doseVolumeMrmlNodeCombobox.setCurrentNodeID(day2Dose.GetID())
      applyButton.click()

      self.assertTrue( len( slicer.util.getNodes('vtkMRMLModelNode*') ) == numOfModelNodesBeforeLoad + 6 )

      # Show day 2 isodose
      day1IsodoseSubjectHierarchy = slicer.util.getNode(self.day1IsodosesName)
      day1IsodoseSubjectHierarchy.SetDisplayVisibilityForBranch(0)

      day2CT = slicer.util.getNode(self.day2CTName)
      self.TestUtility_ShowVolumes(day2CT)

      day2Isodose = slicer.util.getNode(self.day2IsodosesName)
      day2IsodoseSubjectHierarchy = vtkMRMLSubjectHierarchyNode.GetAssociatedSubjectHierarchyNode(day2Isodose)
      day2IsodoseSubjectHierarchy.SetDisplayVisibilityForBranch(1)
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_04_RegisterDay2CTToDay1CT(self):
    try:
      from vtkSlicerSubjectHierarchyModuleMRML import vtkMRMLSubjectHierarchyNode

      scene = slicer.mrmlScene
      slicer.util.selectModule('BRAINSFit')
      brainsFit = slicer.modules.brainsfit

      # Hide isodose
      day2Isodose = slicer.util.getNode(self.day2IsodosesName)
      if (day2Isodose != None):
        day2IsodoseSubjectHierarchy = vtkMRMLSubjectHierarchyNode.GetAssociatedSubjectHierarchyNode(day2Isodose)
        day2IsodoseSubjectHierarchy.SetDisplayVisibilityForBranch(0)

      # Register Day 2 CT to Day 1 CT using rigid registration
      self.delayDisplay("Register Day 2 CT to Day 1 CT using rigid registration.\n  It may take a few minutes...",self.delayMs)

      parametersRigid = {}
      day1CT = slicer.util.getNode(self.day1CTName)
      parametersRigid["fixedVolume"] = day1CT.GetID()

      day2CT = slicer.util.getNode(self.day2CTName)
      parametersRigid["movingVolume"] = day2CT.GetID()

      linearTransform = slicer.vtkMRMLLinearTransformNode()
      linearTransform.SetName(self.transformDay2ToDay1RigidName)
      slicer.mrmlScene.AddNode( linearTransform )
      parametersRigid["linearTransform"] = linearTransform.GetID()

      parametersRigid["useRigid"] = True
      parametersRigid["samplingPercentage"] = 0.0002

      self.cliBrainsFitRigidNode = None
      self.cliBrainsFitRigidNode = slicer.cli.run(brainsFit, None, parametersRigid)
      waitCount = 0
      while self.cliBrainsFitRigidNode.GetStatusString() != 'Completed' and waitCount < 200:
        self.delayDisplay( "Register Day 2 CT to Day 1 CT using rigid registration... %d" % waitCount )
        waitCount += 1
      self.delayDisplay("Register Day 2 CT to Day 1 CT using rigid registration finished",self.delayMs)

      self.assertTrue( self.cliBrainsFitRigidNode.GetStatusString() == 'Completed' )

      # Register Day 2 CT to Day 1 CT using BSpline registration
      # TODO: Move this to workflow 2
      # if self.performDeformableRegistration:
        # self.delayDisplay("Register Day 2 CT to Day 1 CT using BSpline registration.\n  It may take a few minutes...",self.delayMs)

        # parametersBSpline = {}
        # parametersBSpline["fixedVolume"] = day1CT.GetID()
        # parametersBSpline["movingVolume"] = day2CT.GetID()
        # parametersBSpline["initialTransform"] = linearTransform.GetID()

        # bsplineTransform = slicer.vtkMRMLBSplineTransformNode()
        # bsplineTransform.SetName(self.transformDay2ToDay1BSplineName)
        # slicer.mrmlScene.AddNode( bsplineTransform )
        # parametersBSpline["bsplineTransform"] = bsplineTransform.GetID()

        # parametersBSpline["useBSpline"] = True

        # self.cliBrainsFitBSplineNode = None
        # self.cliBrainsFitBSplineNode = slicer.cli.run(brainsFit, None, parametersBSpline)
        # waitCount = 0
        # while self.cliBrainsFitBSplineNode.GetStatusString() != 'Completed' and waitCount < 600:
          # self.delayDisplay( "Register Day 2 CT to Day 1 CT using BSpline registration... %d" % waitCount )
          # waitCount += 1
        # self.delayDisplay("Register Day 2 CT to Day 1 CT using BSpline registration finished",self.delayMs)

        # self.assertTrue( self.cliBrainsFitBSplineNode.GetStatusString() == 'Completed' )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_05_TransformDay2DoseVolume(self):
    try:
      self.delayDisplay("Apply Day 2 CT to Day 1 CT rigid transform to Day 2 Dose",self.delayMs)

      cloneNodePlugin = slicer.qSlicerSubjectHierarchyPluginHandler.instance().pluginByName('CloneNode')
      self.assertTrue( cloneNodePlugin != None)

      day2DoseSH = slicer.util.getNode(self.day2DoseName + slicer.vtkMRMLSubjectHierarchyConstants.GetSubjectHierarchyNodeNamePostfix())
      day2DoseCloneSH = cloneNodePlugin.cloneSubjectHierarchyNode(day2DoseSH, self.day2DoseRigidName)
      transformDay2ToDay1Rigid = slicer.util.getNode(self.transformDay2ToDay1RigidName)
      day2DoseCloneSH.TransformBranch(transformDay2ToDay1Rigid)

      self.assertTrue( day2DoseCloneSH.GetAssociatedNode().GetParentTransformNode() == transformDay2ToDay1Rigid )
      self.assertTrue( day2DoseCloneSH.GetAssociatedNode().GetDisplayNode() )

      # Transform Day 2 Dose using Day 2 CT to Day 1 CT BSpline transform
      # if self.performDeformableRegistration:
        # self.delayDisplay("Apply Day 2 CT to Day 1 CT BSpline transform to Day 2 Dose",self.delayMs)

        # day2DoseCloneBSplineSH = cloneNodePlugin.cloneSubjectHierarchyNode(day2DoseSH, self.day2DoseRigidName)
        # transformDay2ToDay1BSpline = slicer.util.getNode(self.transformDay2ToDay1BSplineName)
        # day2DoseCloneBSplineSH.TransformBranch(transformDay2ToDay1BSpline)

        # self.assertTrue( day2DoseCloneBSplineSH.GetAssociatedNode().GetParentTransformNode() == transformDay2ToDay1BSpline )
        # self.assertTrue( day2DoseCloneBSplineSH.GetAssociatedNode().GetDisplayNode() )

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_06_ComputeGamma(self):
    self.delayDisplay('Computing gamma difference for day 1 does and registered day 2 dose',self.delayMs)

    try:
      scene = slicer.mrmlScene
      slicer.util.selectModule('DoseComparison')
      numOfVolumeNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') )
      
      gammaWidget = slicer.modules.dosecomparison.widgetRepresentation()
      referenceDoseVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=gammaWidget, name='MRMLNodeComboBox_ReferenceDoseVolume')[0]
      compareDoseVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=gammaWidget, name='MRMLNodeComboBox_CompareDoseVolume')[0]
      gammaVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=gammaWidget, name='MRMLNodeComboBox_GammaVolume')[0]
      applyButton = slicer.util.findChildren(widget=gammaWidget, className='QPushButton', name='Apply')[0]

      # Create output gamma volume
      gammaVolume = gammaVolumeMrmlNodeCombobox.addNode()
      self.gammaVolumeName = gammaVolume.GetName()

      # Compute gamma for day 1 dose and registered day 2 dose
      day1Dose = slicer.util.getNode(self.day1DoseName)
      referenceDoseVolumeMrmlNodeCombobox.setCurrentNodeID(day1Dose.GetID())
      day2DoseRigid = slicer.util.getNode(self.day2DoseRigidName)
      compareDoseVolumeMrmlNodeCombobox.setCurrentNodeID(day2DoseRigid.GetID())
      applyButton.click()

      self.assertTrue( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ) == numOfVolumeNodesBeforeLoad + 1 )
      
      self.TestUtility_ShowVolumes(gammaVolume)
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def DoseAccumulationUtility_SelectDoseVolume(self, volumeName, checked):
    logic = slicer.modules.doseaccumulation.logic()
    paramNode = logic.GetDoseAccumulationNode()
    if paramNode == None:
      logging.error('Failed to get dose accumulation parameter set node')
      return
    volumeNode = slicer.util.getNode(volumeName)
    if volumeNode == None:
      return
    if checked:
      paramNode.AddSelectedInputVolumeNode(volumeNode)
    else:
      paramNode.RemoveSelectedInputVolumeNode(volumeNode)

  #------------------------------------------------------------------------------
  def TestSection_07_AccumulateDose(self):
    try:
      slicer.util.selectModule('DoseAccumulation')
      doseAccumulationWidget = slicer.modules.doseaccumulation.widgetRepresentation()
      doseAccumulationLogic = slicer.modules.doseaccumulation.logic()

      day1Dose = slicer.util.getNode(self.day1DoseName)
      inputFrame = slicer.util.findChildren(widget=doseAccumulationWidget, className='ctkCollapsibleButton', text='Input')[0]
      referenceVolumeCombobox = slicer.util.findChildren(widget=inputFrame, className='qMRMLNodeComboBox')[0]
      referenceVolumeCombobox.setCurrentNode(day1Dose)

      outputFrame = slicer.util.findChildren(widget=doseAccumulationWidget, className='ctkCollapsibleButton', text='Output')[0]
      outputMrmlNodeCombobox = slicer.util.findChildren(widget=outputFrame, className='qMRMLNodeComboBox')[0]

      self.assertTrue( referenceVolumeCombobox != None )
      self.assertTrue( outputMrmlNodeCombobox != None )

      # Create output volumes
      accumulatedDoseUnregistered = slicer.vtkMRMLScalarVolumeNode()
      accumulatedDoseUnregistered.SetName(self.accumulatedDoseUnregisteredName)
      slicer.mrmlScene.AddNode( accumulatedDoseUnregistered )

      accumulatedDoseRigid = slicer.vtkMRMLScalarVolumeNode()
      accumulatedDoseRigid.SetName(self.accumulatedDoseRigidName)
      slicer.mrmlScene.AddNode( accumulatedDoseRigid )

      doseAccumulationWidget.updateWidgetFromMRML()

      self.DoseAccumulationUtility_SelectDoseVolume(self.day1DoseName, True)

      # Accumulate Day 1 dose and untransformed Day 2 dose
      self.delayDisplay("Accumulate Day 1 dose with unregistered Day 2 dose",self.delayMs)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseName, True)
      outputMrmlNodeCombobox.setCurrentNode(accumulatedDoseUnregistered)
      doseAccumulationLogic.AccumulateDoseVolumes()

      self.assertTrue( accumulatedDoseUnregistered.GetImageData() )
      self.delayDisplay("Accumulate Day 1 dose with unregistered Day 2 dose finished",self.delayMs)

      self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with rigid registration",self.delayMs)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseName, False)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseRigidName, True)
      outputMrmlNodeCombobox.setCurrentNode(accumulatedDoseRigid)
      doseAccumulationLogic.AccumulateDoseVolumes()
      
      self.assertTrue( accumulatedDoseRigid.GetImageData() )

      self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with rigid registration finished",self.delayMs)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseRigidName, False)

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_08_ComputeDvh(self):
    try:
      scene = slicer.mrmlScene
      slicer.util.selectModule('DoseVolumeHistogram')
      dvhWidget = slicer.modules.dosevolumehistogram.widgetRepresentation()

      numOfDoubleArrayNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLDoubleArrayNode*') )

      computeDvhButton = slicer.util.findChildren(widget=dvhWidget, text='Compute DVH')[0]
      segmentsCollapsibleGroupBox = slicer.util.findChildren(widget=dvhWidget, name='CollapsibleGroupBox_Segments')[0]
      segmentsTable = slicer.util.findChildren(widget=dvhWidget, name='SegmentsTableView')[0]
      mrmlNodeComboboxes = slicer.util.findChildren(widget=dvhWidget, className='qMRMLNodeComboBox')
      for mrmlNodeCombobox in mrmlNodeComboboxes:
        if 'vtkMRMLScalarVolumeNode' in mrmlNodeCombobox.nodeTypes:
          doseVolumeNodeCombobox = mrmlNodeCombobox
        elif 'vtkMRMLSegmentationNode' in mrmlNodeCombobox.nodeTypes:
          segmentationNodeCombobox = mrmlNodeCombobox
        elif 'vtkMRMLChartNode' in mrmlNodeCombobox.nodeTypes:
          chartNodeCombobox = mrmlNodeCombobox

      segmentsCollapsibleGroupBox.collapsed = False
      selectedSegmentIDs = ['PTV1']
      segmentsTable.setSelectedSegmentIDs(selectedSegmentIDs)

      # Compute DVH using untransformed accumulated dose
      self.delayDisplay("Compute DVH of accumulated dose (unregistered)",self.delayMs)
      accumulatedDoseUnregistered = slicer.util.getNode(self.accumulatedDoseUnregisteredName)
      doseVolumeNodeCombobox.setCurrentNode(accumulatedDoseUnregistered)
      computeDvhButton.click()
      
      # Compute DVH using accumulated dose volume that used Day 2 dose after rigid transform
      self.delayDisplay("Compute DVH of accumulated dose (rigid registration)",self.delayMs)
      accumulatedDoseRigid = slicer.util.getNode(self.accumulatedDoseRigidName)
      doseVolumeNodeCombobox.setCurrentNode(accumulatedDoseRigid)
      computeDvhButton.click()

      self.assertTrue( len( slicer.util.getNodes('vtkMRMLDoubleArrayNode*') ) == numOfDoubleArrayNodesBeforeLoad + 2 )

      # Create chart and show plots
      chartNodeCombobox.addNode()
      self.delayDisplay("Show DVH charts",self.delayMs)
      showAllCheckbox = slicer.util.findChildren(widget=dvhWidget, text='Show/hide all', className='qCheckBox')[0]
      self.assertTrue( showAllCheckbox )
      showAllCheckbox.checked = True

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  # Mandatory functions
  #------------------------------------------------------------------------------
  def setUp(self, clearScene=True):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    if clearScene:
      slicer.mrmlScene.Clear(0)

    self.delayMs = 700

    # TODO: Comment out
    # logFile = open('d:/pyTestLog.txt', 'a')
    # logFile.write(repr(slicer.modules.IGRTWorkflow_SelfTest) + '\n')
    # logFile.write(repr(slicer.modules.dicomrtimportexport) + '\n')
    # logFile.write(repr(slicer.modules.segmentations) + '\n')
    # logFile.close()

    self.moduleName = "IGRTWorkflow_SelfTest"

  #------------------------------------------------------------------------------
  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()

    self.test_IGRTWorkflow_SelfTest_FullTest()
      
  #------------------------------------------------------------------------------
  # Utility functions
  #------------------------------------------------------------------------------
  def TestUtility_ClearDatabase(self):
    self.delayDisplay("Clear database",self.delayMs)

    initialized = slicer.dicomDatabase.initializeDatabase()

    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )

    self.delayDisplay("Restoring original database directory",self.delayMs)
    if self.originalDatabaseDirectory:
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      dicomWidget.onDatabaseDirectoryChanged(self.originalDatabaseDirectory)

  #------------------------------------------------------------------------------
  def TestUtility_ShowVolumes(self, back=None, fore=None):
    try:
      self.assertTrue( back != None )

      layoutManager = slicer.app.layoutManager()
      layoutManager.setLayout(3)
      
      sliceWidgetNames = ['Red', 'Green', 'Yellow']
      for sliceWidgetName in sliceWidgetNames:
        slice = layoutManager.sliceWidget(sliceWidgetName)
        sliceLogic = slice.sliceLogic()
        compositeNode = sliceLogic.GetSliceCompositeNode()
        compositeNode.SetBackgroundVolumeID(back.GetID())
        if fore != None:
          compositeNode.SetForegroundVolumeID(fore.GetID())
          compositeNode.SetForegroundOpacity(0.5)
        else:
          compositeNode.SetForegroundVolumeID(None)

    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further")

  #------------------------------------------------------------------------------
  def clickAndDrag(self,widget,button='Left',start=(10,10),end=(10,40),steps=20,modifiers=[]):
    """Send synthetic mouse events to the specified widget (qMRMLSliceWidget or qMRMLThreeDView)
    button : "Left", "Middle", "Right", or "None"
    start, end : window coordinates for action
    steps : number of steps to move in
    modifiers : list containing zero or more of "Shift" or "Control"
    """
    style = widget.interactorStyle()
    interactor = style.GetInteractor()
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
      interactor.SetShiftKey(1)
    if 'Control' in modifiers:
      interactor.SetControlKey(1)
    interactor.SetEventPosition(*start)
    down()
    for step in xrange(steps):
      frac = float(step)/steps
      x = int(start[0] + frac*(end[0]-start[0]))
      y = int(start[1] + frac*(end[1]-start[1]))
      interactor.SetEventPosition(x,y)
      style.OnMouseMove()
    up()
    interactor.SetShiftKey(0)
    interactor.SetControlKey(0)

#
# -----------------------------------------------------------------------------
# IGRTWorkflow_SelfTest_Widget
# -----------------------------------------------------------------------------
#

class IGRTWorkflow_SelfTestWidget(ScriptedLoadableModuleWidget):

  #------------------------------------------------------------------------------
  def setup(self):
    self.developerMode = True
    ScriptedLoadableModuleWidget.setup(self)

    # Buttons to perform parts of the test
    self.layout.addStretch(1)

    # Create groupbox for workflow I
    self.workflow1Groupbox = qt.QGroupBox("Evaluate isocenter shifting")
    self.workflow1GroupboxLayout = qt.QVBoxLayout()
    # self.workflow1GroupboxLayout.setAlignment(qt.Qt.AlignRight)

    # Perform workflow button
    self.performWorkflow1Button = qt.QPushButton("Perform workflow")
    self.performWorkflow1Button.toolTip = "Performs whole workflow (Evaluate isocenter shifting)"
    self.performWorkflow1Button.name = "IGRTWorkflow_SelfTest_LoadData"
    self.workflow1GroupboxLayout.addWidget(self.performWorkflow1Button)
    self.performWorkflow1Button.connect('clicked()', self.onPerformWorkflow1)
    
    # Load data button
    self.loadDataButton = qt.QPushButton("Load data")
    self.loadDataButton.setMaximumWidth(200)
    self.loadDataButton.toolTip = "Download (if necessary), import and load input data."
    self.loadDataButton.name = "IGRTWorkflow_SelfTest_LoadData"
    self.workflow1GroupboxLayout.addWidget(self.loadDataButton)
    self.loadDataButton.connect('clicked()', self.onLoadData)

    # Generate isodose button
    self.generateIsodoseButton = qt.QPushButton("Generate isodose")
    self.generateIsodoseButton.setMaximumWidth(200)
    self.generateIsodoseButton.toolTip = "Generate isodose lines for both dose volumes"
    self.generateIsodoseButton.name = "IGRTWorkflow_SelfTest_LoadData"
    self.workflow1GroupboxLayout.addWidget(self.generateIsodoseButton)
    self.generateIsodoseButton.connect('clicked()', self.onGenerateIsodose)

    # Register button
    self.registerButton = qt.QPushButton("Register")
    self.registerButton.setMaximumWidth(200)
    self.registerButton.toolTip = "Register Day 2 CT to Day 1 CT. Data needs to be loaded!"
    self.registerButton.name = "IGRTWorkflow_SelfTest_Register"
    self.workflow1GroupboxLayout.addWidget(self.registerButton)
    self.registerButton.connect('clicked()', self.onRegister)

    # Transform button
    self.transformButton = qt.QPushButton("Transform")
    self.transformButton.setMaximumWidth(200)
    self.transformButton.toolTip = "Transform Day 2 dose volume using the resulting transformations. Loading and registering steps are needed to be run!"
    self.transformButton.name = "IGRTWorkflow_SelfTest_Transform"
    self.workflow1GroupboxLayout.addWidget(self.transformButton)
    self.transformButton.connect('clicked()', self.onTransform)

    # Compute gamma button
    self.computeGammaButton = qt.QPushButton("Compare dose distributions")
    self.computeGammaButton.setMaximumWidth(200)
    self.computeGammaButton.toolTip = "Compute gamma dose difference for the day 1 dose and the registered day 2 dose."
    self.computeGammaButton.name = "IGRTWorkflow_SelfTest_ComputeDvh"
    self.workflow1GroupboxLayout.addWidget(self.computeGammaButton)
    self.computeGammaButton.connect('clicked()', self.onComputeGamma)

    # Accumulate dose button
    self.accumulateDoseButton = qt.QPushButton("Accumulate dose")
    self.accumulateDoseButton.setMaximumWidth(200)
    self.accumulateDoseButton.toolTip = "Accumulate doses using all the Day 2 variants. All previous steps are needed to be run!"
    self.accumulateDoseButton.name = "IGRTWorkflow_SelfTest_AccumulateDose"
    self.workflow1GroupboxLayout.addWidget(self.accumulateDoseButton)
    self.accumulateDoseButton.connect('clicked()', self.onAccumulateDose)
    
    # Compute DVH button
    self.computeDvhButton = qt.QPushButton("Compute DVH")
    self.computeDvhButton.setMaximumWidth(200)
    self.computeDvhButton.toolTip = "Compute DVH on the accumulated doses. All previous steps are needed to be run!"
    self.computeDvhButton.name = "IGRTWorkflow_SelfTest_ComputeDvh"
    self.workflow1GroupboxLayout.addWidget(self.computeDvhButton)
    self.computeDvhButton.connect('clicked()', self.onComputeDvh)

    self.workflow1Groupbox.setLayout(self.workflow1GroupboxLayout)
    self.layout.addWidget(self.workflow1Groupbox)
    
    # Add vertical spacer
    self.layout.addStretch(4)

  #------------------------------------------------------------------------------
  def onReload(self,moduleName="IGRTWorkflow_SelfTest"):
    """Generic reload method for any scripted module.
    ModuleWizard will subsitute correct default moduleName.
    """
    globals()[moduleName] = slicer.util.reloadScriptedModule(moduleName)

  #------------------------------------------------------------------------------
  def onReloadAndTest(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.runTest()

  #------------------------------------------------------------------------------
  def onPerformWorkflow1(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp()

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.test_IGRTWorkflow_SelfTest_FullTest()
    
  #------------------------------------------------------------------------------
  def onLoadData(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp()

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_01A_OpenTempDatabase()
    tester.TestSection_01B_DownloadDay1Data()
    tester.TestSection_01C_ImportDay1Study()
    tester.TestSection_01D_SelectLoadablesAndLoad()
    tester.TestSection_01E_LoadDay2Data()
    tester.TestSection_01F_AddDay2DataToSubjectHierarchy()
    tester.TestSection_01G_SetDisplayOptions()

  #------------------------------------------------------------------------------
  def onGenerateIsodose(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_03A_ComputeIsodoseForDay1()
    tester.TestSection_03B_ComputeIsodoseForDay2()

  #------------------------------------------------------------------------------
  def onRegister(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_04_RegisterDay2CTToDay1CT()

  #------------------------------------------------------------------------------
  def onTransform(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_05_TransformDay2DoseVolume()

  #------------------------------------------------------------------------------
  def onComputeGamma(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_06_ComputeGamma()

  #------------------------------------------------------------------------------
  def onAccumulateDose(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_07_AccumulateDose()

  #------------------------------------------------------------------------------
  def onComputeDvh(self,moduleName="IGRTWorkflow_SelfTest"):
    self.onReload()
    evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
    tester = eval(evalString)
    tester.setUp(clearScene=False)

    if not hasattr(tester,'setupPathsAndNamesDone'):
      tester.TestSection_00_SetupPathsAndNames()
    tester.TestSection_08_ComputeDvh()

#
# -----------------------------------------------------------------------------
# IGRTWorkflow_SelfTestLogic
# -----------------------------------------------------------------------------
#

class IGRTWorkflow_SelfTestLogic(ScriptedLoadableModuleLogic):
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
