import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *

#
# PlmVectorFieldAnalysis
#

class PlmVectorFieldAnalysis(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Vector Field Analysis"
    self.parent.categories = ["Plastimatch.DIR Validation"]
    self.parent.dependencies = []
    self.parent.contributors = ["Gregory Sharp (MGH)"]
    self.parent.helpText = """
    This is an example of scripted loadable module bundled in an extension.
    """
    self.parent.acknowledgementText = """
    This file was originally developed by Greg Sharp, Massachusetts General Hospital, and was partially funded by NIH grant 2-U54-EB005149.
    """ # replace with organization, grant and thanks.

#
# Class for bidirectional data transfer between Widget (GUI) and Logic
#
class PlmVectorFieldAnalysisWidgetLogicIO: 
  def __init__(self):
    self.minjacobian = 0
    self.maxjacobian = 0
    self.vfImageNode = False
    self.outputJacobianNode = False
    self.fixedImageNode = False # fixed image needed to set geometry info to output image..

  def SetMinJacobian(self, x=0):
    self.minjacobian = x
  
  def SetMaxJacobian(self, x=0):
    self.maxjacobian = x

#
# qPlmVectorFieldAnalysisWidget
#

class PlmVectorFieldAnalysisWidget(ScriptedLoadableModuleWidget):
  def __init__(self, parent = None):
    ScriptedLoadableModuleWidget.__init__(self, parent)
    self.LogicInputWLIO = PlmVectorFieldAnalysisWidgetLogicIO()  # for putting params from Widget to Logic
    self.logic = PlmVectorFieldAnalysisLogic()

  def setup(self):

    ScriptedLoadableModuleWidget.setup(self)

    # Instantiate and connect widgets ...

    ### Input Area
    inputCollapsibleButton = ctk.ctkCollapsibleButton()
    inputCollapsibleButton.text = "Input"
    self.layout.addWidget(inputCollapsibleButton)

    # Layout within the dummy collapsible button
    inputFormLayout = qt.QFormLayout(inputCollapsibleButton)

    # vf image (mrml input)
    self.vfMRMLSelector = slicer.qMRMLNodeComboBox()
    self.vfMRMLSelector.nodeTypes = ["vtkMRMLVectorVolumeNode"]
    self.vfMRMLSelector.selectNodeUponCreation = True
    self.vfMRMLSelector.addEnabled = False
    self.vfMRMLSelector.removeEnabled = False
    self.vfMRMLSelector.noneEnabled = True
    self.vfMRMLSelector.showHidden = False
    self.vfMRMLSelector.setMRMLScene( slicer.mrmlScene )
    self.vfMRMLSelector.setToolTip( "Pick the input to the algorithm." )
    inputFormLayout.addRow("Vector Field image: ", self.vfMRMLSelector)

    # variables
    self.minJacobianValue = 1
    self.maxJacobianValue = 1

    # vf image (directory input)
    self.vfInputDirectory = ctk.ctkDirectoryButton()
    self.vfInputDirectory.directory = qt.QDir.homePath()
    inputFormLayout.addRow("Input Directory:", self.vfInputDirectory)

    # Fixed image (for geometry info)
    self.fixedImage = slicer.qMRMLNodeComboBox()
    self.fixedImage.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.fixedImage.setMRMLScene( slicer.mrmlScene )
    self.fixedImage.selectNodeUponCreation = True
    self.fixedImage.addEnabled = False
    self.fixedImage.renameEnabled = True
    self.fixedImage.noneEnabled = True
    self.fixedImage.setToolTip( "Output image of Jacobian matrix.vtkSlicerPlastimatchModuleLogicPython" )
    inputFormLayout.addRow("Fixed image (for geometry info): ", self.fixedImage)

    # Apply Button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the algorithm."
    self.applyButton.enabled = True
    self.layout.addWidget(self.applyButton)
    
    ### Output Area
    outputCollapsibleButton = ctk.ctkCollapsibleButton()
    outputCollapsibleButton.text = "Output"
    self.layout.addWidget(outputCollapsibleButton)

    # Layout within the dummy collapsible button
    outputFormLayout = qt.QFormLayout(outputCollapsibleButton)

    # Jacobian image (mrml output)
    self.outputJacobian = slicer.qMRMLNodeComboBox()
    self.outputJacobian.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.outputJacobian.setMRMLScene( slicer.mrmlScene )
    self.outputJacobian.addEnabled = True
    self.outputJacobian.renameEnabled = True
    #self.outputJacobian.layout().addWidget(self.outputSelector)
    self.outputJacobian.setToolTip( "Output image of Jacobian matrix.vtkSlicerPlastimatchModuleLogicPython" )
    outputFormLayout.addRow("Jacobian image: ", self.outputJacobian)

    # output directory selector
#    self.outputDirectory = ctk.ctkDirectoryButton()
#    self.outputDirectory.directory = qt.QDir.homePath()
#    outputFormLayout.addRow("Output Directory: ", self.outputDirectory)
    
    # output statistics
    buttonLayout = qt.QHBoxLayout()
    self.minJacobian = qt.QLineEdit()
    self.minJacobian.setToolTip( "Minimum value of Jacobian matrix" ) 
    buttonLayout.addWidget(self.minJacobian)
    outputFormLayout.addRow("Minimum Jacobian:", buttonLayout)

    buttonLayout = qt.QHBoxLayout()
    self.maxJacobian = qt.QLineEdit()
    self.maxJacobian.setToolTip( "Maximum value of Jacobian matrix" ) 
    buttonLayout.addWidget(self.maxJacobian)
    outputFormLayout.addRow("Maximum Jacobian:", buttonLayout)    

    # connections
    self.applyButton.connect('clicked(bool)', self.onJacobianApply)
    self.outputJacobian.connect("currentNodeChanged(vtkMRMLNode*)", self.onOutputJacobianSelect)
    self.fixedImage.connect("currentNodeChanged(vtkMRMLNode*)", self.onFixedImageSelect)
    self.vfMRMLSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onvfMRMLSelect)

    # Add vertical spacer
    self.layout.addStretch(1)

  def cleanup(self):
    pass

