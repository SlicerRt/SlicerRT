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

#include "qMRMLSortFilterPotentialPatientHierarchyProxyModel.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "vtkMRMLContourNode.h"

// qMRML includes
#include "qMRMLSceneModel.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>

// -----------------------------------------------------------------------------
// qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate

// -----------------------------------------------------------------------------
class qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate
{
public:
  qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate();

  bool includeHiddenNodes;
};

// -----------------------------------------------------------------------------
qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate::qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate()
: includeHiddenNodes(false)
{
}

// -----------------------------------------------------------------------------
// qMRMLSortFilterPotentialPatientHierarchyProxyModel

//------------------------------------------------------------------------------
qMRMLSortFilterPotentialPatientHierarchyProxyModel::qMRMLSortFilterPotentialPatientHierarchyProxyModel(QObject *vparent)
  : qMRMLSortFilterProxyModel(vparent)
  , d_ptr(new qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate)
{
}

//------------------------------------------------------------------------------
qMRMLSortFilterPotentialPatientHierarchyProxyModel::~qMRMLSortFilterPotentialPatientHierarchyProxyModel()
{
}

//------------------------------------------------------------------------------
qMRMLSortFilterProxyModel::AcceptType qMRMLSortFilterPotentialPatientHierarchyProxyModel
::filterAcceptsNode(vtkMRMLNode* node)const
{
  Q_D(const qMRMLSortFilterPotentialPatientHierarchyProxyModel);

  if (!node)
  {
    return Accept;
  }

  AcceptType res = this->Superclass::filterAcceptsNode(node);
  if (res == Reject || res == RejectButPotentiallyAcceptable)
  {
    return res;
  }

  // Show only if its type is among the potential patient hierarchy types
  // and it is not associated to a patient hierarchy node
  // and it is not a representation object of a contour node
  if (node && vtkSlicerPatientHierarchyModuleLogic::IsPotentialPatientHierarchyNode(node))
  {
    vtkMRMLHierarchyNode* possiblePhNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->mrmlScene(), node->GetID());
    if (!SlicerRtCommon::IsPatientHierarchyNode(possiblePhNode) && (d->includeHiddenNodes || !node->GetHideFromEditors()))
    {
      // If the node is a contour representation then do not add it to the list
      if (vtkMRMLContourNode::IsNodeAContourRepresentation(this->mrmlScene(), node))
      {
        return RejectButPotentiallyAcceptable;
      }
      else
      {
        return Accept;
      }
    }
  }

  return Reject;
}
