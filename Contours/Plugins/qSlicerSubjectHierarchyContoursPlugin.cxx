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

// SlicerRt includes
#include "SlicerRtCommon.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Contours includes
#include "qSlicerSubjectHierarchyContoursPlugin.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QStandardItem>

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLColorTableNode.h>

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>
#include <vtkLookupTable.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Contours
class qSlicerSubjectHierarchyContoursPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyContoursPlugin);
protected:
  qSlicerSubjectHierarchyContoursPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyContoursPluginPrivate(qSlicerSubjectHierarchyContoursPlugin& object);
  ~qSlicerSubjectHierarchyContoursPluginPrivate();
  void init();
public:
  QIcon ContourIcon;

  QAction* CreateContourAction;
  QAction* ConvertContourToRepresentationAction;
  QAction* ChangeColorAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyContoursPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContoursPluginPrivate::qSlicerSubjectHierarchyContoursPluginPrivate(qSlicerSubjectHierarchyContoursPlugin& object)
: q_ptr(&object)
{
  this->ContourIcon = QIcon(":Icons/Contour.png");

  this->CreateContourAction = NULL;
  this->ConvertContourToRepresentationAction = NULL;
  this->ChangeColorAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyContoursPlugin);

  this->CreateContourAction = new QAction("Create child contour",q);
  QObject::connect(this->CreateContourAction, SIGNAL(triggered()), q, SLOT(createChildContourForCurrentNode()));

  this->ConvertContourToRepresentationAction = new QAction("Convert contour to its active representation",q);
  QObject::connect(this->ConvertContourToRepresentationAction, SIGNAL(triggered()), q, SLOT(convertCurrentNodeContourToRepresentation()));
  this->ConvertContourToRepresentationAction->setEnabled(false); //TODO Remove when the feature is added

  this->ChangeColorAction = new QAction("Change color...",q);
  QObject::connect(this->ChangeColorAction, SIGNAL(triggered()), q, SLOT(changeColorForCurrentNode()));

  QObject::connect(q, SIGNAL(ownerPluginChanged(vtkObject*,void*)), q, SLOT(onNodeClaimed(vtkObject*,void*)));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContoursPluginPrivate::~qSlicerSubjectHierarchyContoursPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContoursPlugin::qSlicerSubjectHierarchyContoursPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyContoursPluginPrivate(*this) )
{
  this->m_Name = QString("Contours");

  Q_D(qSlicerSubjectHierarchyContoursPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContoursPlugin::~qSlicerSubjectHierarchyContoursPlugin()
{
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyContoursPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Contour
  if ( associatedNode && associatedNode->IsA("vtkMRMLContourNode") )
  {
    return 1.0; // Only the Contours plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyContoursPlugin::roleForPlugin()const
{
  return "Contour";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyContoursPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyContoursPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    item->setIcon(d->ContourIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  // Have the default plugin (which is not registered) take care of this
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyContoursPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyContoursPlugin);

  QList<QAction*> actions;
  actions << d->CreateContourAction << d->ConvertContourToRepresentationAction << d->ChangeColorAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyContoursPlugin);
  this->hideAllContextMenuActions();

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Contour (owned)
  if ( this->canOwnSubjectHierarchyNode(node) && this->isThisPluginOwnerOfNode(node) )
  {
    d->ConvertContourToRepresentationAction->setVisible(true);
    d->ChangeColorAction->setVisible(true);
  }

  // Contour set
  if ( node->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    d->CreateContourAction->setVisible(true);
  }
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::createChildContourForCurrentNode()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Check if current (to-be parent) node is a contour set node
  if (!currentNode || !currentNode->IsLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    || !currentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::createChildContourForCurrentNode: Invalid current node for creating contour!";
    return;
  }
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::createChildContourForCurrentNode: Invalid MRML scene!";
    return;
  }

  // Create new empty contour node
  vtkSmartPointer<vtkMRMLContourNode> newContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  scene->AddNode(newContourNode);
  std::string newContourNodeName = SlicerRtCommon::CONTOUR_NEW_CONTOUR_NAME;
  newContourNodeName = scene->GenerateUniqueName(newContourNodeName);
  newContourNode->SetName(newContourNodeName.c_str());

  // Add it to subject hierarchy
  bool success = this->addNodeToSubjectHierarchy(newContourNode, currentNode);
  if (!success)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::createChildContourForCurrentNode: Failed to add new contour to subject hierarchy!";
    return;
  }
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::convertCurrentNodeContourToRepresentation()
{
  //TODO:
  qCritical() << "qSlicerSubjectHierarchyContoursPlugin::convertCurrentNodeContourToRepresentation: Not implemented yet!";
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::onNodeClaimed(vtkObject* node, void* callData)
{
  // Sample code for acquiring the name of the old plugin if necessary
  //char* oldPluginName = reinterpret_cast<char*>(callData);

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
  if (!subjectHierarchyNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::onNodeClaimed: Invalid subject hierarchy node!";
    return;
  }

  // Only force level if this is the new owner plugin
  if (!this->m_Name.compare(subjectHierarchyNode->GetOwnerPluginName()))
  {
    subjectHierarchyNode->SetLevel(vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);
    qDebug() << "qSlicerSubjectHierarchyContoursPlugin::onNodeClaimed: Level of node " << subjectHierarchyNode->GetName() << " changed to subseries on owner plugin change to Contours";
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::editProperties(vtkMRMLSubjectHierarchyNode* node)
{
  // Have the contour sets plugin do this (switch to Contours and set node as selected)
  qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("ContourSets")->editProperties(node);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::changeColorForCurrentNode()
{
  // Get color node for current contour
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!this->canOwnSubjectHierarchyNode(currentNode))
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::changeColorForCurrentNode: Current node is not a contour node!";
    return;
  }
  vtkMRMLColorTableNode* colorNode = vtkMRMLColorTableNode::SafeDownCast(
    currentNode->GetParentNode()->GetNodeReference(
    SlicerRtCommon::CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE.c_str()) );
  if (!colorNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::changeColorForCurrentNode: No color table found for contour " << currentNode->GetName() << " !";
    return;
  }

  // Switch to Colors module and set color table as current color node
  qSlicerAbstractModuleWidget* moduleWidget = qSlicerSubjectHierarchyAbstractPlugin::switchToModule("Colors");
  if (moduleWidget)
  {
    // Get node selector combobox
    qMRMLNodeComboBox* nodeSelector = moduleWidget->findChild<qMRMLNodeComboBox*>("ColorTableComboBox");

    // Choose current data node
    if (nodeSelector)
    {
      nodeSelector->setCurrentNode(colorNode);
    }
  }
}
