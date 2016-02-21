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
#include "qSlicerSegmentEditorPaintEffect_p.h"

#include "vtkOrientedImageData.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentEditorNode.h"

// Qt includes
#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

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

// CTK includes
#include "ctkDoubleSlider.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLSpinBox.h"
#include "vtkMRMLSliceLogic.h"
#include "vtkMRMLSliceLayerLogic.h"
#include "vtkImageSlicePaint.h"

//-----------------------------------------------------------------------------
/// Visualization objects and pipeline for each slice view for the paint brush
class BrushPipeline: public QObject
{
public:
  BrushPipeline()
  {
    this->PolyData = vtkPolyData::New();
    this->Mapper = vtkPolyDataMapper2D::New();
    this->Mapper->SetInputData(this->PolyData);
    this->Actor = vtkActor2D::New();
    this->Actor->SetMapper(this->Mapper);
    this->Actor->VisibilityOff();
  };
  ~BrushPipeline()
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
qSlicerSegmentEditorPaintEffectPrivate::qSlicerSegmentEditorPaintEffectPrivate(qSlicerSegmentEditorPaintEffect& object)
  : q_ptr(&object)
  , Painter(NULL)
  , DelayedPaint(true)
  , IsPainting(false)
  , RadiusFrame(NULL)
  , RadiusSpinBox(NULL)
  , RadiusSlider(NULL)
  , RadiusUnitsToggle(NULL)
  , SphereCheckbox(NULL)
  , SmudgeCheckbox(NULL)
  , PixelModeCheckbox(NULL)
{
  this->PaintIcon = QIcon(":Icons/Paint.png");

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

  foreach (BrushPipeline* brush, this->Brushes)
  {
    delete brush;
  }
  this->Brushes.clear();
}

//-----------------------------------------------------------------------------
BrushPipeline* qSlicerSegmentEditorPaintEffectPrivate::brushForWidget(qMRMLSliceWidget* sliceWidget)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  if (this->Brushes.contains(sliceWidget))
  {
    return this->Brushes[sliceWidget];
  }

  // Create brush if does not yet exist
  BrushPipeline* brush = new BrushPipeline();
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
  Q_Q(qSlicerSegmentEditorPaintEffect);

  this->PaintCoordinates << QPoint(x,y);

  if (this->DelayedPaint && !q->integerParameter("PixelMode"))
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

    BrushPipeline* brush = this->brushForWidget(sliceWidget);
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
  Q_Q(qSlicerSegmentEditorPaintEffect);

  //TODO:
  //if self.paintCoordinates != []:
  //  if self.undoRedo:
  //    self.undoRedo.saveState()

