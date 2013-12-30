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

#include "qMRMLSceneSubjectHierarchyModel.h"

// Subject Hierarchy includes
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "qMRMLSceneSubjectHierarchyModel_p.h"
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSceneViewNode.h>

// Qt includes
#include <QDebug>
#include <QMimeData>

//------------------------------------------------------------------------------
qMRMLSceneSubjectHierarchyModelPrivate::qMRMLSceneSubjectHierarchyModelPrivate(qMRMLSceneSubjectHierarchyModel& object)
: Superclass(object)
{
  this->NodeTypeColumn = -1;

  this->UnknownIcon = QIcon(":Icons/Unknown.png");
  this->WarningIcon = QIcon(":Icons/Warning.png");
}

//------------------------------------------------------------------------------
void qMRMLSceneSubjectHierarchyModelPrivate::init()
{
  Q_Q(qMRMLSceneSubjectHierarchyModel);
  this->Superclass::init();

  q->setNameColumn(0);
  q->setNodeTypeColumn(q->nameColumn());
  q->setVisibilityColumn(1);
  q->setIDColumn(2);

  q->setHorizontalHeaderLabels(
    QStringList() << "Nodes" << "Vis" << "IDs");

  q->horizontalHeaderItem(0)->setToolTip(QObject::tr("Node name and type"));
  q->horizontalHeaderItem(1)->setToolTip(QObject::tr("Show/hide branch or node"));
  q->horizontalHeaderItem(2)->setToolTip(QObject::tr("Node ID"));

  // Set visibility icons from model to the default plugin
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setDefaultVisibilityIcons(this->VisibleIcon, this->HiddenIcon, this->PartiallyVisibleIcon);
}


