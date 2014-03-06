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
  """ Plastimatch registration plugin class
  """
  # generic settings that can (should) be overridden by the subclass
  
  # displayed for the user to select the registration
  name = "Plastimatch"
  tooltip = "B-spline deformable registration with landmark and regularization"

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
  costFunction = "MSE"

  def __init__(self,parent=None):
    super(PlmRegisterPlugin,self).__init__(parent)

  def create(self,registrationState):
    """Make the plugin-specific user interface"""
    super(PlmRegisterPlugin,self).create(registrationState)

    self.registrationState = registrationState

    ## Set up GUI ##

    # Main collapsible bar
    self.collapsibleButton = ctk.ctkCollapsibleButton()
    self.collapsibleButton.text = "Plastimatch Registration"
    plmRegisterFormLayout = qt.QFormLayout()
    self.collapsibleButton.setLayout(plmRegisterFormLayout)
    self.widgets.append(self.collapsibleButton)

    # Cost function
    buttonLayout = qt.QHBoxLayout()
    self.costFunctionButtons = {}
    self.costFunctions = ("MSE", "MI")
    for cost in self.costFunctions:
      self.costFunctionButtons[cost] = qt.QRadioButton()
      self.costFunctionButtons[cost].text = cost
      self.costFunctionButtons[cost].setToolTip(
        "Register using %s cost function." % cost)
      buttonLayout.addWidget(self.costFunctionButtons[cost])
    self.costFunctionButtons[self.costFunction].checked = True
    plmRegisterFormLayout.addRow("Cost function:", buttonLayout)

    # Pre-alignment
    buttonLayout = qt.QHBoxLayout()
    self.prealignmentComboBox = qt.QComboBox()
    self.prealignmentComboBox.insertItem (1, "None")
    self.prealignmentComboBox.insertItem (2, "Local")
    self.prealignmentComboBox.insertItem (3, "Global")
    self.prealignmentComboBox.setToolTip(
      "Pre-alignment method\nEither none, global, or local")
    buttonLayout.addWidget(self.prealignmentComboBox)
    plmRegisterFormLayout.addRow("Pre-alignment:", buttonLayout)

    # Number of stages
    buttonLayout = qt.QHBoxLayout()
    self.numstagesComboBox = qt.QComboBox()
    self.numstagesComboBox.insertItem (1, "1")
    self.numstagesComboBox.insertItem (2, "2")
    self.numstagesComboBox.insertItem (3, "3")
    self.numstagesComboBox.setToolTip("Choose number of stages\nUse more stages for slower, more precise registration.")
    buttonLayout.addWidget(self.numstagesComboBox)
    plmRegisterFormLayout.addRow("B-Spline stages:", buttonLayout)

    # Apply button
    self.applyButton = qt.QPushButton("Click to start registration!")
    self.applyButton.setStyleSheet("background-color: #FFFF99")
    self.applyButton.connect('clicked(bool)', self.onApply)
    plmRegisterFormLayout.addWidget(self.applyButton)

    # Status label
    self.statusLabel = qt.QLabel("")
    plmRegisterFormLayout.addWidget(self.statusLabel)

    # Stage 1
    self.stage1Box = qt.QGroupBox("Stage 1")
    self.stage1Box.setLayout(qt.QFormLayout())

    # Iterations
    buttonLayout = qt.QHBoxLayout()
    self.stage1_iterationsLineEdit = qt.QLineEdit()
    self.stage1_iterationsLineEdit.setText('40')
    self.stage1_iterationsLineEdit.setToolTip( "Maximum number of iterations" ) 
    buttonLayout.addWidget(self.stage1_iterationsLineEdit)
    self.stage1Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.stage1_subsamplingLineEdit = qt.QLineEdit()
    self.stage1_subsamplingLineEdit.setText('4 4 2')
    self.stage1_subsamplingLineEdit.setToolTip( "Subsampling rate" ) 
    buttonLayout.addWidget(self.stage1_subsamplingLineEdit)
    self.stage1Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.stage1_gridSpacingLineEdit = qt.QLineEdit()
    self.stage1_gridSpacingLineEdit.setText('100')
    self.stage1_gridSpacingLineEdit.setToolTip( "Set B-spline grid spacing" )
    buttonLayout.addWidget(self.stage1_gridSpacingLineEdit)
    self.stage1Box.layout().addRow("Grid size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.stage1_regularizationLineEdit = qt.QLineEdit()
    self.stage1_regularizationLineEdit.setText('0.1')
    self.stage1_regularizationLineEdit.setToolTip(
      "Set Regularization penalty term")
    buttonLayout.addWidget(self.stage1_regularizationLineEdit)
    self.stage1Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.stage1_landmarkPenaltyLineEdit = qt.QLineEdit()
    self.stage1_landmarkPenaltyLineEdit.setText('10')
    self.stage1_landmarkPenaltyLineEdit.setToolTip(
      "Set landmark distance penalty term")
    buttonLayout.addWidget(self.stage1_landmarkPenaltyLineEdit)
    self.stage1Box.layout().addRow("Landmark penalty:", buttonLayout)

    plmRegisterFormLayout.addRow(self.stage1Box)

    # Stage 2
    self.stage2Box = qt.QGroupBox("Stage 2")
    self.stage2Box.setLayout(qt.QFormLayout())

    # Iterations
    buttonLayout = qt.QHBoxLayout()
    self.stage2_iterationsLineEdit = qt.QLineEdit()
    self.stage2_iterationsLineEdit.setText('20')
    self.stage2_iterationsLineEdit.setToolTip( "Maximum number of iterations" ) 
    buttonLayout.addWidget(self.stage2_iterationsLineEdit)
    self.stage2Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.stage2_subsamplingLineEdit = qt.QLineEdit()
    self.stage2_subsamplingLineEdit.setText('2 2 1')
    self.stage2_subsamplingLineEdit.setToolTip( "Subsampling rate" ) 
    buttonLayout.addWidget(self.stage2_subsamplingLineEdit)
    self.stage2Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.stage2_gridSpacingLineEdit = qt.QLineEdit()
    self.stage2_gridSpacingLineEdit.setText('50')
    self.stage2_gridSpacingLineEdit.setToolTip( "Set B-spline grid spacing" )
    buttonLayout.addWidget(self.stage2_gridSpacingLineEdit)
    self.stage2Box.layout().addRow("Grid size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.stage2_regularizationLineEdit = qt.QLineEdit()
    self.stage2_regularizationLineEdit.setText('0.1')
    self.stage2_regularizationLineEdit.setToolTip(
      "Set Regularization penalty term")
    buttonLayout.addWidget(self.stage2_regularizationLineEdit)
    self.stage2Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.stage2_landmarkPenaltyLineEdit = qt.QLineEdit()
    self.stage2_landmarkPenaltyLineEdit.setText('10')
    self.stage2_landmarkPenaltyLineEdit.setToolTip(
      "Set landmark distance penalty term")
    buttonLayout.addWidget(self.stage2_landmarkPenaltyLineEdit)
    self.stage2Box.layout().addRow("Landmark penalty:", buttonLayout)

    plmRegisterFormLayout.addRow(self.stage2Box)

    # Stage 3
    self.stage3Box = qt.QGroupBox("Stage 3")
    self.stage3Box.setLayout(qt.QFormLayout())

    # Iterations
    buttonLayout = qt.QHBoxLayout()
    self.stage3_iterationsLineEdit = qt.QLineEdit()
    self.stage3_iterationsLineEdit.setText('10')
    self.stage3_iterationsLineEdit.setToolTip( "Maximum number of iterations" ) 
    buttonLayout.addWidget(self.stage3_iterationsLineEdit)
    self.stage3Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.stage3_subsamplingLineEdit = qt.QLineEdit()
    self.stage3_subsamplingLineEdit.setText('1 1 1')
    self.stage3_subsamplingLineEdit.setToolTip( "Subsampling rate" ) 
    buttonLayout.addWidget(self.stage3_subsamplingLineEdit)
    self.stage3Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.stage3_gridSpacingLineEdit = qt.QLineEdit()
    self.stage3_gridSpacingLineEdit.setText('30')
    self.stage3_gridSpacingLineEdit.setToolTip( "Set B-spline grid spacing" )
    buttonLayout.addWidget(self.stage3_gridSpacingLineEdit)
    self.stage3Box.layout().addRow("Grid size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.stage3_regularizationLineEdit = qt.QLineEdit()
    self.stage3_regularizationLineEdit.setText('0.1')
    self.stage3_regularizationLineEdit.setToolTip(
      "Set Regularization penalty term")
    buttonLayout.addWidget(self.stage3_regularizationLineEdit)
    self.stage3Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.stage3_landmarkPenaltyLineEdit = qt.QLineEdit()
    self.stage3_landmarkPenaltyLineEdit.setText('10')
    self.stage3_landmarkPenaltyLineEdit.setToolTip(
      "Set landmark distance penalty term")
    buttonLayout.addWidget(self.stage3_landmarkPenaltyLineEdit)
    self.stage3Box.layout().addRow("Landmark penalty:", buttonLayout)

    plmRegisterFormLayout.addRow(self.stage3Box)

    # Add UI elements to parent layout
    self.parent.layout().addWidget(self.collapsibleButton)

  def destroy(self):
    """Clean up"""
    super(PlmRegisterPlugin,self).destroy()

  def onLandmarkMoved(self,state):
    pass
  
  def onLandmarkMoved_NOT(self,state):
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

    self.statusLabel.setText("Working...")
    slicer.app.processEvents()
    
    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    
    print ("Gonna SetMRMLScene")
    reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkPlmpyRegistration()
    reg.SetMRMLScene(slicer.mrmlScene)
    print ("Did SetMRMLScene")

    state = self.registrationState()
    reg.SetFixedImageID(state.fixed.GetID())
    reg.SetMovingImageID(state.moving.GetID())
    reg.SetOutputVolumeID(state.transformed.GetID())

    ## This was shamelessly stolen from AffinePlugin.py
    ## It converts the landmarks from MRML Markup format
    ## into vtkPoints format
    if 0:
      points = {}
      point = [0,]*3
      ## GCS FIX: This code assumes that state.fixedFiducials is
      ## a valid node (and is not "NoneType")
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

    volumeNodes = (state.fixed, state.moving)
    fiducialNodes = (state.fixedFiducials,state.movingFiducials)
    points = state.logic.vtkPointsForVolumes( volumeNodes, fiducialNodes )

    reg.SetFixedLandmarks(points[state.fixed])
    reg.SetMovingLandmarks(points[state.moving])


    if (self.prealignmentComboBox.currentText == "Global"):
      print ("Gonna AddStage(Global)")
      reg.AddStage()
      reg.SetPar("xform","translation")
      reg.SetPar("impl","plastimatch")
      reg.SetPar("gridsearch_min_overlap","0.8 0.8 0.8")
      reg.SetPar("res","4 4 2")

    if (self.prealignmentComboBox.currentText == "Local"):
      print ("Gonna AddStage(Local)")
      reg.AddStage()
      reg.SetPar("xform","translation")
      reg.SetPar("impl","itk")
      reg.SetPar("optim","rsg")
      reg.SetPar("res","4 4 2")

    print ("Gonna AddStage(1)")
    reg.AddStage()
    reg.SetPar("xform","bspline")
    reg.SetPar("impl","plastimatch")
    reg.SetPar("res",str(self.stage1_subsamplingLineEdit.text))
    reg.SetPar("iterations",str(self.stage1_iterationsLineEdit.text))
    reg.SetPar("regularization_lambda",
               str(self.stage1_regularizationLineEdit.text))
    reg.SetPar("landmark_stiffness",
               str(self.stage1_landmarkPenaltyLineEdit.text))
    reg.SetPar("grid_spacing",'{0} {0} {0}'
               .format(str(self.stage1_gridSpacingLineEdit.text),
                       str(self.stage1_gridSpacingLineEdit.text),
                       str(self.stage1_gridSpacingLineEdit.text)))

    #comboText = self.numstagesComboBox.currentText()
    comboText = self.numstagesComboBox.currentText
    if (comboText == "2" or comboText == "3"):
      reg.AddStage()
      reg.SetPar("xform","bspline")
      reg.SetPar("impl","plastimatch")
      reg.SetPar("res",str(self.stage2_subsamplingLineEdit.text))
      reg.SetPar("iterations",str(self.stage2_iterationsLineEdit.text))
      reg.SetPar("regularization_lambda",
                 str(self.stage2_regularizationLineEdit.text))
      reg.SetPar("landmark_stiffness",
                 str(self.stage2_landmarkPenaltyLineEdit.text))
      reg.SetPar("grid_spacing",'{0} {0} {0}'
                 .format(str(self.stage2_gridSpacingLineEdit.text),
                         str(self.stage2_gridSpacingLineEdit.text),
                         str(self.stage2_gridSpacingLineEdit.text)))

    if (comboText == "3"):
      reg.AddStage()
      reg.SetPar("xform","bspline")
      reg.SetPar("impl","plastimatch")
      reg.SetPar("res",str(self.stage3_subsamplingLineEdit.text))
      reg.SetPar("iterations",str(self.stage3_iterationsLineEdit.text))
      reg.SetPar("regularization_lambda",
                 str(self.stage3_regularizationLineEdit.text))
      reg.SetPar("landmark_stiffness",
                 str(self.stage3_landmarkPenaltyLineEdit.text))
      reg.SetPar("grid_spacing",'{0} {0} {0}'
                 .format(str(self.stage3_gridSpacingLineEdit.text),
                         str(self.stage3_gridSpacingLineEdit.text),
                         str(self.stage3_gridSpacingLineEdit.text)))
    
    print ("Gonna RunRegistration()")
    print ("prealignment strategy is %s" % str(self.prealignmentComboBox.currentText))
    print ("subsampling is %s" % str(self.stage1_subsamplingLineEdit.text))

    reg.RunRegistration ()
    print ("Did RunRegistration()")
    self.statusLabel.setText("Done.")

# Add this plugin to the dictionary of available registrations.
# Since this module may be discovered before the Editor itself,
# create the list if it doesn't already exist.
try:
  slicer.modules.registrationPlugins
except AttributeError:
  slicer.modules.registrationPlugins = {}
slicer.modules.registrationPlugins['Plm'] = PlmRegisterPlugin
print ("Finished sourcing PlmRegister.py")
