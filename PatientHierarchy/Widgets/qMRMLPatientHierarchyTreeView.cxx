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

// Qt includes
#include <QHeaderView>
#include <QMessageBox>

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

// PatientHierarchy includes
#include "qMRMLPatientHierarchyTreeView.h"
#include "qMRMLScenePatientHierarchyModel.h"
#include "qMRMLSortFilterPatientHierarchyProxyModel.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLApplicationLogic.h>

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_PatientHierarchy
class qMRMLPatientHierarchyTreeViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLPatientHierarchyTreeView);

protected:
  qMRMLPatientHierarchyTreeView* const q_ptr;

public:
  qMRMLPatientHierarchyTreeViewPrivate(qMRMLPatientHierarchyTreeView& object);
  void init();

  qMRMLScenePatientHierarchyModel* SceneModel;
  qMRMLSortFilterPatientHierarchyProxyModel* SortFilterModel;
  vtkSlicerPatientHierarchyModuleLogic* Logic;
};

//------------------------------------------------------------------------------
qMRMLPatientHierarchyTreeViewPrivate::qMRMLPatientHierarchyTreeViewPrivate(qMRMLPatientHierarchyTreeView& object)
  : q_ptr(&object)
{
  this->SceneModel = NULL;
  this->SortFilterModel = NULL;
  this->Logic = NULL;
}

//------------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeViewPrivate::init()
{
  Q_Q(qMRMLPatientHierarchyTreeView);

  this->SceneModel = new qMRMLScenePatientHierarchyModel(q);
  this->SceneModel->setLazyUpdate(false); //TODO: Is this needed?
  q->setSceneModel(this->SceneModel, "PatientHierarchy");

  this->SortFilterModel = new qMRMLSortFilterPatientHierarchyProxyModel(q);
  q->setSortFilterProxyModel(this->SortFilterModel);

  q->setShowScene(false);

  q->header()->setStretchLastSection(false);
  q->header()->setResizeMode(0, QHeaderView::Stretch);
  q->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  q->header()->setResizeMode(2, QHeaderView::ResizeToContents);
}

//------------------------------------------------------------------------------
qMRMLPatientHierarchyTreeView::qMRMLPatientHierarchyTreeView(QWidget *parent)
  : qMRMLTreeView(parent)
  , d_ptr(new qMRMLPatientHierarchyTreeViewPrivate(*this))
{
  Q_D(qMRMLPatientHierarchyTreeView);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLPatientHierarchyTreeView::~qMRMLPatientHierarchyTreeView()
{
}

//------------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeView::toggleVisibility(const QModelIndex& index)
{
  Q_D(qMRMLPatientHierarchyTreeView);
  vtkMRMLNode* node = d->SortFilterModel->mrmlNodeFromIndex(index);
  if (!node)
  {
    return;
  }

  if (SlicerRtCommon::IsPatientHierarchyNode(node))
  {
    vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
    int visible = (vtkSlicerPatientHierarchyModuleLogic::GetBranchVisibility(hnode) == 1 ? 0 : 1);

    vtkSlicerPatientHierarchyModuleLogic::SetBranchVisibility( hnode, visible );
  }
  else if (node->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
    int visible = (contourNode->GetDisplayVisibility() ? 0 : 1);
    contourNode->SetDisplayVisibility(visible);
    // Make sure the icons changes in the tree view
    contourNode->Modified();
    vtkSlicerPatientHierarchyModuleLogic::SetModifiedToAllAncestors(contourNode);
  }
  else if (node->IsA("vtkMRMLVolumeNode"))
  {
    this->showVolume(node);
  }
  else if (node->IsA("vtkMRMLDisplayableNode"))
  {
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(node);
    int visible = (displayableNode->GetDisplayVisibility() ? 0 : 1);
    displayableNode->SetDisplayVisibility(visible);
    // Make sure the icons changes in the tree view
    displayableNode->Modified();
    vtkSlicerPatientHierarchyModuleLogic::SetModifiedToAllAncestors(displayableNode);

    vtkMRMLDisplayNode* displayNode = displayableNode->GetDisplayNode();
    if (displayNode)
    {
      displayNode->SetSliceIntersectionVisibility(visible);
    }
  }
}

//-----------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeView::setLogic(vtkSlicerPatientHierarchyModuleLogic* logic)
{
  if (!logic)
  {
    return;
  }

  Q_D(qMRMLPatientHierarchyTreeView);

  d->Logic = logic;
}

//---------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeView::showVolume(vtkMRMLNode* node)
{
  if (!node)
  {
    std::cerr << "qMRMLPatientHierarchyTreeView::showVolume: NULL node given!" << std::endl;
    return;
  }

  Q_D(qMRMLPatientHierarchyTreeView);

  vtkMRMLSelectionNode* selectionNode = d->Logic->GetApplicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    vtkErrorWithObjectMacro(node, "ShowVolume: Unable to get selection node to show volume node " << node->GetName());
    return;
  }

  vtkMRMLVolumeNode* volumeNode = NULL;
  if ((volumeNode = vtkMRMLVolumeNode::SafeDownCast(node)) == NULL)
  {
    vtkErrorWithObjectMacro(node, "ShowVolume: Attribute node is not a volume node: " << node->GetName());
    return;
  }

  selectionNode->SetSecondaryVolumeID(selectionNode->GetActiveVolumeID());
  selectionNode->SetActiveVolumeID(volumeNode->GetID());
  d->Logic->GetApplicationLogic()->PropagateVolumeSelection();

  vtkMRMLSliceCompositeNode *cnode = NULL;
  const int nnodes = volumeNode->GetScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
  for (int i = 0; i < nnodes; i++)
  {
    cnode = vtkMRMLSliceCompositeNode::SafeDownCast ( volumeNode->GetScene()->GetNthNodeByClass( i, "vtkMRMLSliceCompositeNode" ) );
    if (cnode && cnode->GetForegroundOpacity() == 0.0)
    {
      cnode->SetForegroundOpacity(0.5);
    }
  }
}
