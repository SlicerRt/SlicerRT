import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorSmoothingEffect(AbstractScriptedSegmentEditorEffect):
  """ SmoothingEffect is an Effect that smoothes a selected segment
  """
  
  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)
    scriptedEffect.name = 'Smoothing'
  
  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(SegmentEditorSmoothingEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Smoothing.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()
    
  def helpText(self):
    return "Smooth a selected segment"

  def activate(self):
    pass
    
  def deactivate(self):
    pass

  def setupOptionsFrame(self):

    self.methodSelectorComboBox = qt.QComboBox()
    self.methodSelectorComboBox.addItem("Median", MEDIAN)
    self.methodSelectorComboBox.addItem("Gaussian", GAUSSIAN)
    #itemIndex = self.probePositionSelector.findData(probePositionId)
    #self.probePositionSelector.setItemData(itemIndex, probePositionPreset['description'], qt.Qt.ToolTipRole)
    #self.probePositionSelector.connect("currentIndexChanged(int)", self.onProbePositionChanged)

    self.scriptedEffect.addLabeledOptionsWidget("Smoothing method:", self.methodSelectorComboBox)

    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Apply current threshold settings to the label map.")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    #self.useForPaintButton.connect('clicked()', self.onUseForPaint)
    #self.thresholdSlider.connect('valuesChanged(double,double)', self.onThresholdValuesChanged)
    self.applyButton.connect('clicked()', self.onApply)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def editedLabelmapChanged(self):
    pass # For the sake of example

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameter("MinimumThreshold", 0.)
    self.scriptedEffect.setParameter("MaximumThreshold", 100.)

  def updateGUIFromMRML(self):
    self.thresholdSlider.blockSignals(True)
    self.thresholdSlider.setMinimumValue(self.scriptedEffect.doubleParameter("MinimumThreshold"))
    self.thresholdSlider.setMaximumValue(self.scriptedEffect.doubleParameter("MaximumThreshold"))
    self.thresholdSlider.blockSignals(False)

  def updateMRMLFromGUI(self):
    self.scriptedEffect.setParameter("MinimumThreshold", self.thresholdSlider.minimumValue)
    self.scriptedEffect.setParameter("MaximumThreshold", self.thresholdSlider.maximumValue)
 
  #
  # Effect specific methods (the above ones are the API methods to override)
  #
  def onThresholdValuesChanged(self,min,max):
    self.scriptedEffect.updateMRMLFromGUI()

  def onUseForPaint(self):    
    parameterSetNode = self.scriptedEffect.parameterSetNode()
    parameterSetNode.MasterVolumeIntensityMaskOn()
    parameterSetNode.SetMasterVolumeIntensityMaskRange(self.thresholdSlider.minimumValue, self.thresholdSlider.maximumValue)
    # Switch to paint effect
    self.scriptedEffect.selectEffect("Paint")

  def onApply(self):
    try:
      # Get master volume image data
      import vtkSegmentationCorePython
      masterImageData = self.scriptedEffect.masterVolumeImageData()
      # Get edited labelmap
      editedLabelmap = self.scriptedEffect.editedLabelmap()
      originalImageToWorldMatrix = vtk.vtkMatrix4x4()
      editedLabelmap.GetImageToWorldMatrix(originalImageToWorldMatrix)
      originalExtent = editedLabelmap.GetExtent()
      # Get parameters
      min = self.scriptedEffect.doubleParameter("MinimumThreshold")
      max = self.scriptedEffect.doubleParameter("MaximumThreshold")

      # Save state for undo
      #TODO:
      #self.undoRedo.saveState()

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

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.setEditedLabelmapApplyModeToSet()
    self.scriptedEffect.setEditedLabelmapApplyExtentToWholeExtent()
    self.scriptedEffect.apply()
    
    # De-select effect
    self.scriptedEffect.selectEffect("")

MEDIAN = 'MEDIAN'
GAUSSIAN = 'GAUSSIAN'
