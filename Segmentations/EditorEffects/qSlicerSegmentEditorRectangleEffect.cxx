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
#include "qSlicerSegmentEditorRectangleEffect.h"

#include "vtkOrientedImageData.h"
#include "vtkMRMLSegmentEditorNode.h"

// Qt includes
#include <QDebug>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTrivialProducer.h>
#include <vtkCallbackCommand.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor2D.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
#include "qMRMLSliceView.h"

// Slicer includes
#include "qMRMLSliceWidget.h"

//-----------------------------------------------------------------------------
/// Visualization objects and pipeline for each slice view for rectangle drawing
class RectanglePipeline: public QObject
{
public:
  RectanglePipeline()
  {
    this->PolyData = vtkPolyData::New();
    this->Mapper = vtkPolyDataMapper2D::New();
    this->Mapper->SetInputData(this->PolyData);
    this->Actor = vtkActor2D::New();
    this->Actor->SetMapper(this->Mapper);
    vtkProperty2D* rectangleProperty = this->Actor->GetProperty();
    rectangleProperty->SetColor(1,1,0);
    rectangleProperty->SetLineWidth(1);
  };
  ~RectanglePipeline()
  {
    this->Actor->Delete();
    this->Actor = NULL;
    this->Mapper->Delete();
    this->Mapper = NULL;
    this->PolyData->Delete();
    this->PolyData = NULL;
  };
public:
  bool IsDragging;
  QPoint StartXyPosition;
  QPoint CurrentXyPosition;
  vtkActor2D* Actor;
  vtkPolyDataMapper2D* Mapper;
  vtkPolyData* PolyData;
};

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorRectangleEffectPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorRectangleEffect);
protected:
  qSlicerSegmentEditorRectangleEffect* const q_ptr;
public:
  qSlicerSegmentEditorRectangleEffectPrivate(qSlicerSegmentEditorRectangleEffect& object);
  ~qSlicerSegmentEditorRectangleEffectPrivate();
public:
  /// Draw rectangle glyph
  void createRectangleGlyph(qMRMLSliceWidget* sliceWidget, RectanglePipeline* rectangle);
  /// Update rectangle glyph based on positions
  void updateRectangleGlyph(RectanglePipeline* rectangle);
protected:
  /// Get rectangle object for widget. Create if does not exist
  RectanglePipeline* rectangleForWidget(qMRMLSliceWidget* sliceWidget);
