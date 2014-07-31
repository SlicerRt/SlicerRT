/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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
#include "vtkMRMLContourModelDisplayNode.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"

// Qt includes
#include <QAction>
#include <QDebug>
#include <QMenu>
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

  QAction* RepresentationVisibilityAction;
  QAction* RibbonModelVisibilityAction;
  QAction* LabelmapVisibilityAction;
  QAction* ClosedSurfaceVisibilityAction;

  QAction* CreateContourAction;

  QAction* ConvertContourToRepresentationAction;
  QAction* CreateRibbonModelAction;
  QAction* CreateLabelmapAction;
  QAction* CreateClosedSurfaceAction;

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
  this->CreateRibbonModelAction = NULL;
  this->CreateLabelmapAction = NULL;
  this->CreateClosedSurfaceAction = NULL;
  this->ChangeColorAction = NULL;
  this->RepresentationVisibilityAction = NULL;
  this->RibbonModelVisibilityAction = NULL;
  this->LabelmapVisibilityAction = NULL;
  this->ClosedSurfaceVisibilityAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyContoursPlugin);

  // Show/hide actions
  this->RepresentationVisibilityAction = new QAction("Show/Hide Representation", q);
  QMenu* representationTypeSubMenu = new QMenu();
  this->RepresentationVisibilityAction->setMenu(representationTypeSubMenu);

  this->RibbonModelVisibilityAction = new QAction("Ribbon model",q);
  QObject::connect(this->RibbonModelVisibilityAction, SIGNAL(triggered()), q, SLOT(hideShowRibbonModel()));
  representationTypeSubMenu->addAction(this->RibbonModelVisibilityAction);

  this->LabelmapVisibilityAction = new QAction("Labelmap",q);
  QObject::connect(this->LabelmapVisibilityAction, SIGNAL(triggered()), q, SLOT(hideShowLabelmap()));
  representationTypeSubMenu->addAction(this->LabelmapVisibilityAction);

  this->ClosedSurfaceVisibilityAction = new QAction("Closed surface",q);
  QObject::connect(this->ClosedSurfaceVisibilityAction, SIGNAL(triggered()), q, SLOT(hideShowClosedSurfaceModel()));
  representationTypeSubMenu->addAction(this->ClosedSurfaceVisibilityAction);

  // Create contour action
  this->CreateContourAction = new QAction("Create child contour",q);
  QObject::connect(this->CreateContourAction, SIGNAL(triggered()), q, SLOT(createChildContourForCurrentNode()));

  // Convert to representation action
  this->ConvertContourToRepresentationAction = new QAction("Convert to representation",q);
  QMenu* representationCreateSubMenu = new QMenu();
  this->ConvertContourToRepresentationAction->setMenu(representationCreateSubMenu);

  this->CreateRibbonModelAction = new QAction("Ribbon model",q);
  QObject::connect(this->CreateRibbonModelAction, SIGNAL(triggered()), q, SLOT(createRibbonModelRepresentation()));
  representationCreateSubMenu->addAction(this->CreateRibbonModelAction);

  this->CreateLabelmapAction = new QAction("Labelmap",q);
  QObject::connect(this->CreateLabelmapAction, SIGNAL(triggered()), q, SLOT(createLabelmapRepresentation()));
  representationCreateSubMenu->addAction(this->CreateLabelmapAction);

  this->CreateClosedSurfaceAction = new QAction("Closed surface",q);
  QObject::connect(this->CreateClosedSurfaceAction, SIGNAL(triggered()), q, SLOT(createClosedSurfaceModelRepresentation()));
  representationCreateSubMenu->addAction(this->CreateClosedSurfaceAction);

  // Change color action
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
double qSlicerSubjectHierarchyContoursPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

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
const QString qSlicerSubjectHierarchyContoursPlugin::helpText()const
{
  return QString("<p style=\" margin-top:4px; margin-bottom:1px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; font-weight:600; color:#000000;\">Create contour</span></p>\n"
    "<p style=\" margin-top:0px; margin-bottom:1px; margin-left:26px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; color:#000000;\">In Subject Hierarchy module drag&amp;drop new contour from 'Potential subject hierarchy nodes' list to under a contour set in the tree</span></p>\n"
    "<p style=\" margin-top:0px; margin-bottom:1px; margin-left:26px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; color:#000000;\">In Contours module use 'Create contour from representation' panel</span></p>\n"
    "<p style=\" margin-top:0px; margin-bottom:11px; margin-left:26px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; color:#000000;\">The default representation is an empty ribbon model. It can be converted to indexed labelmap in the Contours module and then the labelmap can be edited in the Editor module</span></p>");
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyContoursPlugin::icon(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::icon: NULL node given!";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchyContoursPlugin);

  if (this->canOwnSubjectHierarchyNode(node))
  {
    return d->ContourIcon;
  }

  // Node unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchyContoursPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyContoursPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyContoursPlugin);

  QList<QAction*> actions;
  actions << d->CreateContourAction << d->ConvertContourToRepresentationAction << d->ChangeColorAction << d->RepresentationVisibilityAction;
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

  vtkMRMLNode* associatedNode = node->GetAssociatedNode();

  // Contour (owned)
  if ( this->canOwnSubjectHierarchyNode(node) && this->isThisPluginOwnerOfNode(node) )
  {
    d->ChangeColorAction->setVisible(true);
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(associatedNode);
    d->RepresentationVisibilityAction->setVisible(contourNode != NULL);
    if( contourNode )
    {
      d->RibbonModelVisibilityAction->setVisible(contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel));
      d->LabelmapVisibilityAction->setVisible(contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap));
      d->ClosedSurfaceVisibilityAction->setVisible(contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel));
    }
    else
    {
      d->RibbonModelVisibilityAction->setVisible(false);
      d->LabelmapVisibilityAction->setVisible(false);
      d->ClosedSurfaceVisibilityAction->setVisible(false);
    }

    bool allRepresentations = contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) 
      && contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap)
      && contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);

    d->ConvertContourToRepresentationAction->setVisible(contourNode != NULL && !allRepresentations);
    if( contourNode )
    {
      d->CreateRibbonModelAction->setVisible(!contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel));
      d->CreateLabelmapAction->setVisible(!contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap));
      d->CreateClosedSurfaceAction->setVisible(!contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel));
    }
    else
    {
      d->CreateRibbonModelAction->setVisible(false);
      d->CreateLabelmapAction->setVisible(false);
      d->CreateClosedSurfaceAction->setVisible(false);
    }
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

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::hideShowRibbonModel()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(currentNode->GetAssociatedNode());
  if( contourNode && contourNode->GetRibbonModelDisplayNode() )
  {
    contourNode->GetRibbonModelDisplayNode()->SetVisibility( (1+contourNode->GetRibbonModelDisplayNode()->GetVisibility())%2 );
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::hideShowLabelmap()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(currentNode->GetAssociatedNode());
  if( contourNode )
  {
    // TODO : 2d vis readdition
    //contourNode->GetRibbonModelDisplayNode()->SetVisibility( (1+contourNode->GetRibbonModelDisplayNode()->GetVisibility())%2 );
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::hideShowClosedSurfaceModel()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(currentNode->GetAssociatedNode());
  if( contourNode && contourNode->GetClosedSurfaceModelDisplayNode() )
  {
    contourNode->GetClosedSurfaceModelDisplayNode()->SetVisibility( (1+contourNode->GetClosedSurfaceModelDisplayNode()->GetVisibility())%2 );
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::createRibbonModelRepresentation()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(currentNode->GetAssociatedNode());
  if( contourNode ) 
  {
    contourNode->GetRibbonModelPolyData();
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::createLabelmapRepresentation()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(currentNode->GetAssociatedNode());
  if( contourNode && contourNode->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) != NULL )
  {
    contourNode->GetLabelmapImageData();
  }
  else
  {
    qCritical() << "Reference volume not set for contour: " << contourNode->GetID() << ". Unable to convert to labelmap. Please use Contours module to convert.";
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::createClosedSurfaceModelRepresentation()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(currentNode->GetAssociatedNode());
  if( contourNode && 
    // Either we have the image data, or we have a reference volume to create the image data
    (contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap)
    || contourNode->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) != NULL) )
  {
    contourNode->GetClosedSurfacePolyData();
  }
  else
  {
    qCritical() << "Image data or reference volume not set for contour: " << contourNode->GetID() << ". Unable to convert to closed surface. Please use Contours module to convert.";
  }
}
