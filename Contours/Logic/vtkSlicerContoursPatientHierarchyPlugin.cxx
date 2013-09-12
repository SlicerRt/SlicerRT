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
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// Contours includes
#include "vtkSlicerContoursPatientHierarchyPlugin.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContoursPatientHierarchyPlugin);

//-----------------------------------------------------------------------------
vtkSlicerContoursPatientHierarchyPlugin::vtkSlicerContoursPatientHierarchyPlugin()
 : vtkSlicerPatientHierarchyPlugin()
{
  this->SetName("Contours");
}

//-----------------------------------------------------------------------------
vtkSlicerContoursPatientHierarchyPlugin::~vtkSlicerContoursPatientHierarchyPlugin()
{
}

//----------------------------------------------------------------------------
void vtkSlicerContoursPatientHierarchyPlugin::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
double vtkSlicerContoursPatientHierarchyPlugin::CanPluginAddNodeToPatientHierarchy(vtkMRMLNode* node, vtkMRMLHierarchyNode* parent/*=NULL*/)
{
  if (node->IsA("vtkMRMLContourNode"))
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

  if ( ( node->IsA("vtkMRMLModelNode") && !node->IsA("vtkMRMLAnnotationNode") )
    || node->IsA("vtkMRMLVolumeNode") )
  {
    if (this->IsNodeAContourRepresentation(node))
    {
      // Node is a representation of an existing contour, so instead of the representation, the parent contour will only be the potential node
      return 0.0;
    }
    else
    {
      // Node is a potential contour node representation. On reparenting under a structure set node in Patient Hierarchy, a contour node will be created
      return 0.7;
    }
  }

  return 0.0;
}

