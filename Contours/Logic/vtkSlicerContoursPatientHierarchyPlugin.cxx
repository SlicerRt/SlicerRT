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
double vtkSlicerContoursPatientHierarchyPlugin::CanPluginAddNodeToPatientHierarchy(vtkMRMLNode* node)
{
  if ( node->IsA("vtkMRMLContourNode")
    || node->IsA("vtkMRMLModelNode") // vtkMRMLModelNode includes annotations too
    || node->IsA("vtkMRMLVolumeNode") )
  {
    if (this->IsNodeAContourRepresentation(node))
    {
      // Node is a representation of an existing contour, so instead of the representation, the parent contour will only be the potential node
      return 0.0;
    }
    else
    {
      // Node is a potential contour node representation. On adding to the Patient Hierarchy, a contour node will be created
      return 1.0;
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
    vtkDebugMacro("AddNodeToPatientHierarchy: Parent node must be a contour hierarchy node!");
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

  double color[4] = { SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1], SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] };
  std::string colorName;

  // Create contour node if dropped node is volume or model
  vtkMRMLContourNode* contourNodeAddedToPatientHierarchy = NULL;
  if ( (nodeToAdd->IsA("vtkMRMLModelNode") && !nodeToAdd->IsA("vtkMRMLAnnotationNode"))
    || nodeToAdd->IsA("vtkMRMLScalarVolumeNode") )
  {
    vtkSmartPointer<vtkMRMLContourNode> newContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
    newContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->AddNode(newContourNode));
    std::string contourName = std::string(nodeToAdd->GetName()) + SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX;
    contourName = mrmlScene->GenerateUniqueName(contourName);
    newContourNode->SetName(contourName.c_str());
    colorName = std::string(nodeToAdd->GetName());

    if (nodeToAdd->IsA("vtkMRMLScalarVolumeNode"))
    {
      newContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(nodeToAdd->GetID());

      // Set the labelmap itself as reference thus indicating there was no conversion from model representation
      newContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(nodeToAdd->GetID());
      newContourNode->SetRasterizationOversamplingFactor(1.0);
    }
    else
    {
      newContourNode->SetAndObserveClosedSurfaceModelNodeId(nodeToAdd->GetID());
      newContourNode->SetDecimationTargetReductionFactor(0.0);

      // Get model color
      vtkMRMLModelNode* modelNodeToAdd = vtkMRMLModelNode::SafeDownCast(nodeToAdd);
      if (modelNodeToAdd && modelNodeToAdd->GetDisplayNode())
      {
        modelNodeToAdd->GetDisplayNode()->GetColor(color[0], color[1], color[2]);
      }
    }
    newContourNode->HideFromEditorsOff();
    contourPatientHierarchyNode->SetAssociatedNodeID(newContourNode->GetID());
    newContourNode->Modified();

    contourNodeAddedToPatientHierarchy = newContourNode.GetPointer();
  }
  else if (nodeToAdd->IsA("vtkMRMLContourNode"))
  {
    contourPatientHierarchyNode->SetAssociatedNodeID(nodeToAdd->GetID());
    contourNodeAddedToPatientHierarchy = vtkMRMLContourNode::SafeDownCast(nodeToAdd);

    // Get color
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast( mrmlScene->GetNodeByID(
      contourNodeAddedToPatientHierarchy->GetRibbonModelNodeId() ? contourNodeAddedToPatientHierarchy->GetRibbonModelNodeId() : contourNodeAddedToPatientHierarchy->GetClosedSurfaceModelNodeId()) );
    if (modelNode && modelNode->GetDisplayNode())
    {
      modelNode->GetDisplayNode()->GetColor(color[0], color[1], color[2]);
    }
    colorName = std::string(nodeToAdd->GetName());
  }
  else
  {
    vtkErrorMacro("AddNodeToPatientHierarchy: Invalid node type to add!");
    return false;
  }

  // Add color to the new color table
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = SlicerRtCommon::COLOR_INDEX_INVALID; // Initializing to this value means that we don't request the color index, just the color table
  contourNodeAddedToPatientHierarchy->GetColor(structureColorIndex, colorNode);
  if (!colorNode)
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: There is no color node in structure set patient hierarchy node'" << parentNode->GetName() << "'!");
    return false;
  }

  int numberOfColors = colorNode->GetNumberOfColors();
  colorNode->SetNumberOfColors(numberOfColors+1);
  colorNode->GetLookupTable()->SetTableRange(0, numberOfColors);
  colorNode->SetColor(numberOfColors, colorName.c_str(), color[0], color[1], color[2], color[3]);

  // Paint labelmap foreground value to match the index of the newly added color
  if (nodeToAdd->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkMRMLScalarVolumeNode* scalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(nodeToAdd);

    // Make sure there is a display node with the right type
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

//----------------------------------------------------------------------------
double vtkSlicerContoursPatientHierarchyPlugin::CanPluginReparentNodeInsidePatientHierarchy(vtkMRMLHierarchyNode* node)
{
  vtkMRMLNode* associatedNode = node->GetAssociatedNode();
  if ( associatedNode && associatedNode->IsA("vtkMRMLContourNode") )
  {
    // Node is a potential contour node representation. On adding to the Patient Hierarchy, a contour node will be created
    return 1.0;
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
  vtkMRMLContourNode* contourNodeToReparent = vtkMRMLContourNode::SafeDownCast(associatedNode);
  if (!contourNodeToReparent)
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: Only contour nodes can be reparented using the Contours PatientHierarchy plugin.");
    return false;
  }

  // Get color index and the color table that has been associated to the contour node before reparenting
  vtkMRMLColorTableNode* oldColorNode = NULL;
  int oldStructureColorIndex = -1;
  contourNodeToReparent->GetColor(oldStructureColorIndex, oldColorNode);

  // Do the reparenting
  vtkMRMLDisplayableHierarchyNode* associatedPatientHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(
    vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(contourNodeToReparent) );
  if (associatedPatientHierarchyNode)
  {
    associatedPatientHierarchyNode->SetParentNodeID(parentNode->GetID());
  }
  else
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: There is no patient hierarchy node associated with the contour to reparent '" << contourNodeToReparent->GetName() << "'!");
    return false;
  }

  // Remove color from the color table that has been associated to the contour node before reparenting
  double color[4];
  std::string colorName;
  if (oldColorNode && oldStructureColorIndex != SlicerRtCommon::COLOR_INDEX_INVALID)
  {
    // Save color for later insertion in the new color node
    oldColorNode->GetColor(oldStructureColorIndex, color);
    colorName = std::string(oldColorNode->GetColorName(oldStructureColorIndex));

    // Set old color entry to invalid
    oldColorNode->SetColor(oldStructureColorIndex, SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
      SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3]);
    oldColorNode->SetColorName(oldStructureColorIndex, SlicerRtCommon::COLOR_NAME_REMOVED);
  }
  else
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: There was no associated color for contour '" << contourNodeToReparent->GetName() << "' before reparenting!");
  }

  // Add color to the new color table
  vtkMRMLColorTableNode* newColorNode = NULL;
  int newStructureColorIndex = SlicerRtCommon::COLOR_INDEX_INVALID; // Initializing to this value means that we don't request the color index, just the color table
  contourNodeToReparent->GetColor(newStructureColorIndex, newColorNode);
  if (!newColorNode)
  {
    vtkErrorMacro("ReparentInsidePatientHierarchy: There is no color node in structure set patient hierarchy node'" << parentNode->GetName() << "'!");
    return false;
  }

  int numberOfColors = newColorNode->GetNumberOfColors();
  newColorNode->SetNumberOfColors(numberOfColors+1);
  newColorNode->GetLookupTable()->SetTableRange(0, numberOfColors);
  newColorNode->SetColor(numberOfColors, colorName.c_str(), color[0], color[1], color[2], color[3]);

  // Paint labelmap foreground value to match the index of the newly added color
  if (contourNodeToReparent->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap))
  {
    vtkMRMLScalarVolumeNode* scalarVolumeNode = contourNodeToReparent->GetIndexedLabelmapVolumeNode();
    if (scalarVolumeNode->GetDisplayNode())
    {
      scalarVolumeNode->GetDisplayNode()->SetAndObserveColorNodeID(newColorNode->GetID());
    }
    else
    {
      vtkErrorMacro("ReparentInsidePatientHierarchy: Scalar volume node '" << scalarVolumeNode->GetName() << "' does not have a display node!");
    }
    vtkSlicerContoursModuleLogic::PaintLabelmapForeground(scalarVolumeNode, numberOfColors);
  }

  return true;
}

//---------------------------------------------------------------------------
vtkMRMLContourNode* vtkSlicerContoursPatientHierarchyPlugin::IsNodeAContourRepresentation(vtkMRMLNode* node)
{
  if (!node)
  {
    vtkErrorMacro("IsNodeAContourRepresentation: Invalid nodeID argument!");
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
