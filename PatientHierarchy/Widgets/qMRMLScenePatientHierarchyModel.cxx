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

#include "qMRMLScenePatientHierarchyModel.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// Patient Hierarchy includes
#include "qMRMLScenePatientHierarchyModel_p.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModelPrivate::qMRMLScenePatientHierarchyModelPrivate(qMRMLScenePatientHierarchyModel& object)
: Superclass(object)
{
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModelPrivate::init()
{
  Q_Q(qMRMLScenePatientHierarchyModel);
  this->Superclass::init();

  q->setCheckableColumn(0);
  q->setVisibilityColumn(1);
  q->setNameColumn(2);
  q->setIDColumn(3);

  q->setHorizontalHeaderLabels(
    QStringList() << "" << "Vis" << "Name" << "ID");
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::qMRMLScenePatientHierarchyModel(QObject *vparent)
: Superclass(new qMRMLScenePatientHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::~qMRMLScenePatientHierarchyModel()
{
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLScenePatientHierarchyModel::parentNode(vtkMRMLNode* node)const
{
  return vtkMRMLHierarchyNode::SafeDownCast(
    this->Superclass::parentNode(node));
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  if (!node)
    {
    return false;
    }
  return node->IsA("vtkMRMLNode");
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAParent(vtkMRMLNode* node)const
{
  vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( hnode
    && SlicerRtCommon::IsPatientHierarchyNode(hnode)
    && hnode->GetAssociatedNodeID() == 0 )
    {
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
int qMRMLScenePatientHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  int maxId = this->Superclass::maxColumnId();
  maxId = qMax(maxId, d->CheckableColumn);
  maxId = qMax(maxId, d->VisibilityColumn);
  maxId = qMax(maxId, d->NameColumn);
  maxId = qMax(maxId, d->IDColumn);
  return maxId;
}

//------------------------------------------------------------------------------
int qMRMLScenePatientHierarchyModel::IDColumn()const
{
  Q_D(const qMRMLScenePatientHierarchyModel);
  return d->IDColumn;
}

//------------------------------------------------------------------------------
void qMRMLScenePatientHierarchyModel::setIDColumn(int column)
{
  Q_D(qMRMLScenePatientHierarchyModel);
  d->IDColumn = column;
  this->updateColumnCount();
}

