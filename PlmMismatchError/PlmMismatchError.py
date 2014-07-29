import os
import unittest
from __main__ import vtk, qt, ctk, slicer


#
# PlmMismatchError
#

class PlmMismatchError:
  def __init__(self, parent):
    parent.title = "Landmark Mismatch Error"
    parent.categories = ["Plastimatch.DIR Validation"]
    parent.dependencies = []
    parent.contributors = ["Gregory Sharp (MGH)"]
    parent.helpText = """
    This is an example of scripted loadable module bundled in an extension.
    """
    parent.acknowledgementText = """
    This file was originally developed by Greg Sharp, Massachusetts General Hospital, and was partially funded by NIH grant 2-U54-EB005149.
    """ # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['PlmMismatchError'] = self.runTest

  def runTest(self):
    tester = PlmMismatchErrorTest()
    tester.runTest()

class PlmMismatchErrorWidgetLogicIO: 
  def __init__(self):
    self.FixedFiducialsNode = False
    self.MovingFiducialsNode = False
    self.average = 0
    self.variance = 0
    self.stdev = 0
    self.separation = {}
    self.landm_distance = []
    
  def SetAverage(self, x=0):
    self.average = x

  def SetVariance(self, x=0):
    self.variance = x

  def SetStdev(self, x=0):
    self.stdev = x

  def SetSeparation(self):
    self.separation = {}


# qPlmMismatchErrorWidget


