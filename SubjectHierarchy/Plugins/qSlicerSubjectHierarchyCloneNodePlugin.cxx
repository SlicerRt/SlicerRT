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

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSubjectHierarchyConstants.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyCloneNodePlugin.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Slicer includes
#include "qSlicerCoreApplication.h"
#include "vtkSlicerApplicationLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLDisplayableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>
#include <vtksys/SystemTools.hxx>

// Qt includes
#include <QDebug>
#include <QStandardItem>
#include <QAction>

//----------------------------------------------------------------------------
const std::string qSlicerSubjectHierarchyCloneNodePlugin::CLONE_NODE_NAME_POSTFIX = std::string(" Copy");

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy_Plugins
class qSlicerSubjectHierarchyCloneNodePluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyCloneNodePlugin);
protected:
  qSlicerSubjectHierarchyCloneNodePlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyCloneNodePluginPrivate(qSlicerSubjectHierarchyCloneNodePlugin& object);
  ~qSlicerSubjectHierarchyCloneNodePluginPrivate();
  void init();
public:
  QAction* CloneNodeAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyCloneNodePluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyCloneNodePluginPrivate::qSlicerSubjectHierarchyCloneNodePluginPrivate(qSlicerSubjectHierarchyCloneNodePlugin& object)
: q_ptr(&object)
{
  this->CloneNodeAction = NULL;
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyCloneNodePluginPrivate::~qSlicerSubjectHierarchyCloneNodePluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyCloneNodePlugin::qSlicerSubjectHierarchyCloneNodePlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyCloneNodePluginPrivate(*this) )
{
  this->m_Name = QString("CloneNode");

  Q_D(qSlicerSubjectHierarchyCloneNodePlugin);
  d->init();
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyCloneNodePluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyCloneNodePlugin);

  this->CloneNodeAction = new QAction("Clone node",q);
  QObject::connect(this->CloneNodeAction, SIGNAL(triggered()), q, SLOT(cloneCurrentNode()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyCloneNodePlugin::~qSlicerSubjectHierarchyCloneNodePlugin()
{
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyCloneNodePlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyCloneNodePlugin);

  QList<QAction*> actions;
  actions << d->CloneNodeAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyCloneNodePlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyCloneNodePlugin);
  this->hideAllContextMenuActions();

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  // Show clone node for every non-scene nodes
  d->CloneNodeAction->setVisible(true);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyCloneNodePlugin::cloneCurrentNode()
{
  // Get currently selected node and scene
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!currentNode || !scene)
  {
    qCritical() << "qSlicerSubjectHierarchyCloneNodePlugin::cloneCurrentNode: Invalid current node or MRML scene!";
    return;
  }

  vtkMRMLNode* associatedDataNode = currentNode->GetAssociatedDataNode();
  if (associatedDataNode)
  {
    // Clone data node
    vtkMRMLNode* clonedDataNode = scene->CreateNodeByClass(associatedDataNode->GetClassName());
    clonedDataNode->Copy(associatedDataNode);
    std::string clonedDataNodeName = std::string(associatedDataNode->GetName()) + CLONE_NODE_NAME_POSTFIX;
    clonedDataNode->SetName(clonedDataNodeName.c_str());
    scene->AddNode(clonedDataNode);
    clonedDataNode->Delete(); // Return the ownership to the scene only

    // Clone display node
    vtkMRMLDisplayableNode* displayableDataNode = vtkMRMLDisplayableNode::SafeDownCast(associatedDataNode);
    if (displayableDataNode && displayableDataNode->GetDisplayNode())
    {
      vtkMRMLDisplayNode* clonedDisplayNode = vtkMRMLDisplayNode::SafeDownCast(
        scene->CreateNodeByClass(displayableDataNode->GetDisplayNode()->GetClassName()) );
      clonedDisplayNode->Copy(displayableDataNode->GetDisplayNode());
      std::string clonedDisplayNodeName = std::string(displayableDataNode->GetDisplayNode()->GetName()) + CLONE_NODE_NAME_POSTFIX;
      clonedDisplayNode->SetName(clonedDisplayNodeName.c_str());
      scene->AddNode(clonedDisplayNode);
      vtkMRMLDisplayableNode* clonedDisplayableDataNode = vtkMRMLDisplayableNode::SafeDownCast(clonedDataNode);
      clonedDisplayableDataNode->SetAndObserveDisplayNodeID(clonedDisplayNode->GetID());
      clonedDisplayNode->Delete(); // Return the ownership to the scene only
    }

    // Clone storage node
    vtkMRMLStorableNode* storableDataNode = vtkMRMLStorableNode::SafeDownCast(associatedDataNode);
    if (storableDataNode && storableDataNode->GetStorageNode())
    {
      vtkMRMLStorageNode* clonedStorageNode = vtkMRMLStorageNode::SafeDownCast(
        scene->CreateNodeByClass(storableDataNode->GetStorageNode()->GetClassName()) );
      clonedStorageNode->Copy(storableDataNode->GetStorageNode());
      std::string clonedStorageNodeFileName = std::string(storableDataNode->GetStorageNode()->GetFileName()) + CLONE_NODE_NAME_POSTFIX;
      clonedStorageNode->SetFileName(clonedStorageNodeFileName.c_str());
      vtkMRMLStorableNode* clonedStorableDataNode = vtkMRMLStorableNode::SafeDownCast(clonedDataNode);
      scene->AddNode(clonedStorableDataNode);
      clonedStorableDataNode->SetAndObserveStorageNodeID(clonedStorageNode->GetID());
      clonedStorableDataNode->Delete(); // Return the ownership to the scene only
    }

    // Get hierarchy nodes
    vtkMRMLHierarchyNode* genericHierarchyNode =
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, associatedDataNode->GetID());

    // Put data node in the same non-subject hierarchy if any
    if (genericHierarchyNode != currentNode)
    {
      vtkMRMLHierarchyNode* clonedHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(
        scene->CreateNodeByClass(genericHierarchyNode->GetClassName()) );
      clonedHierarchyNode->Copy(genericHierarchyNode);
      std::string clonedHierarchyNodeName = std::string(genericHierarchyNode->GetName()) + CLONE_NODE_NAME_POSTFIX;
      clonedHierarchyNode->SetName(clonedHierarchyNodeName.c_str());
      scene->AddNode(clonedHierarchyNode);
      clonedHierarchyNode->SetAssociatedNodeID(clonedDataNode->GetID());
      clonedHierarchyNode->Delete(); // Return the ownership to the scene only
    }

    // Put data node in the same subject hierarchy branch as current node
    vtkMRMLSubjectHierarchyNode* clonedSubjectHierarchyNode =
      vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(scene,
      vtkMRMLSubjectHierarchyNode::SafeDownCast(currentNode->GetParentNode()),
      currentNode->GetLevel(), clonedDataNodeName.c_str(), clonedDataNode);

    // Trigger update
    clonedSubjectHierarchyNode->Modified();
    emit requestInvalidateModels();
  }
  else // No associated node
  {
    std::string clonedSubjectHierarchyNodeName = currentNode->GetName();
    vtksys::SystemTools::ReplaceString(clonedSubjectHierarchyNodeName,
      vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX.c_str(), "");
    clonedSubjectHierarchyNodeName.append(CLONE_NODE_NAME_POSTFIX);

    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(scene,
      vtkMRMLSubjectHierarchyNode::SafeDownCast(currentNode->GetParentNode()),
      currentNode->GetLevel(), clonedSubjectHierarchyNodeName.c_str());
  }
}
