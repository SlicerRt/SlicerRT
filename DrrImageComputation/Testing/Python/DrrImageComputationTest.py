import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# DrrImageComputationTest
#
# Compute DRR Image from CT volume
# by creating vtkMRMLRTPlanNode, vtkMRMLRTBeamNode, vtkMRMLDrrImageComputationNode  
#   1. Load CT volume 
#   2. Create dummy RTPlan node
#   3. Create dummy RTBeam node and add it into the dummy plan
#   4. Setup isocenter position
#   5. Setup RTBeam desired gantry and couch angles (optional)
#   6. Setup required DRR image parameters (optional)
#   7. Compute DRR image and check the results

class DrrImageComputationTest(unittest.TestCase):
  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  #------------------------------------------------------------------------------
  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_DRRComputationTest_FullTest1()

  #------------------------------------------------------------------------------
  def test_DRRComputationTest_FullTest1(self):
    # Check for modules
    self.assertIsNotNone( slicer.modules.subjecthierarchy )
    self.assertIsNotNone( slicer.modules.segmentations )
    self.assertIsNotNone( slicer.modules.beams )
    self.assertIsNotNone( slicer.modules.plastimatch_slicer_drr )
    self.assertIsNotNone( slicer.modules.drrimagecomputation )

    # Main setups
    self.TestSection_SetupPathsAndNames()
    # Load CT volume
    ctVolumeNode = self.TestSection_DownloadCtData()
    # Create dummy RTPlan and RTBeam
    rtImagePlan,rtImageBeam = self.TestSection_CreateDummyPlanAndBeam()
    # Setup isocenter
    self.TestSection_SetupIsocenterForPlan(ctVolumeNode, rtImagePlan)
    # Create DRR parameters for RTImage using beam node
    rtImageParameters = self.TestSection_CreateDrrParametersNode(rtImageBeam)
    # Compute DRR image and check results
    self.TestSection_ComputePlastimatchDrrAndCheckResults( rtImageParameters, ctVolumeNode)

    logging.info("Test finished")

  #------------------------------------------------------------------------------
  def TestSection_SetupPathsAndNames(self):
    pass
    if not os.access(slicer.app.temporaryPath, os.F_OK):
      os.mkdir(slicer.app.temporaryPath)
#
#    self.DRR_SelfTestDir = slicer.app.temporaryPath + '/DRR_SelfTestDir'
#    if not os.access(self.DRR_SelfTestDir, os.F_OK):
#      os.mkdir(self.DRR_SelfTestDir)

    self.CT_FileUrl = 'http://slicer.kitware.com/midas3/download/?items=292307,1' # CT-Chest.rnnd
    self.CT_FileName = 'CT-Chest.nrrd'
    self.CT_FileSize = 42189103

  #------------------------------------------------------------------------------
  def TestSection_DownloadCtData(self):
