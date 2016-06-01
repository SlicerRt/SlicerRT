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
    self.scriptedEffect.addLabeledOptionsWidget("Smoothing method:", self.methodSelectorComboBox)

    self.medianKernelSizeRadiusPixelSpinBox = qt.QDoubleSpinBox()
    self.medianKernelSizeRadiusPixelSpinBox.setToolTip("Kernel size in physical unit (typically mm)")
    self.medianKernelSizeRadiusPixelSpinBox.minimum = 1
    self.medianKernelSizeRadiusPixelSpinBox.maximum = 10
    self.medianKernelSizeRadiusPixelSpinBox.singleStep = 1
    self.medianKernelSizeRadiusPixelSpinBox.value = 3
    self.medianKernelSizeRadiusPixelLabel = self.scriptedEffect.addLabeledOptionsWidget("Kernel size:", self.medianKernelSizeRadiusPixelSpinBox)

    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Apply current threshold settings to the label map.")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    # itemIndex = self.probePositionSelector.findData(probePositionId)
    # self.probePositionSelector.setItemData(itemIndex, probePositionPreset['description'], qt.Qt.ToolTipRole)

    self.methodSelectorComboBox.connect("currentIndexChanged(int)", self.onMethodChanged)
    self.applyButton.connect('clicked()', self.onApply)

  def onMethodChanged(self, methodIndex):
    methodName = self.probePositionSelector.getItemData(methodIndex)
    self.scriptedEffect.setParameter("SmoothingMethod", methodName)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameter("SmoothingMethod", MEDIAN)
    self.scriptedEffect.setParameter("MedianKernelSizeMm", 3)

  def updateGUIFromMRML(self):
    smoothingMethod = self.scriptedEffect.stringParameter("SmoothingMethod")
    methodIndex = self.probePositionSelector.findData(smoothingMethod)
    wasBlocked = self.methodSelectorComboBox.blockSignals(True)
    self.methodSelectorComboBox.setCurrentIndex(methodIndex)
    self.methodSelectorComboBox.blockSignals(wasBlocked)

    editedLabelmapSpacing = [1.0, 1.0, 1.0]
    editedLabelmap = self.scriptedEffect.editedLabelmap()
    if editedLabelmap:
      editedLabelmap.GetSpacing(editedLabelmapSpacing)

    wasBlocked = self.medianKernelSizeRadiusPixelSpinBox.blockSignals(True)
    minimumSpacing = min(editedLabelmapSpacing)
    self.medianKernelSizeRadiusPixelSpinBox.minimum = minimumSpacing
    self.medianKernelSizeRadiusPixelSpinBox.maximum = minimumSpacing*50
    self.medianKernelSizeRadiusPixelSpinBox.singleStep = minimumSpacing
    medianKernelSizeRoundedToMultipleOfMinimumSpacing = round(self.medianKernelSizeRadiusPixelSpinBox.value/minimumSpacing)*minimumSpacing
    if medianKernelSizeRoundedToMultipleOfMinimumSpacing < minimumSpacing:
      medianKernelSizeRoundedToMultipleOfMinimumSpacing = minimumSpacing
    self.medianKernelSizeRadiusPixelSpinBox.value = medianKernelSizeRoundedToMultipleOfMinimumSpacing
    self.medianKernelSizeRadiusPixelSpinBox.blockSignals(wasBlocked)

    self.medianKernelSizeRadiusPixelLabel.setVisible(smoothingMethod==MEDIAN)
    self.medianKernelSizeRadiusPixelSpinBox.setVisible(smoothingMethod==MEDIAN)


  def updateMRMLFromGUI(self):
    methodIndex = self.methodSelectorComboBox.currentIndex()
    smoothingMethod = self.methodSelectorComboBox.itemData(methodIndex)
    self.scriptedEffect.setParameter("SmoothingMethod", smoothingMethod)
    self.scriptedEffect.setParameter("MedianKernelSizeMm", self.medianKernelSizeRadiusPixelSpinBox.value)
 
  #
  # Effect specific methods (the above ones are the API methods to override)
  #
  def editedLabelmapChanged(self):
    self.updateGUIFromMRML()

  def onApply(self):
    try:
      # Get master volume image data
      import vtkSegmentationCorePython
      masterImageData = self.scriptedEffect.masterVolumeImageData()
      # Get edited labelmap
      editedLabelmap = self.scriptedEffect.editedLabelmap()
      #originalImageToWorldMatrix = vtk.vtkMatrix4x4()
      #editedLabelmap.GetImageToWorldMatrix(originalImageToWorldMatrix)
      #originalExtent = editedLabelmap.GetExtent()

      editedLabelmapSpacing = [1.0, 1.0, 1.0]
      editedLabelmap = self.scriptedEffect.editedLabelmap()
      if editedLabelmap:
        editedLabelmap.GetSpacing(editedLabelmapSpacing)
      medianKernelSizeMm = self.scriptedEffect.doubleParameter("MedianKernelSizeMm")
      medianKernelSizePixel = [round(medianKernelSizeMm / editedLabelmapSpacing[componentIndex]) * editedLabelmapSpacing[componentIndex] for componentIndex in range(3)]

      # Save state for undo
      #TODO:
      #self.undoRedo.saveState()

      # Perform thresholding
      medianFilter = vtk.vtkImageMedian3D()
      medianFilter.SetInputData(masterImageData)
      medianFilter.SetKernelSize(medianKernelSizePixel)
      medianFilter.Update()
      editedLabelmap.DeepCopy(medianFilter.GetOutput())
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
