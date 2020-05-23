import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
from DICOMLib import DICOMUtils
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
      slicer.igrt_selftest_instance = self # For debugging

      # Check for modules
      self.assertIsNotNone( slicer.modules.dicomrtimportexport )
      self.assertIsNotNone( slicer.modules.subjecthierarchy )
      self.assertIsNotNone( slicer.modules.segmentations )
      self.assertIsNotNone( slicer.modules.isodose )
      self.assertIsNotNone( slicer.modules.brainsfit )
      self.assertIsNotNone( slicer.modules.dosecomparison )
      self.assertIsNotNone( slicer.modules.doseaccumulation )
      self.assertIsNotNone( slicer.modules.dosevolumehistogram )

      self.TestSection_00_SetupPathsAndNames()
      self.TestSection_01_LoadDicomData()
      self.TestSection_02A_LoadDay2Data()
      self.TestSection_02B_AddDay2DataToSubjectHierarchy()
      self.TestSection_02C_SetDisplayOptions()
      self.TestSection_03A_ComputeIsodoseForDay1()
      self.TestSection_03B_ComputeIsodoseForDay2()
      self.TestSection_04_RegisterDay2CTToDay1CT()
      self.TestSection_05_TransformDay2DoseVolume()
      self.TestSection_06_ComputeGamma()
      self.TestSection_07_AccumulateDose()
      self.TestSection_08_ComputeDvh()

    except Exception as e:
      pass

  #------------------------------------------------------------------------------
  def TestSection_00_SetupPathsAndNames(self):
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
    self.dicomZipFileUrl = 'http://slicer.kitware.com/midas3/download/item/101019/EclipseEntPhantomRtData.zip'
    self.dicomZipFilePath = IGRTWorkflow_SelfTestDir + '/EclipseEntDicomRt.zip'
    self.expectedNumOfFilesInDicomDataDir = 142
    self.tempDir = IGRTWorkflow_SelfTestDir + '/Temp'

    self.day1CTName = '2: ENT IMRT'
    self.day1DoseName = '5: RTDOSE'
    self.day1StructureSetName = '3: RTSTRUCT: ENT'
    self.day1PlanName = '4: RTPLAN: BRAI1'
    self.day1IsodosesName = '5: RTDOSE: BRAI1_IsodoseSurfaces'
    self.day2CTName = '2_ENT_IMRT_Day2'
    self.day2DoseName = '5_RTDOSE_Day2'
    self.day2IsodosesName = self.day2DoseName + '_IsodoseSurfaces'
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
  def TestSection_01_LoadDicomData(self):
    try:
      # Open test database and empty it
      with DICOMUtils.TemporaryDICOMDatabase(self.dicomDatabaseDir) as db:
        self.assertTrue( db.isOpen )
        self.assertEqual( slicer.dicomDatabase, db)

        # Download, unzip, import, and load data. Verify selected plugins and loaded nodes.
        selectedPlugins = { 'Scalar Volume':2, 'RT':3 }
        loadedNodes = { 'vtkMRMLScalarVolumeNode':3, \
                        'vtkMRMLSegmentationNode':1, \
                        'vtkMRMLRTBeamNode':6 }
        with DICOMUtils.LoadDICOMFilesToDatabase( \
            self.dicomZipFileUrl, self.dicomZipFilePath, \
            self.dicomDataDir, self.expectedNumOfFilesInDicomDataDir, \
            {}, loadedNodes) as success:
          self.assertTrue(success)

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)

  #------------------------------------------------------------------------------
  def TestSection_02A_LoadDay2Data(self):
    try:
      import urllib.request, urllib.parse, urllib.error
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
          urllib.request.urlretrieve(url, filePath)
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
      self.assertEqual( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ), numOfScalarVolumeNodesBeforeLoad + 2 )

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_02B_AddDay2DataToSubjectHierarchy(self):
    try:
      shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      self.assertIsNotNone( shNode )

      # Get patient item
      day1CT = slicer.util.getNode(self.day1CTName)
      ct1ShItemID = shNode.GetItemByDataNode(day1CT)
      self.assertIsNotNone( ct1ShItemID )
      study1ItemID = shNode.GetItemParent(ct1ShItemID)
      self.assertIsNotNone( study1ItemID )
      patientItemID = shNode.GetItemParent(study1ItemID)
      self.assertIsNotNone( patientItemID )

      # Add new study for the day 2 data
      study2ItemID = shNode.CreateStudyItem(patientItemID, 'Day2')
      shNode.SetItemUID(study2ItemID, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), 'Day2Study_UID')

      # Add dose unit attributes to the new study item
      doseUnitName = shNode.GetItemAttribute(study1ItemID, self.doseUnitNameAttributeName)
      doseUnitValue = shNode.GetItemAttribute(study1ItemID, self.doseUnitValueAttributeName)
      shNode.SetItemAttribute(study1ItemID, self.doseUnitNameAttributeName, doseUnitName)
      shNode.SetItemAttribute(study1ItemID, self.doseUnitValueAttributeName, doseUnitValue)

      # Add day 2 CT series
      day2CT = slicer.util.getNode(self.day2CTName)
      day2CTShItemID = shNode.CreateItem(study2ItemID, day2CT)
      shNode.SetItemUID(day2CTShItemID, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), 'Day2CT_UID')

      # Set dose attributes for day 2 dose
      day2Dose = slicer.util.getNode(self.day2DoseName)
      day2Dose.SetAttribute(self.doseVolumeIdentifierAttributeName,'1')

      # Add day 2 dose series
      doseShItemID = shNode.CreateItem(study2ItemID, day2Dose)
      shNode.SetItemUID(doseShItemID, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), 'Day2Dose_UID')

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_02C_SetDisplayOptions(self):
    self.delayDisplay('Setting display options for loaded data',self.delayMs)

    try:
      shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      self.assertIsNotNone( shNode )

      # Set default dose color map and W/L
      defaultDoseColorTable = slicer.util.getNode('Dose_ColorTable')
      day2Dose = slicer.util.getNode(self.day2DoseName)
      day2Dose.GetDisplayNode().SetAndObserveColorNodeID(defaultDoseColorTable.GetID())

      day1Dose = slicer.util.getNode(self.day1DoseName)
      day1DoseShItemID = shNode.GetItemByDataNode(day1Dose)
      doseUnitValue = shNode.GetAttributeFromItemAncestor(day1DoseShItemID, self.doseUnitValueAttributeName, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMLevelStudy())
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

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_03A_ComputeIsodoseForDay1(self):
    self.delayDisplay('Computing isodose for day 1',self.delayMs)

    try:
      # Hide beams
      shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      self.assertIsNotNone( shNode )
      planShItemID = shNode.GetItemByDataNode(slicer.util.getNode(self.day1PlanName))
      self.assertIsNotNone( planShItemID )
      shNode.SetDisplayVisibilityForBranch(planShItemID, 0)

      slicer.util.selectModule('Isodose')
      numOfModelNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLModelNode*') )

      isodoseWidget = slicer.modules.isodose.widgetRepresentation()
      doseVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=isodoseWidget, className='qMRMLNodeComboBox', name='MRMLNodeComboBox_DoseVolume')[0]
      applyButton = slicer.util.findChildren(widget=isodoseWidget, className='QPushButton', text='Generate isodose')[0]

      # Compute isodose for day 1 dose
      day1Dose = slicer.util.getNode(self.day1DoseName)
      doseVolumeMrmlNodeCombobox.setCurrentNodeID(day1Dose.GetID())
      applyButton.click()

      self.assertEqual( len( slicer.util.getNodes('vtkMRMLModelNode*') ), numOfModelNodesBeforeLoad + 6 )

      # Show day 1 isodose
      day1CT = slicer.util.getNode(self.day1CTName)
      self.TestUtility_ShowVolumes(day1CT)

      day1DoseShItemID = shNode.GetItemByDataNode(day1Dose)
      day1IsodoseShItemID = shNode.GetItemChildWithName(day1DoseShItemID, self.day1IsodosesName)
      self.assertIsNotNone( day1IsodoseShItemID )
      shNode.SetDisplayVisibilityForBranch(day1IsodoseShItemID, 1)

      self.delayDisplay('Show day 1 isodose lines',self.delayMs)

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_03B_ComputeIsodoseForDay2(self):
    self.delayDisplay('Computing isodose for day 2',self.delayMs)

    try:
      scene = slicer.mrmlScene
      slicer.util.selectModule('Isodose')
      modelNodeCollection = slicer.mrmlScene.GetNodesByClass('vtkMRMLModelNode')
      modelNodeCollection.UnRegister(None)
      numOfModelNodesBeforeLoad = modelNodeCollection.GetNumberOfItems()

      isodoseWidget = slicer.modules.isodose.widgetRepresentation()
      doseVolumeMrmlNodeCombobox = slicer.util.findChildren(widget=isodoseWidget, className='qMRMLNodeComboBox', name='MRMLNodeComboBox_DoseVolume')[0]
      applyButton = slicer.util.findChildren(widget=isodoseWidget, className='QPushButton', text='Generate isodose')[0]

      # Compute isodose for day 2 dose
      day2Dose = slicer.util.getNode(self.day2DoseName)
      doseVolumeMrmlNodeCombobox.setCurrentNodeID(day2Dose.GetID())
      applyButton.click()

      modelNodeCollection = slicer.mrmlScene.GetNodesByClass('vtkMRMLModelNode')
      modelNodeCollection.UnRegister(None)
      self.assertEqual( modelNodeCollection.GetNumberOfItems(), numOfModelNodesBeforeLoad + 6 )

      # Show day 2 isodose
      shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      self.assertIsNotNone( shNode )
      day1Dose = slicer.util.getNode(self.day1DoseName)
      day1DoseShItemID = shNode.GetItemByDataNode(day1Dose)
      day1IsodoseShItemID = shNode.GetItemChildWithName(day1DoseShItemID, self.day1IsodosesName)
      shNode.SetDisplayVisibilityForBranch(day1IsodoseShItemID, 0)

      day2CT = slicer.util.getNode(self.day2CTName)
      self.TestUtility_ShowVolumes(day2CT)

      day2DoseShItemID = shNode.GetItemByDataNode(day2Dose)
      day2IsodoseShItemID = shNode.GetItemChildWithName(day2DoseShItemID, self.day2IsodosesName)
      self.assertIsNotNone( day2IsodoseShItemID )
      shNode.SetDisplayVisibilityForBranch(day2IsodoseShItemID, 1)

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_04_RegisterDay2CTToDay1CT(self):
    try:
      scene = slicer.mrmlScene
      slicer.util.selectModule('BRAINSFit')
      brainsFit = slicer.modules.brainsfit

      # Hide isodose
      shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      self.assertIsNotNone( shNode )
      day2Dose = slicer.util.getNode(self.day2DoseName)
      day2DoseShItemID = shNode.GetItemByDataNode(day2Dose)
      day2IsodoseShItemID = shNode.GetItemChildWithName(day2DoseShItemID, self.day2IsodosesName)
      shNode.SetDisplayVisibilityForBranch(day2IsodoseShItemID, 0)

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
        self.delayDisplay( "Register Day 2 CT to Day 1 CT using rigid registration... %d" % waitCount, self.delayMs )
        waitCount += 1
      self.delayDisplay("Register Day 2 CT to Day 1 CT using rigid registration finished",self.delayMs)

      self.assertEqual( self.cliBrainsFitRigidNode.GetStatusString(), 'Completed' )

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

        # self.assertEqual( self.cliBrainsFitBSplineNode.GetStatusString(), 'Completed' )

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_05_TransformDay2DoseVolume(self):
    try:
      self.delayDisplay("Apply Day 2 CT to Day 1 CT rigid transform to Day 2 Dose",self.delayMs)

      shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
      self.assertIsNotNone( shNode )

      day2DoseShItemID = shNode.GetItemByDataNode(slicer.util.getNode(self.day2DoseName))
      day2DoseCloneShItemID = slicer.vtkSlicerSubjectHierarchyModuleLogic.CloneSubjectHierarchyItem(shNode, day2DoseShItemID, self.day2DoseRigidName)
      transformDay2ToDay1Rigid = slicer.util.getNode(self.transformDay2ToDay1RigidName)
      slicer.vtkSlicerSubjectHierarchyModuleLogic.TransformBranch(shNode, day2DoseCloneShItemID, transformDay2ToDay1Rigid)

      day2DoseRigidVolumeNode = shNode.GetItemDataNode(day2DoseCloneShItemID)
      self.assertIsNotNone( day2DoseRigidVolumeNode )
      self.assertEqual( day2DoseRigidVolumeNode.GetParentTransformNode(), transformDay2ToDay1Rigid )
      self.assertIsNotNone( day2DoseRigidVolumeNode.GetDisplayNode() )

      # Transform Day 2 Dose using Day 2 CT to Day 1 CT BSpline transform
      # if self.performDeformableRegistration:
        # self.delayDisplay("Apply Day 2 CT to Day 1 CT BSpline transform to Day 2 Dose",self.delayMs)

        # day2DoseCloneBSplineSH = cloneNodePlugin.cloneSubjectHierarchyNode(day2DoseSH, self.day2DoseRigidName)
        # transformDay2ToDay1BSpline = slicer.util.getNode(self.transformDay2ToDay1BSplineName)
        # slicer.vtkSlicerSubjectHierarchyModuleLogic.TransformBranch(day2DoseCloneBSplineSH, transformDay2ToDay1BSpline)

        # self.assertEqual( day2DoseCloneBSplineSH.GetAssociatedNode().GetParentTransformNode(), transformDay2ToDay1BSpline )
        # self.assertIsNotNone( day2DoseCloneBSplineSH.GetAssociatedNode().GetDisplayNode() )

    except Exception as e:
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
      applyButton = slicer.util.findChildren(widget=gammaWidget, className='QPushButton', text='Calculate gamma')[0]

      # Create output gamma volume
      gammaVolume = gammaVolumeMrmlNodeCombobox.addNode()
      self.gammaVolumeName = gammaVolume.GetName()

      # Compute gamma for day 1 dose and registered day 2 dose
      day1Dose = slicer.util.getNode(self.day1DoseName)
      referenceDoseVolumeMrmlNodeCombobox.setCurrentNodeID(day1Dose.GetID())
      day2DoseRigid = slicer.util.getNode(self.day2DoseRigidName)
      compareDoseVolumeMrmlNodeCombobox.setCurrentNodeID(day2DoseRigid.GetID())
      applyButton.click()

      self.assertEqual( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ), numOfVolumeNodesBeforeLoad + 1 )

      self.TestUtility_ShowVolumes(gammaVolume)

    except Exception as e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Test caused exception!\n' + str(e),self.delayMs*2)
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def DoseAccumulationUtility_SelectDoseVolume(self, volumeName, checked):
    volumeNode = slicer.util.getNode(volumeName)
    if volumeNode == None:
      return
    if checked:
      self.doseAccumulationParamNode.AddSelectedInputVolumeNode(volumeNode)
    else:
      self.doseAccumulationParamNode.RemoveSelectedInputVolumeNode(volumeNode)

  #------------------------------------------------------------------------------
  def TestSection_07_AccumulateDose(self):
    try:
      slicer.util.selectModule('DoseAccumulation')
      doseAccumulationWidget = slicer.modules.doseaccumulation.widgetRepresentation()
      doseAccumulationLogic = slicer.modules.doseaccumulation.logic()

      self.doseAccumulationParamNode = slicer.util.getNode('DoseAccumulation')
      self.assertIsNotNone( self.doseAccumulationParamNode )

      day1Dose = slicer.util.getNode(self.day1DoseName)
      inputFrame = slicer.util.findChildren(widget=doseAccumulationWidget, className='ctkCollapsibleButton', text='Input')[0]
      referenceVolumeCombobox = slicer.util.findChildren(widget=inputFrame, className='qMRMLNodeComboBox')[0]
      referenceVolumeCombobox.setCurrentNode(day1Dose)

      outputFrame = slicer.util.findChildren(widget=doseAccumulationWidget, className='ctkCollapsibleButton', text='Output')[0]
      outputMrmlNodeCombobox = slicer.util.findChildren(widget=outputFrame, className='qMRMLNodeComboBox')[0]

      self.assertIsNotNone( referenceVolumeCombobox )
      self.assertIsNotNone( outputMrmlNodeCombobox )

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
      doseAccumulationLogic.AccumulateDoseVolumes(self.doseAccumulationParamNode)

      self.assertIsNotNone( accumulatedDoseUnregistered.GetImageData() )
      self.delayDisplay("Accumulate Day 1 dose with unregistered Day 2 dose finished",self.delayMs)

      self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with rigid registration",self.delayMs)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseName, False)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseRigidName, True)
      outputMrmlNodeCombobox.setCurrentNode(accumulatedDoseRigid)
      doseAccumulationLogic.AccumulateDoseVolumes(self.doseAccumulationParamNode)

      self.assertIsNotNone( accumulatedDoseRigid.GetImageData() )

      self.delayDisplay("Accumulate Day 1 dose with Day 2 dose registered with rigid registration finished",self.delayMs)
      self.DoseAccumulationUtility_SelectDoseVolume(self.day2DoseRigidName, False)

    except Exception as e:
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

      numOfTableNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLTableNode*') )

      computeDvhButton = slicer.util.findChildren(widget=dvhWidget, text='Compute DVH')[0]
      segmentsCollapsibleGroupBox = slicer.util.findChildren(widget=dvhWidget, name='CollapsibleGroupBox_Segments')[0]
      segmentsTable = slicer.util.findChildren(widget=dvhWidget, name='SegmentsTableView')[0]
      mrmlNodeComboboxes = slicer.util.findChildren(widget=dvhWidget, className='qMRMLNodeComboBox')
      for mrmlNodeCombobox in mrmlNodeComboboxes:
        if 'vtkMRMLScalarVolumeNode' in mrmlNodeCombobox.nodeTypes:
          doseVolumeNodeCombobox = mrmlNodeCombobox
        elif 'vtkMRMLSegmentationNode' in mrmlNodeCombobox.nodeTypes:
          segmentationNodeCombobox = mrmlNodeCombobox

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

      self.assertEqual( len( slicer.util.getNodes('vtkMRMLTableNode*') ), numOfTableNodesBeforeLoad + 2 )

      # Create chart and show plots
      self.delayDisplay("Show DVH charts",self.delayMs)
      showAllButton = slicer.util.findChildren(widget=dvhWidget, text='Show all', className='QPushButton')[0]
      self.assertTrue( showAllButton )
      showAllButton.click()

    except Exception as e:
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

    slicer.dicomDatabase.initializeDatabase()
    slicer.dicomDatabase.closeDatabase()
    self.assertFalse( slicer.dicomDatabase.isOpen )

    self.delayDisplay("Restoring original database directory",self.delayMs)
    if self.originalDatabaseDirectory:
      dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
      dicomWidget.onDatabaseDirectoryChanged(self.originalDatabaseDirectory)

  #------------------------------------------------------------------------------
  def TestUtility_ShowVolumes(self, back=None, fore=None):
    try:
      self.assertIsNotNone( back )

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

    except Exception as e:
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
    for step in range(steps):
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
    tester.TestSection_01_LoadDicomData()
    tester.TestSection_02A_LoadDay2Data()
    tester.TestSection_02B_AddDay2DataToSubjectHierarchy()
    tester.TestSection_02C_SetDisplayOptions()

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