//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLSceneSubjectHierarchyModel::qMRMLSceneSubjectHierarchyModel(QObject *vparent)
: Superclass(new qMRMLSceneSubjectHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLSceneSubjectHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLSceneSubjectHierarchyModel::~qMRMLSceneSubjectHierarchyModel()
{
}

//------------------------------------------------------------------------------
QStringList qMRMLSceneSubjectHierarchyModel::mimeTypes()const
{
  QStringList types;
  types << "application/vnd.text.list";
  return types;
}

//------------------------------------------------------------------------------
QMimeData* qMRMLSceneSubjectHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
  Q_D(const qMRMLSceneSubjectHierarchyModel);

  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (const QModelIndex &index, indexes)
  {
    // Only add one pointer per row
    if (index.isValid() && index.column() == 0)
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
bool qMRMLSceneSubjectHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( hnode && hnode->IsA("vtkMRMLSubjectHierarchyNode") )
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool qMRMLSceneSubjectHierarchyModel::canBeAParent(vtkMRMLNode* node)const
{
  vtkMRMLHierarchyNode* hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( hnode && hnode->IsA("vtkMRMLSubjectHierarchyNode") )
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int qMRMLSceneSubjectHierarchyModel::nodeTypeColumn()const
{
  Q_D(const qMRMLSceneSubjectHierarchyModel);
  return d->NodeTypeColumn;
}

//------------------------------------------------------------------------------
void qMRMLSceneSubjectHierarchyModel::setNodeTypeColumn(int column)
{
  Q_D(qMRMLSceneSubjectHierarchyModel);
  d->NodeTypeColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLSceneSubjectHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLSceneSubjectHierarchyModel);
  int maxId = this->Superclass::maxColumnId();
  maxId = qMax(maxId, d->VisibilityColumn);
  maxId = qMax(maxId, d->NodeTypeColumn);
  maxId = qMax(maxId, d->NameColumn);
  maxId = qMax(maxId, d->IDColumn);
  return maxId;
}

//------------------------------------------------------------------------------
void qMRMLSceneSubjectHierarchyModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLSceneSubjectHierarchyModel);

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
  if (!subjectHierarchyNode)
  {
    return;
  }
  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyNode(subjectHierarchyNode);
  if (!ownerPlugin)
  {
    // Set warning icon if the column is the node type column
    if (column == this->nodeTypeColumn())
    {
      item->setIcon(d->WarningIcon);
    }

    qCritical() << "qMRMLSceneSubjectHierarchyModel::updateItemDataFromNode: No owner plugin defined for subject hierarchy node '" << subjectHierarchyNode->GetName() << "'!";
    return;
  }

  vtkMRMLNode* associatedNode = subjectHierarchyNode->GetAssociatedDataNode();

  // Name column
  if (column == this->nameColumn())
  {
    QString nodeText(subjectHierarchyNode->GetName());
    if (nodeText.endsWith(QString(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX.c_str())))
    {
      nodeText = nodeText.left( nodeText.size() - vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX.size() );
    }
    item->setText(nodeText);

    // Have owner plugin set the tooltip
    ownerPlugin->setTooltip(subjectHierarchyNode, item);
  }
  // ID column
  if (column == this->idColumn())
  {
    item->setText(QString(subjectHierarchyNode->GetID()));
  }
  // Visibility column
  if (column == this->visibilityColumn())
  {
    // Have owner plugin set the visibility icon
    ownerPlugin->setVisibilityIcon(subjectHierarchyNode, item);
  }
  // Node type column
  if (column == this->nodeTypeColumn())
  {
    // Have owner plugin set the icon
    bool iconSetSuccessfullyByPlugin = ownerPlugin->setIcon(subjectHierarchyNode, item);
    if (!iconSetSuccessfullyByPlugin)
    {
      item->setIcon(d->UnknownIcon);
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLSceneSubjectHierarchyModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  Q_D(qMRMLSceneSubjectHierarchyModel);

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
  if (!subjectHierarchyNode)
  {
    qCritical() << "qMRMLSceneSubjectHierarchyModel::updateNodeFromItemData: Invalid node in subject hierarchy tree! Nodes must all be subject hierarchy nodes";
    return;
  }
  qSlicerSubjectHierarchyAbstractPlugin* ownerPlugin =
    qSlicerSubjectHierarchyPluginHandler::instance()->getOwnerPluginForSubjectHierarchyNode(subjectHierarchyNode);

  if ( item->column() == this->nameColumn() )
  {
    subjectHierarchyNode->SetName(item->text().append(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX.c_str()).toLatin1().constData());
  }
  if ( item->column() == this->visibilityColumn()
    && !item->data(VisibilityRole).isNull() )
  {
    int visible = item->data(VisibilityRole).toInt();
    if (visible > -1)
    {
      // Have owner plugin set the display visibility
      ownerPlugin->setDisplayVisibility(subjectHierarchyNode, visible);
    }
  }
}

//------------------------------------------------------------------------------
bool qMRMLSceneSubjectHierarchyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  Q_D(const qMRMLSceneSubjectHierarchyModel);
  Q_UNUSED(row);
  Q_UNUSED(column);

  if (action == Qt::IgnoreAction)
  {
    return true;
  }
  if (!this->mrmlScene())
  {
    std::cerr << "qMRMLSceneSubjectHierarchyModel::dropMimeData: Invalid MRML scene!" << std::endl;
    return false;
  }
  if (!data->hasFormat("application/vnd.text.list"))
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::dropMimeData: Plain text MIME type is expected");
    return false;
  }

  // Nothing can be dropped to the top level (patients can only be loaded at from the DICOM browser or created manually)
  if (!parent.isValid())
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::dropMimeData: Items cannot be dropped on top level!");
    return false;
  }
  vtkMRMLNode* parentNode = this->mrmlNodeFromIndex(parent);
  if (!parentNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::dropMimeData: Unable to get parent node!");
    return false;
  }

  // Decode MIME data
  QByteArray encodedData = data->data("application/vnd.text.list");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  QStringList streamItems;
  int rows = 0;

  while (!stream.atEnd())
  {
    QString text;
    stream >> text;
    streamItems << text;
    ++rows;
  }

  if (rows == 0)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::dropMimeData: Unable to decode dropped MIME data!");
    return false;
  }
  if (rows > 1)
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::dropMimeData: More than one data item decoded from dropped MIME data! Only the first one will be used.");
  }

  QString nodePointerString = streamItems[0];

  vtkMRMLNode* droppedNode = vtkMRMLNode::SafeDownCast(reinterpret_cast<vtkObject*>(nodePointerString.toULongLong()));
  if (!droppedNode)
  {
    vtkErrorWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::dropMimeData: Unable to get MRML node from dropped MIME text (" << nodePointerString.toLatin1().constData() << ")!");
    return false;
  }

  //TODO: Move this in the reparent function and handle disappearing nodes there (#473)
  d->DraggedNodes.clear();

  // Reparent the node
  return this->reparent(droppedNode, parentNode);
}

