import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorDrawEffect(AbstractScriptedSegmentEditorLabelEffect):
  """ DrawEffect is a LabelEffect implementing the interactive draw
      tool in the segment editor
  """
  
  # Necessary static member to be able to set python source to scripted subject hierarchy plugin
  filePath = __file__

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'Draw'
    AbstractScriptedSegmentEditorLabelEffect.__init__(self, scriptedEffect)

    # Effect-specific members
    self.drawPipelines = {}

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedLabelEffect(None)
    clonedEffect.setPythonSource(SegmentEditorDrawEffect.filePath)
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/Draw.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()
    
  def helpText(self):
    return "Use this tool to draw an outline.\n\nLeft Click: add point.\nLeft Drag: add multiple points.\nx: delete last point.\na: apply outline."
    
  def deactivate(self):
    # Clear draw pipelines
    self.drawPipelines = {}

  def setupOptionsFrame(self):
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Apply current outline.\nUse the 'a' or 'Enter' hotkey to apply in slice window")
    self.scriptedEffect.addOptionsWidget(self.applyButton)

    self.applyButton.connect('clicked()', self.onApply)

  def processInteractionEvents(self, callerInteractor, eventId, viewWidget):
    # Only allow for slice views
    if viewWidget.className() != "qMRMLSliceWidget":
      return
    # Get draw pipeline for current slice
    pipeline = self.pipelineForWidget(viewWidget)
    if pipeline is None:
      return

    if eventId == vtk.vtkCommand.LeftButtonPressEvent:
      pipeline.actionState = "drawing"
      self.scriptedEffect.cursorOff(viewWidget)
      xy = callerInteractor.GetEventPosition()
      ras = self.xyToRas(xy, viewWidget)
      pipeline.addPoint(ras)
      self.scriptedEffect.abortEvent(callerInteractor, eventId, viewWidget)
    elif eventId == vtk.vtkCommand.LeftButtonReleaseEvent:
      pipeline.actionState = ""
      self.scriptedEffect.cursorOn(viewWidget)
    elif eventId == vtk.vtkCommand.RightButtonPressEvent:
      sliceNode = viewWidget.sliceLogic().GetSliceNode()
      pipeline.lastInsertSliceNodeMTime = sliceNode.GetMTime()
    elif eventId == vtk.vtkCommand.RightButtonReleaseEvent:
      sliceNode = viewWidget.sliceLogic().GetSliceNode()
      if abs(pipeline.lastInsertSliceNodeMTime - sliceNode.GetMTime()) < 2:
        pipeline.apply()
        pipeline.actionState = None
    elif eventId == vtk.vtkCommand.MouseMoveEvent:
      if pipeline.actionState == "drawing":
        xy = callerInteractor.GetEventPosition()
        ras = self.xyToRas(xy, viewWidget)
        pipeline.addPoint(ras)
        self.scriptedEffect.abortEvent(callerInteractor, eventId, viewWidget)
    elif eventId == vtk.vtkCommand.KeyPressEvent:
      key = callerInteractor.GetKeySym()
      if key == 'a' or key == 'Return':
        pipeline.apply()
        self.scriptedEffect.abortEvent(callerInteractor, eventId, viewWidget)
      if key == 'x':
        pipeline.deleteLastPoint()
        self.scriptedEffect.abortEvent(callerInteractor, eventId, viewWidget)
    else:
      pass

    pipeline.positionActors()

  def processViewNodeEvents(self, callerViewNode, eventId, viewWidget):
    if callerViewNode and callerViewNode.IsA('vtkMRMLSliceNode'):
      # Get draw pipeline for current slice
      pipeline = self.pipelineForWidget(viewWidget)
      if pipeline is None:
        return

      # Make sure all points are on the current slice plane.
      # If the SliceToRAS has been modified, then we're on a different plane
      sliceLogic = viewWidget.sliceLogic()
      lineMode = "solid"
      currentSlice = sliceLogic.GetSliceOffset()
      if pipeline.activeSlice:
        offset = abs(currentSlice - pipeline.activeSlice)
        if offset > 0.01:
          lineMode = "dashed"
      pipeline.setLineMode(lineMode)

    pipeline.positionActors()

  def onApply(self):
    for sliceWidget in self.drawPipelines:
      pipeline = self.drawPipelines[sliceWidget]
      pipeline.apply()

  def pipelineForWidget(self, sliceWidget):
    if sliceWidget in self.drawPipelines:
      return self.drawPipelines[sliceWidget]

    # Create pipeline if does not yet exist
    pipeline = DrawPipeline(self.scriptedEffect, sliceWidget)

    # Add actor
    renderer = self.scriptedEffect.renderer(sliceWidget)
    if renderer is None:
      logging.error("setupPreviewDisplay: Failed to get renderer!")
      return None
    self.scriptedEffect.addActor2D(sliceWidget, pipeline.actor)

    self.drawPipelines[sliceWidget] = pipeline
    return pipeline

