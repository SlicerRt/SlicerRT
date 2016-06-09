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
    return "<html>Smooth selected segment. Avilable methods:<ul><li>Median: removes small details while keeps smooth contours mostly unchanged.</li><li>Opening: removes extrusions smaller than the specified kernel size.</li><li>Closing: fills sharp corners and holes smaller than the specified kernel size.</li><li>Gaussian: smoothes all contours, tends to shrink the segment.</li></ul></html>"

  def activate(self):
    self.updateGUIFromMRML()

  def deactivate(self):
    pass

  def setupOptionsFrame(self):

    self.methodSelectorComboBox = qt.QComboBox()
    self.methodSelectorComboBox.addItem("Median", MEDIAN)
    self.methodSelectorComboBox.addItem("Opening (remove extrusions)", MORPHOLOGICAL_OPENING)
    self.methodSelectorComboBox.addItem("Closing (fill holes)", MORPHOLOGICAL_CLOSING)
    self.methodSelectorComboBox.addItem("Gaussian", GAUSSIAN)

    self.scriptedEffect.addLabeledOptionsWidget("Smoothing method:", self.methodSelectorComboBox)
   
    self.kernelSizeMmSpinBox = slicer.qMRMLSpinBox()
    self.kernelSizeMmSpinBox.setToolTip("Diameter of the neighborhood that will be considered around each voxel. Higher value makes smoothing stronger (more details are suppressed).")
    self.kernelSizeMmSpinBox.quantity = "length"
    self.kernelSizeMmSpinBox.value = 3
    self.kernelSizeMmLabel = self.scriptedEffect.addLabeledOptionsWidget("Kernel size:", self.kernelSizeMmSpinBox)

    self.gaussianStandardDeviationMmSpinBox = slicer.qMRMLSpinBox()
    self.gaussianStandardDeviationMmSpinBox.setToolTip("Standard deviation of the Gaussian smoothing filter coefficients. Higher value makes smoothing stronger (more details are suppressed).")
    self.gaussianStandardDeviationMmSpinBox.quantity = "length"
    self.gaussianStandardDeviationMmSpinBox.value = 3
    self.gaussianStandardDeviationMmLabel = self.scriptedEffect.addLabeledOptionsWidget("Standard deviation:", self.gaussianStandardDeviationMmSpinBox)

    
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Apply smoothing to selected segment")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.methodSelectorComboBox.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
    self.kernelSizeMmSpinBox.connect("valueChanged(double)", self.updateMRMLFromGUI)
    self.gaussianStandardDeviationMmSpinBox.connect("valueChanged(double)", self.updateMRMLFromGUI)
    self.applyButton.connect('clicked()', self.onApply)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameter("SmoothingMethod", MEDIAN)
    self.scriptedEffect.setParameter("KernelSizeMm", 3)
    self.scriptedEffect.setParameter("GaussianStandardDeviationMm", 3)    

  def updateParameterWidgetsVisibility(self):
    methodIndex = self.methodSelectorComboBox.currentIndex
    smoothingMethod = self.methodSelectorComboBox.itemData(methodIndex)
    self.kernelSizeMmLabel.setVisible(smoothingMethod!=GAUSSIAN)
    self.kernelSizeMmSpinBox.setVisible(smoothingMethod!=GAUSSIAN)    
    self.gaussianStandardDeviationMmLabel.setVisible(smoothingMethod==GAUSSIAN)
    self.gaussianStandardDeviationMmSpinBox.setVisible(smoothingMethod==GAUSSIAN)    
    
  def updateGUIFromMRML(self):
    methodIndex = self.methodSelectorComboBox.findData(self.scriptedEffect.parameter("SmoothingMethod"))
    wasBlocked = self.methodSelectorComboBox.blockSignals(True)
    self.methodSelectorComboBox.setCurrentIndex(methodIndex)
    self.methodSelectorComboBox.blockSignals(wasBlocked)
   
    wasBlocked = self.kernelSizeMmSpinBox.blockSignals(True)
    self.kernelSizeMmSpinBox.value = self.scriptedEffect.doubleParameter("KernelSizeMm")
    self.kernelSizeMmSpinBox.blockSignals(wasBlocked)
  
    wasBlocked = self.gaussianStandardDeviationMmSpinBox.blockSignals(True)
    self.gaussianStandardDeviationMmSpinBox.value = self.scriptedEffect.doubleParameter("GaussianStandardDeviationMm")
    self.gaussianStandardDeviationMmSpinBox.blockSignals(wasBlocked)
    
    self.updateParameterWidgetsVisibility()

  def updateMRMLFromGUI(self):
    methodIndex = self.methodSelectorComboBox.currentIndex
    smoothingMethod = self.methodSelectorComboBox.itemData(methodIndex)
    self.scriptedEffect.setParameter("SmoothingMethod", smoothingMethod)
    self.scriptedEffect.setParameter("KernelSizeMm", self.kernelSizeMmSpinBox.value)
    self.scriptedEffect.setParameter("GaussianStandardDeviationMm", self.gaussianStandardDeviationMmSpinBox.value)
    
    self.updateParameterWidgetsVisibility()
    
  #
  # Effect specific methods (the above ones are the API methods to override)
  #
  def editedLabelmapChanged(self):
    #self.updateGUIFromMRML()
    pass

  def onApply(self):
    try:
      # Get master volume image data
      import vtkSegmentationCorePython
      # Get edited labelmap
      editedLabelmap = self.scriptedEffect.editedLabelmap()

      selectedSegmentLabelmap = self.scriptedEffect.selectedSegmentLabelmap()
      selectedSegmentLabelmapSpacing = [1.0, 1.0, 1.0]
      if editedLabelmap:
        selectedSegmentLabelmapSpacing = editedLabelmap.GetSpacing()

      # Save state for undo
      #TODO:
      #self.undoRedo.saveState()
      
      # This can be a long operation - indicate it to the user
      qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
      try:      
      
        smoothingMethod = self.scriptedEffect.parameter("SmoothingMethod")
        if smoothingMethod == GAUSSIAN:
          maxValue = 255
        
          thresh = vtk.vtkImageThreshold()
          thresh.SetInputData(selectedSegmentLabelmap)
          thresh.ThresholdByLower(0)
          thresh.SetInValue(0)
          thresh.SetOutValue(maxValue)
          thresh.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)

          standardDeviationMm = self.scriptedEffect.doubleParameter("GaussianStandardDeviationMm")
          gaussianFilter = vtk.vtkImageGaussianSmooth()
          gaussianFilter.SetInputConnection(thresh.GetOutputPort())
          gaussianFilter.SetStandardDeviation(standardDeviationMm)
          gaussianFilter.SetRadiusFactor(4)
          
          thresh2 = vtk.vtkImageThreshold()
          thresh2.SetInputConnection(gaussianFilter.GetOutputPort())
          thresh2.ThresholdByUpper(maxValue/2)
          thresh2.SetInValue(1)
          thresh2.SetOutValue(0)
          thresh2.SetOutputScalarType(selectedSegmentLabelmap.GetScalarType())
          thresh2.Update()
          editedLabelmap.DeepCopy(thresh2.GetOutput())
          
        else:
          kernelSizeMm = self.scriptedEffect.doubleParameter("KernelSizeMm")
          # size rounded to nearest odd number. If kernel size is even then image gets shifted.
          kernelSizePixel = [int(round((kernelSizeMm / selectedSegmentLabelmapSpacing[componentIndex]+1)/2)*2-1) for componentIndex in range(3)]
          
          if smoothingMethod == MEDIAN:
            # Median filter does not require a particular label value
            smoothingFilter = vtk.vtkImageMedian3D()
            smoothingFilter.SetInputData(selectedSegmentLabelmap)

          else:
            # We need to know exactly the value of the segment voxels, apply threshold to make force the selected label value
            labelValue = 1
            thresh = vtk.vtkImageThreshold()
            thresh.SetInputData(selectedSegmentLabelmap)
            thresh.ThresholdByLower(0)
            thresh.SetInValue(0)
            thresh.SetOutValue(labelValue)
            thresh.SetOutputScalarType(selectedSegmentLabelmap.GetScalarType())
            
            smoothingFilter = vtk.vtkImageOpenClose3D()
            smoothingFilter.SetInputConnection(thresh.GetOutputPort())
            if smoothingMethod == MORPHOLOGICAL_OPENING:
              smoothingFilter.SetOpenValue(labelValue)
              smoothingFilter.SetCloseValue(0)
            else: # must be smoothingMethod == MORPHOLOGICAL_CLOSING:
              smoothingFilter.SetOpenValue(0)
              smoothingFilter.SetCloseValue(labelValue)

          smoothingFilter.SetKernelSize(kernelSizePixel[0],kernelSizePixel[1],kernelSizePixel[2])
          smoothingFilter.Update()
          editedLabelmap.DeepCopy(smoothingFilter.GetOutput())

      except:
        qt.QApplication.restoreOverrideCursor()
        raise

      qt.QApplication.restoreOverrideCursor() 
          
    except IndexError:
      logging.error('apply: Failed to apply smoothing')
      pass

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.setEditedLabelmapApplyModeToSet()
    self.scriptedEffect.setEditedLabelmapApplyExtentToWholeExtent()
    self.scriptedEffect.apply()

MEDIAN = 'MEDIAN'
GAUSSIAN = 'GAUSSIAN'
MORPHOLOGICAL_OPENING = 'MORPHOLOGICAL_OPENING'
MORPHOLOGICAL_CLOSING = 'MORPHOLOGICAL_CLOSING'
