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

#include "qMRMLSortFilterPatientHierarchyProxyModel.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// qMRML includes
#include "qMRMLSceneModel.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>

// -----------------------------------------------------------------------------
// qMRMLSortFilterPatientHierarchyProxyModelPrivate

// -----------------------------------------------------------------------------
class qMRMLSortFilterPatientHierarchyProxyModelPrivate
{
public:
  qMRMLSortFilterPatientHierarchyProxyModelPrivate();
};

// -----------------------------------------------------------------------------
qMRMLSortFilterPatientHierarchyProxyModelPrivate::qMRMLSortFilterPatientHierarchyProxyModelPrivate()
{
}

// -----------------------------------------------------------------------------
// qMRMLSortFilterPatientHierarchyProxyModel

//------------------------------------------------------------------------------
qMRMLSortFilterPatientHierarchyProxyModel::qMRMLSortFilterPatientHierarchyProxyModel(QObject *vparent)
  : qMRMLSortFilterProxyModel(vparent)
  , d_ptr(new qMRMLSortFilterPatientHierarchyProxyModelPrivate)
{
}

//------------------------------------------------------------------------------
qMRMLSortFilterPatientHierarchyProxyModel::~qMRMLSortFilterPatientHierarchyProxyModel()
{
}

//------------------------------------------------------------------------------
qMRMLSortFilterProxyModel::AcceptType qMRMLSortFilterPatientHierarchyProxyModel
::filterAcceptsNode(vtkMRMLNode* node)const
{
  if (!node)
  {
    return Accept;
  }

  AcceptType res = this->Superclass::filterAcceptsNode(node);
  if (res == Reject || res == RejectButPotentiallyAcceptable)
  {
    return res;
  }

  vtkMRMLHierarchyNode* hNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (hNode && SlicerRtCommon::IsPatientHierarchyNode(hNode))
  {
    return Accept;
  }

  return Reject;
}
