import os, sys
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
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

class PlmRegister(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Plastimatch"
    self.parent.categories = ["Plastimatch"]
    self.parent.dependencies = []
    self.parent.contributors = ["Gregory Sharp (MGH)"]
    self.parent.helpText = """
    This is an example of scripted loadable module bundled in an extension.
    """
    self.parent.acknowledgementText = """
    This file was originally developed by Greg Sharp, Nadya Shusharina, and Paolo Zaffino and was partially funded by NIH grant 2-U54-EB005149.
    """ # replace with organization, grant and thanks.
    self.hidden = True


#
# qPlmRegisterWidget
#

class PlmRegisterWidget(ScriptedLoadableModuleWidget):

  def setup(self):

    ScriptedLoadableModuleWidget.setup(self)

    # Add vertical spacer
    self.layout.addStretch(1)

  def cleanup(self):
    pass

class PlmRegisterPlugin(RegistrationLib.RegistrationPlugin):
  """ Plastimatch registration plugin class
  """
  # generic settings that can (should) be overridden by the subclass
  
  # displayed for the user to select the registration
  name = "Plastimatch"
  tooltip = "B-spline deformable registration with landmark and regularization"

  stages = ["","","","",""] # transaltion gridsearch, translation, bspline1, bspline2, bspline3

  # can be true or false
  # - True: landmarks are displayed and managed by LandmarkRegistration
  # - False: landmarks are hidden
  usesLandmarks = True

  # can be any non-negative number
  # - widget will be disabled until landmarks are defined
  landmarksNeededToEnable = 1

  # Landmarks/parameters have been moved/changed during the stoppable registration
  landmarkMoved = False
  parameterStage1Changed = False
  parameterStage2Changed = False
  parameterStage3Changed = False

  # used for reloading - every concrete class should include this
  sourceFile = __file__

  # default settings
  costFunction = "MSE"

  def __init__(self,parent=None):
    super(PlmRegisterPlugin,self).__init__(parent)
    import os, sys, vtk
    import vtkSlicerPlastimatchPyModuleLogicPython
    self.reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkPlmpyRegistration()
    self.reg.SetMRMLScene(slicer.mrmlScene)


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

    # Output transform
    self.outputTransformComboBox = slicer.qMRMLNodeComboBox()
    self.outputTransformComboBox.nodeTypes = (
      ("vtkMRMLGridTransformNode"), "" )
    self.outputTransformComboBox.selectNodeUponCreation = True
    self.outputTransformComboBox.addEnabled = True
    self.outputTransformComboBox.removeEnabled = True
    self.outputTransformComboBox.noneEnabled = True
    self.outputTransformComboBox.showHidden = False
    self.outputTransformComboBox.showChildNodeTypes = False
    self.outputTransformComboBox.setMRMLScene( slicer.mrmlScene )
    plmRegisterFormLayout.addRow("Output transform:",
                                 self.outputTransformComboBox)

    # Apply button
    self.applyButton = qt.QPushButton("Click to start registration!")
    self.applyButton.setStyleSheet("background-color: #FFFF99")
    self.applyButton.connect('clicked(bool)', self.onApply)
    plmRegisterFormLayout.addWidget(self.applyButton)

    # Stop button
    self.stopButton = qt.QPushButton("Click to stop registration!")
    self.stopButton.setStyleSheet("background-color: #FF3232")
    self.stopButton.connect('clicked(bool)', self.onStop)
    plmRegisterFormLayout.addWidget(self.stopButton)

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
    self.stage1_iterationsLineEdit.connect('textChanged(QString)', self.parameterStage1IsChanged)
    buttonLayout.addWidget(self.stage1_iterationsLineEdit)
    self.stage1Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.stage1_subsamplingLineEdit = qt.QLineEdit()
    self.stage1_subsamplingLineEdit.setText('4 4 2')
    self.stage1_subsamplingLineEdit.setToolTip( "Subsampling rate" ) 
    self.stage1_subsamplingLineEdit.connect('textChanged(QString)', self.parameterStage1IsChanged)
    buttonLayout.addWidget(self.stage1_subsamplingLineEdit)
    self.stage1Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.stage1_gridSpacingLineEdit = qt.QLineEdit()
    self.stage1_gridSpacingLineEdit.setText('100')
    self.stage1_gridSpacingLineEdit.setToolTip( "Set B-spline grid spacing" )
    self.stage1_gridSpacingLineEdit.connect('textChanged(QString)', self.parameterStage1IsChanged)
    buttonLayout.addWidget(self.stage1_gridSpacingLineEdit)
    self.stage1Box.layout().addRow("Grid size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.stage1_regularizationLineEdit = qt.QLineEdit()
    self.stage1_regularizationLineEdit.setText('0.1')
    self.stage1_regularizationLineEdit.setToolTip(
      "Set Regularization penalty term")
    self.stage1_regularizationLineEdit.connect('textChanged(QString)', self.parameterStage1IsChanged)
    buttonLayout.addWidget(self.stage1_regularizationLineEdit)
    self.stage1Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.stage1_landmarkPenaltyLineEdit = qt.QLineEdit()
    self.stage1_landmarkPenaltyLineEdit.setText('10')
    self.stage1_landmarkPenaltyLineEdit.setToolTip(
      "Set landmark distance penalty term")
    self.stage1_landmarkPenaltyLineEdit.connect('textChanged(QString)', self.parameterStage1IsChanged)
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
    self.stage2_iterationsLineEdit.connect('textChanged(QString)', self.parameterStage2IsChanged)
    buttonLayout.addWidget(self.stage2_iterationsLineEdit)
    self.stage2Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.stage2_subsamplingLineEdit = qt.QLineEdit()
    self.stage2_subsamplingLineEdit.setText('2 2 1')
    self.stage2_subsamplingLineEdit.setToolTip( "Subsampling rate" ) 
    self.stage2_subsamplingLineEdit.connect('textChanged(QString)', self.parameterStage2IsChanged)
    buttonLayout.addWidget(self.stage2_subsamplingLineEdit)
    self.stage2Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.stage2_gridSpacingLineEdit = qt.QLineEdit()
    self.stage2_gridSpacingLineEdit.setText('50')
    self.stage2_gridSpacingLineEdit.setToolTip( "Set B-spline grid spacing" )
    self.stage2_gridSpacingLineEdit.connect('textChanged(QString)', self.parameterStage2IsChanged)
    buttonLayout.addWidget(self.stage2_gridSpacingLineEdit)
    self.stage2Box.layout().addRow("Grid size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.stage2_regularizationLineEdit = qt.QLineEdit()
    self.stage2_regularizationLineEdit.setText('0.1')
    self.stage2_regularizationLineEdit.setToolTip(
      "Set Regularization penalty term")
    self.stage2_regularizationLineEdit.connect('textChanged(QString)', self.parameterStage2IsChanged)
    buttonLayout.addWidget(self.stage2_regularizationLineEdit)
    self.stage2Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.stage2_landmarkPenaltyLineEdit = qt.QLineEdit()
    self.stage2_landmarkPenaltyLineEdit.setText('10')
    self.stage2_landmarkPenaltyLineEdit.setToolTip(
      "Set landmark distance penalty term")
    self.stage2_landmarkPenaltyLineEdit.connect('textChanged(QString)', self.parameterStage2IsChanged)
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
    self.stage3_iterationsLineEdit.connect('textChanged(QString)', self.parameterStage3IsChanged)
    buttonLayout.addWidget(self.stage3_iterationsLineEdit)
    self.stage3Box.layout().addRow("Iterations:", buttonLayout)

    # Subsampling rate
    buttonLayout = qt.QHBoxLayout()
    self.stage3_subsamplingLineEdit = qt.QLineEdit()
    self.stage3_subsamplingLineEdit.setText('1 1 1')
    self.stage3_subsamplingLineEdit.setToolTip( "Subsampling rate" ) 
    self.stage3_subsamplingLineEdit.connect('textChanged(QString)', self.parameterStage3IsChanged)
    buttonLayout.addWidget(self.stage3_subsamplingLineEdit)
    self.stage3Box.layout().addRow("Subsampling rate (vox):", buttonLayout)

    # Grid spacing
    buttonLayout = qt.QHBoxLayout()
    self.stage3_gridSpacingLineEdit = qt.QLineEdit()
    self.stage3_gridSpacingLineEdit.setText('30')
    self.stage3_gridSpacingLineEdit.setToolTip( "Set B-spline grid spacing" )
    self.stage3_gridSpacingLineEdit.connect('textChanged(QString)', self.parameterStage3IsChanged)
    buttonLayout.addWidget(self.stage3_gridSpacingLineEdit)
    self.stage3Box.layout().addRow("Grid size (mm):", buttonLayout)

    # Regularization
    buttonLayout = qt.QHBoxLayout()
    self.stage3_regularizationLineEdit = qt.QLineEdit()
    self.stage3_regularizationLineEdit.setText('0.1')
    self.stage3_regularizationLineEdit.setToolTip(
      "Set Regularization penalty term")
    self.stage3_regularizationLineEdit.connect('textChanged(QString)', self.parameterStage3IsChanged)
    buttonLayout.addWidget(self.stage3_regularizationLineEdit)
    self.stage3Box.layout().addRow("Regularization:", buttonLayout)

    # Landmark penalty
    buttonLayout = qt.QHBoxLayout()
    self.stage3_landmarkPenaltyLineEdit = qt.QLineEdit()
    self.stage3_landmarkPenaltyLineEdit.setText('10')
    self.stage3_landmarkPenaltyLineEdit.setToolTip(
      "Set landmark distance penalty term")
    self.stage3_landmarkPenaltyLineEdit.connect('textChanged(QString)', self.parameterStage3IsChanged)
    buttonLayout.addWidget(self.stage3_landmarkPenaltyLineEdit)
    self.stage3Box.layout().addRow("Landmark penalty:", buttonLayout)

    plmRegisterFormLayout.addRow(self.stage3Box)

    # Add UI elements to parent layout
    self.parent.layout().addWidget(self.collapsibleButton)

  def destroy(self):
    """Clean up"""
    super(PlmRegisterPlugin,self).destroy()

  def parameterStage1IsChanged(self):
    """ Function called when some stage 1 parameter in the textbox is changed """
    self.parameterStage1Changed = True
    print("PARAMETERS VALUE STAGE 1 CHANGED!!!!!")
    """
    self.StopRegistration()
    self.stages[0] = ''
    self.stages[1] = ''
    self.stages[2] = self.getStage1Parameters()
    reg.SetRegistrationParameters("".join(self.stages))
    DO SOMETHING TO SET THE NEW INPUT AND XFORM (IF REQUIRED)
    self.StartRegistration()
    """

  def parameterStage2IsChanged(self):
    """ Function called when some stage 2 parameter in the textbox is changed """
    self.parameterStage2Changed = True
    print("PARAMETERS VALUE STAGE 2 CHANGED!!!!!")

  def parameterStage3IsChanged(self):
    """ Function called when some stage 3 parameter in the textbox is changed """
    self.parameterStage3Changed = True
    print("PARAMETERS VALUE STAGE 3 CHANGED!!!!!")

  def getStage1Parameters(self):
    parameters_stage1 = ""
    parameters_stage1 += "[STAGE]\n"
    parameters_stage1 += "resume = true\n"
    parameters_stage1 += "xform = bspline\n"
    parameters_stage1 += "impl = plastimatch\n"
    parameters_stage1 += "res = %s\n" % str(self.stage1_subsamplingLineEdit.text)
    parameters_stage1 += "max_its = %s\n" % str(self.stage1_iterationsLineEdit.text)
    parameters_stage1 += "regularization_lambda = %s\n" % \
               str(self.stage1_regularizationLineEdit.text)
    parameters_stage1 += "landmark_stiffness = %s\n" % \
               str(self.stage1_landmarkPenaltyLineEdit.text)
    parameters_stage1 += "grid_spacing = {0} {0} {0}\n".format\
               (str(self.stage1_gridSpacingLineEdit.text)) 
   
    return parameters_stage1

  def getStage2Parameters(self):
    parameters_stage2 = ""
    parameters_stage2 += "[STAGE]\n"
    parameters_stage2 += "resume = true\n"
    parameters_stage2 += "xform = bspline\n"
    parameters_stage2 += "impl = plastimatch\n"
    parameters_stage2 += "res = %s\n" % str(self.stage2_subsamplingLineEdit.text)
    parameters_stage2 += "max_its = %s\n" % str(self.stage2_iterationsLineEdit.text)
    parameters_stage2 += "regularization_lambda = %s\n" % \
                 str(self.stage2_regularizationLineEdit.text)
    parameters_stage2 += "landmark_stiffness = %s\n" % \
                 str(self.stage2_landmarkPenaltyLineEdit.text)
    parameters_stage2 += "grid_spacing = {0} {0} {0}\n".format\
                 (str(self.stage2_gridSpacingLineEdit.text))
    
    return parameters_stage2

  def getStage3Parameters(self):
    parameters_stage3 = ""
    parameters_stage3 += "[STAGE]\n"
    parameters_stage3 += "resume = true\n"
    parameters_stage3 += "xform = bspline\n"
    parameters_stage3 += "impl = plastimatch\n"
    parameters_stage3 += "res = %s\n" % str(self.stage3_subsamplingLineEdit.text)
    parameters_stage3 += "max_its = %s\n" % str(self.stage3_iterationsLineEdit.text)
    parameters_stage3 += "regularization_lambda = %s\n" % \
                 str(self.stage3_regularizationLineEdit.text)
    parameters_stage3 += "landmark_stiffness = %s\n" % \
                 str(self.stage3_landmarkPenaltyLineEdit.text)
    parameters_stage3 += "grid_spacing = {0} {0} {0}\n".format\
                 (str(self.stage3_gridSpacingLineEdit.text))
    
    return parameters_stage3

  def onLandmarkMoved(self,state):
    self.landmarkMoved = True
    print ("MOVED!!!")
  
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
    print ("I know that you pushed the apply button!")
    self.InitializeRegistration()
    self.runOneIteration()

  def onStop(self):
    print ("I know that you pushed the stop button!")
    self.reg.StopRegistration()
    self.reg.ReturnDataToSlicer()

  def InitializeRegistration(self):
    import os, sys, vtk
    print ("Hello from InitializeRegistration")

    self.statusLabel.setText("Working...")
    self.statusLabel.repaint()
    self.statusLabel.repaint()

    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    
    state = self.registrationState()
    self.reg.SetFixedImageID(state.fixed.GetID())
    self.reg.SetMovingImageID(state.moving.GetID())
    self.reg.SetOutputVolumeID(state.transformed.GetID())
    self.output_transform = self.outputTransformComboBox.currentNode()
    if self.output_transform:
      self.reg.SetOutputVectorFieldID(self.output_transform.GetID())
    ### WHY DOESN'T THIS WORK?
    #else:
    #  self.output_transform = slicer.vtkOrientedBSplineTransform()
    #  state.transform.SetAndObserveTransformToParent(self.output_transform)
    #  self.reg.SetOutputVectorFieldID(state.transform.GetID())

    volumeNodes = (state.fixed, state.moving)
    fiducialNodes = (state.fixedFiducials,state.movingFiducials)
    points = state.logic.vtkPointsForVolumes( volumeNodes, fiducialNodes )

    self.reg.SetFixedLandmarks(points[state.fixed])
    self.reg.SetMovingLandmarks(points[state.moving])

  def runOneIteration(self):
    print ("Hello from runOneIteration")

    if (self.prealignmentComboBox.currentText == "Global"):
      print ("Gonna AddStage(Global)")
      self.stages[0] = "[STAGE]\nxform = translation\nimpl = plastimatch\ngridsearch_min_overlap = 0.8 0.8 0.8\nres = 4 4 2\n"

    if (self.prealignmentComboBox.currentText == "Local"):
      print ("Gonna AddStage(Local)")
      self.stages[1] = "[STAGE]\nxform = translation\nimpl = itk\noptim = rsg\nres = 4 4 2\n"

    print ("Gonna AddStage(1)")
    self.stages[2] = self.getStage1Parameters()

    comboText = self.numstagesComboBox.currentText
    if (comboText == "2" or comboText == "3"):
      self.stages[3] = self.getStage2Parameters()
    
    if (comboText == "3"):\
      self.stages[4] = self.getStage3Parameters()

    stages_string = "".join(self.stages)
    print ("\n\n\nPARAMETERS ARE:\n%s\nEND OF PARAMETERS!\n\n\n") % stages_string

    print ("prealignment strategy is %s" % str(self.prealignmentComboBox.currentText))
    print ("subsampling is %s" % str(self.stage1_subsamplingLineEdit.text))

    self.reg.SetRegistrationParameters(stages_string)
    # Eventually, we will only start the registration, and it will finish
    # later.  But for now, let's wait for it to complete.
    #self.reg.StartRegistration ()
    print ("Gonna RunRegistration()")
    self.reg.RunRegistration ()
    self.statusLabel.setText("Done.")

    ### WHY DOESN'T THIS WORK?
    # Here we want to send the B-Spline transform to Slicer.
    #state = self.registrationState()
    #state.transform.SetAndObserveTransformToParent(self.output_transform)

# Add this plugin to the dictionary of available registrations.
# Since this module may be discovered before the Editor itself,
# create the list if it doesn't already exist.
try:
  slicer.modules.registrationPlugins
except AttributeError:
  slicer.modules.registrationPlugins = {}
slicer.modules.registrationPlugins['Plm'] = PlmRegisterPlugin
#print ("Finished sourcing PlmRegister.py")
