import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorGrowCutEffect(AbstractScriptedSegmentEditorEffect):
  """ GrowCutEffect is an Effect that implements the
      GrowCut segmentation in segment editor
  """

  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'GrowCut'
    # Indicates that effect does not operate on one segment, but the whole segmentation.
    # This means that while this effect is active, no segment can be selected
    scriptedEffect.perSegment = False
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(SegmentEditorGrowCutEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/GrowCut.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()

  def helpText(self):
    return "Use this tool to apply grow cut segmentation.\n\n Paint as many different classes as you want on separate segments "
           "using the standard drawing tools. Each segment in the segmentation is used as an input class.\n"
           "To run segmentation correctly, you need to supply a minimum or two class labels.\n"
           "If input class segments overlap, then a segment shown below another in the table will overwrite its values for the GrowCut operation!"

  def setupOptionsFrame(self):
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("GrowCut selected segment")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.applyButton.connect('clicked()', self.onApply)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def onApply(self):
    # Get master volume image data
    masterImageData = vtkSegmentationCore.vtkOrientedImageData()
    self.scriptedEffect.masterVolumeImageData(masterImageData)

    # Check validity of master volume: whether scalar type is supported
    if masterImageData.GetScalarType() != vtk.VTK_SHORT:
      logging.warning("GrowCut is attempted with image type '{0}', which is not supported!".format(masterImageData.GetScalarTypeAsString()))
      if not slicer.util.confirmOkCancel("Current image type is '{0}', which is not supported by GrowCut.\n\n"
                                         "If the segmentation result is not satisfacory, then cast the master image "
                                         "to 'short' type (using Cast Scalar Volume module"
                                         .format(masterImageData.GetScalarTypeAsString()), windowTitle='Segment editor'):
        logging.warning('GrowCut is cancelled by the user')
        return

    slicer.util.showStatusMessage("Running GrowCut...", 2000)
    #TODO:
    #self.logic.undoRedo = self.undoRedo
    self.growCut()
    slicer.util.showStatusMessage("GrowCut Finished", 2000)

  def growCut(self):
    # Get master volume image data
    masterImageData = vtkSegmentationCore.vtkOrientedImageData()
    self.scriptedEffect.masterVolumeImageData(masterImageData)
    # Get segmentation
    import vtkSegmentationCore
    import vtkSlicerSegmentationsModuleMRML
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()

    # Generate merged labelmap as input to GrowCut
    mergedImage = vtk.vtkImageData()
    mergedImageToWorldMatrix = vtk.vtkMatrix4x4()
    segmentationNode.GenerateMergedLabelmap(mergedImage, mergedImageToWorldMatrix, masterImageData)

    # Perform grow cut
    growCutFilter = vtkITK.vtkITKGrowCutSegmentationImageFilter()
    growCutFilter.SetInputData( 0, masterImageData )
    growCutFilter.SetInputData( 1, mergedImage )
    #TODO: This call sets an empty image for the optional "previous segmentation" input at line 188 in EditorLib/GrowCut.py
    #growCutFilter.SetInputConnection( 2, thresh.GetOutputPort() )

    #TODO: These are magic numbers inherited from EditorLib/GrowCut.py
    objectSize = 5.
    contrastNoiseRatio = 0.8
    priorStrength = 0.003
    segmented = 2
    conversion = 1000

    spacing = mergedImage.GetSpacing()
    voxelVolume = reduce(lambda x,y: x*y, spacing)
    voxelAmount = objectSize / voxelVolume
    voxelNumber = round(voxelAmount) * conversion

    cubeRoot = 1./3.
    oSize = int(round(pow(voxelNumber,cubeRoot)))

    growCutFilter.SetObjectSize( oSize )
    growCutFilter.SetContrastNoiseRatio( contrastNoiseRatio )
    growCutFilter.SetPriorSegmentConfidence( priorStrength )
    growCutFilter.Update()

    outputLabelmap = vtkSegmentationCore.vtkOrientedImageData()
    outputLabelmap.DeepCopy( growCutFilter.GetOutput() )

    # Write output segmentation results in segments
    import vtkSlicerSegmentationsModuleLogic
    segmentIDs = vtk.vtkStringArray()
    segmentationNode.GetSegmentation().GetSegmentIDs(segmentIDs)
    for index in segmentIDs.GetNumberOfValues():
      segmentID = segmentIDs.GetValue(index)
      segment = segmentationNode.GetSegmentation().GetSegment(segmentID)

      # Get label corresponding to segment in merged labelmap (and so GrowCut output)
      tagFound = segment.GetTag(vtkSlicerSegmentationsModuleMRML.vtkMRMLSegmentationDisplayNode.GetColorIndexTag(), colorIndexStr);
      if not tagFound:
        logging.error('Failed to apply GrowCut result on segment ' + segmentID)
        continue
      colorIndex = int(colorIndexStr)

      # Get only the label of the current segment from the output image
      thresh = vtk.vtkImageThreshold()
      thresh.ReplaceInOn()
      thresh.ReplaceOutOn()
      thresh.SetInValue(1)
      thresh.SetOutValue(0)
      thresh.ThresholdBetween(colorIndex, colorIndex);
      thresh.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
      thresh.SetInputData(outputLabelmap)
      thresh.Update()

      # Write label to segment
      newSegmentLabelmap = vtkSegmentationCore.vtkOrientedImageData()
      newSegmentLabelmap.ShallowCopy(thresh.GetOutput())
      newSegmentLabelmap.SetGeometryFromImageToWorldMatrix(mergedImageToWorldMatrix)
      vtkSlicerSegmentationsModuleLogic.vtkSlicerSegmentationsModuleLogic.SetBinaryLabelmapToSegment(newSegmentLabelmap, segmentationNode, segmentID)
