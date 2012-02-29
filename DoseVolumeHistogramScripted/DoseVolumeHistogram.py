from __main__ import vtk, qt, ctk, slicer

#
# DoseVolumeHistogram
#

class DoseVolumeHistogram:
  def __init__(self, parent):
    parent.title = "Dose volume histogram"
    parent.categories = ["Radiotherapy"]
    parent.contributor = "Andras Lasso"
    parent.helpText = """
Use this module to calculate Dose Volume Histogram.
    """
    parent.acknowledgementText = """
    Supported by Cancer Care Ontario, and the Slicer Community. See http://www.slicer.org for details.
    """
    self.parent = parent

#
# DoseVolumeHistogramWidget
#

class DoseVolumeHistogramWidget:
  def __init__(self, parent=None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.logic = None
    self.doseImageNode = None
    self.labelNode = None
    self.fileName = None
    self.fileDialog = None
    if not parent:
      self.setup()
      self.grayscaleSelector.setMRMLScene(slicer.mrmlScene)
      self.labelSelector.setMRMLScene(slicer.mrmlScene)
      self.parent.show()
    
  def setup(self):
    #
    # the grayscale volume selector
    #
    self.grayscaleSelectorFrame = qt.QFrame(self.parent)
    self.grayscaleSelectorFrame.setLayout(qt.QHBoxLayout())
    self.parent.layout().addWidget(self.grayscaleSelectorFrame)

    self.grayscaleSelectorLabel = qt.QLabel("Dose volume: ", self.grayscaleSelectorFrame)
    self.grayscaleSelectorLabel.setToolTip( "Select the grayscale volume (background grayscale scalar volume node) for statistics calculations")
    self.grayscaleSelectorFrame.layout().addWidget(self.grayscaleSelectorLabel)

    self.grayscaleSelector = slicer.qMRMLNodeComboBox(self.grayscaleSelectorFrame)
    self.grayscaleSelector.nodeTypes = ( ("vtkMRMLScalarVolumeNode"), "" )
    self.grayscaleSelector.addAttribute( "vtkMRMLScalarVolumeNode", "LabelMap", 0 )
    self.grayscaleSelector.selectNodeUponCreation = False
    self.grayscaleSelector.addEnabled = False
    self.grayscaleSelector.removeEnabled = False
    self.grayscaleSelector.noneEnabled = True
    self.grayscaleSelector.showHidden = False
    self.grayscaleSelector.showChildNodeTypes = False
    self.grayscaleSelector.setMRMLScene( slicer.mrmlScene )
    # TODO: need to add a QLabel
    # self.grayscaleSelector.SetLabelText( "Master Volume:" )
    self.grayscaleSelectorFrame.layout().addWidget(self.grayscaleSelector)
    
    #
    # the label volume selector
    #
    self.labelSelectorFrame = qt.QFrame()
    self.labelSelectorFrame.setLayout( qt.QHBoxLayout() )
    self.parent.layout().addWidget( self.labelSelectorFrame )

    self.labelSelectorLabel = qt.QLabel()
    self.labelSelectorLabel.setText( "Label Map: " )
    self.labelSelectorFrame.layout().addWidget( self.labelSelectorLabel )

    self.labelSelector = slicer.qMRMLNodeComboBox()
    self.labelSelector.nodeTypes = ( "vtkMRMLScalarVolumeNode", "" )
    self.labelSelector.addAttribute( "vtkMRMLScalarVolumeNode", "LabelMap", "1" )
    # todo addAttribute
    self.labelSelector.selectNodeUponCreation = False
    self.labelSelector.addEnabled = False
    self.labelSelector.noneEnabled = True
    self.labelSelector.removeEnabled = False
    self.labelSelector.showHidden = False
    self.labelSelector.showChildNodeTypes = False
    self.labelSelector.setMRMLScene( slicer.mrmlScene )
    self.labelSelector.setToolTip( "Pick the label map to edit" )
    self.labelSelectorFrame.layout().addWidget( self.labelSelector )
    
    # Apply button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Calculate Statistics."
    self.applyButton.enabled = False
    self.parent.layout().addWidget(self.applyButton)

    # model and view for stats table
    self.view = qt.QTableView()
    self.view.sortingEnabled = True
    self.parent.layout().addWidget(self.view)

    # Chart button
    self.chartButton = qt.QPushButton("Chart")
    self.chartButton.toolTip = "Make a chart from the current statistics."
    self.chartButton.enabled = False
    self.parent.layout().addWidget(self.chartButton)

    # Save button
    self.saveButton = qt.QPushButton("Save")
    self.saveButton.toolTip = "Calculate Statistics."
    self.saveButton.enabled = False
    self.parent.layout().addWidget(self.saveButton)

    # Add vertical spacer
    self.parent.layout().addStretch(1)

    # connections
    self.applyButton.connect('clicked()', self.onApply)
    self.chartButton.connect('clicked()', self.onChart)
    self.saveButton.connect('clicked()', self.onSave)
    self.grayscaleSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onGrayscaleSelect)
    self.labelSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onLabelSelect)
    
  def onGrayscaleSelect(self, node):
    self.doseImageNode = node
    self.applyButton.enabled = bool(self.doseImageNode) and bool(self.labelNode)

  def onLabelSelect(self, node):
    self.labelNode = node
    self.applyButton.enabled = bool(self.doseImageNode) and bool(self.labelNode)

  def onApply(self):
    """Calculate the label statistics
    """
    self.applyButton.text = "Working..."
    # TODO: why doesn't processEvents alone make the label text change?
    self.applyButton.repaint()
    slicer.app.processEvents()
    self.logic = DoseVolumeHistogramLogic(self.doseImageNode, self.labelNode)
    self.populateStats()
    self.chartButton.enabled = True
    self.saveButton.enabled = True
    self.applyButton.text = "Apply"

  def onChart(self):
    """chart the label statistics
    """
    self.logic.createStatsChart(self.doseImageNode, self.labelNode)

  def onSave(self):
    """save the label statistics
    """
    if not self.fileDialog:
      self.fileDialog = qt.QFileDialog(self.parent)
      self.fileDialog.acceptMode = 1 # save dialog
      self.fileDialog.defaultSuffix = "csv"
      self.fileDialog.setNameFilter("Comma Separated Values (*.csv)")
      self.fileDialog.connect("fileSelected(QString)", self.onFileSelected)
    self.fileDialog.show()

  def onFileSelected(self,fileName):
    self.logic.saveStats(fileName)

  def populateStats(self):
    if not self.logic:
      return
    self.items = []
    self.model = qt.QStandardItemModel()
    self.view.setModel(self.model)
    row = 0
    for i in self.logic.labelStats["Labels"]:
      col = 0
      for k in self.logic.keys:
        item = qt.QStandardItem()
        item.setText(str(self.logic.labelStats[i,k]))
        self.model.setItem(row,col,item)
        self.items.append(item)
        col += 1
      row += 1

    col = 0
    for k in self.logic.keys:
      self.view.setColumnWidth(col,15*len(k))
      self.model.setHeaderData(col,1,k)
      col += 1

