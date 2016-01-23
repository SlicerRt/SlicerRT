/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "qSlicerSegmentEditorPaintEffect.h"

// Qt includes
#include <QDebug>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTrivialProducer.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyData.h>
#include <vtkCollection.h>
#include <vtkActor2D.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
//#include <vtkInteractorObserver.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
#include <vtkMRMLSliceNode.h>

// Slicer includes
#include "qMRMLSliceWidget.h"

//-----------------------------------------------------------------------------
/// Container class handling objects for painting for each slice view
class PaintEffectBrush: public QObject
{
public:
  PaintEffectBrush()
  {
    this->PolyData = vtkPolyData::New();
    this->Mapper = vtkPolyDataMapper2D::New();
    this->Mapper->SetInputData(this->PolyData);
    this->Actor = vtkActor2D::New();
    this->Actor->SetMapper(this->Mapper);
    this->Actor->VisibilityOff();
  };
  ~PaintEffectBrush()
  {
    this->Actor->Delete();
    this->Actor = NULL;
    this->Mapper->Delete();
    this->Mapper = NULL;
    this->PolyData->Delete();
    this->PolyData = NULL;
  };
public:
  vtkActor2D* Actor;
  vtkPolyDataMapper2D* Mapper;
  vtkPolyData* PolyData;
};

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorPaintEffectPrivate methods

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorPaintEffectPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorPaintEffect);
protected:
  qSlicerSegmentEditorPaintEffect* const q_ptr;
public:
  qSlicerSegmentEditorPaintEffectPrivate(qSlicerSegmentEditorPaintEffect& object);
  ~qSlicerSegmentEditorPaintEffectPrivate();

  /// Depending on the \sa DelayedPaint mode, either paint the given point or queue
  /// it up with a marker for later painting
  void paintAddPoint(qMRMLSliceWidget* sliceWidget, int x, int y);

  /// Draw paint circle glyph
  void createBrushGlyph(qMRMLSliceWidget* sliceWidget);

protected:
  /// Get brush object for widget. Create if does not exist
  PaintEffectBrush* brushForWidget(qMRMLSliceWidget* sliceWidget);

  /// Add a feedback actor (copy of the paint radius actor) for any points that don't
  /// have one yet. If the list is empty, clear out the old actors
  void paintFeedback(qMRMLSliceWidget* sliceWidget);

  /// Paint labelmap
  void paintApply(qMRMLSliceWidget* sliceWidget);

  /// Paint with a brush that is circular (or optionally spherical) in XY space
  /// (could be stretched or rotate when transformed to IJK)
  /// - Make sure to hit every pixel in IJK space
  /// - Apply the threshold if selected
  void paintBrush(QPoint xy);

  /// Paint one pixel to coordinate
  void paintPixel(QPoint xy);