#
# DrawPipeline
#
class DrawPipeline:
  """ Visualization objects and pipeline for each slice view for drawing
  """
  def __init__(self, scriptedEffect, sliceWidget):
    self.scriptedEffect = scriptedEffect
    self.sliceWidget = sliceWidget
    self.activeSlice = None
    self.lastInsertSliceNodeMTime = None
    self.actionState = None

    self.xyPoints = vtk.vtkPoints()
    self.rasPoints = vtk.vtkPoints()
    self.polyData = self.createPolyData()

    self.mapper = vtk.vtkPolyDataMapper2D()
    self.actor = vtk.vtkActor2D()
    self.mapper.SetInputData(self.polyData)
    self.actor.SetMapper(self.mapper)
    actorProperty = self.actor.GetProperty()
    actorProperty.SetColor(1,1,0)
    actorProperty.SetLineWidth(1)

  def createPolyData(self):
    # Make an empty single-polyline polydata
    polyData = vtk.vtkPolyData()
    polyData.SetPoints(self.xyPoints)

    lines = vtk.vtkCellArray()
    polyData.SetLines(lines)
    idArray = lines.GetData()
    idArray.Reset()
    idArray.InsertNextTuple1(0)

    polygons = vtk.vtkCellArray()
    polyData.SetPolys(polygons)
    idArray = polygons.GetData()
    idArray.Reset()
    idArray.InsertNextTuple1(0)

    return polyData

  def addPoint(self,ras):
    # Add a world space point to the current outline

    # Store active slice when first point is added
    sliceLogic = self.sliceWidget.sliceLogic()
    currentSlice = sliceLogic.GetSliceOffset()
    if not self.activeSlice:
      self.activeSlice = currentSlice
      self.setLineMode("solid")

    # Don't allow adding points on except on the active slice
    # (where first point was laid down)
    if self.activeSlice != currentSlice: return

    # Keep track of node state (in case of pan/zoom)
    sliceNode = sliceLogic.GetSliceNode()
    self.lastInsertSliceNodeMTime = sliceNode.GetMTime()

    p = self.rasPoints.InsertNextPoint(ras)
    lines = self.polyData.GetLines()
    idArray = lines.GetData()
    idArray.InsertNextTuple1(p)
    idArray.SetTuple1(0, idArray.GetNumberOfTuples()-1)
    lines.SetNumberOfCells(1)

  def setLineMode(self,mode="solid"):
    actorProperty = self.actor.GetProperty()
    if mode == "solid":
      actorProperty.SetLineStipplePattern(0xffff)
    elif mode == "dashed":
      actorProperty.SetLineStipplePattern(0xff00)

  def positionActors(self):
    # Update draw feedback to follow slice node
    sliceLogic = self.sliceWidget.sliceLogic()
    sliceNode = sliceLogic.GetSliceNode()
    rasToXY = vtk.vtkTransform()
    rasToXY.SetMatrix(sliceNode.GetXYToRAS())
    rasToXY.Inverse()
    self.xyPoints.Reset()
    rasToXY.TransformPoints(self.rasPoints, self.xyPoints)
    self.polyData.Modified()
    self.sliceWidget.sliceView().scheduleRender()

  def apply(self):
    lines = self.polyData.GetLines()
    if lines.GetNumberOfCells() == 0: return

    # Close the polyline back to the first point
    idArray = lines.GetData()
    p = idArray.GetTuple1(1)
    idArray.InsertNextTuple1(p)
    idArray.SetTuple1(0, idArray.GetNumberOfTuples() - 1)

    # Save state for undo
    #TODO:
    # self.logic.undoRedo = self.undoRedo

    # Get edited labelmap
    import vtkSegmentationCore
    editedLabelmap = self.scriptedEffect.parameterSetNode().GetEditedLabelmap()

    # Apply poly data on edited labelmap
    self.scriptedEffect.appendPolyMask(editedLabelmap, self.polyData, self.sliceWidget)
    self.resetPolyData()

    # Notify editor about changes.
    # This needs to be called so that the changes are written back to the edited segment
    self.scriptedEffect.apply()

  def resetPolyData(self):
    # Return the polyline to initial state with no points
    lines = self.polyData.GetLines()
    idArray = lines.GetData()
    idArray.Reset()
    idArray.InsertNextTuple1(0)
    self.xyPoints.Reset()
    self.rasPoints.Reset()
    lines.SetNumberOfCells(0)
    self.activeSlice = None

  def deleteLastPoint(self):
    # Unwind through addPoint list back to empty polydata
    pcount = self.rasPoints.GetNumberOfPoints()
    if pcount <= 0: return

    pcount = pcount - 1
    self.rasPoints.SetNumberOfPoints(pcount)

    lines = self.polyData.GetLines()
    idArray = lines.GetData()
    idArray.SetTuple1(0, pcount)
    idArray.SetNumberOfTuples(pcount+1)

    self.positionActors()