public:
  QIcon RectangleIcon;
  QMap<qMRMLSliceWidget*, RectanglePipeline*> Rectangles;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffectPrivate::qSlicerSegmentEditorRectangleEffectPrivate(qSlicerSegmentEditorRectangleEffect& object)
  : q_ptr(&object)
{
  this->RectangleIcon = QIcon(":Icons/Rectangle.png");

  this->Rectangles.clear();
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffectPrivate::~qSlicerSegmentEditorRectangleEffectPrivate()
{
  foreach (RectanglePipeline* rectangle, this->Rectangles)
  {
    delete rectangle;
  }
  this->Rectangles.clear();
}

//-----------------------------------------------------------------------------
RectanglePipeline* qSlicerSegmentEditorRectangleEffectPrivate::rectangleForWidget(qMRMLSliceWidget* sliceWidget)
{
  Q_Q(qSlicerSegmentEditorRectangleEffect);

  if (this->Rectangles.contains(sliceWidget))
  {
    return this->Rectangles[sliceWidget];
  }

  // Create rectangle if does not yet exist
  RectanglePipeline* rectangle = new RectanglePipeline();
  this->createRectangleGlyph(sliceWidget, rectangle);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(sliceWidget);
  if (!renderer)
  {
    qCritical() << "qSlicerSegmentEditorPaintEffectPrivate::rectangleForWidget: Failed to get renderer!";
  }
  else
  {
    renderer->AddActor2D(rectangle->Actor);
    q->addActor(sliceWidget, rectangle->Actor);
  }

  this->Rectangles[sliceWidget] = rectangle;
  return rectangle;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorRectangleEffectPrivate::createRectangleGlyph(qMRMLSliceWidget* sliceWidget, RectanglePipeline* rectangle)
{
  Q_Q(qSlicerSegmentEditorRectangleEffect);

  // Clear rectangle glyph in case it was already created
  rectangle->PolyData->Initialize();

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  rectangle->PolyData->SetPoints(points);
  rectangle->PolyData->SetLines(lines);

  int previousPointIndex = -1;
  int firstPointIndex = -1;
  int pointIndex = -1;
  for (int corner=0; corner<4; ++corner)
  {
    pointIndex = points->InsertNextPoint(0.0, 0.0, 0.0);
    if (previousPointIndex != -1)
    {
      vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
      idList->InsertNextId(previousPointIndex);
      idList->InsertNextId(pointIndex);
      rectangle->PolyData->InsertNextCell(VTK_LINE, idList);
    }
    previousPointIndex = pointIndex;
    if (firstPointIndex == -1)
    {
      firstPointIndex = pointIndex;
    }
  }

  // Make the last line in the polydata
  vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
  idList->InsertNextId(pointIndex);
  idList->InsertNextId(firstPointIndex);
  rectangle->PolyData->InsertNextCell(VTK_LINE, idList);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorRectangleEffectPrivate::updateRectangleGlyph(RectanglePipeline* rectangle)
{
  vtkPoints* points = rectangle->PolyData->GetPoints();
  points->SetPoint(0, rectangle->StartXyPosition.x(),   rectangle->StartXyPosition.y(),   0.0);
  points->SetPoint(1, rectangle->StartXyPosition.x(),   rectangle->CurrentXyPosition.y(), 0.0);
  points->SetPoint(2, rectangle->CurrentXyPosition.x(), rectangle->CurrentXyPosition.y(), 0.0);
  points->SetPoint(3, rectangle->CurrentXyPosition.x(), rectangle->StartXyPosition.y(),   0.0);
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffect::qSlicerSegmentEditorRectangleEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorRectangleEffectPrivate(*this) )
{
  this->m_Name = QString("Rectangle");
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffect::~qSlicerSegmentEditorRectangleEffect()
{
}

//---------------------------------------------------------------------------
QIcon qSlicerSegmentEditorRectangleEffect::icon()
{
  Q_D(qSlicerSegmentEditorRectangleEffect);

  return d->RectangleIcon;
}

//---------------------------------------------------------------------------
QString const qSlicerSegmentEditorRectangleEffect::helpText()const
{
  return QString("Use this tool to draw a rectangle.\n\nLeft Click and Drag: sweep out an outline that will draw when the button is released.");
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorRectangleEffect::clone()
{
  return new qSlicerSegmentEditorRectangleEffect();
}

//---------------------------------------------------------------------------
void qSlicerSegmentEditorRectangleEffect::processInteractionEvents(
  vtkRenderWindowInteractor* callerInteractor,
  unsigned long eid,
  qMRMLWidget* viewWidget )
{
  Q_D(qSlicerSegmentEditorRectangleEffect);

  // This effect only supports interactions in the 2D slice views currently
  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  if (!sliceWidget)
  {
    return;
  }
  RectanglePipeline* rectangle = d->rectangleForWidget(sliceWidget);
  if (!rectangle)
  {
    qCritical() << "qSlicerSegmentEditorRectangleEffect::processInteractionEvents: Failed to create rectangle!";
    return;
  }

  if (eid == vtkCommand::LeftButtonPressEvent)
  {
    rectangle->IsDragging = true;
    this->cursorOff(sliceWidget);
    int x = 0;
    int y = 0;
    callerInteractor->GetEventPosition(x, y);
    rectangle->StartXyPosition.setX(x);
    rectangle->StartXyPosition.setY(y);
    rectangle->CurrentXyPosition.setX(x);
    rectangle->CurrentXyPosition.setY(y);
    d->updateRectangleGlyph(rectangle);
    this->abortEvent(callerInteractor, eid, sliceWidget);
  }
  else if (eid == vtkCommand::MouseMoveEvent)
  {
    if (rectangle->IsDragging)
    {
      int x = 0;
      int y = 0;
      callerInteractor->GetEventPosition(x, y);
      rectangle->CurrentXyPosition.setX(x);
      rectangle->CurrentXyPosition.setY(y);
      d->updateRectangleGlyph(rectangle);
      sliceWidget->sliceView()->scheduleRender();
      this->abortEvent(callerInteractor, eid, sliceWidget);
    }
  }
  else if (eid == vtkCommand::LeftButtonReleaseEvent)
  {
    rectangle->IsDragging = false;
    this->cursorOn(sliceWidget);

    // Paint rectangle on edited labelmap
    vtkCellArray* lines = rectangle->PolyData->GetLines();
    if (lines->GetNumberOfCells() > 0)
    {
      //TODO:
      //self.logic.undoRedo = self.undoRedo

      vtkOrientedImageData* editedLabelmap = this->parameterSetNode()->GetEditedLabelmap();
      if (editedLabelmap)
      {
        this->appendPolyMask(editedLabelmap, rectangle->PolyData, sliceWidget);
      }
    }
    // Notify editor about changes
    this->apply();

    rectangle->StartXyPosition.setX(0);
    rectangle->StartXyPosition.setY(0);
    rectangle->CurrentXyPosition.setX(0);
    rectangle->CurrentXyPosition.setY(0);
    d->updateRectangleGlyph(rectangle);
    this->abortEvent(callerInteractor, eid, sliceWidget);
  }
}
