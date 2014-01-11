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
    super(PlmRegisterPlugin,self).create(registrationState)

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

    # Pre-alignment
    buttonLayout = qt.QHBoxLayout()
    self.prealignmentComboBox = qt.QComboBox()
    self.prealignmentComboBox.insertItem (1, "None")
    self.prealignmentComboBox.insertItem (2, "Local")
    self.prealignmentComboBox.insertItem (3, "Global")
    self.prealignmentComboBox.setToolTip("Pre-alignment method\nEither none, global, or local")
    buttonLayout.addWidget(self.prealignmentComboBox)
    plmRegisterFormLayout.addRow("Pre-alignment:", buttonLayout)

    # Stage 1
    self.stage1Box = qt.QGroupBox("Stage 1")
    self.stage1Box.setLayout(qt.QFormLayout())

    # Iterations
    buttonLayout = qt.QHBoxLayout()
    self.iterationsLineEdit = qt.QLineEdit()
    self.iterationsLineEdit.setText('20')
    self.iterationsLineEdit.setToolTip( "Maximum number of iterations" ) 
    buttonLayout.addWidget(self.iterationsLineEdit)
    self.stage1Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.hybridSubsampling = qt.QLineEdit()
    self.hybridSubsampling.setText('2 2 2')
    self.hybridSubsampling.setToolTip( "Subsampling rate" ) 
    buttonLayout.addWidget(self.hybridSubsampling)
    self.stage1Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.hybridGridSize = qt.QLineEdit()
    self.hybridGridSize.setText('50')
    self.hybridGridSize.setToolTip( "Set B-spline grid spacing" )
    buttonLayout.addWidget(self.hybridGridSize)
    self.stage1Box.layout().addRow("Grid Size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.regularizationLineEdit = qt.QLineEdit()
    self.regularizationLineEdit.setText('0.001')
    self.regularizationLineEdit.setToolTip("Set Regularization penalty term")
    buttonLayout.addWidget(self.regularizationLineEdit)
    self.stage1Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.landmarkPenaltyLineEdit = qt.QLineEdit()
    self.landmarkPenaltyLineEdit.setText('0.001')
    self.landmarkPenaltyLineEdit.setToolTip("Set landmark distance penalty term")
    buttonLayout.addWidget(self.landmarkPenaltyLineEdit)
    self.stage1Box.layout().addRow("LandmarkPenalty:", buttonLayout)

    plmRegisterFormLayout.addRow(self.stage1Box)

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
    
    print ("Gonna SetMRMLScene")
    reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkSlicerPlastimatchPyModuleLogic()
    reg.SetMRMLScene(slicer.mrmlScene)
    print ("Did SetMRMLScene")

    state = self.registrationState()
    reg.SetFixedImageID(state.fixed.GetID())
    reg.SetMovingImageID(state.moving.GetID())
    reg.SetOutputVolumeID(state.transformed.GetID())

    ## This was shamelessly stolen from AffinePlugin.py
    ## It converts the landmarks from MRML Markup format
    ## into vtkPoints format
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
        print("%s: %s" % (volumeNode.GetName(), str(point)))

    reg.SetFixedLandmarks(points[state.fixed])
    reg.SetMovingLandmarks(points[state.moving])
    print ("Gonna AddStage()")
    reg.AddStage()
    reg.SetPar("xform","bspline")
    reg.SetPar("res",str(self.hybridSubsampling.text))
    reg.SetPar("iterations",str(self.iterationsLineEdit.text))
    #reg.SetPar("img_out","c:/tmp/gcs.nrrd")

    print ("Gonna RunRegistration()")
    print("prealignment strategy is %s" % str(self.prealignmentComboBox.currentText))
    print("subsampling is %s" % str(self.hybridSubsampling.text))

    reg.RunRegistration ()
    print ("Did RunRegistration()")

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
