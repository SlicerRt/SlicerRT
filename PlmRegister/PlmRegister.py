import os
from __main__ import vtk, qt, ctk, slicer
import RegistrationLib


#########################################################
#
#
comment = """

  RegistrationPlugin is a superclass for code that plugs into the
  slicer LandmarkRegistration module.

  These classes are Abstract.

# TODO :
"""
#
#########################################################



#
# RegistrationPlugin
#

class PlmRegister:
  def __init__(self, parent):
    parent.title = "Plastimatch"
    parent.categories = ["Plastimatch"]
    parent.dependencies = []
    parent.contributors = ["Gregory Sharp (MGH)"]
    parent.helpText = """
    This is an example of scripted loadable module bundled in an extension.
    """
    parent.acknowledgementText = """
    This file was originally developed by Greg Sharp, Nadya Shusharina, and Paolo Zaffino and was partially funded by NIH grant 2-U54-EB005149.
    """ # replace with organization, grant and thanks.
    self.parent = parent
    self.hidden = True


class PlmRegisterPlugin(RegistrationLib.RegistrationPlugin):
#class PlmRegister:
  """ Base class for Registration plugins
  """

  #
  # generic settings that can (should) be overridden by the subclass
  #
  print ("PlmRegister begin")
  
  # displayed for the user to select the registration
  name = "Plm Registration"
  tooltip = "Uses landmarks to define B-spline matrices"

  # can be true or false
  # - True: landmarks are displayed and managed by LandmarkRegistration
  # - False: landmarks are hidden
  usesLandmarks = True

  # can be any non-negative number
  # - widget will be disabled until landmarks are defined
  landmarksNeededToEnable = 1

  # used for reloading - every concrete class should include this
  sourceFile = __file__

  # default settings
  hybridCost = "MSE"

  def __init__(self,parent=None):
    super(PlmRegisterPlugin,self).__init__(parent)

  def create(self,registrationState):
    """Make the plugin-specific user interface"""
    super(PlmRegisterPlugin,self).create()

    self.registrationState = registrationState

    #
    # Linear Registration Pane - initially hidden
    # - interface options for linear registration
    # - TODO: move registration code into separate plugins
    #
    self.collapsibleButton = ctk.ctkCollapsibleButton()
    self.collapsibleButton.text = "Plastimatch Registration"
    plmRegisterFormLayout = qt.QFormLayout()
    self.collapsibleButton.setLayout(plmRegisterFormLayout)
    self.widgets.append(self.collapsibleButton)

    ## Set up GUI ##

    # Cost function radio buttons
    buttonLayout = qt.QHBoxLayout()
    self.hybridCostButtons = {}
    self.hybridCosts = ("MSE", "MI")
    for cost in self.hybridCosts:
      self.hybridCostButtons[cost] = qt.QRadioButton()
      self.hybridCostButtons[cost].text = cost
      self.hybridCostButtons[cost].setToolTip(
        "Register using %s cost function." % cost)
      buttonLayout.addWidget(self.hybridCostButtons[cost])
      #self.hybridCostButtons[cost].connect('clicked(bool)', self.onHybridCost)
    self.hybridCostButtons[self.hybridCost].checked = True
    plmRegisterFormLayout.addRow("Cost Function:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.hybridSubsampling = qt.QLineEdit()
    self.hybridSubsampling.setText('2 2 2')
    self.hybridSubsampling.setToolTip( "Subsampling rate" ) 
    buttonLayout.addWidget(self.hybridSubsampling)
    #self.hybridSubsampling.connect('textEdited(QString)', self.onHybridSubsampling)
    plmRegisterFormLayout.addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.hybridGridSize = qt.QLineEdit()
    self.hybridGridSize.setText('50 50 50')
    self.hybridGridSize.setToolTip( "Set B-spline grid spacing" )
    buttonLayout.addWidget(self.hybridGridSize)
    #self.hybridGridSize.connect('textEdited(QString)', self.onHybridGridSize)
    plmRegisterFormLayout.addRow("Grid Size (mm):", buttonLayout)

    # Apply button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.connect('clicked(bool)', self.onApply)
    plmRegisterFormLayout.addWidget(self.applyButton)

    # Add UI elements to parent layout
    self.parent.layout().addWidget(self.collapsibleButton)

  def destroy(self):
    """Clean up"""
    super(PlmRegisterPlugin,self).destroy()

  def onLandmarkMoved(self,state):
    """Perform the linear transform using the vtkLandmarkTransform class"""
    if state.transformed.GetTransformNodeID() != state.linearTransform.GetID():
      state.transformed.SetAndObserveTransformNodeID(state.linearTransform.GetID())

    self.linearMode = "Rigid"

    # try to use user selection, but fall back if not enough points are available
    landmarkTransform = vtk.vtkLandmarkTransform()
    if self.linearMode == 'Rigid':
      landmarkTransform.SetModeToRigidBody()
    if self.linearMode == 'Similarity':
      landmarkTransform.SetModeToSimilarity()
    if self.linearMode == 'Affine':
      landmarkTransform.SetModeToAffine()
    if state.fixedFiducials.GetNumberOfFiducials() < 3:
      landmarkTransform.SetModeToRigidBody()

    points = {}
    point = [0,]*3
    for volumeNode in (state.fixed,state.moving):
      points[volumeNode] = vtk.vtkPoints()
    indices = range(state.fixedFiducials.GetNumberOfFiducials())
    fiducialLists = (state.fixedFiducials,state.movingFiducials)
    volumeNodes = (state.fixed,state.moving)
    for fiducials,volumeNode in zip(fiducialLists,volumeNodes):
      for index in indices:
        fiducials.GetNthFiducialPosition(index,point)
        points[volumeNode].InsertNextPoint(point)
    landmarkTransform.SetSourceLandmarks(points[state.moving])
    landmarkTransform.SetTargetLandmarks(points[state.fixed])
    landmarkTransform.Update()
    t = state.linearTransform
    t.SetAndObserveMatrixTransformToParent(landmarkTransform.GetMatrix())

  def onLinearTransform(self):
    pass

  def onApply(self):
    import os, sys, vtk
    import vtkSlicerPlastimatchPyModuleLogicPython
    print ("I know that you pushed the apply button!")
    self.runOneIteration()

  def runOneIteration(self):
    import os, sys, vtk
    import vtkSlicerPlastimatchPyModuleLogicPython
    print ("Hello from runOneIteration")

    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    
    reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkSlicerPlastimatchPyModuleLogic()
    reg.SetMRMLScene(slicer.mrmlScene)

    state = self.registrationState()
    # Set input/output images
    # reg.SetFixedImageID(
    #   state.volumeSelectors["Fixed"].currentNode().GetID())
    # reg.SetMovingImageID(
    #   self.parent.volumeSelectors["Moving"].currentNode().GetID())
    # reg.SetOutputVolumeID(
    #   self.parent.volumeSelectors["Transformed"].currentNode().GetID())


# Add this plugin to the dictionary of available registrations.
# Since this module may be discovered before the Editor itself,
# create the list if it doesn't already exist.
try:
  slicer.modules.registrationPlugins
except AttributeError:
  slicer.modules.registrationPlugins = {}
slicer.modules.registrationPlugins['Plm'] = PlmRegisterPlugin
print ("Finished sourcing PlmRegister.py")
