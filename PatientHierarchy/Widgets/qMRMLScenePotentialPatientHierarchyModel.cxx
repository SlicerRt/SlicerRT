/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

#include "qMRMLScenePotentialPatientHierarchyModel.h"

// Patient Hierarchy includes
#include "qMRMLScenePotentialPatientHierarchyModel_p.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "vtkSlicerPatientHierarchyPluginHandler.h"
#include "vtkSlicerPatientHierarchyPlugin.h"

// MRML includes
#include <vtkMRMLNode.h>

// Qt includes
#include <QMimeData>
#include <QTimer>

//------------------------------------------------------------------------------
qMRMLScenePotentialPatientHierarchyModelPrivate::qMRMLScenePotentialPatientHierarchyModelPrivate(qMRMLScenePotentialPatientHierarchyModel& object)
: Superclass(object)
{
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModelPrivate::init()
{
  Q_Q(qMRMLScenePotentialPatientHierarchyModel);
  this->Superclass::init();

  QObject::connect( q, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), q, SLOT(onRowsRemoved(QModelIndex,int,int)) );
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLScenePotentialPatientHierarchyModel::qMRMLScenePotentialPatientHierarchyModel(QObject *vparent)
: Superclass(new qMRMLScenePotentialPatientHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLScenePotentialPatientHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLScenePotentialPatientHierarchyModel::~qMRMLScenePotentialPatientHierarchyModel()
{
}

//------------------------------------------------------------------------------
QStringList qMRMLScenePotentialPatientHierarchyModel::mimeTypes()const
{
  QStringList types;
  types << "application/vnd.text.list";
  return types;
}

//------------------------------------------------------------------------------
QMimeData* qMRMLScenePotentialPatientHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
  Q_D(const qMRMLScenePotentialPatientHierarchyModel);

  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (const QModelIndex &index, indexes)
  {
    if (index.isValid())
    {
      d->DraggedNodes << this->mrmlNodeFromIndex(index);
      QString text = data(index, PointerRole).toString();
      stream << text;
    }
  }

  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_UNUSED(column);

  item->setText(QString(node->GetName()));
  item->setToolTip(QString(node->GetNodeTagName()) + " type (" + QString(node->GetID()) + ")");
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  node->SetName(item->text().toLatin1().constData());
}

//------------------------------------------------------------------------------
bool qMRMLScenePotentialPatientHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  // Volumes and models can be patient hierarchy leaves by default
  // TODO: Outsource this decision to the modules? (e.g. Beams in case of models)
  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLModelNode"))
  {
    return true;
  }

  // Otherwise, if there is a plugin that can handle adding the node, then it also can be a child
  vtkSlicerPatientHierarchyPlugin* foundPlugin = vtkSlicerPatientHierarchyPluginHandler::GetInstance()->GetPluginForAddToPatientHierarchyForNode(node);
  if (foundPlugin)
  {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
Qt::DropActions qMRMLScenePotentialPatientHierarchyModel::supportedDropActions()const
{
  return Qt::MoveAction;
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::onRowsRemoved(const QModelIndex parent, int start, int end)
{
  Q_D(const qMRMLScenePotentialPatientHierarchyModel);
  Q_UNUSED(parent);
  Q_UNUSED(start);
  Q_UNUSED(end);

  if (d->DraggedNodes.count())
  {
    d->DraggedNodes.clear();

    // Force updating the whole scene (TODO: this should not be needed)
    QTimer::singleShot(200, this, SLOT(delayedUpdateScene()));
  }
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::delayedUpdateScene()
{
  this->updateScene();
}
