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
#include "qSlicerSegmentEditorAbstractEffect.h"

#include "vtkMRMLSegmentEditorEffectNode.h"
#include "vtkOrientedImageData.h"

// Qt includes
#include <QDebug>
#include <QCursor>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLThreeDView.h"
#include "vtkMRMLSliceLogic.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCommand.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMatrix4x4.h>
#include <vtkProp.h>

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorAbstractEffectPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorAbstractEffect);
protected:
  qSlicerSegmentEditorAbstractEffect* const q_ptr;
public:
  qSlicerSegmentEditorAbstractEffectPrivate(qSlicerSegmentEditorAbstractEffect& object);
  ~qSlicerSegmentEditorAbstractEffectPrivate();
public:
  /// Cursor to restore after custom cursor is not needed any more
  QCursor* SavedCursor;

  /// MRML ID of the parameter set node corresponding to the effect
  QString ParameterSetNodeID;

  /// List of actors used by the effect. Removed when effect is deactivated
  QMap<qMRMLWidget*, QList<vtkProp*> > Actors;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::qSlicerSegmentEditorAbstractEffectPrivate(qSlicerSegmentEditorAbstractEffect& object)
  : q_ptr(&object)
  , SavedCursor(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffectPrivate::~qSlicerSegmentEditorAbstractEffectPrivate()
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::qSlicerSegmentEditorAbstractEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorAbstractEffectPrivate(*this) )
 , m_Scene(NULL)
 , m_EditedLabelmap(NULL)
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::~qSlicerSegmentEditorAbstractEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::deactivate()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  // Remove actors from container
  QMapIterator<qMRMLWidget*, QList<vtkProp*> > actorsIterator(d->Actors);
  while (actorsIterator.hasNext())
  {
    actorsIterator.next();
    qMRMLWidget* viewWidget = actorsIterator.key();
    foreach (vtkProp* actor, actorsIterator.value())
    {
      vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
      if (!renderer)
      {
        qCritical() << "qSlicerSegmentEditorAbstractEffect::deactivate: Failed to get renderer for view widget!";
        continue;
      }

      // Call both actor removal functions to support both slice and 3D views
      // (the 2D actors are members of vtkViewport, base class of vtkRenderer,
      // while vtkRenderer has another, general actor list)
      renderer->RemoveActor(actor);
      renderer->RemoveActor2D(actor);
    }

    // Schedule render
    qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
    qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
    if (sliceWidget)
    {
      sliceWidget->sliceView()->scheduleRender();
    }
    else if (threeDWidget)
    {
      threeDWidget->threeDView()->scheduleRender();
    }
    else
    {
      qCritical() << "qSlicerSegmentEditorAbstractEffect::deactivate: Unsupported view widget!";
      continue;
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::addActor(qMRMLWidget* viewWidget, vtkProp* actor)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (d->Actors.contains(viewWidget))
  {
    d->Actors[viewWidget] << actor;
  }
  else
  {
    QList<vtkProp*> actorList;
    actorList << actor;
    d->Actors[viewWidget] = actorList;
  }
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentEditorEffectNode* qSlicerSegmentEditorAbstractEffect::parameterSetNode()
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (!m_Scene)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::parameterSetNode: Invalid MRML scene!";
    return NULL;
  }

  // Create if does not yet exist
  if (d->ParameterSetNodeID.isEmpty())
  {
    vtkMRMLSegmentEditorEffectNode* node = vtkMRMLSegmentEditorEffectNode::New();
    QString nodeName = QString("%1_ParameterSet").arg(this->name());
    std::string uniqueNodeName = m_Scene->GenerateUniqueName(nodeName.toLatin1().constData());
    node->SetName(uniqueNodeName.c_str());
    node->HideFromEditorsOn();
    m_Scene->AddNode(node);
    node->Delete(); // Pass ownership to MRML scene only

    // Connect node modified event to update user interface
    qvtkConnect(node, vtkCommand::ModifiedEvent, this, SLOT( updateGUIFromMRML(vtkObject*,void*) ) );

    return node;
  }

  // Find and return if already exists
  vtkMRMLSegmentEditorEffectNode* node = vtkMRMLSegmentEditorEffectNode::SafeDownCast(
    m_Scene->GetNodeByID(d->ParameterSetNodeID.toLatin1().constData()) );
  if (!node)
  {
    qWarning() << "qSlicerSegmentEditorAbstractEffect::parameterSetNode: Unable to find node in scene with ID " << d->ParameterSetNodeID << ", creating a new one";
    d->ParameterSetNodeID = QString();
    return this->parameterSetNode();
  }
  return node;
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorAbstractEffect::parameter(QString name)
{
  vtkMRMLSegmentEditorEffectNode* node = this->parameterSetNode();
  if (!node)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::parameter: Unable to find effect parameter node for effect " << this->name();
    return QString();
  }

  const char* value = node->GetAttribute(name.toLatin1().constData());
  if (!value)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::parameter: Parameter named " << name << " cannot be found for effect " << this->name();
    return QString();
  }

  return QString(value);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::setParameter(QString name, QString value)
{
  vtkMRMLSegmentEditorEffectNode* node = this->parameterSetNode();
  if (!node)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::parameter: Unable to find effect parameter node for effect " << this->name();
    return;
  }

  node->SetAttribute(name.toLatin1().constData(), value.toLatin1().constData());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOff(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->SavedCursor = &(viewWidget->cursor());
  viewWidget->setCursor(QCursor(Qt::BlankCursor));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOn(qMRMLWidget* viewWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (d->SavedCursor)
  {
    viewWidget->setCursor(*(d->SavedCursor));
  }
  else
  {
    viewWidget->unsetCursor();
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::abortEvent(vtkRenderWindowInteractor* interactor, unsigned long eventId, qMRMLWidget* viewWidget)
{
  if (!interactor || !viewWidget)
  {
    return;
  }

  QVariant tagsVariant = viewWidget->property(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier());
  QList<QVariant> tagsList = tagsVariant.toList();
  foreach(QVariant tagVariant, tagsList)
  {
    unsigned long tag = tagVariant.toULongLong();
    vtkCommand* command = interactor->GetCommand(tag);
    command->SetAbortFlag(1);
  }
}

//-----------------------------------------------------------------------------
vtkRenderWindow* qSlicerSegmentEditorAbstractEffect::renderWindow(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return NULL;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceView()->renderWindow();
  }
  else if (threeDWidget)
  {
    return threeDWidget->threeDView()->renderWindow();
  }

  qCritical() << "qSlicerSegmentEditorAbstractEffect::renderWindow: Unsupported view widget type!";
  return NULL;
}

//-----------------------------------------------------------------------------
vtkRenderer* qSlicerSegmentEditorAbstractEffect::renderer(qMRMLWidget* viewWidget)
{
  vtkRenderWindow* renderWindow = qSlicerSegmentEditorAbstractEffect::renderWindow(viewWidget);
  if (!renderWindow)
  {
    return NULL;
  }

  return vtkRenderer::SafeDownCast(renderWindow->GetRenderers()->GetItemAsObject(0));
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractViewNode* qSlicerSegmentEditorAbstractEffect::viewNode(qMRMLWidget* viewWidget)
{
  if (!viewWidget)
  {
    return NULL;
  }

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
  {
    return sliceWidget->sliceLogic()->GetSliceNode();
  }
  else if (threeDWidget)
  {
    return threeDWidget->mrmlViewNode();
  }

  qCritical() << "qSlicerSegmentEditorAbstractEffect::renderWindow: Unsupported view widget type!";
  return NULL;
}

//-----------------------------------------------------------------------------
QPoint qSlicerSegmentEditorAbstractEffect::rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget)
{
  QPoint xy(0,0);

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::rasToXy: Failed to get slice node!";
    return xy;
  }

  double rast[4] = {ras[0], ras[1], ras[2], 1.0};
  double xyzw[4] = {0.0, 0.0, 0.0, 1.0};
  vtkSmartPointer<vtkMatrix4x4> rasToXyMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  rasToXyMatrix->DeepCopy(sliceNode->GetXYToRAS());
  rasToXyMatrix->Invert();
  rasToXyMatrix->MultiplyPoint(rast, xyzw);

  xy.setX(xyzw[0]);
  xy.setY(xyzw[1]);
  return xy;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  outputRas[0] = outputRas[1] = outputRas[2] = 0.0;

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
    qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget) );
  if (!sliceNode)
  {
    qCritical() << "qSlicerSegmentEditorAbstractEffect::xyToRas: Failed to get slice node!";
    return;
  }
  
  // x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  // slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  // coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  double xyzw[4] = {inputXyz[0], inputXyz[1], inputXyz[2], 1.0};
  double rast[4] = {0.0, 0.0, 0.0, 1.0};
  sliceNode->GetXYToRAS()->MultiplyPoint(xyzw, rast);
  outputRas[0] = rast[0];
  outputRas[1] = rast[1];
  outputRas[2] = rast[2];
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget)
{
  double xyz[3] = {xy.x(), xy.y(), 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(xyz, outputRas, sliceWidget);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  outputIjk[0] = outputIjk[1] = outputIjk[2] = 0;

  if (!sliceWidget || !image)
  {
    return;
  }

  // Convert from XY to RAS first
  double ras[3] = {0.0, 0.0, 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToRas(inputXyz, ras, sliceWidget);

  // Convert RAS to image IJK
  double rast[4] = {ras[0], ras[1], ras[2], 1.0};
  double ijkl[4] = {0.0, 0.0, 0.0, 1.0};
  vtkSmartPointer<vtkMatrix4x4> rasToIjkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image->GetImageToWorldMatrix(rasToIjkMatrix);
  rasToIjkMatrix->Invert();
  rasToIjkMatrix->MultiplyPoint(rast, ijkl);

  outputIjk[0] = (int)(ijkl[0] + 0.5);
  outputIjk[1] = (int)(ijkl[1] + 0.5);
  outputIjk[2] = (int)(ijkl[2] + 0.5);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::xyToIjk(QPoint xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image)
{
  double xyz[3] = {xy.x(), xy.y(), 0.0};
  qSlicerSegmentEditorAbstractEffect::xyzToIjk(xyz, outputIjk, sliceWidget, image);
}
