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

#include "vtkOrientedImageData.h"
#include "vtkMRMLSegmentationNode.h"

// Qt includes
#include <QDebug>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyData.h>
#include <vtkCollection.h>
#include <vtkActor2D.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkPoints.h>
#include <vtkIdList.h>
#include <vtkCellArray.h>
#include <vtkCommand.h>
#include <vtkMath.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLVolumeNode.h>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "vtkMRMLSliceLogic.h"
#include "vtkMRMLSliceLayerLogic.h"
#include "vtkImageSlicePaint.h"

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
  void createBrushGlyph(qMRMLSliceWidget* sliceWidget, PaintEffectBrush* brush);

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
  void paintBrush(qMRMLSliceWidget* sliceWidget, QPoint xy);

  /// Paint one pixel to coordinate
  void paintPixel(qMRMLSliceWidget* sliceWidget, QPoint xy);

  /// Scale brush radius and save it in parameter node
  void scaleRadius(double scaleFactor);

public:
  QIcon EffectIcon;

  QList<QPoint> PaintCoordinates;
  QList<vtkActor2D*> FeedbackActors;
  QMap<qMRMLWidget*, PaintEffectBrush*> Brushes;
  vtkImageSlicePaint* Painter;
  bool DelayedPaint;
  //TODO: Remove members, add set/get functions that work directly with parameter node (PaintEffect:263)
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
  , Painter(NULL)
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

  this->Painter = vtkImageSlicePaint::New();

  this->PaintCoordinates.clear();
  this->FeedbackActors.clear();
  this->Brushes.clear();
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffectPrivate::~qSlicerSegmentEditorPaintEffectPrivate()
{
  if (this->Painter)
  {
    this->Painter->Delete();
    this->Painter = NULL;
  }
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
  Q_Q(qSlicerSegmentEditorPaintEffect);

  if (this->Brushes.contains(sliceWidget))
  {
    return this->Brushes[sliceWidget];
  }

  // Create brush if does not yet exist
  PaintEffectBrush* brush = new PaintEffectBrush();
  this->createBrushGlyph(sliceWidget, brush);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(sliceWidget);
  if (!renderer)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::brushForWidget: Failed to get renderer!";
  }
  else
  {
    renderer->AddActor2D(brush->Actor);
    q->addActor(sliceWidget, brush->Actor);
  }

  this->Brushes[sliceWidget] = brush;
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
  //TODO:
  //if self.paintCoordinates != []:
  //  if self.undoRedo:
  //    self.undoRedo.saveState()

  foreach (QPoint xy, this->PaintCoordinates)
  {
    if (this->PixelMode)
    {
      this->paintPixel(sliceWidget, xy);
    }
    else
    {
      this->paintBrush(sliceWidget, xy);
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
void qSlicerSegmentEditorPaintEffectPrivate::paintBrush(qMRMLSliceWidget* sliceWidget, QPoint xy)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  vtkOrientedImageData* labelImage = q->m_EditedLabelmap;
  if (!labelImage)
  {
    return;
  }
  
  int x = xy.x();
  int y = xy.y();

  // Get brush for slice widget
  PaintEffectBrush* brush = this->brushForWidget(sliceWidget);

  // Get the brush bounding box in IJK coordinates
  // - Get the xy bounds
  // - Transform to IJK
  // - Clamp the bounds to the dimensions of the label image
  double brushBounds[6];
  vtkOrientedImageData::UninitializeBounds(brushBounds);
  brush->PolyData->GetPoints()->GetBounds(brushBounds);

  int left =   x + int(brushBounds[0] + 0.5);
  int right =  x + int(brushBounds[1] + 0.5);
  int bottom = y + int(brushBounds[2] + 0.5);
  int top =    y + int(brushBounds[3] + 0.5);
  int topLeftIjk[3] =     {0, 0, 0};
  int topRightIjk[3] =    {0, 0, 0};
  int bottomLeftIjk[3] =  {0, 0, 0};
  int bottomRightIjk[3] = {0, 0, 0};
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(left, top),     topLeftIjk, sliceWidget, labelImage);
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(right, top),    topRightIjk, sliceWidget, labelImage);
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(left, bottom),  bottomLeftIjk, sliceWidget, labelImage);
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(right, bottom), bottomRightIjk, sliceWidget, labelImage);

  // Clamp the top, bottom, left, right to the valid dimensions of the label image
  int dims[3] = {0, 0, 0};
  labelImage->GetDimensions(dims);

  int topLeft[3] =     {0, 0, 0};
  int topRight[3] =    {0, 0, 0};
  int bottomLeft[3] =  {0, 0, 0};
  int bottomRight[3] = {0, 0, 0};
  for (int i=0; i<3; ++i)
  {
    topLeft[i] = std::max(topLeftIjk[i], 0);
    topLeft[i] = std::min(topLeftIjk[i], dims[i]-1);
    topRight[i] = std::max(topRightIjk[i], 0);
    topRight[i] = std::min(topRightIjk[i], dims[i]-1);
    bottomLeft[i] = std::max(bottomLeftIjk[i], 0);
    bottomLeft[i] = std::min(bottomLeftIjk[i], dims[i]-1);
    bottomRight[i] = std::max(bottomRightIjk[i], 0);
    bottomRight[i] = std::min(bottomRightIjk[i], dims[i]-1);
  }

  // If the region is smaller than a pixel then paint it using paintPixel mode,
  // to make sure at least one pixel is filled on each click
  int maxRowDelta = 0;
  int maxColumnDelta = 0;
  for (int i=0; i<3; ++i)
  {
    int d = abs(topRight[i] - topLeft[i]);
    if (d > maxColumnDelta)
    {
      maxColumnDelta = d;
    }
    d = abs(bottomRight[i] - bottomLeft[i]);
    if (d > maxColumnDelta)
    {
      maxColumnDelta = d;
    }
    d = abs(bottomLeft[i] - topLeft[i]);
    if (d > maxRowDelta)
    {
      maxRowDelta = d;
    }
    d = abs(bottomRight[i] - topRight[i]);
    if (d > maxRowDelta)
    {
      maxRowDelta = d;
    }
    if (maxRowDelta <= 1 || maxColumnDelta <= 1)
    {
      this->paintPixel(sliceWidget, xy);
      return;
    }
  }

  // Get IJK to RAS transform matrices for edited labelmap and background volume
  vtkMRMLSliceLogic* sliceLogic = sliceWidget->sliceLogic();
  vtkMRMLSliceNode* sliceNode = sliceLogic->GetSliceNode();

  vtkMRMLSliceLayerLogic* backgroundLogic = sliceLogic->GetBackgroundLayer();
  vtkMRMLVolumeNode* backgroundNode = backgroundLogic->GetVolumeNode();
  vtkSmartPointer<vtkMatrix4x4> backgroundIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  qSlicerSegmentEditorLabelEffect::ijkToRasMatrix(backgroundNode, backgroundIjkToRasMatrix);

  vtkMRMLSliceLayerLogic* labelLogic = sliceLogic->GetLabelLayer();
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(labelLogic->GetVolumeNode());
  vtkSmartPointer<vtkMatrix4x4> labelIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  qSlicerSegmentEditorLabelEffect::ijkToRasMatrix(labelImage, segmentationNode, labelIjkToRasMatrix);

  double brushCenterRas[3] = {0.0, 0.0, 0.0};
  q->xyToRas(xy, brushCenterRas, sliceWidget);

  QString paintOverStr = q->parameter(qSlicerSegmentEditorLabelEffect::paintOverParameterName());
  bool paintOver(paintOverStr.toInt());
  QString paintThresholdStr = q->parameter(qSlicerSegmentEditorLabelEffect::paintThresholdParameterName());
  int paintThreshold(paintThresholdStr.toInt());
  QString paintThresholdMinStr = q->parameter(qSlicerSegmentEditorLabelEffect::paintThresholdMinParameterName());
  int paintThresholdMin(paintThresholdMinStr.toInt());
  QString paintThresholdMaxStr = q->parameter(qSlicerSegmentEditorLabelEffect::paintThresholdMaxParameterName());
  int paintThresholdMax(paintThresholdMaxStr.toInt());

  this->Painter->SetBackgroundImage(backgroundNode->GetImageData());
  this->Painter->SetBackgroundIJKToWorld(backgroundIjkToRasMatrix);
  this->Painter->SetWorkingImage(labelImage);
  this->Painter->SetWorkingIJKToWorld(labelIjkToRasMatrix);
  this->Painter->SetTopLeft(topLeft);
  this->Painter->SetTopRight(topRight);
  this->Painter->SetBottomLeft(bottomLeft);
  this->Painter->SetBottomRight(bottomRight);
  this->Painter->SetBrushCenter(brushCenterRas);
  this->Painter->SetBrushRadius(this->Radius);
  this->Painter->SetPaintLabel(1); // Segment binary labelmaps all have voxel values of 1 for foreground
  this->Painter->SetPaintOver(paintOver);
  this->Painter->SetThresholdPaint(paintThreshold);
  this->Painter->SetThresholdPaintRange(paintThresholdMin, paintThresholdMax);
  
  // Fill volume of a sphere rather than a circle on the currently displayed image slice
  if (this->Sphere)
  {
    // Algorithm:
    //   Assume brush radius is in mm
    //   Estimate zVoxelSize
    //   Compute number of slices spanned by sphere, that are still within the volume
    //   For each spanned slice
    //     Reposition the brush center using xy(z)ToRAS transform: i.e. canvas to patient world coordinates
    //     Resize the radius: brushRadiusOffset=sqrt(brushRadius*brushRadius - zOffset_mm*zOffset_mm)
    //     Invoke Paint()
    //   Finally paint on the center slice, leaving the GUI on the center slice being most visibly edited

    // Estimate Z voxel size
    double zVoxelSizeMm = 1.0;
    double brushCenter1Xyz[3] = {xy.x(), xy.y(), 0.0};
    double brushCenter1Ras[3] = {0.0, 0.0, 0.0};
    q->xyzToRas(brushCenter1Xyz, brushCenter1Ras, sliceWidget);
    double brushCenter2Xyz[3] = {xy.x(), xy.y(), 100.0};
    double brushCenter2Ras[3] = {0.0, 0.0, 0.0};
    q->xyzToRas(brushCenter2Xyz, brushCenter2Ras, sliceWidget);
    double dx1 = brushCenter1Ras[0] - brushCenter2Ras[0];
    double dx2 = brushCenter1Ras[1] - brushCenter2Ras[1];
    double dx3 = brushCenter1Ras[2] - brushCenter2Ras[2];
    double distanceSpannedBy100Slices = sqrt(dx1*dx1+dx2*dx2+dx3*dx3);  // Compute L2 norm
    if (distanceSpannedBy100Slices > 0)
    {
      zVoxelSizeMm = distanceSpannedBy100Slices / 100.0;
    }

    // Compute number of slices spanned by sphere
    int numberOfSlicesInEachDirection = (int)((this->Radius / zVoxelSizeMm) - 1); // Floor instead of round as in the original PaintEffect code
    for (int sliceNumber=1; sliceNumber<=numberOfSlicesInEachDirection; ++sliceNumber)
    {
      // Each slice in both directions
      for (int direction=-1; direction<=1; direction+=2)
      {
        int sliceOffset = sliceNumber * direction;

        // x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
        // slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
        // coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
        double currentBrushCenterXyz[3] = {xy.x(), xy.y(), sliceOffset};
        double currentBrushCenterRas[3] = {0.0, 0.0, 0.0};
        q->xyzToRas(currentBrushCenterXyz, currentBrushCenterRas, sliceWidget);
        this->Painter->SetBrushCenter(currentBrushCenterRas);

        double zOffsetMm = zVoxelSizeMm * sliceOffset;
        double brushRadiusOffset = sqrt(this->Radius*this->Radius - zOffsetMm*zOffsetMm);
        this->Painter->SetBrushRadius(brushRadiusOffset);

        double topLeftXyz[3] = {left, top, sliceOffset};
        int currentTopLeftIjk[3] = {0, 0, 0};
        q->xyzToIjk(topLeftXyz, currentTopLeftIjk, sliceWidget, labelImage);
        double topRightXyz[3] = {right, top, sliceOffset};
        int currentTopRightIjk[3] = {0, 0, 0};
        q->xyzToIjk(topRightXyz, currentTopRightIjk, sliceWidget, labelImage);
        double bottomLeftXyz[3] = {left, bottom, sliceOffset};
        int currentBottomLeftIjk[3] = {0, 0, 0};
        q->xyzToIjk(bottomLeftXyz, currentBottomLeftIjk, sliceWidget, labelImage);
        double bottomRightXyz[3] = {right, bottom, sliceOffset};
        int currentBottomRightIjk[3] = {0, 0, 0};
        q->xyzToIjk(bottomRightXyz, currentBottomRightIjk, sliceWidget, labelImage);

        // Clamp the top, bottom, left, right to the valid dimensions of the label image
        int currentTopLeft[3] =     {0, 0, 0};
        int currentTopRight[3] =    {0, 0, 0};
        int currentBottomLeft[3] =  {0, 0, 0};
        int currentBottomRight[3] = {0, 0, 0};
        for (int i=0; i<3; ++i)
        {
          currentTopLeft[i] = std::max(currentTopLeftIjk[i], 0);
          currentTopLeft[i] = std::min(currentTopLeftIjk[i], dims[i]-1);
          currentTopRight[i] = std::max(currentTopRightIjk[i], 0);
          currentTopRight[i] = std::min(currentTopRightIjk[i], dims[i]-1);
          currentBottomLeft[i] = std::max(currentBottomLeftIjk[i], 0);
          currentBottomLeft[i] = std::min(currentBottomLeftIjk[i], dims[i]-1);
          currentBottomRight[i] = std::max(currentBottomRightIjk[i], 0);
          currentBottomRight[i] = std::min(currentBottomRightIjk[i], dims[i]-1);
        }
        this->Painter->SetTopLeft(currentTopLeft);
        this->Painter->SetTopRight(currentTopRight);
        this->Painter->SetBottomLeft(currentBottomLeft);
        this->Painter->SetBottomRight(currentBottomRight);

        this->Painter->Paint();
      } // For each slice to paint
    }
  } // If spherical brush

  // Paint the slice: same for circular and spherical brush modes
  this->Painter->SetBrushCenter(brushCenterRas);
  this->Painter->SetBrushRadius(this->Radius);
  this->Painter->SetTopLeft(topLeft);
  this->Painter->SetTopRight(topRight);
  this->Painter->SetBottomLeft(bottomLeft);
  this->Painter->SetBottomRight(bottomRight);

  this->Painter->Paint();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintPixel(qMRMLSliceWidget* sliceWidget, QPoint xy)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  vtkOrientedImageData* labelImage = q->m_EditedLabelmap;
  if (!labelImage)
  {
    return;
  }

  vtkMRMLSliceLogic* sliceLogic = sliceWidget->sliceLogic();

  vtkMRMLSliceLayerLogic* labelLogic = sliceLogic->GetLabelLayer();
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(labelLogic->GetVolumeNode());
  vtkSmartPointer<vtkMatrix4x4> labelIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  qSlicerSegmentEditorLabelEffect::ijkToRasMatrix(labelImage, segmentationNode, labelIjkToRasMatrix);

  int ijk[3] = {0, 0, 0};
  q->xyToIjk(xy, ijk, sliceWidget, labelImage);

  // Clamp to image extent
  int dims[3] = {0, 0, 0};
  labelImage->GetDimensions(dims);
  for (int i=0; i<3; ++i)
  {
    if (ijk[i] < 0 || ijk[i] >= dims[i])
    {
      return;
    }
  }

  labelImage->SetScalarComponentFromFloat(ijk[0],ijk[1],ijk[2], 0, 1); // Segment binary labelmaps all have voxel values of 1 for foreground
  /*
    TODO:
    EditUtil.markVolumeNodeAsModified(labelNode)
  */
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::scaleRadius(double scaleFactor)
{
  this->Radius *= scaleFactor;
  //TODO:
    //radius = float(self.parameterNode.GetParameter("PaintEffect,radius"))
    //self.parameterNode.SetParameter( "PaintEffect,radius", str(radius * scaleFactor) )
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::createBrushGlyph(qMRMLSliceWidget* sliceWidget, PaintEffectBrush* brush)
{
  // Create a brush circle of the right radius in XY space.
  // Assume uniform scaling between XY and RAS which is enforced by the view interactors
  Q_Q(qSlicerSegmentEditorPaintEffect);

  double xyRadius = 0.01; // Default value for pixel mode
  if (!this->PixelMode)
  {
    // Calculate radius if not in pixel mode
    vtkMRMLSliceLogic* sliceLogic = sliceWidget->sliceLogic();
    vtkMRMLSliceNode* sliceNode = sliceLogic->GetSliceNode();
    vtkSmartPointer<vtkMatrix4x4> rasToXyMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    rasToXyMatrix->DeepCopy(sliceNode->GetXYToRAS());
    rasToXyMatrix->Invert();

    //TODO: What does this do exactly?
    double maximum = 0.0;
    int maxIndex = 0;
    for (int index=0; index<3; ++index)
    {
      if (fabs(rasToXyMatrix->GetElement(0, index)) > maximum)
      {
        maximum = fabs(rasToXyMatrix->GetElement(0, index));
        maxIndex = index;
      }
    }

    double point[4] = {0.0, 0.0, 0.0, 0.0};
    point[maxIndex] = this->Radius;
    double radiusPoint[4] = {0.0, 0.0, 0.0, 0.0};
    rasToXyMatrix->MultiplyPoint(point, radiusPoint);
    xyRadius = sqrt(radiusPoint[0]*radiusPoint[0] + radiusPoint[1]*radiusPoint[1] + radiusPoint[2]*radiusPoint[2]);
  }

  // Make a circle paint brush
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  brush->PolyData->SetPoints(points);
  brush->PolyData->SetLines(lines);
  double twoPi = vtkMath::Pi() * 2.0;
  double piOverSixteen = vtkMath::Pi() / 16.0;
  int previousPointId = -1;
  int firstPointId = -1;
  int currentPointId = -1;
  double angle = 0.0;
  while (angle <= twoPi)
  {
    double x = xyRadius * cos(angle);
    double y = xyRadius * sin(angle);
    currentPointId = points->InsertNextPoint(x, y, 0.0);
    if (previousPointId != -1)
    {
      vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
      idList->InsertNextId(previousPointId);
      idList->InsertNextId(currentPointId);
      brush->PolyData->InsertNextCell(VTK_LINE, idList);
    }
    if (firstPointId == -1)
    {
      firstPointId = currentPointId;
    }
    angle += piOverSixteen;
  }

  // Make the last line in the circle
  vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
  idList->InsertNextId(currentPointId);
  idList->InsertNextId(firstPointId);
  brush->PolyData->InsertNextCell(VTK_LINE, idList);
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
  Superclass::deactivate();
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
  PaintEffectBrush* brush = d->brushForWidget(sliceWidget);
  if (!brush)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffect::processInteractionEvents: Failed to create brush!";
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
      //TODO:
      //EditUtil.setLabel(self.getLabelPixel(xy))
    }
    d->paintAddPoint(sliceWidget, x, y);
    this->abortEvent(callerInteractor, eid, sliceWidget);
  }
  else if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    d->paintApply(sliceWidget);
    d->IsPainting = false;
    this->cursorOn(sliceWidget);
  }
  else if (eid == vtkCommand::MouseMoveEvent)
  {
    brush->Actor->VisibilityOn();
    if (d->IsPainting)
    {
      int eventPosition[2] = {0,0};
      callerInteractor->GetEventPosition(eventPosition);
      d->paintAddPoint(sliceWidget, eventPosition[0], eventPosition[1]);
      this->abortEvent(callerInteractor, eid, sliceWidget);
    }
  }
  else if (eid == vtkCommand::EnterEvent)
  {
    brush->Actor->VisibilityOn();
  }
  else if (eid == vtkCommand::LeaveEvent)
  {
    brush->Actor->VisibilityOff();
  }
  else if (eid == vtkCommand::KeyPressEvent)
  {
    const char* key = callerInteractor->GetKeySym();
    if (!strcmp(key, "plus") || !strcmp(key, "equal"))
    {
      d->scaleRadius(1.2);
    }
    if (!strcmp(key, "minus") || !strcmp(key, "underscore"))
    {
      d->scaleRadius(0.8);
    }
  }

  //TODO:
  //if caller and caller.IsA('vtkMRMLSliceNode'):
  //  if hasattr(self,'brush'):
  //    self.createGlyph(self.brush)

  // Update paint feedback glyph to follow mouse
  int eventPosition[2] = {0,0};
  callerInteractor->GetEventPosition(eventPosition);
  brush->Actor->SetPosition(eventPosition[0], eventPosition[1]);
  sliceWidget->sliceView()->scheduleRender();
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
void qSlicerSegmentEditorPaintEffect::updateMRMLFromGUI()
{
  //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::apply()
{
  //TODO:
}