public:
  QIcon EffectIcon;

  QList<QPoint> PaintCoordinates;
  QList<vtkActor2D*>  FeedbackActors;
  QMap<qMRMLWidget*, PaintEffectBrush*> Brushes;
  bool DelayedPaint;
  bool PixelMode;
  bool IsPainting;
  bool Smudge;
  bool Sphere;
  double Radius;
  double MinimumRadius;
  double MaximumRadius;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffectPrivate::qSlicerSegmentEditorPaintEffectPrivate(qSlicerSegmentEditorPaintEffect& object)
  : q_ptr(&object)
  , DelayedPaint(true)
  , PixelMode(false)
  , IsPainting(false)
  , Smudge(false)
  , Sphere(false)
  , Radius(0.0)
  , MinimumRadius(0.0)
  , MaximumRadius(0.0)
{
  this->EffectIcon = QIcon(":Icons/Paint.png");

  /*
    self.createGlyph(self.brush)
    self.mapper = vtk.vtkPolyDataMapper2D()
    self.actor = vtk.vtkActor2D()
    self.mapper.SetInputData(self.brush)
    self.actor.SetMapper(self.mapper)
    self.actor.VisibilityOff()

    self.renderer.AddActor2D(self.actor)
    self.actors.append(self.actor)
  */

  this->PaintCoordinates.clear();
  this->FeedbackActors.clear();
  this->Brushes.clear();
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffectPrivate::~qSlicerSegmentEditorPaintEffectPrivate()
{
  this->PaintCoordinates.clear();
  this->FeedbackActors.clear();

  foreach (PaintEffectBrush* brush, this->Brushes)
  {
    delete brush;
  }
  this->Brushes.clear();
}

//-----------------------------------------------------------------------------
PaintEffectBrush* qSlicerSegmentEditorPaintEffectPrivate::brushForWidget(qMRMLSliceWidget* sliceWidget)
{
  if (this->Brushes.contains(sliceWidget))
  {
    return this->Brushes[sliceWidget];
  }

  // Create brush if does not yet exist
  PaintEffectBrush* brush = new PaintEffectBrush();

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(sliceWidget);
  if (!renderer)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::brushForWidget: Failed to get renderer!";
  }
  else
  {
    renderer->AddActor2D(brush->Actor);
  }

  this->Brushes[sliceWidget] = brush;
  this->createBrushGlyph(sliceWidget);
  return brush;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintAddPoint(qMRMLSliceWidget* sliceWidget, int x, int y)
{
  this->PaintCoordinates << QPoint(x,y);

  if (this->DelayedPaint && !this->PixelMode)
  {
    this->paintFeedback(sliceWidget);
  }
  else
  {
    this->paintApply(sliceWidget);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintFeedback(qMRMLSliceWidget* sliceWidget)
{
  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(sliceWidget);
  if (!renderer)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::paintFeedback: Failed to get renderer!";
    return;
  }

  if (this->PaintCoordinates.isEmpty())
  {
    foreach (vtkActor2D* actor, this->FeedbackActors)
    {
      renderer->RemoveActor2D(actor);
    }
    this->FeedbackActors.clear();
    return;
  }

  foreach (QPoint xy, this->PaintCoordinates)
  {
    vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
    this->FeedbackActors << actor.GetPointer();

    PaintEffectBrush* brush = this->brushForWidget(sliceWidget);
    actor->SetMapper(brush->Mapper);
    actor->SetPosition(xy.x(), xy.y());
    vtkProperty2D* property = actor->GetProperty();
    property->SetColor(0.7, 0.7, 0.0);
    property->SetOpacity(0.5);
    renderer->AddActor2D(actor);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintApply(qMRMLSliceWidget* sliceWidget)
{
    //if self.paintCoordinates != []:
    //  if self.undoRedo:
    //    self.undoRedo.saveState()

  foreach (QPoint xy, this->PaintCoordinates)
  {
    if (this->PixelMode)
    {
      this->paintPixel(xy);
    }
    else
    {
      this->paintBrush(xy);
    }
    this->PaintCoordinates.clear();
    this->paintFeedback(sliceWidget);
  }

    //# TODO: workaround for new pipeline in slicer4
    //# - editing image data of the calling modified on the node
    //#   does not pull the pipeline chain
    //# - so we trick it by changing the image data first
    //sliceLogic = self.sliceWidget.sliceLogic()
    //labelLogic = sliceLogic.GetLabelLayer()
    //labelNode = labelLogic.GetVolumeNode()
    //EditUtil.markVolumeNodeAsModified(labelNode)
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintBrush(QPoint xy)
{
  /*
    sliceLogic = self.sliceWidget.sliceLogic()
    sliceNode = sliceLogic.GetSliceNode()
    labelLogic = sliceLogic.GetLabelLayer()
    labelNode = labelLogic.GetVolumeNode()
    labelImage = labelNode.GetImageData()
    backgroundLogic = sliceLogic.GetBackgroundLayer()
    backgroundNode = backgroundLogic.GetVolumeNode()
    backgroundImage = backgroundNode.GetImageData()

    if not labelNode:
      # if there's no label, we can't paint
      return

    #
    # get the brush bounding box in ijk coordinates
    # - get the xy bounds
    # - transform to ijk
    # - clamp the bounds to the dimensions of the label image
    #
    bounds = self.brush.GetPoints().GetBounds()
    left = x + bounds[0]
    right = x + bounds[1]
    bottom = y + bounds[2]
    top = y + bounds[3]

    xyToIJK = labelLogic.GetXYToIJKTransform()
    tlIJK = xyToIJK.TransformDoublePoint( (left, top, 0) )
    trIJK = xyToIJK.TransformDoublePoint( (right, top, 0) )
    blIJK = xyToIJK.TransformDoublePoint( (left, bottom, 0) )
    brIJK = xyToIJK.TransformDoublePoint( (right, bottom, 0) )

    dims = labelImage.GetDimensions()

    # clamp the top, bottom, left, right to the
    # valid dimensions of the label image
    tl = [0,0,0]
    tr = [0,0,0]
    bl = [0,0,0]
    br = [0,0,0]
    for i in xrange(3):
      tl[i] = int(round(tlIJK[i]))
      if tl[i] < 0:
        tl[i] = 0
      if tl[i] >= dims[i]:
        tl[i] = dims[i] - 1
      tr[i] = int(round(trIJK[i]))
      if tr[i] < 0:
        tr[i] = 0
      if tr[i] > dims[i]:
        tr[i] = dims[i] - 1
      bl[i] = int(round(blIJK[i]))
      if bl[i] < 0:
        bl[i] = 0
      if bl[i] > dims[i]:
        bl[i] = dims[i] - 1
      br[i] = int(round(brIJK[i]))
      if br[i] < 0:
        br[i] = 0
      if br[i] > dims[i]:
        br[i] = dims[i] - 1

    # If the region is smaller than a pixel then paint it using paintPixel mode,
    # to make sure at least one pixel is filled on each click
    maxRowDelta = 0
    maxColumnDelta = 0
    for i in xrange(3):
      d = abs(tr[i] - tl[i])
      if d > maxColumnDelta:
        maxColumnDelta = d
      d = abs(br[i] - bl[i])
      if d > maxColumnDelta:
        maxColumnDelta = d
      d = abs(bl[i] - tl[i])
      if d > maxRowDelta:
        maxRowDelta = d
      d = abs(br[i] - tr[i])
      if d > maxRowDelta:
        maxRowDelta = d
    if maxRowDelta<=1 or maxColumnDelta<=1 :
      self.paintPixel(x,y)
      return

    #
    # get the layers and nodes
    # and ijk to ras matrices including transforms
    #
    labelLogic = self.sliceLogic.GetLabelLayer()
    labelNode = labelLogic.GetVolumeNode()
    backgroundLogic = self.sliceLogic.GetLabelLayer()
    backgroundNode = backgroundLogic.GetVolumeNode()
    backgroundIJKToRAS = self.logic.getIJKToRASMatrix(backgroundNode)
    labelIJKToRAS = self.logic.getIJKToRASMatrix(labelNode)


    xyToRAS = sliceNode.GetXYToRAS()
    brushCenter = xyToRAS.MultiplyPoint( (x, y, 0, 1) )[:3]


    brushRadius = self.radius
    bSphere = self.sphere

    parameterNode = EditUtil.getParameterNode()
    paintLabel = int(parameterNode.GetParameter("label"))
    paintOver = int(parameterNode.GetParameter("LabelEffect,paintOver"))
    paintThreshold = int(parameterNode.GetParameter("LabelEffect,paintThreshold"))
    paintThresholdMin = float(
        parameterNode.GetParameter("LabelEffect,paintThresholdMin"))
    paintThresholdMax = float(
        parameterNode.GetParameter("LabelEffect,paintThresholdMax"))

    #
    # set up the painter class and let 'r rip!
    #
    if not hasattr(self,"painter"):
      self.painter = slicer.vtkImageSlicePaint()

    self.painter.SetBackgroundImage(backgroundImage)
    self.painter.SetBackgroundIJKToWorld(backgroundIJKToRAS)
    self.painter.SetWorkingImage(labelImage)
    self.painter.SetWorkingIJKToWorld(labelIJKToRAS)
    self.painter.SetTopLeft( tl[0], tl[1], tl[2] )
    self.painter.SetTopRight( tr[0], tr[1], tr[2] )
    self.painter.SetBottomLeft( bl[0], bl[1], bl[2] )
    self.painter.SetBottomRight( br[0], br[1], br[2] )
    self.painter.SetBrushCenter( brushCenter[0], brushCenter[1], brushCenter[2] )
    self.painter.SetBrushRadius( brushRadius )
    self.painter.SetPaintLabel(paintLabel)
    self.painter.SetPaintOver(paintOver)
    self.painter.SetThresholdPaint(paintThreshold)
    self.painter.SetThresholdPaintRange(paintThresholdMin, paintThresholdMax)

    if bSphere:  # fill volume of a sphere rather than a circle on the currently displayed image slice
        # Algorithm:
        ###########################
        # Assume brushRadius is in mm
        # Estimate zVoxelSize
        # Compute number of slices spanned by sphere, that are still within the volume
        # For each spanned slice
        #    reposition the brushCenter using xy(z)ToRAS transform: i.e. canvas to patient world coordinates
        #    resize the radius: brushRadiusOffset=sqrt(brushRadius*brushRadius - zOffset_mm*zOffset_mm)
        #    invoke Paint()
        # Finally paint on the center slice, leaving the gui on the center slice being most visibly edited
        #------------------
        # Estimate zVoxelSize_mm
        brushCenter1 = xyToRAS.MultiplyPoint( (x, y, 0, 1) )[:3]
        brushCenter2 = xyToRAS.MultiplyPoint( (x, y, 100, 1) )[:3]
        dx1=brushCenter1[0]-brushCenter2[0]
        dx2=brushCenter1[1]-brushCenter2[1]
        dx3=brushCenter1[2]-brushCenter2[2]
        distanceSpannedBy100Slices = sqrt(dx1*dx1+dx2*dx2+dx3*dx3)  # compute L2 norm
        if distanceSpannedBy100Slices==0:
            zVoxelSize_mm=1
        else:
            zVoxelSize_mm = distanceSpannedBy100Slices/100
        # --
        # Compute number of slices spanned by sphere
        nNumSlicesInEachDirection=brushRadius / zVoxelSize_mm;
        nNumSlicesInEachDirection=nNumSlicesInEachDirection-1
        sliceOffsetArray=numpy.concatenate((-1*numpy.arange(1,nNumSlicesInEachDirection+1,),  numpy.arange(1,nNumSlicesInEachDirection+1)))
        for iSliceOffset in sliceOffsetArray:
            # x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the slice you're looking at)
            # hence xyToRAS is really performing xyzToRAS.   RAS is patient world coordinate system. Note the 1 is because the trasform uses homogeneous coordinates
            iBrushCenter = xyToRAS.MultiplyPoint( (x, y, iSliceOffset, 1) )[:3]
            self.painter.SetBrushCenter( iBrushCenter[0], iBrushCenter[1], iBrushCenter[2] )
            # [ ] Need to just continue (pass this loop iteration if the brush center is not within the volume
            zOffset_mm=zVoxelSize_mm*iSliceOffset;
            brushRadiusOffset=sqrt(brushRadius*brushRadius - zOffset_mm*zOffset_mm)
            self.painter.SetBrushRadius( brushRadiusOffset )

            # --
            tlIJKtemp = xyToIJK.TransformDoublePoint( (left, top, iSliceOffset) )
            trIJKtemp = xyToIJK.TransformDoublePoint( (right, top, iSliceOffset) )
            blIJKtemp = xyToIJK.TransformDoublePoint( (left, bottom, iSliceOffset) )
            brIJKtemp = xyToIJK.TransformDoublePoint( (right, bottom, iSliceOffset) )
            # clamp the top, bottom, left, right to the
            # valid dimensions of the label image
            tltemp = [0,0,0]
            trtemp = [0,0,0]
            bltemp = [0,0,0]
            brtemp = [0,0,0]
            for i in xrange(3):
              tltemp[i] = int(round(tlIJKtemp[i]))
              if tltemp[i] < 0:
                tltemp[i] = 0
              if tltemp[i] >= dims[i]:
                tltemp[i] = dims[i] - 1
              trtemp[i] = int(round(trIJKtemp[i]))
              if trtemp[i] < 0:
                trtemp[i] = 0
              if trtemp[i] > dims[i]:
                trtemp[i] = dims[i] - 1
              bltemp[i] = int(round(blIJKtemp[i]))
              if bltemp[i] < 0:
                bltemp[i] = 0
              if bltemp[i] > dims[i]:
                bltemp[i] = dims[i] - 1
              brtemp[i] = int(round(brIJKtemp[i]))
              if brtemp[i] < 0:
                brtemp[i] = 0
              if brtemp[i] > dims[i]:
                brtemp[i] = dims[i] - 1
            self.painter.SetTopLeft( tltemp[0], tltemp[1], tltemp[2] )
            self.painter.SetTopRight( trtemp[0], trtemp[1], trtemp[2] )
            self.painter.SetBottomLeft( bltemp[0], bltemp[1], bltemp[2] )
            self.painter.SetBottomRight( brtemp[0], brtemp[1], brtemp[2] )

            self.painter.Paint()

    # paint the slice: same for circular and spherical brush modes
    self.painter.SetTopLeft( tl[0], tl[1], tl[2] )
    self.painter.SetTopRight( tr[0], tr[1], tr[2] )
    self.painter.SetBottomLeft( bl[0], bl[1], bl[2] )
    self.painter.SetBottomRight( br[0], br[1], br[2] )
    self.painter.SetBrushCenter( brushCenter[0], brushCenter[1], brushCenter[2] )
    self.painter.SetBrushRadius( brushRadius )
    self.painter.Paint()
  */
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintPixel(QPoint xy)
{
  /*
    sliceLogic = self.sliceWidget.sliceLogic()
    labelLogic = sliceLogic.GetLabelLayer()
    labelNode = labelLogic.GetVolumeNode()
    labelImage = labelNode.GetImageData()

    if not labelNode:
      # if there's no label, we can't paint
      return

    xyToIJK = labelLogic.GetXYToIJKTransform()
    ijkFloat = xyToIJK.TransformDoublePoint( (x, y, 0) )
    ijk = []
    for e in ijkFloat:
      try:
        index = int(round(e))
      except ValueError:
        return
      ijk.append(index)
    dims = labelImage.GetDimensions()
    for e,d in zip(ijk,dims): # clamp to volume extent
      if e < 0 or e >= d:
        return

    parameterNode = EditUtil.getParameterNode()
    paintLabel = int(parameterNode.GetParameter("label"))
    labelImage.SetScalarComponentFromFloat(ijk[0],ijk[1],ijk[2],0, paintLabel)
    EditUtil.markVolumeNodeAsModified(labelNode)
  */
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::createBrushGlyph(qMRMLSliceWidget* sliceWidget)
{
  /*
    sliceNode = self.sliceWidget.sliceLogic().GetSliceNode()
    self.rasToXY.DeepCopy(sliceNode.GetXYToRAS())
    self.rasToXY.Invert()
    maximum, maxIndex = 0,0
    for index in range(3):
      if abs(self.rasToXY.GetElement(0, index)) > maximum:
        maximum = abs(self.rasToXY.GetElement(0, index))
        maxIndex = index
    point = [0, 0, 0, 0]
    point[maxIndex] = self.radius
    xyRadius = self.rasToXY.MultiplyPoint(point)
    import math
    xyRadius = math.sqrt( xyRadius[0]**2 + xyRadius[1]**2 + xyRadius[2]**2 )

    if self.pixelMode:
      xyRadius = 0.01

    # make a circle paint brush
    points = vtk.vtkPoints()
    lines = vtk.vtkCellArray()
    polyData.SetPoints(points)
    polyData.SetLines(lines)
    PI = 3.1415926
    TWOPI = PI * 2
    PIoverSIXTEEN = PI / 16
    prevPoint = -1
    firstPoint = -1
    angle = 0
    while angle <= TWOPI:
      x = xyRadius * math.cos(angle)
      y = xyRadius * math.sin(angle)
      p = points.InsertNextPoint( x, y, 0 )
      if prevPoint != -1:
        idList = vtk.vtkIdList()
        idList.InsertNextId(prevPoint)
        idList.InsertNextId(p)
        polyData.InsertNextCell( vtk.VTK_LINE, idList )
      prevPoint = p
      if firstPoint == -1:
        firstPoint = p
      angle = angle + PIoverSIXTEEN

    # make the last line in the circle
    idList = vtk.vtkIdList()
    idList.InsertNextId(p)
    idList.InsertNextId(firstPoint)
    polyData.InsertNextCell( vtk.VTK_LINE, idList )
  */
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorPaintEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffect::qSlicerSegmentEditorPaintEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorPaintEffectPrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffect::~qSlicerSegmentEditorPaintEffect()
{
}

//---------------------------------------------------------------------------
QString qSlicerSegmentEditorPaintEffect::name()
{
  return QString("Paint");
}

//---------------------------------------------------------------------------
QIcon qSlicerSegmentEditorPaintEffect::icon()
{
  Q_D(qSlicerSegmentEditorPaintEffect);

  return d->EffectIcon;
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorPaintEffect::clone()
{
  return new qSlicerSegmentEditorPaintEffect();
}

//---------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::deactivate()
{
  /*
    for a in self.feedbackActors:
      self.renderer.RemoveActor2D(a)
    self.sliceView.scheduleRender()
  */
}

//---------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::processInteractionEvents(
  vtkRenderWindowInteractor* callerInteractor,
  unsigned long eid,
  qMRMLWidget* viewWidget )
{
  Q_D(qSlicerSegmentEditorPaintEffect);

  // This effect only supports interactions in the 2D slice views currently
  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  if (!sliceWidget)
  {
    return;
  }

  if (eid == vtkCommand::LeftButtonPressEvent)
  {
    d->IsPainting = true;
    if (!d->PixelMode)
    {
      this->cursorOff(sliceWidget);
    }
    int x = 0;
    int y = 0;
    callerInteractor->GetEventPosition(x, y);
    if (d->Smudge)
    {
      //EditUtil.setLabel(self.getLabelPixel(xy))
    }
    d->paintAddPoint(sliceWidget, x, y);
    this->abortEvent(callerInteractor, eid, sliceWidget);
  }
  else if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    //self.paintApply()
    //self.actionState = None
    //self.cursorOn()
  }
  else if (eid == vtkCommand::MouseMoveEvent)
  {
    //self.actor.VisibilityOn()
    //if self.actionState == "painting":
    //  xy = callerInteractor->GetEventPosition()
    //  self.paintAddPoint(xy[0], xy[1])
    //  self.abortEvent(event)
  }
  else if (eid == vtkCommand::EnterEvent)
  {
    //self.actor.VisibilityOn()
  }
  else if (eid == vtkCommand::LeaveEvent)
  {
    //self.actor.VisibilityOff()
  }
  else if (eid == vtkCommand::KeyPressEvent)
  {
    //key = callerInteractor->GetKeySym()
    //if key == 'plus' or key == 'equal':
    //  self.scaleRadius(1.2)
    //if key == 'minus' or key == 'underscore':
    //  self.scaleRadius(0.8)
  }

  //if caller and caller.IsA('vtkMRMLSliceNode'):
  //  if hasattr(self,'brush'):
  //    self.createGlyph(self.brush)

  //self.positionActors()
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::updateGUIFromMRML(vtkObject* caller, void* callData)
{
  Q_D(qSlicerSegmentEditorPaintEffect);

  // Get parameter set node
  vtkMRMLSegmentEditorEffectNode* parameterNode = reinterpret_cast<vtkMRMLSegmentEditorEffectNode*>(caller);
  if (!parameterNode)
  {
    return;
  }

  //TODO: Update effect options widgets

  // Update brushes
  //d->updateBrushes(); //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::apply()
{
  //TODO:
}
