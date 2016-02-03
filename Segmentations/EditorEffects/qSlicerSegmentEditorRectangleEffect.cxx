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

// Qt includes
#include <QDebug>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTrivialProducer.h>
#include <vtkMatrix4x4.h>
#include <vtkCallbackCommand.h>
#include <vtkPolyData.h>
#include <vtkCollection.h>
#include <vtkRenderWindowInteractor.h>
//#include <vtkInteractorObserver.h>
//#include <vtkRenderWindow.h>
//#include <vtkRendererCollection.h>
//#include <vtkRenderer.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>
//#include <vtkMRMLSliceNode.h>

// Slicer includes
#include "qMRMLSliceWidget.h"

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
  QIcon RectangleIcon;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffectPrivate::qSlicerSegmentEditorRectangleEffectPrivate(qSlicerSegmentEditorRectangleEffect& object)
  : q_ptr(&object)
{
  this->RectangleIcon = QIcon(":Icons/Rectangle.png");
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffectPrivate::~qSlicerSegmentEditorRectangleEffectPrivate()
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffect::qSlicerSegmentEditorRectangleEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorRectangleEffectPrivate(*this) )
{
  //this->Brush = vtkPolyData::New();
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
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorRectangleEffect::~qSlicerSegmentEditorRectangleEffect()
{
  //if (this->Brush)
  //{
  //  this->Brush->Delete();
  //  this->Brush = NULL;
  //}
}

//---------------------------------------------------------------------------
QString qSlicerSegmentEditorRectangleEffect::name()
{
  return QString("Rectangle");
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
  //TODO:
}
