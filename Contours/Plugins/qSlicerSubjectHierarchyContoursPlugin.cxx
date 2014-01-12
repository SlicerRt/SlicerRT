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
#include "qSlicerSubjectHierarchyContoursPlugin.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"

// Qt includes
#include <QDebug>
#include <QAction>
#include <QStandardItem>

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
  QIcon ColorTableIcon;
  QIcon ContourIcon;
  QIcon StructureSetIcon;

  QAction* CreateStructureSetNodeAction;
  QAction* CreateContourAction;
  QAction* ConvertContourToRepresentationAction;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyContoursPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyContoursPluginPrivate::qSlicerSubjectHierarchyContoursPluginPrivate(qSlicerSubjectHierarchyContoursPlugin& object)
: q_ptr(&object)
{
  this->ColorTableIcon = QIcon(":Icons/ColorTable.png");
  this->ContourIcon = QIcon(":Icons/Contour.png");
  this->StructureSetIcon = QIcon(":Icons/StructureSet.png");

  this->CreateStructureSetNodeAction = NULL;
  this->CreateContourAction = NULL;
  this->ConvertContourToRepresentationAction = NULL;
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchyContoursPlugin);

  this->CreateStructureSetNodeAction = new QAction("Create child structure set",q);
  QObject::connect(this->CreateStructureSetNodeAction, SIGNAL(triggered()), q, SLOT(createChildStructureSetForCurrentNode()));

  this->CreateContourAction = new QAction("Create child contour",q);
  QObject::connect(this->CreateContourAction, SIGNAL(triggered()), q, SLOT(createChildContourForCurrentNode()));

  this->ConvertContourToRepresentationAction = new QAction("Convert contour to its active representation",q);
  QObject::connect(this->ConvertContourToRepresentationAction, SIGNAL(triggered()), q, SLOT(convertCurrentNodeContourToRepresentation()));
  this->ConvertContourToRepresentationAction->setEnabled(false); //TODO Remove when the feature is added
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

