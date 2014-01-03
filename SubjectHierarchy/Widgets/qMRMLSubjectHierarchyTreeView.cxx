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
#include <QAction>
#include <QActionGroup>
#include <QMenu>

// SubjectHierarchy includes
#include "qMRMLSubjectHierarchyTreeView.h"
#include "qMRMLSceneSubjectHierarchyModel.h"
#include "qMRMLSortFilterSubjectHierarchyProxyModel.h"

#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

// MRML includes
#include <vtkMRMLScene.h>

// MRML Widgets includes
#include "qMRMLTreeView_p.h"

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qMRMLSubjectHierarchyTreeViewPrivate// : public qMRMLTreeViewPrivate //TODO: Uncomment when qMRMLTreeViewPrivate is exported
{
  Q_DECLARE_PUBLIC(qMRMLSubjectHierarchyTreeView);
protected:
  qMRMLSubjectHierarchyTreeView* const q_ptr;

public:
  qMRMLSubjectHierarchyTreeViewPrivate(qMRMLSubjectHierarchyTreeView& object);
  void init();

  qMRMLSceneSubjectHierarchyModel* SceneModel;
  qMRMLSortFilterSubjectHierarchyProxyModel* SortFilterModel;
  QList<QAction*> SelectPluginActions;
  QActionGroup* SelectPluginActionGroup;
};

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyTreeViewPrivate::qMRMLSubjectHierarchyTreeViewPrivate(qMRMLSubjectHierarchyTreeView& object)
//  : qMRMLTreeViewPrivate(object) //TODO: Uncomment when qMRMLTreeViewPrivate is exported
//  , q_ptr(&object)
  : q_ptr(&object)
{
  this->SceneModel = NULL;
  this->SortFilterModel = NULL;
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeViewPrivate::init()
{
  Q_Q(qMRMLSubjectHierarchyTreeView);

  // Set up scene model and sort and proxy model
  this->SceneModel = new qMRMLSceneSubjectHierarchyModel(q);
  QObject::connect( this->SceneModel, SIGNAL(saveTreeExpandState()), q, SLOT(saveTreeExpandState()) );
  QObject::connect( this->SceneModel, SIGNAL(loadTreeExpandState()), q, SLOT(loadTreeExpandState()) );
  q->setSceneModel(this->SceneModel, "SubjectHierarchy");

  this->SortFilterModel = new qMRMLSortFilterSubjectHierarchyProxyModel(q);
  q->setSortFilterProxyModel(this->SortFilterModel);

  q->QTreeView::setModel(this->SortFilterModel);

  // Change item visibility
  q->setShowScene(true);
  q->setUniformRowHeights(false);
  q->setEditMenuActionVisible(false);
  q->setDeleteMenuActionVisible(false);

  // Set up headers
  q->header()->setStretchLastSection(false);
  q->header()->setResizeMode(0, QHeaderView::Stretch);
  q->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  q->header()->setResizeMode(2, QHeaderView::ResizeToContents);

  // Set connection to handle current node change
  QObject::connect( q, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onCurrentNodeChanged(vtkMRMLNode*)) );

  // Perform tasks need for all plugins
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
  {
    // Add node context menu actions
    foreach (QAction* action, plugin->nodeContextMenuActions())
    {
      q->appendNodeMenuAction(action);
    }

    // Add scene context menu actions
    foreach (QAction* action, plugin->sceneContextMenuActions())
    {
      q->appendSceneMenuAction(action);
    }

    // Connect plugin events to be handled by the tree view
    QObject::connect( plugin, SIGNAL(requestExpandNode(vtkMRMLSubjectHierarchyNode*)),
      q, SLOT(expandNode(vtkMRMLSubjectHierarchyNode*)) );
  }

  // Create a plugin selection action for each plugin in a sub-menu
  QAction* separatorAction = new QAction(q);
  separatorAction->setSeparator(true);
  q->appendNodeMenuAction(separatorAction);
  this->SelectPluginActionGroup = new QActionGroup(q);
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
  {
    QAction* selectPluginAction = new QAction(plugin->name(),q);
    selectPluginAction->setCheckable(true);
    selectPluginAction->setActionGroup(this->SelectPluginActionGroup);
    q->appendNodeMenuAction(selectPluginAction);
    QObject::connect(selectPluginAction, SIGNAL(triggered()), q, SLOT(selectPluginForCurrentNode()));
    this->SelectPluginActions << selectPluginAction;
  }
  //TODO: Change back to the code below when qMRMLTreeViewPrivate is exported
  //QMenu* selectPluginSubMenu = this->NodeMenu->addMenu("Select owner plugin");
  //this->SelectPluginActionGroup = new QActionGroup(q);
  //foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
  //{
  //  QAction* selectPluginAction = new QAction(plugin->name(),q);
  //  selectPluginAction->setCheckable(true);
  //  selectPluginAction->setActionGroup(this->SelectPluginActionGroup);
  //  selectPluginSubMenu->addAction(selectPluginAction);
  //  QObject::connect(selectPluginAction, SIGNAL(triggered()), q, SLOT(selectPluginForActiveNode()));
  //  this->SelectPluginActions << selectPluginAction;
  //}
}

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyTreeView::qMRMLSubjectHierarchyTreeView(QWidget *parent)
  : qMRMLTreeView(parent)
  , d_ptr(new qMRMLSubjectHierarchyTreeViewPrivate(*this))
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLSubjectHierarchyTreeView::~qMRMLSubjectHierarchyTreeView()
{
}

