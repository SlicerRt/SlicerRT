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

// Qt includes
#include <QDebug>
#include <QCursor>

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLThreeDWidget.h"

// MRML includes
#include "vtkMRMLScene.h"

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
  QCursor* SavedCursor;
  QString ParameterSetNodeID;
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
{
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect::~qSlicerSegmentEditorAbstractEffect()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOff(qMRMLSliceWidget* sliceWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  d->SavedCursor = &(sliceWidget->cursor());
  sliceWidget->setCursor(QCursor(Qt::BlankCursor));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorAbstractEffect::cursorOn(qMRMLSliceWidget* sliceWidget)
{
  Q_D(qSlicerSegmentEditorAbstractEffect);

  if (d->SavedCursor)
  {
    sliceWidget->setCursor(*(d->SavedCursor));
  }
  else
  {
    sliceWidget->unsetCursor();
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
