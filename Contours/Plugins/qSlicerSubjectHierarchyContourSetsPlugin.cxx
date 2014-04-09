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
#include "vtkSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Contours includes
#include "qSlicerSubjectHierarchyContourSetsPlugin.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QStandardItem>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerModuleManager.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>
#include <vtkLookupTable.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Contours
class qSlicerSubjectHierarchyContourSetsPluginPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyContourSetsPlugin);
protected:
  qSlicerSubjectHierarchyContourSetsPlugin* const q_ptr;
public:
  qSlicerSubjectHierarchyContourSetsPluginPrivate(qSlicerSubjectHierarchyContourSetsPlugin& object);
  ~qSlicerSubjectHierarchyContourSetsPluginPrivate();
  void init();
public:
  QIcon ContourSetIcon;

  QAction* CreateContourSetNodeAction;
  QAction* ConvertRepresentationIntoContourAction;
  QAction* EditColorTableAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyContourSetsPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContourSetsPluginPrivate::qSlicerSubjectHierarchyContourSetsPluginPrivate(qSlicerSubjectHierarchyContourSetsPlugin& object)
: q_ptr(&object)
, ContourSetIcon(QIcon(":Icons/ContourSet.png"))
, CreateContourSetNodeAction(NULL)
, ConvertRepresentationIntoContourAction(NULL)
, EditColorTableAction(NULL)
{
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyContourSetsPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyContourSetsPlugin);

  this->CreateContourSetNodeAction = new QAction("Create child contour set",q);
  QObject::connect(this->CreateContourSetNodeAction, SIGNAL(triggered()), q, SLOT(createChildContourSetForCurrentNode()));

  this->ConvertRepresentationIntoContourAction = new QAction("Create child contour from representation...",q);
  QObject::connect(this->ConvertRepresentationIntoContourAction, SIGNAL(triggered()), q, SLOT(convertRepresentationAction()));

  this->EditColorTableAction = new QAction("Edit contour set color table...",q);
  QObject::connect(this->EditColorTableAction, SIGNAL(triggered()), q, SLOT(onEditColorTable()));
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContourSetsPluginPrivate::~qSlicerSubjectHierarchyContourSetsPluginPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContourSetsPlugin::qSlicerSubjectHierarchyContourSetsPlugin(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSubjectHierarchyContourSetsPluginPrivate(*this) )
{
  this->m_Name = QString("ContourSets");

  Q_D(qSlicerSubjectHierarchyContourSetsPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContourSetsPlugin::~qSlicerSubjectHierarchyContourSetsPlugin()
{
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyContourSetsPlugin::canAddNodeToSubjectHierarchy(vtkMRMLNode* node, vtkMRMLSubjectHierarchyNode* parent/*=NULL*/)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::canAddNodeToSubjectHierarchy: Input node is NULL!";
    return 0.0;
  }
  else if (node->IsA("vtkMRMLContourNode"))
  {
    // Node is a contour
    return 1.0;
  }

  // Cannot add if new parent is not a contour set node
  // Do not examine parent if the pointer is NULL. In that case the parent is ignored, the confidence numbers are got based on the to-be child node alone.
  if (parent && !parent->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()))
  {
    return 0.0;
  }

  if (node->IsA("vtkMRMLModelNode") || node->IsA("vtkMRMLScalarVolumeNode"))
  {
    if (this->isNodeAContourRepresentation(node))
    {
      // Node is a representation of an existing contour, so instead of the representation, the parent contour will only be the potential node
      return 0.0;
    }
    else
    {
      // Node is a potential contour node representation. On reparenting under a contour set node in subject hierarchy, a contour node will be created
      if ( parent && parent->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
        && parent->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
      {
        return 1.0; // Only the Contours plugin can handle this add operation
      }

      return 0.7; // Might be other plugin that can handle adding volumes and models in the subject hierarchy
    }
  }

  return 0.0;
}

//----------------------------------------------------------------------------
bool qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parentNode)
{
  if (!nodeToAdd || !parentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Invalid argument!";
    return false;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Invalid MRML scene!";
    return false;
  }
  if (!scene || scene != parentNode->GetScene())
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Invalid MRML scene!";
    return false;
  }

  if (! (parentNode->IsA("vtkMRMLSubjectHierarchyNode") && parentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str())) )
  {
    qDebug() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Parent node must be a contour hierarchy node! Cancelling adding to subject hierarchy.";
    return false;
  }

  // Create hierarchy node for contour node
  vtkMRMLSubjectHierarchyNode* contourSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    scene, parentNode, vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES, nodeToAdd->GetName());

  QString colorName("");

  // Get added contour node and associate it with the new subject hierarchy node
  vtkMRMLContourNode* contourNodeAddedToSubjectHierarchy = NULL;
  if (nodeToAdd->IsA("vtkMRMLModelNode") || nodeToAdd->IsA("vtkMRMLScalarVolumeNode"))
  {
    // Create contour from the dropped representation
    contourNodeAddedToSubjectHierarchy = vtkSlicerContoursModuleLogic::CreateContourFromRepresentation( vtkMRMLDisplayableNode::SafeDownCast(nodeToAdd) );
    if (!contourNodeAddedToSubjectHierarchy)
    {
      qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Failed to create contour from representation '" << nodeToAdd->GetName() << "'";
      return false;
    }

    colorName = QString(nodeToAdd->GetName());
  }
  else if (nodeToAdd->IsA("vtkMRMLContourNode"))
  {
    contourNodeAddedToSubjectHierarchy = vtkMRMLContourNode::SafeDownCast(nodeToAdd);
    colorName = QString(nodeToAdd->GetName());
  }
  else
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Invalid node type to add!";
    return false;
  }

  contourSubjectHierarchyNode->SetAssociatedNodeID(contourNodeAddedToSubjectHierarchy->GetID());

  // Add color to the new color table
  if (!this->addContourColorToCorrespondingColorTable(contourNodeAddedToSubjectHierarchy, colorName))
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addNodeToSubjectHierarchy: Failed to add contour color to corresponding color table!";
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyContourSetsPlugin::canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::canReparentNodeInsideSubjectHierarchy: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (!associatedNode)
  {
    return 0.0;
  }
  
  // Cannot reparent if new parent is not a contour set node
  // Do not examine parent if the pointer is NULL. In that case the parent is ignored, the confidence numbers are got based on the to-be child node alone.
  if (parent && !parent->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()))
  {
    return 0.0;
  }

  if ( associatedNode && associatedNode->IsA("vtkMRMLContourNode") )
  {
    // Node is a contour
    return 1.0;
  }

  if (associatedNode->IsA("vtkMRMLModelNode") || associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    // Node is a potential contour node representation. On reparenting under a contour set node in subject hierarchy, a contour node will be created
    if ( parent->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
      && parent->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
    {
      return 1.0; // Only the Contours plugin can handle this reparent operation
    }
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parentNode)
{
  if (!nodeToReparent || !parentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: Invalid argument!";
    return false;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: Invalid MRML scene!";
    return false;
  }
  if (!scene || scene != parentNode->GetScene())
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: Invalid MRML scene!";
    return false;
  }

  if (! (parentNode->IsA("vtkMRMLSubjectHierarchyNode") && parentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str())) )
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: New parent is not a contour hierarchy node! Cancelling reparenting.";
    return false;
  }

  vtkMRMLNode* associatedNode = nodeToReparent->GetAssociatedDataNode();
  vtkMRMLContourNode* reparentedContourNode = NULL;
  QString colorName("");

  // Get added contour node and associate it with the new subject hierarchy node
  if (associatedNode->IsA("vtkMRMLModelNode") || associatedNode->IsA("vtkMRMLScalarVolumeNode"))
  {
    // Create contour from the dropped representation
    reparentedContourNode = vtkSlicerContoursModuleLogic::CreateContourFromRepresentation( vtkMRMLDisplayableNode::SafeDownCast(associatedNode) );
    if (!reparentedContourNode)
    {
      qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: Failed to create contour from representation '" << associatedNode->GetName() << "'";
      return false;
    }

    // Replace the reparented contour representation with the created contour itself and do the reparenting
    nodeToReparent->SetAssociatedNodeID(reparentedContourNode->GetID());
    nodeToReparent->SetParentNodeID(parentNode->GetID());
    nodeToReparent->SetLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES);
    nodeToReparent->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), associatedNode->GetName() );

    colorName = QString(associatedNode->GetName());
  }
  else if (associatedNode->IsA("vtkMRMLContourNode"))
  {
    reparentedContourNode = vtkMRMLContourNode::SafeDownCast(associatedNode);

    // Get color index and the color table that has been associated to the contour node before reparenting
    vtkMRMLColorTableNode* oldColorNode = NULL;
    int oldStructureColorIndex = -1;
    reparentedContourNode->GetColor(oldStructureColorIndex, oldColorNode);
    if (oldColorNode)
    {
      colorName = QString(oldColorNode->GetColorName(oldStructureColorIndex));
    }
    else
    {
      qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: There is no color node for the contour to reparent '" << reparentedContourNode->GetName() << "'!";
      return false;
    }

    // Do the reparenting
    nodeToReparent->SetParentNodeID(parentNode->GetID());

    // Remove color from the color table that has been associated to the contour node before reparenting
    double color[4];
    if (oldStructureColorIndex != SlicerRtCommon::COLOR_INDEX_INVALID)
    {
      // Save color for later insertion in the new color node
      oldColorNode->GetColor(oldStructureColorIndex, color);

      // Set old color entry to invalid
      oldColorNode->SetColor(oldStructureColorIndex, SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
        SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3]);
      oldColorNode->SetColorName(oldStructureColorIndex, SlicerRtCommon::COLOR_NAME_REMOVED);
    }
    else
    {
      qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: There was no associated color for contour '" << reparentedContourNode->GetName() << "' before reparenting!";
    }
  }
  else
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: Invalid node type to reparent!";
    return false;
  }

  // Add color to the new color table
  if (!this->addContourColorToCorrespondingColorTable(reparentedContourNode, colorName))
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::reparentNodeInsideSubjectHierarchy: Failed to add contour color to corresponding color table!";
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLContourNode* qSlicerSubjectHierarchyContourSetsPlugin::isNodeAContourRepresentation(vtkMRMLNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::isNodeAContourRepresentation: Input argument is NULL!";
    return NULL;
  }
  vtkMRMLScene* mrmlScene = node->GetScene();
  if (!mrmlScene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::isNodeAContourRepresentation: Invalid MRML scene!";
    return NULL;
  }

  // If the node is neither a model nor a volume, the it cannot be a representation
  if (!node->IsA("vtkMRMLModelNode") && !node->IsA("vtkMRMLScalarVolumeNode"))
  {
    return NULL;
  }

  // Check every contour node if they have the argument node among the representations
  const char* nodeID = node->GetID();
  vtkSmartPointer<vtkCollection> contourNodes = vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByClass("vtkMRMLContourNode") );
  vtkObject* nextObject = NULL;
  for (contourNodes->InitTraversal(); (nextObject = contourNodes->GetNextItemAsObject()); )
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(nextObject);
    if ( (contourNode->GetRibbonModelNodeId() && !STRCASECMP(contourNode->GetRibbonModelNodeId(), nodeID))
      || (contourNode->GetIndexedLabelmapVolumeNodeId() && !STRCASECMP(contourNode->GetIndexedLabelmapVolumeNodeId(), nodeID))
      || (contourNode->GetClosedSurfaceModelNodeId() && !STRCASECMP(contourNode->GetClosedSurfaceModelNodeId(), nodeID)) )
    {
      return contourNode;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyContourSetsPlugin::addContourColorToCorrespondingColorTable(vtkMRMLContourNode* contourNode, QString colorName)
{
  if (!contourNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addContourColorToCorrespondingColorTable: Input contour node is NULL!";
    return false;
  }
  vtkMRMLScene* mrmlScene = contourNode->GetScene();
  if (!mrmlScene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addContourColorToCorrespondingColorTable: Invalid MRML scene!";
    return NULL;
  }

  // Initialize color to invalid in case it cannot be found
  double color[4] = { SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1], SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] };

  // Get contour color
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast( mrmlScene->GetNodeByID(
    contourNode->GetRibbonModelNodeId() ? contourNode->GetRibbonModelNodeId() : contourNode->GetClosedSurfaceModelNodeId()) );
  if (modelNode && modelNode->GetDisplayNode())
  {
    modelNode->GetDisplayNode()->GetColor(color[0], color[1], color[2]);
  }

  // Add color to the new color table
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = SlicerRtCommon::COLOR_INDEX_INVALID; // Initializing to this value means that we don't request the color index, just the color table
  contourNode->GetColor(structureColorIndex, colorNode);
  if (!colorNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::addContourColorToCorrespondingColorTable: There is no color node in contour set subject hierarchy node belonging to contour '" << contourNode->GetName() << "'!";
    return false;
  }

  int numberOfColors = colorNode->GetNumberOfColors();
  colorNode->SetNumberOfColors(numberOfColors+1);
  colorNode->GetLookupTable()->SetTableRange(0, numberOfColors);
  colorNode->SetColor(numberOfColors, colorName.toLatin1().constData(), color[0], color[1], color[2], color[3]);

  // Paint labelmap foreground value to match the index of the newly added color
  if (contourNode->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap))
  {
    vtkMRMLScalarVolumeNode* scalarVolumeNode = contourNode->GetIndexedLabelmapVolumeNode();

    // Make sure there is a display node with the right type
    // TODO: Use Volumes logic function when available (http://www.na-mic.org/Bug/view.php?id=3304)
    vtkMRMLDisplayNode* oldDisplayNode = scalarVolumeNode->GetDisplayNode();

    vtkMRMLVolumeDisplayNode* scalarVolumeDisplayNode = vtkMRMLLabelMapVolumeDisplayNode::New();
    scalarVolumeDisplayNode->SetAndObserveColorNodeID(colorNode->GetID());
    mrmlScene->AddNode(scalarVolumeDisplayNode);
    scalarVolumeNode->SetLabelMap(1);
    scalarVolumeNode->SetAndObserveDisplayNodeID( scalarVolumeDisplayNode->GetID() );
    scalarVolumeDisplayNode->Delete();

    if (oldDisplayNode)
    {
      mrmlScene->RemoveNode(oldDisplayNode);
    }

    // Do the painting
    vtkSlicerContoursModuleLogic::PaintLabelmapForeground(scalarVolumeNode, numberOfColors);
  }

  return true;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchyContourSetsPlugin::canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::canOwnSubjectHierarchyNode: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Contour set
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only the Contours plugin can handle this node
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchyContourSetsPlugin::roleForPlugin()const
{
  return "Contour set";
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyContourSetsPlugin::setIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  if (!node || !item)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::setIcon: NULL node or item given!";
    return false;
  }

  Q_D(qSlicerSubjectHierarchyContourSetsPlugin);

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Contour set
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    item->setIcon(d->ContourSetIcon);
    return true;
  }

  // Node unknown by plugin
  return false;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContourSetsPlugin::setVisibilityIcon(vtkMRMLSubjectHierarchyNode* node, QStandardItem* item)
{
  // Have the default plugin (which is not registered) take care of this
  qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->setVisibilityIcon(node, item);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchyContourSetsPlugin::nodeContextMenuActions()const
{
  Q_D(const qSlicerSubjectHierarchyContourSetsPlugin);

  QList<QAction*> actions;
  actions << d->CreateContourSetNodeAction << d->ConvertRepresentationIntoContourAction << d->EditColorTableAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContourSetsPlugin::showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyContourSetsPlugin);

  d->CreateContourSetNodeAction->setVisible(false);
  d->ConvertRepresentationIntoContourAction->setVisible(false);
  d->EditColorTableAction->setVisible(false);

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  // Contour set (owned)
  if ( this->canOwnSubjectHierarchyNode(node) && this->isThisPluginOwnerOfNode(node) )
  {
    d->EditColorTableAction->setVisible(true);
  }

  // Structure set only
  if (node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str() ) != NULL &&
    STRCASECMP(node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str() ), "1") == 0 )
  {
    d->ConvertRepresentationIntoContourAction->setVisible(true);
  }

  // Study
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    d->CreateContourSetNodeAction->setVisible(true);
  }
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContourSetsPlugin::createChildContourSetForCurrentNode()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Check if contour set can be created for current parent node
  if (!currentNode || !currentNode->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::createChildContourSetForCurrentNode: Invalid current node for creating contour set!";
    return;
  }
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::createChildContourSetForCurrentNode: Invalid MRML scene!";
    return;
  }

  // Create child contour set subject hierarchy node
  vtkMRMLSubjectHierarchyNode* childContourSetSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    scene, currentNode, vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES, SlicerRtCommon::CONTOURHIERARCHY_NEW_CONTOUR_SET_NAME.c_str());
  childContourSetSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Add color table node and default colors
  vtkSmartPointer<vtkMRMLColorTableNode> contourSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  scene->AddNode(contourSetColorTableNode);
  std::string contourSetColorTableNodeName;
  contourSetColorTableNodeName = SlicerRtCommon::CONTOURHIERARCHY_NEW_CONTOUR_SET_NAME +
    SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  contourSetColorTableNodeName = scene->GenerateUniqueName(contourSetColorTableNodeName);
  contourSetColorTableNode->SetName(contourSetColorTableNodeName.c_str());
  contourSetColorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  contourSetColorTableNode->SetTypeToUser();
  contourSetColorTableNode->HideFromEditorsOff();

  contourSetColorTableNode->SetNumberOfColors(2);
  contourSetColorTableNode->GetLookupTable()->SetTableRange(0,1);
  contourSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_BACKGROUND, 0.0, 0.0, 0.0, 0.0); // Black background
  contourSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_INVALID,
    SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
    SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] ); // Color indicating invalid index

  // Add reference from contour set to color table
  childContourSetSubjectHierarchyNode->SetNodeReferenceID(SlicerRtCommon::CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE.c_str(), contourSetColorTableNode->GetID());

  emit requestExpandNode(childContourSetSubjectHierarchyNode);
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContourSetsPlugin::convertRepresentationAction()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Check if contour set can be created for current parent node
  if (!currentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::convertRepresentationAction: Invalid current node!";
    return;
  }
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::convertRepresentationAction: Invalid MRML scene!";
    return;
  }

  // Switch to contours module with box expanded and structure set already chosen in drop down
  qSlicerAbstractCoreModule* module = qSlicerApplication::application()->moduleManager()->module(QString("Contours"));
  if( module != NULL )
  {
    qSlicerAbstractModule* moduleWithAction = qobject_cast<qSlicerAbstractModule*>(module);
    if (moduleWithAction)
    {
      moduleWithAction->widgetRepresentation();
    }
  }
  else
  {
    qCritical() << "Contours module not found. Unable to open it.";
  }

  emit createContourFromRepresentationClicked( QString(currentNode->GetID()) );
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContourSetsPlugin::onEditColorTable()
{
  // Get color node for current structure set
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  if (!this->canOwnSubjectHierarchyNode(currentNode))
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::onEditColorTable: Current node is not a contour set node!";
    return;
  }
  vtkMRMLColorTableNode* colorNode = vtkMRMLColorTableNode::SafeDownCast(
    currentNode->GetNodeReference(SlicerRtCommon::CONTOUR_SET_COLOR_TABLE_REFERENCE_ROLE.c_str()) );
  if (!colorNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContourSetsPlugin::onEditColorTable: No color table found for contour set " << currentNode->GetName() << " !";
    return;
  }

  // Switch to Colors module and set color table as current color node
  // TODO: Uncomment when related topic is integrated into Slicer core
  //qSlicerApplication::application()->editNode();
}