//------------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::toggleVisibility(const QModelIndex& index)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  vtkMRMLNode* node = d->SortFilterModel->mrmlNodeFromIndex(index);
  if (!node)
  {
    return;
  }

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
  if (!subjectHierarchyNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(),"toggleVisibility: Invalid node in subject hierarchy tree! Nodes must all be subject hierarchy nodes.");
    return;
  }
  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyNode(subjectHierarchyNode);

  int visible = (ownerPlugin->getDisplayVisibility(subjectHierarchyNode) > 0 ? 0 : 1);
  ownerPlugin->setDisplayVisibility(subjectHierarchyNode, visible);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::onCurrentNodeChanged(vtkMRMLNode* newCurrentNode)
{
  Q_D(qMRMLSubjectHierarchyTreeView);

  // Hide all context menu items first
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
  {
    plugin->hideAllContextMenuActions();
  }

  // Set new current node to plugin handler (even if it's NULL which means the scene is selected)
  qSlicerSubjectHierarchyPluginHandler::instance()->setCurrentNode(
    vtkMRMLSubjectHierarchyNode::SafeDownCast(newCurrentNode) );

  // Scene is selected
  if (!newCurrentNode)
  {
    foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
    {
      plugin->showContextMenuActionsForHandlingNode(NULL);
      plugin->showContextMenuActionsForCreatingChildForNode(NULL);
    }
    return;
  }

  // Node is selected, do a sanity check
  vtkMRMLSubjectHierarchyNode* currentSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(newCurrentNode);
  if (!currentSubjectHierarchyNode)
  {
    qCritical() << "qMRMLSubjectHierarchyTreeView::onCurrentNodeChanged: Selected node is not a subject hierarchy node!";
    return;
  }

  // Show only the context menu items for handling the selected node
  // that have been added by the owner plugin or its dependencies
  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyNode(currentSubjectHierarchyNode);
  QList<qSlicerSubjectHierarchyAbstractPlugin*> dependencyList =
    qSlicerSubjectHierarchyPluginHandler::instance()->dependenciesForPlugin(ownerPlugin);
  dependencyList << ownerPlugin; // The owner plugin needs to add its context menu actions too
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, dependencyList)
  {
    plugin->showContextMenuActionsForHandlingNode(currentSubjectHierarchyNode);
  }

  // Show context menu items for creating child node for all plugins
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
  {
    plugin->showContextMenuActionsForCreatingChildForNode(currentSubjectHierarchyNode);
  }

  // Check the plugin action to the one that owns the current node
  this->updatePluginActionCheckedStates(currentSubjectHierarchyNode);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::expandNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  if (node)
  {
    QModelIndex nodeIndex = d->SortFilterModel->indexFromMRMLNode(node);
    this->expand(nodeIndex);
  }
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::selectPluginForCurrentNode()
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!currentNode)
  {
    qCritical() << "qMRMLSubjectHierarchyTreeView::selectPluginForCurrentNode: Invalid current node for manually selecting owner plugin!";
    return;
  }
  QString selectedPluginName = d->SelectPluginActionGroup->checkedAction()->text();
  if (!selectedPluginName.compare(currentNode->GetOwnerPluginName()))
  {
    // Do nothing if the owner plugin stays the same
    return;
  }

  // Check if the user is setting the plugin that would otherwise be chosen automatically
  qSlicerSubjectHierarchyAbstractPlugin* mostSuitablePluginByConfidenceNumbers =
    qSlicerSubjectHierarchyPluginHandler::instance()->findOwnerPluginForSubjectHierarchyNode(currentNode);
  bool mostSuitablePluginByConfidenceNumbersSelected =
    !mostSuitablePluginByConfidenceNumbers->name().compare(selectedPluginName);
  // Set owner plugin auto search flag to false if the user manually selected a plugin other
  // than the most suitable one by confidence numbers
  currentNode->SetOwnerPluginAutoSearch(mostSuitablePluginByConfidenceNumbersSelected);

  // Set new owner plugin
  currentNode->SetOwnerPluginName(selectedPluginName.toLatin1().constData());
  qDebug() << "qMRMLSubjectHierarchyTreeView::selectPluginForCurrentNode: Owner plugin of subject hierarchy node '"
    << currentNode->GetName() << "' has been manually changed to '" << d->SelectPluginActionGroup->checkedAction()->text() << "'";

  // Check selected plugin
  this->updatePluginActionCheckedStates(currentNode);
}

//--------------------------------------------------------------------------
void qMRMLSubjectHierarchyTreeView::updatePluginActionCheckedStates(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qMRMLSubjectHierarchyTreeView);
  foreach (QAction* selectPluginAction, d->SelectPluginActions)
  {
    bool checked = !(selectPluginAction->text().compare(node->GetOwnerPluginName()));
    selectPluginAction->blockSignals(true);
    selectPluginAction->setChecked(checked);
    selectPluginAction->blockSignals(false);
  }
}