//----------------------------------------------------------------------------
bool vtkSlicerContoursPatientHierarchyPlugin::AddNodeToPatientHierarchy(vtkMRMLNode* nodeToAdd, vtkMRMLHierarchyNode* parentNode)
{
  if (!nodeToAdd || !parentNode)
  {
    vtkErrorMacro("AddNodeToPatientHierarchy: Invalid argument!");
    return false;
  }
  vtkMRMLScene* mrmlScene = nodeToAdd->GetScene();
  if (!mrmlScene || (mrmlScene != parentNode->GetScene()))
  {
    vtkErrorMacro("AddNodeToPatientHierarchy: Invalid MRML scene!");
    return false;
  }

  if (! (parentNode->IsA("vtkMRMLDisplayableHierarchyNode") && parentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str())) )
  {
    vtkDebugMacro("AddNodeToPatientHierarchy: Parent node must be a contour hierarchy node! Cancelling adding to patient hierarchy.");
    return false;
  }

  // Create hierarchy node for contour node
  vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> contourPatientHierarchyNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
  contourPatientHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(mrmlScene->AddNode(contourPatientHierarchyNode));
  std::string phNodeName;
  phNodeName = std::string(nodeToAdd->GetName()) + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  phNodeName = mrmlScene->GenerateUniqueName(phNodeName);
  contourPatientHierarchyNode->SetName(phNodeName.c_str());
  contourPatientHierarchyNode->HideFromEditorsOff();
  contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  //TODO: Subseries level is the default for now. This and UID has to be specified before export (need the tag editor widget)
  contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  contourPatientHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(),
    nodeToAdd->GetName() );
  contourPatientHierarchyNode->SetParentNodeID(parentNode->GetID());

  std::string colorName;

  // Get added contour node and associate it with the new patient hierarchy node
  vtkMRMLContourNode* contourNodeAddedToPatientHierarchy = NULL;
  if ( (nodeToAdd->IsA("vtkMRMLModelNode") && !nodeToAdd->IsA("vtkMRMLAnnotationNode"))
    || nodeToAdd->IsA("vtkMRMLScalarVolumeNode") )
  {
    // Create contour from the dropped representation
    contourNodeAddedToPatientHierarchy = vtkSlicerContoursModuleLogic::CreateContourFromRepresentation( vtkMRMLDisplayableNode::SafeDownCast(nodeToAdd) );
    if (!contourNodeAddedToPatientHierarchy)
    {
      vtkErrorMacro("AddNodeToPatientHierarchy: Failed to create contour from representation '" << nodeToAdd->GetName() << "'");
      return false;
    }

    colorName = std::string(nodeToAdd->GetName());
  }
  else if (nodeToAdd->IsA("vtkMRMLContourNode"))
  {
    contourNodeAddedToPatientHierarchy = vtkMRMLContourNode::SafeDownCast(nodeToAdd);
    colorName = std::string(nodeToAdd->GetName());
  }
  else
  {
    vtkErrorMacro("CreateContourFromRepresentation: Invalid node type to add!");
    return false;
  }

  contourPatientHierarchyNode->SetAssociatedNodeID(contourNodeAddedToPatientHierarchy->GetID());

  // Add color to the new color table
  if (!this->AddContourColorToCorrespondingColorTable(contourNodeAddedToPatientHierarchy, colorName))
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: Failed to add contour color to corresponding color table!");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
double vtkSlicerContoursPatientHierarchyPlugin::CanPluginReparentNodeInsidePatientHierarchy(vtkMRMLHierarchyNode* node, vtkMRMLHierarchyNode* parent/*=NULL*/)
{
  vtkMRMLNode* associatedNode = node->GetAssociatedNode();
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

  if ( ( associatedNode->IsA("vtkMRMLModelNode") && !associatedNode->IsA("vtkMRMLAnnotationNode") )
    || associatedNode->IsA("vtkMRMLVolumeNode") )
  {
    // Node is a potential contour node representation. On reparenting under a structure set node in Patient Hierarchy, a contour node will be created
    return 0.7;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool vtkSlicerContoursPatientHierarchyPlugin::ReparentNodeInsidePatientHierarchy(vtkMRMLHierarchyNode* nodeToReparent, vtkMRMLHierarchyNode* parentNode)
{
  if (!nodeToReparent || !parentNode)
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: Invalid argument!");
    return false;
  }
  vtkMRMLScene* mrmlScene = nodeToReparent->GetScene();
  if (!mrmlScene || (mrmlScene != parentNode->GetScene()))
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: Invalid MRML scene!");
    return false;
  }

  if (! (parentNode->IsA("vtkMRMLDisplayableHierarchyNode") && parentNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str())) )
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: New parent is not a contour hierarchy node! Cancelling reparenting.");
    return false;
  }

  vtkMRMLNode* associatedNode = nodeToReparent->GetAssociatedNode();
  vtkMRMLContourNode* reparentedContourNode = NULL;
  std::string colorName;

  // Get added contour node and associate it with the new patient hierarchy node
  if ( (associatedNode->IsA("vtkMRMLModelNode") && !associatedNode->IsA("vtkMRMLAnnotationNode"))
    || associatedNode->IsA("vtkMRMLScalarVolumeNode") )
  {
    // Create contour from the dropped representation
    reparentedContourNode = vtkSlicerContoursModuleLogic::CreateContourFromRepresentation( vtkMRMLDisplayableNode::SafeDownCast(associatedNode) );
    if (!reparentedContourNode)
    {
      vtkErrorMacro("ReparentInsidePatientHierarchy: Failed to create contour from representation '" << associatedNode->GetName() << "'");
      return false;
    }

    // Replace the reparented contour representation with the created contour itself and do the reparenting
    nodeToReparent->SetAssociatedNodeID(reparentedContourNode->GetID());
    nodeToReparent->SetParentNodeID(parentNode->GetID());
    nodeToReparent->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
    nodeToReparent->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), associatedNode->GetName() );

    colorName = std::string(associatedNode->GetName());
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
      colorName = std::string(oldColorNode->GetColorName(oldStructureColorIndex));
    }
    else
    {
      vtkErrorMacro("ReparentInsidePatientHierarchy: There is no color node for the contour to reparent '" << reparentedContourNode->GetName() << "'!");
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
      vtkErrorMacro("ReparentInsidePatientHierarchy: There was no associated color for contour '" << reparentedContourNode->GetName() << "' before reparenting!");
    }
  }
  else
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: Invalid node type to reparent!");
    return false;
  }

  // Add color to the new color table
  if (!this->AddContourColorToCorrespondingColorTable(reparentedContourNode, colorName))
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: Failed to add contour color to corresponding color table!");
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLContourNode* vtkSlicerContoursPatientHierarchyPlugin::IsNodeAContourRepresentation(vtkMRMLNode* node)
{
  if (!node)
  {
    vtkErrorMacro("IsNodeAContourRepresentation: Input argument is NULL!");
    return NULL;
  }
  vtkMRMLScene* mrmlScene = node->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("IsNodeAContourRepresentation: Invalid MRML scene!");
    return NULL;
  }

  // If the node is neither a model nor a volume, the it cannot be a representation
  if (!node->IsA("vtkMRMLModelNode") && !node->IsA("vtkMRMLVolumeNode"))
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
bool vtkSlicerContoursPatientHierarchyPlugin::AddContourColorToCorrespondingColorTable(vtkMRMLContourNode* contourNode, std::string colorName)
{
  if (!contourNode)
  {
    vtkErrorMacro("AddContourColorToCorrespondingColorTable: Input contour node is NULL!");
    return false;
  }
  vtkMRMLScene* mrmlScene = contourNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("IsNodeAContourRepresentation: Invalid MRML scene!");
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
    vtkErrorMacro("CreateContourFromRepresentation: There is no color node in structure set patient hierarchy node belonging to contour '" << contourNode->GetName() << "'!");
    return false;
  }

  int numberOfColors = colorNode->GetNumberOfColors();
  colorNode->SetNumberOfColors(numberOfColors+1);
  colorNode->GetLookupTable()->SetTableRange(0, numberOfColors);
  colorNode->SetColor(numberOfColors, colorName.c_str(), color[0], color[1], color[2], color[3]);

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
