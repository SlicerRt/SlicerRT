import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorThresholdEffect(AbstractScriptedSegmentEditorEffect):
  """ ThresholdEffect is an Effect implementing the global threshold
      operation in the segment editor
      
      This is also an example for scripted effects, and some methods have no
      function. The methods that are not needed (i.e. the default implementation in
      qSlicerSegmentEditorAbstractEffect is satisfactory) can simply be omitted.
  """
  
  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedPlugin):
    scriptedPlugin.name = 'Threshold'
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedPlugin)

    # Effect-specific members
    self.timer = qt.QTimer()
    self.previewState = 0
    self.previewStep = 1
    self.previewSteps = 5
    self.timer.connect('timeout()', self.preview)
    
    self.previewPipelines = {}
    self.setupPreviewDisplay()

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt
    clonedEffect = qSlicerSegmentationsEditorEffectsPythonQt.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(SegmentEditorThresholdEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Threshold.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()
    
  def helpText(self):
    return "Set segment based on threshold range.\nNote: this replaces the current values."

  def activate(self):
    self.setupPreviewDisplay()
    self.timer.start(200)

  def deactivate(self):
    self.timer.stop()

  def setupOptionsFrame(self):
    self.thresholdLabel = qt.QLabel("Threshold Range:")
    self.thresholdLabel.setToolTip("Set the range of the background values that should be labeled.")
    self.scriptedEffect.addOptionsWidget(self.thresholdLabel)

    self.threshold = ctk.ctkRangeWidget()
    self.threshold.spinBoxAlignment = qt.Qt.AlignTop
    self.threshold.singleStep = 0.01
    # Set min/max based on current range
    lo, hi = 0, 0
    success = self.scriptedEffect.masterVolumeScalarRange(lo, hi)
    if success:
      self.threshold.minimum, self.threshold.maximum = lo, hi
      self.threshold.singleStep = (hi - lo) / 1000.
    self.scriptedEffect.addOptionsWidget(self.threshold)

    self.useForPainting = qt.QPushButton("Use For Paint")
    self.useForPainting.setToolTip("Transfer the current threshold settings to be used for labeling operations such as Paint and Draw.")
    self.scriptedEffect.addOptionsWidget(self.useForPainting)

    self.apply = qt.QPushButton("Apply")
    self.apply.objectName = self.__class__.__name__ + 'Apply'
    self.apply.setToolTip("Apply current threshold settings to the label map.")
    self.scriptedEffect.addOptionsWidget(self.apply)

    self.useForPainting.connect('clicked()', self.onUseForPainting)
    self.threshold.connect('valuesChanged(double,double)', self.onThresholdValuesChanged)
    self.apply.connect('clicked()', self.apply)

  def apply(self):
    try:
      # Get master volume image data
      import vtkSegmentationCore
      masterImageData = vtkSegmentationCore.vtkOrientedImageData()
      self.scriptedEffect.masterVolumeImageData(masterImageData)
      # Get edited labelmap
      editedLabelmap = self.scriptedEffect.parameterSetNode().GetEditedLabelmap()
      # Get parameters
      min = self.scriptedEffect.doubleParameter("MinimumThreshold")
      max = self.scriptedEffect.doubleParameter("MaximumThreshold")

      # Perform thresholding
      thresh = vtk.vtkImageThreshold()
      thresh.SetInputData(masterImageData)
      thresh.ThresholdBetween(min, max)
      thresh.SetInValue(1)
      thresh.SetOutValue(0)
      thresh.SetOutputScalarType(editedLabelmap.GetScalarType())
      thresh.Update()
      editedLabelmap.DeepCopy(thresh.GetOutput())
    except IndexError:
      logging.error('apply: Failed to threshold master volume!')
      pass

    # De-select effect
    #TODO: Needed?
    #self.defaultEffect()

  def editedLabelmapChanged(self):
    pass # For the sake of example

  def masterVolumeNodeChanged(self):
    lo, hi = 0, 0
    success = self.scriptedEffect.masterVolumeScalarRange(lo, hi)
    if success:
      self.threshold.minimum, self.threshold.maximum = lo, hi
      self.threshold.singleStep = (hi - lo) / 1000.

  def layoutChanged(self):
    self.setupPreviewDisplay()

  def processInteractionEvents(self, callerInteractor, eventId, viewWidget):
    pass # For the sake of example

  def processViewNodeEvents(self, callerViewNode, eventId, viewWidget):
    pass # For the sake of example

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameter("MinimumThreshold", 0)
    self.scriptedEffect.setParameter("MinimumThreshold", 100)

  def updateGUIFromMRML(self):
    self.threshold.blockSignals(True)
    self.threshold.setMinimumValue(self.scriptedEffect.doubleParameter("MinimumThreshold"))
    self.threshold.setMaximumValue(self.scriptedEffect.doubleParameter("MaximumThreshold"))
    self.threshold.blockSignals(False)

  def updateMRMLFromGUI(self):
    self.scriptedEffect.setParameter("MinimumThreshold", self.threshold.minimumValue)
    self.scriptedEffect.setParameter("MinimumThreshold", self.threshold.maximumValue)
 
  #
  # Effect specific methods (the above ones are the API methods to override)
  #
  def onThresholdValuesChanged(self,min,max):
    self.updateMRMLFromGUI()

  def onUseForPainting(self):
    pass #TODO:
    # disableState = self.parameterNode.GetDisableModifiedEvent()
    # self.parameterNode.SetDisableModifiedEvent(1)
    # self.parameterNode.SetParameter( "LabelEffect,paintThreshold", "1" )
    # self.parameterNode.SetParameter( "LabelEffect,paintThresholdMin", str(self.threshold.minimumValue) )
    # self.parameterNode.SetParameter( "LabelEffect,paintThresholdMax", str(self.threshold.maximumValue) )
    # self.parameterNode.SetDisableModifiedEvent(disableState)
    # self.parameterNode.InvokePendingModifiedEvent()

  def setupPreviewDisplay(self):
    # Clear previous pipelines before setting up the new ones
    self.previewPipelines = {}

    # Add a pipeline for each 2D slice view
    layoutManager = slicer.app.layoutManager()
    for sliceViewName in layoutManager.sliceViewNames():
      sliceWidget = layoutManager.sliceWidget(sliceViewName)
      renderer = self.scriptedEffect.renderer(sliceWidget)
      if renderer is None:
        logging.error("setupPreviewDisplay: Failed to get renderer!")
        continue

      # Create pipeline
      pipeline = PreviewPipeline()
      self.previewPipelines[sliceWidget] = pipeline

      # Add actor
      renderer.AddActor2D(pipeline.actor)
      self.scriptedEffect.addActor(sliceWidget, pipeline.actor)

  def preview(self):
    opacity = 0.5 + self.previewState / (2. * self.previewSteps)
    min = self.scriptedEffect.doubleParameter("MinimumThreshold")
    max = self.scriptedEffect.doubleParameter("MaximumThreshold")
    
    # Get color of edited segment
    import vtkSlicerSegmentationsModuleMRML
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    displayNode = segmentationNode.GetDisplayNode()
    if displayNode is None:
      logging.error("preview: Invalid segmentation display node!")
      color = [0.5,0.5,0.5,1]
    segmentID = self.scriptedEffect.parameterSetNode().GetSelectedSegmentID()
    colorVtk = displayNode.GetSegmentColor(segmentID)
    color = [colorVtk.GetX(), colorVtk.GetY(), colorVtk.GetZ(), displayNode.GetSegmentOpacity2DFill(segmentID)]
    r,g,b,a = color

    # Set values to pipelines
    for sliceWidget, pipeline in self.previewPipelines:
      pipeline.lookupTable.SetTableValue(1,  r, g, b,  a)
      pipeline.actor.VisibilityOn()
      sliceWidget.sliceView().scheduleRender()
    
    self.previewState += self.previewStep
    if self.previewState >= self.previewSteps:
      self.previewStep = -1
    if self.previewState <= 0:
      self.previewStep = 1

class PreviewPipeline:
  """ Visualization objects and pipeline for each slice view
  """
  
  def __init__(self):
    self.lookupTable = vtk.vtkLookupTable()
    self.lookupTable.SetRampToLinear()
    self.lookupTable.SetNumberOfTableValues(2)
    self.lookupTable.SetTableRange(0, 1)
    self.lookupTable.SetTableValue(0,  0, 0, 0,  0)
    self.colorMapper = vtk.vtkImageMapToRGBA()
    self.colorMapper.SetOutputFormatToRGBA()
    self.colorMapper.SetLookupTable(self.lookupTable)
    self.thresholdFilter = vtk.vtkImageThreshold()
    self.thresholdFilter.SetInValue(1)
    self.thresholdFilter.SetOutValue(0)
    self.thresholdFilter.SetOutputScalarTypeToUnsignedChar()

    # Feedback actor
    self.mapper = vtk.vtkImageMapper()
    self.dummyImage = vtk.vtkImageData()
    self.dummyImage.AllocateScalars(vtk.VTK_UNSIGNED_INT, 1)
    self.mapper.SetInputData(self.dummyImage)
    self.actor = vtk.vtkActor2D()
    self.actor.VisibilityOff()
    self.actor.SetMapper(self.mapper)
    self.mapper.SetColorWindow(255)
    self.mapper.SetColorLevel(128)

    # Setup pipeline
    self.colorMapper.SetInputConnection(self.thresholdFilter.GetOutputPort())
    self.mapper.SetInputConnection(self.colorMapper.GetOutputPort())