  foreach (QPoint xy, this->PaintCoordinates)
  {
    if (q->integerParameter("PixelMode"))
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

  // Notify editor about changes
  q->apply();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::paintBrush(qMRMLSliceWidget* sliceWidget, QPoint xy)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  if (!q->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::paintBrush: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* editedLabelmap = q->parameterSetNode()->GetEditedLabelmap();
  if (!editedLabelmap)
  {
    return;
  }

  int x = xy.x();
  int y = xy.y();
  double radius = q->doubleParameter("Radius");

  // Get brush for slice widget
  BrushPipeline* brush = this->brushForWidget(sliceWidget);

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
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(left, top),     topLeftIjk, sliceWidget, editedLabelmap);
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(right, top),    topRightIjk, sliceWidget, editedLabelmap);
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(left, bottom),  bottomLeftIjk, sliceWidget, editedLabelmap);
  qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint(right, bottom), bottomRightIjk, sliceWidget, editedLabelmap);

  // Clamp the top, bottom, left, right to the valid dimensions of the label image
  int dims[3] = {0, 0, 0};
  editedLabelmap->GetDimensions(dims);

  int topLeft[3] =     {0, 0, 0};
  int topRight[3] =    {0, 0, 0};
  int bottomLeft[3] =  {0, 0, 0};
  int bottomRight[3] = {0, 0, 0};
  for (int i=0; i<3; ++i)
  {
    topLeft[i] = qMax(topLeftIjk[i], 0);
    topLeft[i] = qMin(topLeftIjk[i], dims[i]-1);
    topRight[i] = qMax(topRightIjk[i], 0);
    topRight[i] = qMin(topRightIjk[i], dims[i]-1);
    bottomLeft[i] = qMax(bottomLeftIjk[i], 0);
    bottomLeft[i] = qMin(bottomLeftIjk[i], dims[i]-1);
    bottomRight[i] = qMax(bottomRightIjk[i], 0);
    bottomRight[i] = qMin(bottomRightIjk[i], dims[i]-1);
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
  }
  if (maxRowDelta <= 1 || maxColumnDelta <= 1)
  {
    this->paintPixel(sliceWidget, xy);
    return;
  }

  // Get IJK to RAS transform matrices for edited labelmap and master volume
  vtkMRMLScalarVolumeNode* masterVolumeNode = q->parameterSetNode()->GetMasterVolumeNode();
  vtkSmartPointer<vtkMatrix4x4> masterIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  qSlicerSegmentEditorAbstractLabelEffect::imageToWorldMatrix(masterVolumeNode, masterIjkToRasMatrix);

  vtkMRMLSegmentationNode* segmentationNode = q->parameterSetNode()->GetSegmentationNode();
  vtkSmartPointer<vtkMatrix4x4> labelIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  qSlicerSegmentEditorAbstractLabelEffect::imageToWorldMatrix(editedLabelmap, segmentationNode, labelIjkToRasMatrix);

  double brushCenterRas[3] = {0.0, 0.0, 0.0};
  q->xyToRas(xy, brushCenterRas, sliceWidget);

  int paintOver = q->integerParameter(qSlicerSegmentEditorAbstractLabelEffect::paintOverParameterName());
  int paintThreshold = q->integerParameter(qSlicerSegmentEditorAbstractLabelEffect::paintThresholdParameterName());
  double paintThresholdMin = q->doubleParameter(qSlicerSegmentEditorAbstractLabelEffect::paintThresholdMinParameterName());
  double paintThresholdMax = q->doubleParameter(qSlicerSegmentEditorAbstractLabelEffect::paintThresholdMaxParameterName());

  this->Painter->SetBackgroundImage(masterVolumeNode->GetImageData());
  this->Painter->SetBackgroundIJKToWorld(masterIjkToRasMatrix);
  this->Painter->SetWorkingImage(editedLabelmap);
  this->Painter->SetWorkingIJKToWorld(labelIjkToRasMatrix);
  this->Painter->SetTopLeft(topLeft);
  this->Painter->SetTopRight(topRight);
  this->Painter->SetBottomLeft(bottomLeft);
  this->Painter->SetBottomRight(bottomRight);
  this->Painter->SetBrushCenter(brushCenterRas);
  this->Painter->SetBrushRadius(radius);
  this->Painter->SetPaintLabel(q->m_Erase ? 0 : 1); // Segment binary labelmaps all have voxel values of 1 for foreground
  this->Painter->SetPaintOver(paintOver);
  this->Painter->SetThresholdPaint(paintThreshold);
  this->Painter->SetThresholdPaintRange(paintThresholdMin, paintThresholdMax);
  
  // Fill volume of a sphere rather than a circle on the currently displayed image slice
  if (q->integerParameter("Sphere"))
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
    int numberOfSlicesInEachDirection = (int)((radius / zVoxelSizeMm) - 1); // Floor instead of round as in the original PaintEffect code
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
        double brushRadiusOffset = sqrt(radius*radius - zOffsetMm*zOffsetMm);
        this->Painter->SetBrushRadius(brushRadiusOffset);

        double topLeftXyz[3] = {left, top, sliceOffset};
        int currentTopLeftIjk[3] = {0, 0, 0};
        q->xyzToIjk(topLeftXyz, currentTopLeftIjk, sliceWidget, editedLabelmap);
        double topRightXyz[3] = {right, top, sliceOffset};
        int currentTopRightIjk[3] = {0, 0, 0};
        q->xyzToIjk(topRightXyz, currentTopRightIjk, sliceWidget, editedLabelmap);
        double bottomLeftXyz[3] = {left, bottom, sliceOffset};
        int currentBottomLeftIjk[3] = {0, 0, 0};
        q->xyzToIjk(bottomLeftXyz, currentBottomLeftIjk, sliceWidget, editedLabelmap);
        double bottomRightXyz[3] = {right, bottom, sliceOffset};
        int currentBottomRightIjk[3] = {0, 0, 0};
        q->xyzToIjk(bottomRightXyz, currentBottomRightIjk, sliceWidget, editedLabelmap);

        // Clamp the top, bottom, left, right to the valid dimensions of the label image
        int currentTopLeft[3] =     {0, 0, 0};
        int currentTopRight[3] =    {0, 0, 0};
        int currentBottomLeft[3] =  {0, 0, 0};
        int currentBottomRight[3] = {0, 0, 0};
        for (int i=0; i<3; ++i)
        {
          currentTopLeft[i] = qMax(currentTopLeftIjk[i], 0);
          currentTopLeft[i] = qMin(currentTopLeftIjk[i], dims[i]-1);
          currentTopRight[i] = qMax(currentTopRightIjk[i], 0);
          currentTopRight[i] = qMin(currentTopRightIjk[i], dims[i]-1);
          currentBottomLeft[i] = qMax(currentBottomLeftIjk[i], 0);
          currentBottomLeft[i] = qMin(currentBottomLeftIjk[i], dims[i]-1);
          currentBottomRight[i] = qMax(currentBottomRightIjk[i], 0);
          currentBottomRight[i] = qMin(currentBottomRightIjk[i], dims[i]-1);
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
  this->Painter->SetBrushRadius(radius);
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

  if (!q->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::paintPixel: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* labelImage = q->parameterSetNode()->GetEditedLabelmap();
  if (!labelImage)
  {
    return;
  }

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

  labelImage->SetScalarComponentFromDouble(ijk[0],ijk[1],ijk[2], 0, (q->m_Erase ? 0 : 1)); // Segment binary labelmaps all have voxel values of 1 for foreground
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::scaleRadius(double scaleFactor)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  q->setParameter("Radius", q->doubleParameter("Radius") * scaleFactor, true); // Emit parameter modified event
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::onRadiusUnitsClicked()
{
  if (this->RadiusUnitsToggle->text().compare("mm:"))
  {
    this->RadiusUnitsToggle->setText("mm:");
  }
  else
  {
    this->RadiusUnitsToggle->setText("px:");
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::onQuickRadiusButtonClicked()
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  if (!q->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::onQuickRadiusButtonClicked: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* labelImage = q->parameterSetNode()->GetEditedLabelmap();
  QPushButton* senderButton = dynamic_cast<QPushButton*>(sender());
  int radius = senderButton->property("Radius").toInt();

  double radiusMm = 0.0;
  if (!this->RadiusUnitsToggle->text().compare("px:"))
  {
    if (labelImage)
    {
      double spacing[3] = {0.0, 0.0, 0.0};
      labelImage->GetSpacing(spacing);
      double minimumSpacing = qMin(spacing[0], qMin(spacing[1], spacing[2]));
      radiusMm = minimumSpacing * radius;
    }
  }
  else
  {
    radiusMm = radius;
  }

  this->onRadiusValueChanged(radiusMm);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::onRadiusValueChanged(double value)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  q->setParameter("Radius", value, true); // Emit parameter modified event
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffectPrivate::createBrushGlyph(qMRMLSliceWidget* sliceWidget, BrushPipeline* brush)
{
  Q_Q(qSlicerSegmentEditorPaintEffect);

  // Clear brush glyph in case it was already created
  brush->PolyData->Initialize();

  // Create a brush circle of the right radius in XY space.
  // Assume uniform scaling between XY and RAS which is enforced by the view interactors
  double xyRadius = 0.01; // Default value for pixel mode
  if (!q->integerParameter("PixelMode"))
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
    point[maxIndex] = q->doubleParameter("Radius");
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
    previousPointId = currentPointId;
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
void qSlicerSegmentEditorPaintEffectPrivate::updateBrushes()
{
  QMapIterator<qMRMLSliceWidget*, BrushPipeline*> brushIt(this->Brushes);
  while (brushIt.hasNext())
  {
    brushIt.next();
    this->createBrushGlyph(brushIt.key(), brushIt.value());
  }
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qSlicerSegmentEditorPaintEffect methods

//----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffect::qSlicerSegmentEditorPaintEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorPaintEffectPrivate(*this) )
{
  this->m_Name = QString("Paint");
  this->m_Erase = false;
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorPaintEffect::~qSlicerSegmentEditorPaintEffect()
{
}

//---------------------------------------------------------------------------
QIcon qSlicerSegmentEditorPaintEffect::icon()
{
  Q_D(qSlicerSegmentEditorPaintEffect);

  return d->PaintIcon;
}

//---------------------------------------------------------------------------
QString const qSlicerSegmentEditorPaintEffect::helpText()const
{
  return QString("Use this tool to paint with a round brush of the selected radius");
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

  Q_D(qSlicerSegmentEditorPaintEffect);

  // Delete brushes because actors are removed in base class deactivate call above,
  // and when re-activated, they will be thus created and added again
  foreach (BrushPipeline* brush, d->Brushes)
  {
    delete brush;
  }
  d->Brushes.clear();
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
  BrushPipeline* brush = d->brushForWidget(sliceWidget);
  if (!brush)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffect::processInteractionEvents: Failed to create brush!";
    return;
  }

  if (eid == vtkCommand::LeftButtonPressEvent)
  {
    d->IsPainting = true;
    if (!this->integerParameter("PixelMode"))
    {
      this->cursorOff(sliceWidget);
    }
    int x = 0;
    int y = 0;
    callerInteractor->GetEventPosition(x, y);
    if (this->integerParameter("Smudge"))
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
    brush->Actor->SetVisibility(!this->integerParameter("PixelMode"));
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

  // Update paint feedback glyph to follow mouse
  int eventPosition[2] = {0,0};
  callerInteractor->GetEventPosition(eventPosition);
  brush->Actor->SetPosition(eventPosition[0], eventPosition[1]);
  sliceWidget->sliceView()->scheduleRender();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::processViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorPaintEffect);

  // This effect only supports interactions in the 2D slice views currently
  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  if (!sliceWidget)
  {
    return;
  }
  BrushPipeline* brush = d->brushForWidget(sliceWidget);
  if (!brush)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffect::processViewNodeEvents: Failed to create brush!";
    return;
  }

  d->createBrushGlyph(sliceWidget, brush);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::setupOptionsFrame()
{
  // Setup widgets corresponding to the parent class of this effect
  Superclass::setupOptionsFrame();

  Q_D(qSlicerSegmentEditorPaintEffect);

  // Create options frame for this effect
  d->RadiusFrame = new QFrame();
  d->RadiusFrame->setLayout(new QHBoxLayout());
  this->addOptionsWidget(d->RadiusFrame);

  QLabel* radiusLabel = new QLabel("Radius:", d->RadiusFrame);
  radiusLabel->setToolTip("Set the radius of the paint brush in millimeters");
  d->RadiusFrame->layout()->addWidget(radiusLabel);

  d->RadiusSpinBox = new qMRMLSpinBox(d->RadiusFrame);
  d->RadiusSpinBox->setToolTip("Set the radius of the paint brush in millimeters");
  d->RadiusSpinBox->setQuantity("length");
  d->RadiusSpinBox->setUnitAwareProperties(qMRMLSpinBox::Prefix | qMRMLSpinBox::Suffix);
  d->RadiusFrame->layout()->addWidget(d->RadiusSpinBox);

  d->RadiusUnitsToggle = new QPushButton("px:");
  d->RadiusUnitsToggle->setToolTip("Toggle radius quick set buttons between mm and label volume pixel size units");
  d->RadiusUnitsToggle->setFixedWidth(35);
  d->RadiusFrame->layout()->addWidget(d->RadiusUnitsToggle);

  QList<int> quickRadii;
  quickRadii << 2 << 3 << 4 << 5 << 10 << 20;
  foreach (int radius, quickRadii)
  {
    QPushButton* quickRadiusButton = new QPushButton(QString::number(radius));
    quickRadiusButton->setProperty("Radius", QVariant(radius));
    quickRadiusButton->setFixedWidth(25);
    quickRadiusButton->setToolTip("Set radius based on mm or label voxel size units depending on toggle value");
    d->RadiusFrame->layout()->addWidget(quickRadiusButton);
    QObject::connect(quickRadiusButton, SIGNAL(clicked()), d, SLOT(onQuickRadiusButtonClicked()));
  }

  d->RadiusSlider = new ctkDoubleSlider();
  d->RadiusSlider->setOrientation(Qt::Horizontal);
  this->addOptionsWidget(d->RadiusSlider);

  d->SphereCheckbox = new QCheckBox("Sphere");
  d->SphereCheckbox->setToolTip("Use a 3D spherical brush rather than a 2D circular brush.");
  this->addOptionsWidget(d->SphereCheckbox);

  d->SmudgeCheckbox = new QCheckBox("Smudge");
  //TODO: Smudge is not yet implemented. It is now a more complex function,
  //  as it involves switching segment instead of simply changing label color.
  //d->SmudgeCheckbox->setToolTip("Set the label number automatically by sampling the pixel location where the brush stroke starts.");
  d->SmudgeCheckbox->setToolTip("Smudge function is not yet implemented!");
  d->SmudgeCheckbox->setEnabled(false);
  this->addOptionsWidget(d->SmudgeCheckbox);

  d->PixelModeCheckbox = new QCheckBox("Pixel mode");
  d->PixelModeCheckbox->setToolTip("Paint exactly the pixel under the cursor, ignoring the radius, threshold, and paint over.");
  this->addOptionsWidget(d->PixelModeCheckbox);

  QObject::connect(d->RadiusUnitsToggle, SIGNAL(clicked()), d, SLOT(onRadiusUnitsClicked()));
  QObject::connect(d->SphereCheckbox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  QObject::connect(d->SmudgeCheckbox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  QObject::connect(d->PixelModeCheckbox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  QObject::connect(d->RadiusSlider, SIGNAL(valueChanged(double)), d, SLOT(onRadiusValueChanged(double)));
  QObject::connect(d->RadiusSpinBox, SIGNAL(valueChanged(double)), d, SLOT(onRadiusValueChanged(double)));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::setMRMLDefaults()
{
  Superclass::setMRMLDefaults();

  this->setParameter("MinimumRadius", 0.01);
  this->setParameter("MaximumRadius", 100.0);
  this->setParameter("Radius", 0.5);
  this->setParameter("Sphere", 0);
  this->setParameter("Smudge", 0);
  this->setParameter("PixelMode", 0);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::updateGUIFromMRML()
{
  Q_D(qSlicerSegmentEditorPaintEffect);

  if (!this->scene())
  {
    return;
  }

  d->SphereCheckbox->blockSignals(true);
  d->SphereCheckbox->setChecked(this->integerParameter("Sphere"));
  d->SphereCheckbox->blockSignals(false);

  d->SmudgeCheckbox->blockSignals(true);
  d->SmudgeCheckbox->setChecked(this->integerParameter("Smudge"));
  d->SmudgeCheckbox->blockSignals(false);

  bool pixelMode = this->integerParameter("PixelMode");
  d->PixelModeCheckbox->blockSignals(true);
  d->PixelModeCheckbox->setChecked(pixelMode);
  d->PixelModeCheckbox->blockSignals(false);

  // Pixel mode prevents using threshold and paint over functions
  this->setParameter(this->thresholdAvailableParameterName(), !pixelMode);
  this->setParameter(this->paintOverAvailableParameterName(), !pixelMode);
  // Update label options based on constraints set by pixel mode
  Superclass::updateGUIFromMRML();

  // Radius is also disabled if pixel mode is on
  d->RadiusFrame->setEnabled(!pixelMode);

  d->RadiusSlider->blockSignals(true);
  d->RadiusSlider->setMinimum(this->doubleParameter("MinimumRadius"));
  d->RadiusSlider->setMaximum(this->doubleParameter("MaximumRadius"));
  d->RadiusSlider->setValue(this->doubleParameter("Radius"));
  d->RadiusSlider->setSingleStep(this->doubleParameter("MinimumRadius"));
  d->RadiusSlider->blockSignals(false);

  d->RadiusSpinBox->blockSignals(true);
  d->RadiusSpinBox->setMRMLScene(this->scene());
  d->RadiusSpinBox->setMinimum(this->doubleParameter("MinimumRadius"));
  d->RadiusSpinBox->setMaximum(this->doubleParameter("MaximumRadius"));
  d->RadiusSpinBox->setValue(this->doubleParameter("Radius"));
  int decimals = (int)(log10(this->doubleParameter("MinimumRadius")));
  if (decimals < 0)
  {
    d->RadiusSpinBox->setDecimals(-decimals * 2);
  }
  d->RadiusSpinBox->blockSignals(false);

  // Update brushes
  d->updateBrushes();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::updateMRMLFromGUI()
{
  Superclass::updateMRMLFromGUI();

  Q_D(qSlicerSegmentEditorPaintEffect);

  this->setParameter("Sphere", (int)d->SphereCheckbox->isChecked());
  this->setParameter("Smudge", (int)d->SmudgeCheckbox->isChecked());
  bool pixelMode = d->PixelModeCheckbox->isChecked();
  bool pixelModeChanged = (pixelMode != (bool)this->integerParameter("PixelMode"));
  this->setParameter("PixelMode", (int)pixelMode);
  this->setParameter("Radius", d->RadiusSlider->value());

  // If pixel mode changed, then other GUI changes are due
  if (pixelModeChanged)
  {
    // Pixel mode prevents using threshold and paint over functions
    this->setParameter(this->thresholdAvailableParameterName(), !pixelMode);
    this->setParameter(this->paintOverAvailableParameterName(), !pixelMode);
    // Update label options based on constraints set by pixel mode
    Superclass::updateGUIFromMRML();

    d->RadiusFrame->setEnabled(!pixelMode);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorPaintEffect::editedLabelmapChanged()
{
  Superclass::editedLabelmapChanged();

  if (!this->parameterSetNode())
  {
    qCritical() << "qSlicerSegmentEditorPaintEffect::editedLabelmapChanged: Invalid segment editor parameter set node!";
    return;
  }

  vtkOrientedImageData* labelmap = this->parameterSetNode()->GetEditedLabelmap();
  if (labelmap)
  {
    double spacing[3] = {0.0, 0.0, 0.0};
    labelmap->GetSpacing(spacing);
    double minimumSpacing = qMin(spacing[0], qMin(spacing[1], spacing[2]));
    double minimumRadius = 0.5 * minimumSpacing;
    this->setParameter("MinimumRadius", minimumRadius);

    int dimensions[3] = {0, 0, 0};
    labelmap->GetDimensions(dimensions);
    double bounds[3] = {spacing[0]*dimensions[0], spacing[1]*dimensions[1], spacing[2]*dimensions[2]};
    double maximumBounds = qMax(bounds[0], qMax(bounds[1], bounds[2]));
    double maximumRadius = 0.5 * maximumBounds;
    this->setParameter("MaximumRadius", maximumRadius);

    this->setParameter("Radius", qMin(50.0 * minimumRadius, 0.5 * maximumRadius));

    this->updateGUIFromMRML();
  }
}
