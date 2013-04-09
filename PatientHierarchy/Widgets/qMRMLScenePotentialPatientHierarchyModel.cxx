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

// MRML includes
#include <vtkMRMLNode.h>

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
void qMRMLScenePotentialPatientHierarchyModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLScenePotentialPatientHierarchyModel);

  item->setText(QString(node->GetName()));
  item->setToolTip(QString(node->GetNodeTagName()) + " type (" + QString(node->GetID()) + ")");
  item->setData(QVariant(node->GetID()), Qt::UserRole);
}

//------------------------------------------------------------------------------
void qMRMLScenePotentialPatientHierarchyModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  node->SetName(item->text().toLatin1().constData());
}