//-----------------------------------------------------------------------------
QStringList qSlicerSubjectHierarchyContoursPlugin::dependencies()const
{
  return QStringList() << "DICOM";
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyContoursPlugin::canAddNodeToSubjectHierarchy(vtkMRMLNode* node, vtkMRMLSubjectHierarchyNode* parent/*=NULL*/)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::canAddNodeToSubjectHierarchy: Input node is NULL!";
    return 0.0;
  }
  else if (node->IsA("vtkMRMLContourNode"))
  {
    // Node is a contour
    return 1.0;
  }

  // Cannot add if new parent is not a structure set node
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
      // Node is a potential contour node representation. On reparenting under a structure set node in subject hierarchy, a contour node will be created
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
bool qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLSubjectHierarchyNode* parentNode)
{
  if (!nodeToAdd || !parentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Invalid argument!";
    return false;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Invalid MRML scene!";
    return false;
  }
  if (!scene || scene != parentNode->GetScene())
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Invalid MRML scene!";
    return false;
  }

  if (! (parentNode->IsA("vtkMRMLSubjectHierarchyNode") && parentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str())) )
  {
    qDebug() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Parent node must be a contour hierarchy node! Cancelling adding to subject hierarchy.";
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
      qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Failed to create contour from representation '" << nodeToAdd->GetName() << "'";
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
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Invalid node type to add!";
    return false;
  }

  contourSubjectHierarchyNode->SetAssociatedNodeID(contourNodeAddedToSubjectHierarchy->GetID());

  // Add color to the new color table
  if (!this->addContourColorToCorrespondingColorTable(contourNodeAddedToSubjectHierarchy, colorName))
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addNodeToSubjectHierarchy: Failed to add contour color to corresponding color table!";
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchyContoursPlugin::canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::canReparentNodeInsideSubjectHierarchy: Input node is NULL!";
    return 0.0;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();
  if (!associatedNode)
  {
    return 0.0;
  }
  
  // Cannot reparent if new parent is not a structure set node
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
    // Node is a potential contour node representation. On reparenting under a structure set node in subject hierarchy, a contour node will be created
    if ( parent->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
      && parent->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
    {
      return 1.0; // Only the Contours plugin can handle this reparent operation
    }
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* nodeToReparent, vtkMRMLSubjectHierarchyNode* parentNode)
{
  if (!nodeToReparent || !parentNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: Invalid argument!";
    return false;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: Invalid MRML scene!";
    return false;
  }
  if (!scene || scene != parentNode->GetScene())
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: Invalid MRML scene!";
    return false;
  }

  if (! (parentNode->IsA("vtkMRMLSubjectHierarchyNode") && parentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str())) )
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: New parent is not a contour hierarchy node! Cancelling reparenting.";
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
      qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: Failed to create contour from representation '" << associatedNode->GetName() << "'";
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
      qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: There is no color node for the contour to reparent '" << reparentedContourNode->GetName() << "'!";
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
      qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: There was no associated color for contour '" << reparentedContourNode->GetName() << "' before reparenting!";
    }
  }
  else
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: Invalid node type to reparent!";
    return false;
  }

  // Add color to the new color table
  if (!this->addContourColorToCorrespondingColorTable(reparentedContourNode, colorName))
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::reparentNodeInsideSubjectHierarchy: Failed to add contour color to corresponding color table!";
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLContourNode* qSlicerSubjectHierarchyContoursPlugin::isNodeAContourRepresentation(vtkMRMLNode* node)
{
  if (!node)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::isNodeAContourRepresentation: Input argument is NULL!";
    return NULL;
  }
  vtkMRMLScene* mrmlScene = node->GetScene();
  if (!mrmlScene)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::isNodeAContourRepresentation: Invalid MRML scene!";
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
bool qSlicerSubjectHierarchyContoursPlugin::addContourColorToCorrespondingColorTable(vtkMRMLContourNode* contourNode, QString colorName)
{
  if (!contourNode)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addContourColorToCorrespondingColorTable: Input contour node is NULL!";
    return false;
  }
  vtkMRMLScene* mrmlScene = contourNode->GetScene();
  if (!mrmlScene)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addContourColorToCorrespondingColorTable: Invalid MRML scene!";
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
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::addContourColorToCorrespondingColorTable: There is no color node in structure set subject hierarchy node belonging to contour '" << contourNode->GetName() << "'!";
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

  // Structure set
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    return 1.0; // Only the Contours plugin can handle this node
  }

  // Color table
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLColorTableNode") )
  {
    return 0.5;
  }

  return 0.0;
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

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Contour
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLContourNode") )
  {
    item->setIcon(d->ContourIcon);
    return true;
  }
  // Structure set
  else if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    item->setIcon(d->StructureSetIcon);
    return true;
  }
  // Color table
  else if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode->IsA("vtkMRMLColorTableNode") )
  {
    item->setIcon(d->ColorTableIcon);
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
  actions << d->CreateStructureSetNodeAction << d->CreateContourAction << d->ConvertContourToRepresentationAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::hideAllContextMenuActions()
{
  Q_D(qSlicerSubjectHierarchyContoursPlugin);

  d->CreateStructureSetNodeAction->setVisible(false);
  d->CreateContourAction->setVisible(false);
  d->ConvertContourToRepresentationAction->setVisible(false);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::showContextMenuActionsForHandlingNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyContoursPlugin);

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  vtkMRMLNode* associatedNode = node->GetAssociatedDataNode();

  // Contour
  if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES)
    && associatedNode && associatedNode->IsA("vtkMRMLContourNode") )
  {
    d->ConvertContourToRepresentationAction->setVisible(true);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::showContextMenuActionsForCreatingChildForNode(vtkMRMLSubjectHierarchyNode* node)
{
  Q_D(qSlicerSubjectHierarchyContoursPlugin);

  if (!node)
  {
    // There are no scene actions in this plugin
    return;
  }

  // Study
  if (node->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    d->CreateStructureSetNodeAction->setVisible(true);
  }
  // Structure set
  else if ( node->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
  {
    d->CreateContourAction->setVisible(true);
  }
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::createChildStructureSetForCurrentNode()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Check if structure set can be created for current parent node
  if (!currentNode || !currentNode->IsLevel(vtkSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY))
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::createChildStructureSetForCurrentNode: Invalid current node for creating structure set!";
    return;
  }
  if (!scene)
  {
    qCritical() << "qSlicerSubjectHierarchyContoursPlugin::createChildStructureSetForCurrentNode: Invalid MRML scene!";
    return;
  }

  // Create child structure set subject hierarchy node
  vtkMRMLSubjectHierarchyNode* childStructureSetSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    scene, currentNode, vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES, SlicerRtCommon::CONTOURHIERARCHY_NEW_STRUCTURE_SET_NAME.c_str());
  childStructureSetSubjectHierarchyNode->SetSaveWithScene(0);
  childStructureSetSubjectHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Add color table node and default colors
  vtkSmartPointer<vtkMRMLColorTableNode> structureSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  scene->AddNode(structureSetColorTableNode);
  std::string structureSetColorTableNodeName;
  structureSetColorTableNodeName = SlicerRtCommon::CONTOURHIERARCHY_NEW_STRUCTURE_SET_NAME +
    SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  structureSetColorTableNodeName = scene->GenerateUniqueName(structureSetColorTableNodeName);
  structureSetColorTableNode->SetName(structureSetColorTableNodeName.c_str());
  structureSetColorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  structureSetColorTableNode->SetSaveWithScene(0);
  structureSetColorTableNode->SetTypeToUser();

  structureSetColorTableNode->SetNumberOfColors(2);
  structureSetColorTableNode->GetLookupTable()->SetTableRange(0,1);
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_BACKGROUND, 0.0, 0.0, 0.0, 0.0); // Black background
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_INVALID,
    SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
    SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] ); // Color indicating invalid index

  // Add color table in subject hierarchy
  vtkMRMLSubjectHierarchyNode* structureSetColorTableSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    scene, childStructureSetSubjectHierarchyNode, vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES, structureSetColorTableNodeName.c_str(), structureSetColorTableNode);
  structureSetColorTableSubjectHierarchyNode->SetSaveWithScene(0);

  emit requestExpandNode(childStructureSetSubjectHierarchyNode);
}

//--------------------------------------------------------------------------
void qSlicerSubjectHierarchyContoursPlugin::createChildContourForCurrentNode()
{
  vtkMRMLSubjectHierarchyNode* currentNode = qSlicerSubjectHierarchyPluginHandler::instance()->currentNode();
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->scene();

  // Check if current (to-be parent) node is a structure set node
  if (!currentNode|| !currentNode->IsLevel(vtkSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES)
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
