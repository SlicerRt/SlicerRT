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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// SlicerRT includes
#include "vtkMRMLPatientHierarchyNode.h"

// Qt includes

// qMRML includes
#include "qMRMLSceneModel.h"
#include "qMRMLSortFilterPatientHierarchyProxyModel.h"

// VTK includes

// MRML includes
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLVolumeNode.h>

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
  //TODO: Only PatientHierarchy nodes should be visible
  // (maybe (option to) hide the hierarchy node if a displayable node is
  // associated with it)

  if (!node)
  {
    return Accept;
  }

  AcceptType res = this->Superclass::filterAcceptsNode(node);
  if (res == Reject || res == RejectButPotentiallyAcceptable)
  {
    return res;
  }

  vtkMRMLPatientHierarchyNode* hNode = vtkMRMLPatientHierarchyNode::SafeDownCast(node);
  // Don't show vtkMRMLPatientHierarchyNode if they are tied to a displayable node
  // The only vtkMRMLPatientHierarchyNode to display are the ones who reference other
  // vtkMRMLPatientHierarchyNode (tree parent) or empty (tree parent to be)
  if (hNode)
  {
    if (hNode->GetAssociatedNode())
    {
      return Accept;//RejectButPotentiallyAcceptable;
    }
    else
    {
      return Accept;
    }
  }

  vtkMRMLModelNode* mNode = vtkMRMLModelNode::SafeDownCast(node);
  if (mNode)
  {
    return AcceptButPotentiallyRejectable;
  }
  vtkMRMLVolumeNode* vNode = vtkMRMLVolumeNode::SafeDownCast(node);
  if (vNode)
  {
    return AcceptButPotentiallyRejectable;
  }

  return Reject;
}