#  def onSelect(self):
#    if self.vfMRMLSelector.currentNode():
#      self.applyButton.enabled = True
#    else:
#      self.applyButton.enabled = False

  def onOutputJacobianSelect(self):
    print "output image selected"
    self.LogicInputWLIO.outputJacobianNode = self.outputJacobian.currentNode()

  def onFixedImageSelect(self):
    print "fixed image selected"
    self.LogicInputWLIO.fixedImageNode = self.fixedImage.currentNode()

  def onvfMRMLSelect(self):
    print "VF image selected"
    self.LogicInputWLIO.vfImageNode = self.vfMRMLSelector.currentNode()

  def onJacobianApply(self):
    # placeholder for the actual calculation..
    #	
    self.LogicInputWLIO.vfImageNode = self.vfMRMLSelector.currentNode()
    self.LogicInputWLIO.fixedImageNode = self.fixedImage.currentNode()

    outputWLIO = self.logic.RunVectorFieldAnalysis( self.LogicInputWLIO )
    print "Done analysis onApply"

    self.minJacobian.setText(outputWLIO.minjacobian)
    self.maxJacobian.setText(outputWLIO.maxjacobian)

#
# PlmVectorFieldAnalysisLogic
#

class PlmVectorFieldAnalysisLogic(ScriptedLoadableModuleLogic):
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
      print('no volume node')
      return False
    if volumeNode.GetImageData() == None:
      print('no image data')
      return False
    return True

  def RunVectorFieldAnalysis(self, inputWLIO):
    import os, sys, vtk
    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    import vtkSlicerPlastimatchPyModuleLogicPython
    reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkSlicerPlastimatchPyModuleLogic()
    reg.SetMRMLScene(slicer.mrmlScene)

    print "running RunVectorFieldAnalysis"

    #Set input/output images
    reg.SetOutputVolumeID(inputWLIO.outputJacobianNode.GetID() )    
    reg.SetFixedImageID(inputWLIO.fixedImageNode.GetID() )    
    reg.SetVFImageID(inputWLIO.vfImageNode.GetID() )

    print inputWLIO.vfImageNode.GetID()
    print inputWLIO.outputJacobianNode.GetID()
    print inputWLIO.fixedImageNode.GetID()


   # Run Vector Field statistics
    print "starting RunJacobian"
    reg.RunJacobian()
    print "control went past RunJacobian"
 
    # return min/max values to caller
    outputWLIO = PlmVectorFieldAnalysisWidgetLogicIO()
    outputWLIO.SetMinJacobian(reg.GetJacobianMinString())
    outputWLIO.SetMaxJacobian(reg.GetJacobianMaxString())

  # Done

    #outputVolume.SetAndObserveImageData(jacobian.GetOutput())
    # make the output volume appear in all the slice views
    #selectionNode = slicer.app.applicationLogic().GetSelectionNode()
    #selectionNode.SetReferenceActiveVolumeID(outputVolume.GetID())
    #slicer.app.applicationLogic().PropagateVolumeSelection(0)

    #return True

    return outputWLIO