class PlmMismatchErrorWidget:
  def __init__(self, parent = None):
    self.logic = PlmMismatchErrorLogic()
    self.WLIO = PlmMismatchErrorWidgetLogicIO()

    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.layout = self.parent.layout()
    if not parent:
      self.setup()
      self.parent.show()

  def setup(self):
    # Instantiate and connect widgets ...

    ### Parameters Area
    inputCollapsibleButton = ctk.ctkCollapsibleButton()
    inputCollapsibleButton.text = "Input"
    self.layout.addWidget(inputCollapsibleButton)

    # Layout within the dummy collapsible button
    inputFormLayout = qt.QFormLayout(inputCollapsibleButton)

   # fixed landmarks (mrml input)
    self.fixedLandmarks = slicer.qMRMLNodeComboBox()
    self.fixedLandmarks.nodeTypes = ( ("vtkMRMLMarkupsFiducialNode"), "" )
    self.fixedLandmarks.selectNodeUponCreation = True
    self.fixedLandmarks.addEnabled = False
    self.fixedLandmarks.removeEnabled = False
    self.fixedLandmarks.noneEnabled = True
    self.fixedLandmarks.showHidden = False
    self.fixedLandmarks.renameEnabled = True
    self.fixedLandmarks.setMRMLScene( slicer.mrmlScene )
    self.fixedLandmarks.setToolTip( "Landmarks on fixed image." )
    inputFormLayout.addRow("Fixed landmarks: ", self.fixedLandmarks)

    # fixed landmarks (directory input)
    self.fixedLandmarksDirectory = ctk.ctkDirectoryButton()
    self.fixedLandmarksDirectory.directory = qt.QDir.homePath()
    inputFormLayout.addRow("", self.fixedLandmarksDirectory)

    # moving landmarks (mrml input)
    self.movingLandmarks = slicer.qMRMLNodeComboBox()
    self.movingLandmarks.nodeTypes = ( ("vtkMRMLMarkupsFiducialNode"), "" )
    self.movingLandmarks.selectNodeUponCreation = True
    self.movingLandmarks.addEnabled = False
    self.movingLandmarks.removeEnabled = False
    self.movingLandmarks.noneEnabled = True
    self.movingLandmarks.showHidden = False
    self.movingLandmarks.showChildNodeTypes = False
    self.movingLandmarks.setMRMLScene( slicer.mrmlScene )
    self.movingLandmarks.setToolTip( "Landmarks on moving image." )
    inputFormLayout.addRow("Moving landmarks: ", self.movingLandmarks)

    self.view = qt.QTableView()
    self.view.sortingEnabled = True
    self.parent.layout().addWidget(self.view)

    # moving landmarks (directory input)
    self.movingLandmarksDirectory = ctk.ctkDirectoryButton()
    self.movingLandmarksDirectory.directory = qt.QDir.homePath()
    inputFormLayout.addRow("", self.movingLandmarksDirectory) 

    # Apply Button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the algorithm."
    self.applyButton.enabled = True
    self.layout.addWidget(self.applyButton)

    ### Output Area
    outputCollapsibleButton = ctk.ctkCollapsibleButton()
    outputCollapsibleButton.text = "Output statistics"
    self.layout.addWidget(outputCollapsibleButton)

    # Layout within the dummy collapsible button
    outputFormLayout = qt.QFormLayout(outputCollapsibleButton)

    # output statistics 
    buttonLayout = qt.QHBoxLayout()
    self.averageError = qt.QLineEdit()
    self.averageErrorValue = 1
    self.averageError.setToolTip( "Average landmark separation" )
    buttonLayout.addWidget(self.averageError)
    outputFormLayout.addRow("Average error:", buttonLayout)
     
    buttonLayout = qt.QHBoxLayout()
    self.Variance = qt.QLineEdit()
    self.VarianceValue = 1
    self.Variance.setToolTip( "Variance" )
    buttonLayout.addWidget(self.Variance)
    outputFormLayout.addRow("Variance:", buttonLayout)

    buttonLayout = qt.QHBoxLayout()
    self.stDev = qt.QLineEdit()
    self.stDevValue = 1
    self.stDev.setToolTip( "Standard deviation" )
    buttonLayout.addWidget(self.stDev)
    outputFormLayout.addRow("Standard deviation:", buttonLayout)

    # model and view for stats table
    self.view = qt.QTableView()
    self.view.sortingEnabled = True
    self.parent.layout().addWidget(self.view)

    # connections
    self.fixedLandmarks.connect("currentNodeChanged(vtkMRMLNode*)",  self.onFixedLandmarksSelect)
    self.movingLandmarks.connect("currentNodeChanged(vtkMRMLNode*)", self.onMovingLandmarksSelect)
    self.applyButton.connect('clicked(bool)', self.onMismatchErrorApply)

    # Add vertical spacer
    #self.layout.addStretch(1)

  def cleanup(self):
    pass

  def onFixedLandmarksSelect(self):
    print "fixed landmarks selected"
    self.WLIO.FixedFiducialsNode  = self.fixedLandmarks.currentNode()
    
  def onMovingLandmarksSelect(self):
    print "moving landmarks selected"
    self.WLIO.MovingFiducialsNode = self.movingLandmarks.currentNode()

  def onMismatchErrorApply(self):
    print "apply button\n"
    myWLIO = self.logic.RunMismatchError( self.WLIO )
    self.averageError.setText(myWLIO.average)
    self.Variance.setText(myWLIO.variance)
    self.stDev.setText(myWLIO.stdev)
    print "ready to draw table:"
    for dist in myWLIO.landm_distance:
      print dist
  # the table.. - separate into a function?
    self.items = []
    self.model = qt.QStandardItemModel()
    self.view.setModel(self.model)
    self.view.verticalHeader().visible = False
    row = 0
    landmarkcounter = 0
    for dist in myWLIO.landm_distance:
      # 1st column (row,0)
      item = qt.QStandardItem()
      landmarkcounter += 1
      item.setText(landmarkcounter)
      self.model.setItem(row,0,item)
      self.items.append(item)
      # 2nd column (row,1)
      item = qt.QStandardItem()
      item.setText(dist)
      self.model.setItem(row,1,item)
      self.items.append(item)
      row += 1
    #done!
    #self.view.setColumnWidth(0,30)
    self.model.setHeaderData(0,1,"Landmark")
    self.model.setHeaderData(1,1,"dist, mm")

  def populateDistances(self):
    if not self.logic:
      return
    self.items = []
    self.model = qt.QStandardItemModel()
    self.view.setModel(self.model)
    self.view.verticalHeader().visible = False
    row = 0
    for i in self.logic.landmarkPairs["Landmarks"]:
      item = qt.QStandardItem()
      item.setData(points[fixed])
      #item.setToolTip(colorNode.GetColorName(i))
      self.model.setItem(row,0,item)
      self.items.append(item)
      col = 1
      for k in self.logic.keys:
        item = qt.QStandardItem()
        item.setText(str(self.logic.landmarkPairs[i,k]))
        #item.setToolTip(colorNode.GetColorName(i))
        self.model.setItem(row,col,item)
        self.items.append(item)
        col += 1
      row += 1

    self.view.setColumnWidth(0,30)
    self.model.setHeaderData(0,1," ")
    col = 1
    for k in self.logic.keys:
      self.view.setColumnWidth(col,15*len(k))
      self.model.setHeaderData(col,1,k)
      col += 1