#    import SampleData
#    sampleDataLogic = SampleData.SampleDataLogic()
#    ctVolumeNode = sampleDataLogic.downloadCTChest()
#    self.assertIsNotNone( ctVolumeNode )
#    return ctVolumeNode
    
    try:
      import urllib.request, urllib.parse, urllib.error
      
      numOfScalarVolumeNodesBeforeLoad = len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') )
      
      ctVolumeNode = None
      downloaded = False
      filePath = slicer.app.temporaryPath + '/' + self.CT_FileName
      if not os.path.exists(filePath) or os.stat(filePath).st_size != self.CT_FileSize:
        if not downloaded:
          logging.info('Downloading DRR CT data to folder\n' + slicer.app.temporaryPath + '\n\n  It may take a few minutes...')
        logging.info('Requesting download from %s...\n' % (self.CT_FileUrl))
        newfilePath, httpMsg = urllib.request.urlretrieve(self.CT_FileUrl, filePath)
 
        self.assertTrue( filePath == newfilePath )
        
        if os.stat(filePath).st_size == self.CT_FileSize:
          downloaded = True
        else:
          logging.warning('Wrong filesize: ' + os.stat(filePath).st_size + ' bytes, instead of ' + self.CT_FileSize + ' bytes')
          raise Exception("Downloaded CT file has a wrong size!")
      else:
        downloaded = True
        logging.info('DRR CT data has been found in folder ' + slicer.app.temporaryPath)

      logging.info("Finished with download test CT data")
      
      if downloaded:
        logging.info('Loading %s...' % (os.path.split(filePath)[1]))
        ctVolumeNode = slicer.util.loadVolume(filePath)

      # Verify that the CT volume has been loaded
      self.assertEqual( len( slicer.util.getNodes('vtkMRMLScalarVolumeNode*') ), numOfScalarVolumeNodesBeforeLoad + 1 )      
      self.assertIsNotNone( ctVolumeNode )

      return ctVolumeNode

    except Exception as e:
      import traceback
      traceback.print_exc()
      logging.info('Test caused exception!\n' + str(e))
      raise Exception("Exception occurred, handled, thrown further to workflow level")

  #------------------------------------------------------------------------------
  def TestSection_CreateDummyPlanAndBeam(self):
    # Create dummy RT plan
    rtImagePlan = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTPlanNode', 'rtImagePlan')
    # Check that the plan node is valid
    self.assertIsNotNone( rtImagePlan )

    # Create dummy RT beam for DRR RT image
    rtImageBeam = slicer.mrmlScene.AddNewNodeByClass( 'vtkMRMLRTBeamNode', 'rtImageBeam')
    # Check that the beam node is valid
    self.assertIsNotNone( rtImageBeam )
    # Add beam to the plan
    rtImagePlan.AddBeam(rtImageBeam)
    # Set required beam parameters 
    rtImageBeam.SetGantryAngle(90.)
    rtImageBeam.SetCouchAngle(12.)
    logging.info('Gantry angle = 90. deg, Patient support rotation angle = 12. deg')
    return (rtImagePlan, rtImageBeam)

  #------------------------------------------------------------------------------
  def TestSection_SetupIsocenterForPlan(self, volumeNode, planNode):
    # Set and observe CT volume by the plan
    planNode.SetAndObserveReferenceVolumeNode(volumeNode)

    # Set isocenter position as a point
    planNode.SetIsocenterSpecification(planNode.ArbitraryPoint)
    isocenterPosition = [ 0., 0., -230. ]
    if (planNode.SetIsocenterPosition(isocenterPosition)):
      logging.info('Isocenter is set as (0., 0., -230.)')
    else:
      logging.warning('Unable to set isocenter')
    
  #------------------------------------------------------------------------------
  def TestSection_CreateDrrParametersNode(self, beamNode):
    # Create DRR RTImage computation node for user imager parameters
    drrParameters = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLDrrImageComputationNode', 'rtImageParams')
    # Set and observe RTImage beam by the DRR node
    drrParameters.SetAndObserveBeamNode(beamNode)
    # Set required DRR parameters
    drrParameters.SetHUThresholdBelow(150)
    drrImagerResolution = [1024, 768]; # [columns, row]
    drrImagerSpacing = [0.3, 0.3]; # in mm [columns, row]
    drrParameters.SetImagerResolution(drrImagerResolution)
    drrParameters.SetImagerSpacing(drrImagerSpacing)
    return drrParameters

  #------------------------------------------------------------------------------
  def TestSection_ComputePlastimatchDrrAndCheckResults(self, rtImageParameters, volumeNode):
    # Get DRR computation logic
    drrLogic = slicer.modules.drrimagecomputation.logic()
    # Update imager markups for the 3D view and slice views (optional)
    drrLogic.UpdateMarkupsNodes(rtImageParameters)
    # Update imager normal and view-up vectors (mandatory)
    drrLogic.UpdateNormalAndVupVectors(rtImageParameters) # REQUIRED
    # Compute DRR image
    result = drrLogic.ComputePlastimatchDRR( rtImageParameters, volumeNode)
    self.assertTrue(result)
    logging.info('DRR image has been computed!')
    # Check that image is valid
    drrImage = rtImageParameters.GetRtImageVolumeNode()
    drrImageDim = drrImage.GetImageData().GetDimensions()
    drrParametersDim = rtImageParameters.GetImagerResolution()
    self.assertTrue( drrImageDim == (drrParametersDim[0], drrParametersDim[1], 1) )
    self.assertEqual( drrImage.GetImageData().GetScalarType(), vtk.VTK_FLOAT )
    logging.info('It looks like that DRR image is correct!')