//------------------------------------------------------------------------------
bool qMRMLSceneSubjectHierarchyModel::reparent(vtkMRMLNode* node, vtkMRMLNode* newParent)
{
  if (!node || this->parentNode(node) == newParent)
  {
    return false;
  }
  Q_ASSERT(newParent != node);

  if (!this->mrmlScene())
  {
    std::cerr << "qMRMLSceneSubjectHierarchyModel::reparent: Invalid MRML scene!" << std::endl;
    return false;
  }

  vtkMRMLSubjectHierarchyNode* parentSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(newParent);
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);

  if (!this->canBeAParent(newParent))
  {
    vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::reparent: Target parent node (" << newParent->GetName() << ") is not a valid subject hierarchy parent node!");
  }

  // Prevent collapse of the subject hierarchy tree view (TODO: This is a workaround)
  emit saveTreeExpandState();

  // If dropped from within the subject hierarchy tree
  if (subjectHierarchyNode)
  {
    bool successfullyReparentedByPlugin = false;
    QList<qSlicerSubjectHierarchyAbstractPlugin*> foundPlugins =
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginsForReparentingInsideSubjectHierarchyForNode(subjectHierarchyNode, parentSubjectHierarchyNode);
    qSlicerSubjectHierarchyAbstractPlugin* selectedPlugin = NULL;
    if (foundPlugins.size() > 1)
    {
      // Let the user choose a plugin if more than one returned the same non-zero confidence value
      vtkMRMLNode* associatedNode = (subjectHierarchyNode->GetAssociatedDataNode() ? subjectHierarchyNode->GetAssociatedDataNode() : subjectHierarchyNode);
      QString textToDisplay = QString("Equal confidence number found for more than one subject hierarchy plugin for reparenting.\n\nSelect plugin to reparent node named\n'%1'\n(type %2)\nParent node: %3").arg(associatedNode->GetName()).arg(associatedNode->GetNodeTagName()).arg(parentSubjectHierarchyNode->GetName());
      selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->selectPluginFromDialog(textToDisplay, foundPlugins);
    }
    else if (foundPlugins.size() == 1)
    {
      selectedPlugin = foundPlugins[0];
    }
    else
    {
      // Choose default plugin if all registered plugins returned confidence value 0
      selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin();
    }

    // Have the selected plugin reparent the node
    successfullyReparentedByPlugin = selectedPlugin->reparentNodeInsideSubjectHierarchy(subjectHierarchyNode, parentSubjectHierarchyNode);
    if (!successfullyReparentedByPlugin)
    {
      // TODO: Does this cause #473?
      // Put back to its original place
      subjectHierarchyNode->SetParentNodeID( subjectHierarchyNode->GetParentNodeID() );

      vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::reparent: Failed to reparent node "
        << subjectHierarchyNode->GetName() << " through plugin '" << selectedPlugin->name().toLatin1().constData() << "'");
    }
  }
  // If dropped from the potential subject hierarchy nodes list
  else
  {
    // If there is a plugin that can handle the dropped node then let it take care of it
    bool successfullyAddedByPlugin = false;
    QList<qSlicerSubjectHierarchyAbstractPlugin*> foundPlugins =
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginsForAddingToSubjectHierarchyForNode(node, parentSubjectHierarchyNode);
    qSlicerSubjectHierarchyAbstractPlugin* selectedPlugin = NULL;
    if (foundPlugins.size() > 1)
    {
      // Let the user choose a plugin if more than one returned the same non-zero confidence value
      QString textToDisplay = QString("Equal confidence number found for more than one subject hierarchy plugin for adding potential node to subject hierarchy.\n\nSelect plugin to add node named\n'%1'\n(type %2)\nParent node: %3").arg(node->GetName()).arg(node->GetNodeTagName()).arg(parentSubjectHierarchyNode->GetName());
      selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->selectPluginFromDialog(textToDisplay, foundPlugins);
    }
    else if (foundPlugins.size() == 1)
    {
      selectedPlugin = foundPlugins[0];
    }
    else
    {
      // Choose default plugin if all registered plugins returned confidence value 0
      selectedPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin();
    }

    // Have the selected plugin add the potential node to subject hierarchy
    successfullyAddedByPlugin = selectedPlugin->addNodeToSubjectHierarchy(node, parentSubjectHierarchyNode);
    if (!successfullyAddedByPlugin)
    {
      vtkWarningWithObjectMacro(this->mrmlScene(), "qMRMLSceneSubjectHierarchyModel::reparent: Failed to add node "
        << node->GetName() << " through plugin '" << selectedPlugin->name().toLatin1().constData() << "'");
    }
  }

  // Update scene in related widgets (such as potential nodes list)
  emit updateRelatedWidgets();

  // Force updating the whole scene (TODO: this should not be needed)
  this->updateScene();

  // Prevent collapse of the subject hierarchy tree view (TODO: This is a workaround)
  emit loadTreeExpandState();

  return true;
}

//------------------------------------------------------------------------------
void qMRMLSceneSubjectHierarchyModel::forceUpdateScene()
{
  // Force updating the whole scene (TODO: this should not be needed)
  this->updateScene();
}
