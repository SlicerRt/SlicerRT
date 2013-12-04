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
#include <QAction>

// SlicerRT includes
#include "SlicerRtCommon.h"

// PatientHierarchy includes
#include "qMRMLPatientHierarchyTreeView.h"
#include "qMRMLScenePatientHierarchyModel.h"
#include "qMRMLSortFilterPatientHierarchyProxyModel.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLScene.h>

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
  QAction* CreateGenericNodeAction;
  QAction* CreateStructureSetNodeAction;
};

//------------------------------------------------------------------------------
qMRMLPatientHierarchyTreeViewPrivate::qMRMLPatientHierarchyTreeViewPrivate(qMRMLPatientHierarchyTreeView& object)
  : q_ptr(&object)
{
  this->SceneModel = NULL;
  this->SortFilterModel = NULL;
  this->Logic = NULL;
  this->CreateGenericNodeAction = NULL;
  this->CreateStructureSetNodeAction = NULL;
}

//------------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeViewPrivate::init()
{
  Q_Q(qMRMLPatientHierarchyTreeView);

  // Set up scene model and sort and proxy model
  this->SceneModel = new qMRMLScenePatientHierarchyModel(q);
  QObject::connect( this->SceneModel, SIGNAL(saveTreeExpandState()), q, SLOT(saveTreeExpandState()) );
  QObject::connect( this->SceneModel, SIGNAL(loadTreeExpandState()), q, SLOT(loadTreeExpandState()) );
  q->setSceneModel(this->SceneModel, "PatientHierarchy");

  this->SortFilterModel = new qMRMLSortFilterPatientHierarchyProxyModel(q);
  q->setSortFilterProxyModel(this->SortFilterModel);

  // Change item visibility
  q->setShowScene(true);
  q->setUniformRowHeights(false);
  q->setEditMenuActionVisible(false);
  q->setDeleteMenuActionVisible(false);

  // Add scene context menu actions
  QAction* createPatientAction = new QAction(qMRMLPatientHierarchyTreeView::tr("Create new patient"),q);
  q->appendSceneMenuAction(createPatientAction);
  QObject::connect(createPatientAction, SIGNAL(triggered()), q, SLOT(createChildNode()));

  // Add special node context menu actions
  this->CreateGenericNodeAction = new QAction(qMRMLPatientHierarchyTreeView::tr("Create generic child node"),q);
  q->appendNodeMenuAction(this->CreateGenericNodeAction);
  QObject::connect(this->CreateGenericNodeAction, SIGNAL(triggered()), q, SLOT(createChildNode()));

  this->CreateStructureSetNodeAction = new QAction(qMRMLPatientHierarchyTreeView::tr("Create child structure set"),q);
  q->appendNodeMenuAction(this->CreateStructureSetNodeAction);
  QObject::connect(this->CreateStructureSetNodeAction, SIGNAL(triggered()), q, SLOT(createStructureSetNode()));

  // Set up headers
  q->header()->setStretchLastSection(false);
  q->header()->setResizeMode(0, QHeaderView::Stretch);
  q->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  q->header()->setResizeMode(2, QHeaderView::ResizeToContents);

  // Set connection to handle current node change
  QObject::connect( q, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(onCurrentNodeChanged(vtkMRMLNode*)) );
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

  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!hierarchyNode || !SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(),"qMRMLPatientHierarchyTreeView::toggleVisibility: Invalid node in patient hierarchy tree! Nodes must all be patient hierarchy nodes.");
    return;
  }
  vtkMRMLNode* associatedNode = hierarchyNode->GetAssociatedNode();

  if (associatedNode && associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    // Show/hide is available in case of RT images. Not propagated to possible children
    if (hierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_RTIMAGE_IDENTIFIER_ATTRIBUTE_NAME.c_str()))
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        associatedNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      if (!modelNode)
      {
        vtkErrorWithObjectMacro(this->mrmlScene(),"qMRMLPatientHierarchyTreeView::toggleVisibility: No displayed model found for planar image '" << associatedNode->GetName() << "'!");
        return;
      }
      modelNode->SetDisplayVisibility( !modelNode->GetDisplayVisibility() );
      hierarchyNode->Modified(); // Triggers icon refresh in patient hierarchy tree
    }
    else
    {
      // Showing volume node does not trigger visibility change on the possible children
      this->showVolume(associatedNode);
    }
  }
  else
  {
    int visible = (vtkSlicerPatientHierarchyModuleLogic::GetBranchVisibility(hierarchyNode) > 0 ? 0 : 1);
    vtkSlicerPatientHierarchyModuleLogic::SetBranchVisibility(hierarchyNode, visible);
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

  vtkMRMLScalarVolumeNode* volumeNode = NULL;
  if ((volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node)) == NULL)
  {
    vtkErrorWithObjectMacro(node, "ShowVolume: Attribute node is not a volume node: " << node->GetName());
    return;
  }

  // Set input volume as background volume, set the original background to foreground with opacity of 0.5
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

//--------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeView::createChildNode()
{
  Q_D(qMRMLPatientHierarchyTreeView);

  vtkMRMLHierarchyNode* newNode = vtkSlicerPatientHierarchyModuleLogic::CreateGenericChildNodeForPatientHierarchyNode(this->currentNode(), this->mrmlScene());
  if (newNode)
  {
    QModelIndex nodeIndex = d->SortFilterModel->indexFromMRMLNode(newNode);
    this->expand(nodeIndex);
  }
}

//--------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeView::createStructureSetNode()
{
  if (!this->currentNode())
  {
    qCritical() << "qMRMLPatientHierarchyTreeView::createChildNode: No current node!";
    return;
  }

  Q_D(qMRMLPatientHierarchyTreeView);

  vtkMRMLHierarchyNode* newNode = vtkSlicerPatientHierarchyModuleLogic::CreateChildStructureSetNodeForPatientHierarchyNode(this->currentNode());
  if (newNode)
  {
    QModelIndex nodeIndex = d->SortFilterModel->indexFromMRMLNode(newNode);
    this->expand(nodeIndex);
  }
}

//--------------------------------------------------------------------------
void qMRMLPatientHierarchyTreeView::onCurrentNodeChanged(vtkMRMLNode* newCurrentNode)
{
  if (!newCurrentNode)
  {
    // Scene is selected
    return;
  }

  Q_D(qMRMLPatientHierarchyTreeView);

  const char* nodeLevel = newCurrentNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
  // Disable create structure set action if node level is not study
  if (!nodeLevel || STRCASECMP(nodeLevel, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY))
  {
    d->CreateStructureSetNodeAction->setVisible(false);
  }
  else
  {
    d->CreateStructureSetNodeAction->setVisible(true);
  }
}
