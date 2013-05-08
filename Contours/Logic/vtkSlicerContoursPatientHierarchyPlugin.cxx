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

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

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
    vtkErrorMacro("AddNodeToPatientHierarchy: Ivalid argument!");
    return false;
  }
  vtkMRMLScene* mrmlScene = nodeToAdd->GetScene();
  if (!mrmlScene || (mrmlScene != parentNode->GetScene()))
  {
    vtkErrorMacro("AddNodeToPatientHierarchy: Invalid MRML scene!");
    return false;
  }

  if (!parentNode->IsA("vtkMRMLContourHierarchyNode"))
  {
    vtkDebugMacro("AddNodeToPatientHierarchy: Parent node must be a contour hierarchy node!");
    return false;
  }

  // Create hierarchy node for contour node
  vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> associatedPatientHierarchyNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
  associatedPatientHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(mrmlScene->AddNode(associatedPatientHierarchyNode));
  std::string phNodeName;
  phNodeName = std::string(nodeToAdd->GetName()) + SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX;
  phNodeName = mrmlScene->GenerateUniqueName(phNodeName);
  associatedPatientHierarchyNode->SetName(phNodeName.c_str());
  associatedPatientHierarchyNode->HideFromEditorsOff();
  associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  //TODO: Subseries level is the default for now. This and UID has to be specified before export (need the tag editor widget)
  associatedPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  associatedPatientHierarchyNode->SetParentNodeID(parentNode->GetID());

  // Create contour node if dropped node is volume or model
  if ( (nodeToAdd->IsA("vtkMRMLModelNode") && !nodeToAdd->IsA("vtkMRMLAnnotationNode"))
    || nodeToAdd->IsA("vtkMRMLScalarVolumeNode") )
  {
    vtkSmartPointer<vtkMRMLContourNode> newContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
    newContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->AddNode(newContourNode));
    std::string contourName(nodeToAdd->GetName());
    contourName.append(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_NODE_NAME_POSTFIX.c_str());
    contourName = mrmlScene->GenerateUniqueName(contourName);
    newContourNode->SetName(contourName.c_str());
    //contourNode->SetStructureName(roiLabel); //TODO: Utilize PH so that this variable is not needed
    if (nodeToAdd->IsA("vtkMRMLScalarVolumeNode"))
    {
      newContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(nodeToAdd->GetID());

      // Make sure the volume is treated as a labelmap
      nodeToAdd->SetAttribute("LabelMap", "1");

      // Set the labelmap itself as reference thus indicating there was no conversion from model representation
      newContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(nodeToAdd->GetID());
      newContourNode->SetRasterizationOversamplingFactor(1.0);
    }
    else
    {
      newContourNode->SetAndObserveClosedSurfaceModelNodeId(nodeToAdd->GetID());
      newContourNode->SetDecimationTargetReductionFactor(0.0);
    }
    newContourNode->HideFromEditorsOff();
    associatedPatientHierarchyNode->SetAssociatedNodeID(newContourNode->GetID());
    newContourNode->Modified();
  }
  else if (nodeToAdd->IsA("vtkMRMLContourNode"))
  {
    associatedPatientHierarchyNode->SetAssociatedNodeID(nodeToAdd->GetID());
  }
  else
  {
    vtkErrorMacro("AddNodeToPatientHierarchy: Invalid node type to add!");
    return false;
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

