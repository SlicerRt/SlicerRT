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

// SubjectHierarchy includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "qSlicerSubjectHierarchyPluginHandler.h"

// Qt includes
#include <QDebug>
#include <QStandardItem>
#include <QAction>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyAbstractPlugin::qSlicerSubjectHierarchyAbstractPlugin(QObject *parent)
  : Superclass(parent)
  , m_Name(QString())
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyAbstractPlugin::~qSlicerSubjectHierarchyAbstractPlugin()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchyAbstractPlugin::name()const
{
  if (m_Name.isEmpty())
  {
    qCritical() << "qSlicerSubjectHierarchyAbstractPlugin::name: Empty plugin name!";
  }
  return this->m_Name;
}

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyAbstractPlugin::dependencies()const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyAbstractPlugin::exportNode(vtkMRMLSubjectHierarchyNode* node)
{
  qCritical() << "qSlicerSubjectHierarchyAbstractPlugin::exportNode: This plugin ("
    << this->m_Name << ") does not support exporting!";
}

//-----------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyAbstractPlugin::nodeContextMenuActions()const
{
  return QList<QAction*>();
}

//-----------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyAbstractPlugin::sceneContextMenuActions()const
{
  return QList<QAction*>();
}

//----------------------------------------------------------------------------
bool qSlicerSubjectHierarchyAbstractPlugin::addNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parentNode)
{
  if (!nodeToAdd || !parentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyAbstractPlugin::addNodeToSubjectHierarchy: Invalid node to add or parent node!";
    return false;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyAbstractPlugin::addNodeToSubjectHierarchy: Invalid MRML scene!";
    return false;
  }

  // Associate to a new hierarchy node and put it in the tree under the parent
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> subjectHierarchyNode =
    vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
  subjectHierarchyNode->SetAssociatedNodeID(nodeToAdd->GetID());
  std::string shNodeName = std::string(nodeToAdd->GetName()) + vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_NODE_NAME_POSTFIX;
  shNodeName = scene->GenerateUniqueName(shNodeName);
  subjectHierarchyNode->SetName(shNodeName.c_str());
  subjectHierarchyNode->SetLevel( this->childLevel(parentNode->GetLevel()).toLatin1().constData() );
  //TODO: UID?
  scene->AddNode(subjectHierarchyNode);
  subjectHierarchyNode->SetParentNodeID(parentNode->GetID());

  return true;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyAbstractPlugin::reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parentNode)
{
  nodeToReparent->SetParentNodeID(parentNode->GetID());
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyAbstractPlugin::setTooltip(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyAbstractPlugin::setTooltip: Subject hierarchy node is NULL!";
    item->setToolTip("Invalid!");
  }

  // Display node type and level in the tooltip
  QString tooltipString("");
  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (associatedNode)
  {
    tooltipString.append(associatedNode->GetNodeTagName());
    tooltipString.append(" (");
  }

  tooltipString.append("Level:");
  tooltipString.append(node->GetLevel());
  tooltipString.append(" Plugin:");
  tooltipString.append(node->GetOwnerPluginName() ? node->GetOwnerPluginName() : "None");

  if (associatedNode)
  {
    tooltipString.append(")");
  }

  item->setToolTip(tooltipString);
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyAbstractPlugin::setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible)
{
  // Default behavior is to call SetDisplayVisibility on all displayable
  // associated nodes in the whole branch
  node->SetDisplayVisibilityForBranch(visible);
}

//-----------------------------------------------------------------------------
int qSlicerSubjectHierarchyAbstractPlugin::getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)
{
  return node->GetDisplayVisibilityForBranch();
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchyAbstractPlugin::childLevel(QString parentLevel)
{
  // Get child level from this plugin
  QString childLevel;
  if (this->m_ChildLevelMap.contains(parentLevel))
  {
    childLevel = this->m_ChildLevelMap[parentLevel];
  }
  // If this plugin does not have child level for this parent level, then log a warning
  else
  {
    qCritical() << "qSlicerSubjectHierarchyAbstractPlugin::childLevel: Could not get child level for level '"
      << parentLevel << "'!";
    return QString();
  }

  return childLevel;
}