# PlmMismatchErrorLogic


class PlmMismatchErrorLogic:
  """This class should implement all the actual 
  computation done by your module.  The interface 
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    pass

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

    self.keys = ("Separation")
    self.landmarkPairs = {}
    self.landmarkPairs['Landmarks'] = []
    self.landmarkPairs["Landmarks"].append(i)
    self.landmarkPairs[i,"Separation"] = reg.GetSeparationString()

  def RunMismatchError(self, WLIO):
    import os, sys, vtk, math
    loadablePath = os.path.join(slicer.modules.plastimatch_slicer_bspline.path,'..'+os.sep+'..'+os.sep+'qt-loadable-modules')
    if loadablePath not in sys.path:
      sys.path.append(loadablePath)
    import vtkSlicerPlastimatchPyModuleLogicPython
    reg = vtkSlicerPlastimatchPyModuleLogicPython.vtkSlicerPlastimatchPyModuleLogic()
    reg.SetMRMLScene(slicer.mrmlScene)

    print "running RunMismatchError"

    # Set landmarks from Slicer 
    print "trying to SetFixed/MovingLandmarks"

    fixpoints = vtk.vtkPoints()
    movpoints = vtk.vtkPoints()
    point = [0,]*3
 
    fixFidNode = WLIO.FixedFiducialsNode  # it is a vtkMRMLMarkupsFiducialNode 
    nfix = fixFidNode.GetNumberOfFiducials()
    print( "found %s fiducials in fixed" % str(nfix) )
    for n in range(nfix):
      fixFidNode.GetNthFiducialPosition(n, point)
      fixpoints.InsertNextPoint(point)
      #print("fix %s:" %  str(point))

    movFidNode = WLIO.MovingFiducialsNode  # it is a vtkMRMLMarkupsFiducialNode 
    nmov = movFidNode.GetNumberOfFiducials()
    print( "found %s fiducials in moving" % str(nmov) )
    for n in range(nmov):
      movFidNode.GetNthFiducialPosition(n, point)
      movpoints.InsertNextPoint(point)
      #print("mov %s:" %  str(point))

    WidgetLogicIO = PlmMismatchErrorWidgetLogicIO() # this is for output to GUI widget

    p1 = [0,]*3
    p2 = [0,]*3
    counter = 0
    d1sum = 0
    d2sum = 0
    d3sum = 0
    for n in range(nfix):
      fixpoints.GetPoint(n, p1)
      movpoints.GetPoint(n, p2)
      d2 = (p1[0]-p2[0])*(p1[0]-p2[0]) + (p1[1]-p2[1])*(p1[1]-p2[1]) + (p1[2]-p2[2])*(p1[2]-p2[2])
      d1 = math.sqrt(d2)
      d1sum += d1
      d2sum += d2
      counter += 1
      WidgetLogicIO.landm_distance.append(d1)
      #print (n, d1, d2, d1sum, d2sum)

    average = d1sum/counter

    for n in range(nfix):
      fixpoints.GetPoint(n, p1)
      movpoints.GetPoint(n, p2)
      d2 = (p1[0]-p2[0])*(p1[0]-p2[0]) + (p1[1]-p2[1])*(p1[1]-p2[1]) + (p1[2]-p2[2])*(p1[2]-p2[2])
      d1 = math.sqrt(d2)
      d3 = (d1 - average)*(d1 - average)
      d3sum += d3
      counter += 1 	    
 
    variance = d3sum/counter 
    stdev = math.sqrt(variance)

    #reg.SetFixedLandmarks( fixpoints)
    #reg.SetMovingLandmarks(movpoints)
    #print "Done SetFixed/MovingLandmarks"
    # Run Mismatch Error
    #print "starting RunMismatchError"
    #reg.RunMismatchError()
    #print "control went past RunMismatchError"
    # Done

    # return error values to caller
    WidgetLogicIO.SetAverage(average)
    WidgetLogicIO.SetVariance(variance)
    WidgetLogicIO.SetStdev(stdev)
    #WidgetLogicIO.SetSeparation(reg.GetSeparationString())
    
    return WidgetLogicIO