class DoseVolumeHistogramLogic:
  """Implement the logic to calculate label statistics.
  Nodes are passed in as arguments.
  Results are stored as 'statistics' instance variable.
  """
  
  def __init__(self, doseImageNode, labelNode, fileName=None):
    #import numpy

    self.keys = ("Index", "Count", "Mean dose", "Total volume (cc)", "Max dose", "Min dose")        

    cubicMMPerVoxel = reduce(lambda x,y: x*y, labelNode.GetSpacing())
    ccPerCubicMM = 0.001
    
    # TODO: progress and status updates
    # this->InvokeEvent(vtkDoseVolumeHistogramLogic::StartLabelStats, (void*)"start label stats")
    
    self.labelStats = {}
    self.labelStats['Labels'] = []
   
    stataccum = vtk.vtkImageAccumulate()
    stataccum.SetInput(labelNode.GetImageData())
    stataccum.Update()
    lo = int(stataccum.GetMin()[0])
    # don't compute DVH the background (voxels)
    if lo == 0:
      lo = 1
    hi = int(stataccum.GetMax()[0])

    for i in xrange(lo,hi+1):

      # this->SetProgress((float)i/hi);
      # std::string event_message = "Label "; std::stringstream s; s << i; event_message.append(s.str());
      # this->InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsOuterLoop, (void*)event_message.c_str());

      # logic copied from slicer3 DoseVolumeHistogram
      # to create the binary volume of the label
      # //logic copied from slicer2 DoseVolumeHistogram MaskStat
      # // create the binary volume of the label
      thresholder = vtk.vtkImageThreshold()
      thresholder.SetInput(labelNode.GetImageData())
      thresholder.SetInValue(1)
      thresholder.SetOutValue(0)
      thresholder.ReplaceOutOn()
      thresholder.ThresholdBetween(i,i)
      thresholder.SetOutputScalarType(doseImageNode.GetImageData().GetScalarType())
      thresholder.Update()
      
      # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"0.25");
      
      #  use vtk's statistics class with the binary labelmap as a stencil
      stencil = vtk.vtkImageToImageStencil()
      stencil.SetInput(thresholder.GetOutput())
      stencil.ThresholdBetween(1, 1)
      
      # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"0.5")
      
      stat1 = vtk.vtkImageAccumulate()
      stat1.SetInput(doseImageNode.GetImageData())
      stat1.SetStencil(stencil.GetOutput())
      stat1.Update()
    
      # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"0.75")

      if stat1.GetVoxelCount() > 0:
        # add an entry to the LabelStats list
        self.labelStats["Labels"].append(i)
        self.labelStats[i,"Index"] = i
        self.labelStats[i,"Count"] = stat1.GetVoxelCount()
        self.labelStats[i,"Mean dose"] = stat1.GetMean()[0]
        self.labelStats[i,"Total volume (cc)"] = self.labelStats[i,"Count"] * cubicMMPerVoxel * ccPerCubicMM
        self.labelStats[i,"Max dose"] = stat1.GetMax()[0]
        self.labelStats[i,"Min dose"] = stat1.GetMin()[0]
        
        # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"1")

    # this.InvokeEvent(vtkDoseVolumeHistogramLogic::EndLabelStats, (void*)"end label stats")

  def createStatsChart(self,doseImageNode, labelNode):
    """Make a MRML chart of the current stats
    """
    layoutNodes = slicer.mrmlScene.GetNodesByClass('vtkMRMLLayoutNode')
    layoutNodes.InitTraversal()
    layoutNode = layoutNodes.GetNextItemAsObject()
    layoutNode.SetViewArrangement(slicer.vtkMRMLLayoutNode.SlicerLayoutConventionalQuantitativeView)

    chartViewNodes = slicer.mrmlScene.GetNodesByClass('vtkMRMLChartViewNode')
    chartViewNodes.InitTraversal()
    chartViewNode = chartViewNodes.GetNextItemAsObject()


    stataccum = vtk.vtkImageAccumulate()
    stataccum.SetInput(labelNode.GetImageData())
    stataccum.Update()
    lo = int(stataccum.GetMin()[0])
    # don't compute DVH the background (voxels)
    if lo == 0:
      lo = 1
    hi = int(stataccum.GetMax()[0])

    chartNode = slicer.mrmlScene.CreateNodeByClass('vtkMRMLChartNode')
    chartNode = slicer.mrmlScene.AddNode(chartNode)
    
    for i in xrange(lo,hi+1):

      # this->SetProgress((float)i/hi);
      # std::string event_message = "Label "; std::stringstream s; s << i; event_message.append(s.str());
      # this->InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsOuterLoop, (void*)event_message.c_str());

      # logic copied from slicer3 DoseVolumeHistogram
      # to create the binary volume of the label
      # //logic copied from slicer2 DoseVolumeHistogram MaskStat
      # // create the binary volume of the label
      thresholder = vtk.vtkImageThreshold()
      thresholder.SetInput(labelNode.GetImageData())
      thresholder.SetInValue(1)
      thresholder.SetOutValue(0)
      thresholder.ReplaceOutOn()
      thresholder.ThresholdBetween(i,i)
      thresholder.SetOutputScalarType(doseImageNode.GetImageData().GetScalarType())
      thresholder.Update()
      
      # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"0.25");
      
      #  use vtk's statistics class with the binary labelmap as a stencil
      stencil = vtk.vtkImageToImageStencil()
      stencil.SetInput(thresholder.GetOutput())
      stencil.ThresholdBetween(1, 1)
      
      # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"0.5")
      
      stat1 = vtk.vtkImageAccumulate()
      stat1.SetInput(doseImageNode.GetImageData())
      stat1.SetStencil(stencil.GetOutput())
      stat1.Update()

      numBins=100
      rangeMin=stat1.GetMin()[0]
      rangeMax=stat1.GetMax()[0]
      spacing=(rangeMax - rangeMin) / numBins

      stat1.SetComponentExtent(0,numBins-1,0,0,0,0)
      stat1.SetComponentOrigin(rangeMin,0,0)
      stat1.SetComponentSpacing(spacing,1,1)
      stat1.Update()
	  
      self.stat1=stat1
  
      # this.InvokeEvent(vtkDoseVolumeHistogramLogic::LabelStatsInnerLoop, (void*)"0.75")
      
      if stat1.GetVoxelCount() > 0:
        arrayNode = slicer.mrmlScene.CreateNodeByClass('vtkMRMLDoubleArrayNode')
        arrayNode = slicer.mrmlScene.AddNode(arrayNode)
        array = arrayNode.GetArray()
        array.SetNumberOfTuples(numBins+1) # +1 because there is a fixed point at (0.0, 1.0)

        totalVoxels=stat1.GetVoxelCount()
        voxelBelowDose=0;
        print "------------------"
        arrayName = 'DVH ' + str(i)
        print arrayName
        array.SetComponent(0, 0, 0.0)
        array.SetComponent(0, 1, 1.0)
        array.SetComponent(0, 2, 0)          
        for sampleIndex in xrange(0,numBins):		  
          voxelsInBin=stat1.GetOutput().GetScalarComponentAsDouble(sampleIndex,0,0,0)
          print "X=",sampleIndex
          print "Y=",voxelsInBin
          array.SetComponent(sampleIndex+1, 0, rangeMin+sampleIndex*spacing)
          array.SetComponent(sampleIndex+1, 1, 1.0-voxelBelowDose/totalVoxels)
          array.SetComponent(sampleIndex+1, 2, 0)          
          voxelBelowDose+=voxelsInBin
                  
        chartNode.AddArray(arrayName, arrayNode.GetID())

    chartViewNode.SetChartNodeID(chartNode.GetID())

    chartNode.SetProperty('default', 'title', 'DVH')
    chartNode.SetProperty('default', 'xAxisLabel', 'Dose') # TODO: compute in Gy
    chartNode.SetProperty('default', 'yAxisLabel', 'Fractional volume')
    chartNode.SetProperty('default', 'type', 'Line');

    chartViewNode.SetChartNodeID(chartNode.GetID())

  def statsAsCSV(self):
    """
    print comma separated value file with header keys in quotes
    """
    csv = ""
    header = ""
    for k in self.keys[:-1]:
      header += "\"%s\"" % k + ","
    header += "\"%s\"" % self.keys[-1] + "\n"
    csv = header
    for i in self.labelStats["Labels"]:
      line = ""
      for k in self.keys[:-1]:
        line += str(self.labelStats[i,k]) + ","
      line += str(self.labelStats[i,self.keys[-1]]) + "\n"
      csv += line
    return csv

  def saveStats(self,fileName):
    fp = open(fileName, "w")
    fp.write(self.statsAsCSV())
    fp.close()

      

class Slicelet(object):
  """A slicer slicelet is a module widget that comes up in stand alone mode
  implemented as a python class.
  This class provides common wrapper functionality used by all slicer modlets.
  """
  # TODO: put this in a SliceletLib 
  # TODO: parse command line arge


  def __init__(self, widgetClass=None):
    self.parent = qt.QFrame()
    self.parent.setLayout( qt.QVBoxLayout() )

    # TODO: should have way to pop up python interactor
    self.buttons = qt.QFrame()
    self.buttons.setLayout( qt.QHBoxLayout() )
    self.parent.layout().addWidget(self.buttons)
    self.addDataButton = qt.QPushButton("Add Data")
    self.buttons.layout().addWidget(self.addDataButton)
    self.addDataButton.connect("clicked()",slicer.app.ioManager().openAddDataDialog)
    self.loadSceneButton = qt.QPushButton("Load Scene")
    self.buttons.layout().addWidget(self.loadSceneButton)
    self.loadSceneButton.connect("clicked()",slicer.app.ioManager().openLoadSceneDialog)

    if widgetClass:
      self.widget = widgetClass(self.parent)
      self.widget.setup()
    self.parent.show()

class DoseVolumeHistogramSlicelet(Slicelet):
  """ Creates the interface when module is run as a stand alone gui app.
  """

  def __init__(self):
    super(DoseVolumeHistogramSlicelet,self).__init__(DoseVolumeHistogramWidget)


if __name__ == "__main__":
  # TODO: need a way to access and parse command line arguments
  # TODO: ideally command line args should handle --xml

  import sys
  print( sys.argv )

  slicelet = DoseVolumeHistogramSlicelet()
