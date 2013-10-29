/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

// PatientHierarchy includes
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

//----------------------------------------------------------------------------
const char* vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_PATIENT = "Patient";
const char* vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY = "Study";
const char* vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES = "Series";
const char* vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES = "Subseries";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPatientHierarchyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerPatientHierarchyModuleLogic::vtkSlicerPatientHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerPatientHierarchyModuleLogic::~vtkSlicerPatientHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode*
vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(vtkMRMLScene* scene, const char* uid)
{
  if (!scene || !uid)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID: Invalid scene or searched UID!" << std::endl;
    return NULL;
  }

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLHierarchyNode", patientHierarchyNodes);

  for (unsigned int phNodeIndex=0; phNodeIndex<numberOfNodes; phNodeIndex++)
  {
    vtkMRMLHierarchyNode* node = vtkMRMLHierarchyNode::SafeDownCast(patientHierarchyNodes[phNodeIndex]);
    if (node && SlicerRtCommon::IsPatientHierarchyNode(node))
    {
      const char* nodeUID = node->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME);
      if (nodeUID && !STRCASECMP(uid, nodeUID))
      {
        return node;
      }
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
  vtkMRMLScene *scene, const char* patientId, const char* studyInstanceUID, const char* seriesInstanceUID )
{
  if ( !scene || !patientId || !studyInstanceUID || !seriesInstanceUID )
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Invalid input arguments!" << std::endl;
    return NULL;
  }

  vtkMRMLHierarchyNode* patientNode = NULL;
  vtkMRMLHierarchyNode* studyNode = NULL;
  vtkMRMLHierarchyNode* seriesNode = NULL;

  std::vector<vtkMRMLNode*> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLHierarchyNode", patientHierarchyNodes);

  // Find referenced nodes
  for (unsigned int i=0; i<numberOfNodes; i++)
  {
    vtkMRMLHierarchyNode *node = vtkMRMLHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if ( node && SlicerRtCommon::IsPatientHierarchyNode(node) )
    {
      const char* nodeUID = node->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME);
      if (!nodeUID)
      {
        if ( !node->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME)
          || STRCASECMP(node->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME), vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES) )
        {
          vtkWarningWithObjectMacro(scene,
            "Patient hierarchy node '" << node->GetName() << "' does not have a UID thus invalid!");
        }
        continue;
      }
      if (!STRCASECMP(patientId, nodeUID))
      {
        patientNode = node;
      }
      else if (!STRCASECMP(studyInstanceUID, nodeUID))
      {
        studyNode = node;
      }
      else if (!STRCASECMP(seriesInstanceUID, nodeUID))
      {
        seriesNode = node;
      }
    }
  }

  if (!seriesNode)
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Patient hierarchy node with ID="
      << seriesInstanceUID << " cannot be found!");
    return NULL;
  }

  // Create patient and study nodes if they do not exist yet
  if (!patientNode)
  {
    patientNode = vtkMRMLHierarchyNode::New();
    scene->AddNode(patientNode);
    patientNode->Delete(); // Return ownership to the scene only
    patientNode->AllowMultipleChildrenOn();
    patientNode->HideFromEditorsOff();
    patientNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
      SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
    patientNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_PATIENT);
    patientNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
      patientId);
  }

  if (!studyNode)
  {
    studyNode = vtkMRMLHierarchyNode::New();
    scene->AddNode(studyNode);
    studyNode->Delete(); // Return ownership to the scene only
    studyNode->AllowMultipleChildrenOn();
    studyNode->HideFromEditorsOff();
    studyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
      SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
    studyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY);
    studyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
      studyInstanceUID);
    studyNode->SetParentNodeID(patientNode->GetID());
  }

  seriesNode->SetParentNodeID(studyNode->GetID());

  return seriesNode;
}

//---------------------------------------------------------------------------
bool vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch(vtkMRMLNode* node1, vtkMRMLNode* node2,
                                                                const char* lowestCommonLevel)
{
  if ( !node1 || !node2 || node1->GetScene() != node2->GetScene() )
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Invalid input nodes or they are not in the same scene!" << std::endl;
    return false;
  }

  if (!lowestCommonLevel)
  {
    vtkErrorWithObjectMacro(node1, "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Invalid lowest common level!");
    return false;
  }

  // If not hierarchy nodes, get the associated patient hierarchy node
  vtkMRMLHierarchyNode* hierarchyNode1 = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(node1);
  vtkMRMLHierarchyNode* hierarchyNode2 = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(node2);

  // Check if valid nodes are found
  if (!hierarchyNode1 || !hierarchyNode2)
  {
    return false;
  }

  // Walk the hierarchy up until we reach the lowest common level
  while (true)
  {
    hierarchyNode1 = vtkMRMLHierarchyNode::SafeDownCast(hierarchyNode1->GetParentNode());
    if (!SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode1))
    {
      hierarchyNode1 = NULL;
      vtkWarningWithObjectMacro(node1, "Patient hierarchy node ('" << hierarchyNode1->GetName() << "') has no ancestor with DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    const char* node1Level = hierarchyNode1->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
    if (!node1Level)
    {
      hierarchyNode1 = NULL;
      vtkWarningWithObjectMacro(node1, "Patient hierarchy node ('" << hierarchyNode1->GetName() << "') has no DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    if (!STRCASECMP(node1Level, lowestCommonLevel))
    {
      break;
    }
  }

  while (true)
  {
    hierarchyNode2 = vtkMRMLHierarchyNode::SafeDownCast(hierarchyNode2->GetParentNode());
    if (!SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode2))
    {
      hierarchyNode2 = NULL;
      vtkWarningWithObjectMacro(node1, "Patient hierarchy node ('" << hierarchyNode2->GetName() << "') has no ancestor with DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    const char* node2Level = hierarchyNode2->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
    if (!node2Level)
    {
      hierarchyNode2 = NULL;
      vtkWarningWithObjectMacro(node1, "Patient hierarchy node ('" << hierarchyNode2->GetName() << "') has no DICOM level '" << lowestCommonLevel << "'");
      break;
    }
    if (!STRCASECMP(node2Level, lowestCommonLevel))
    {
      break;
    }
  }

  const char* node1UID = hierarchyNode1->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME);
  const char* node2UID = hierarchyNode2->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME);
  if ( !node1UID || !node2UID )
  {
    vtkErrorWithObjectMacro(node1, "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Found ancestor node contains empty UID!");
    return false;
  }

  // Check if the found nodes match
  // (handling the case when two patient hierarchy nodes point to the same DICOM object)
  if ( !strcmp(node1UID, node2UID) )
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::SetBranchVisibility(vtkMRMLHierarchyNode* node, int visible)
{
  if (visible != 0 && visible != 1)
  {
    vtkDebugWithObjectMacro(node, "SetBranchVisibility: Invalid visibility value to set: " << visible);
    return;
  }
  if (node->GetScene()->IsBatchProcessing())
  {
    vtkDebugWithObjectMacro(node, "SetBranchVisibility: Batch processing is on, returning");
    return;
  }

  vtkSmartPointer<vtkCollection> childDisplayableNodes = vtkSmartPointer<vtkCollection>::New();
  node->GetAssociatedChildrendNodes(childDisplayableNodes, "vtkMRMLDisplayableNode");
  childDisplayableNodes->InitTraversal();
  std::set<vtkMRMLHierarchyNode*> parentNodes;
  for (int i=0; i<childDisplayableNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(childDisplayableNodes->GetItemAsObject(i));
    if (displayableNode)
    {
      displayableNode->SetDisplayVisibility(visible);

      vtkMRMLDisplayNode* displayNode = displayableNode->GetDisplayNode();
      if (displayNode)
      {
        displayNode->SetSliceIntersectionVisibility(visible);
      }

      displayableNode->Modified();

      // Collect all parents
      vtkMRMLHierarchyNode* parentNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode( node->GetScene(), displayableNode->GetID() );
      do 
      {
        parentNodes.insert(parentNode);
      }
      while ((parentNode = parentNode->GetParentNode()) != NULL); // The double parentheses avoids a Linux build warning
    }
  }

  // Set Modified flag for all parent nodes so that their icons are refreshed in the tree view
  for (std::set<vtkMRMLHierarchyNode*>::iterator parentsIt = parentNodes.begin(); parentsIt != parentNodes.end(); ++ parentsIt)
  {
    (*parentsIt)->Modified();
  }
}

//---------------------------------------------------------------------------
int vtkSlicerPatientHierarchyModuleLogic::GetBranchVisibility(vtkMRMLHierarchyNode* node)
{
  int visible = -1;
  vtkSmartPointer<vtkCollection> childDisplayableNodes = vtkSmartPointer<vtkCollection>::New();
  node->GetAssociatedChildrendNodes(childDisplayableNodes, "vtkMRMLDisplayableNode");
  childDisplayableNodes->InitTraversal();

  for (int i=0; i<childDisplayableNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(childDisplayableNodes->GetItemAsObject(i));
    if (displayableNode && !displayableNode->IsA("vtkMRMLScalarVolumeNode"))
    {
      // If we set visibility
      if (visible == -1)
      {
        visible = displayableNode->GetDisplayVisibility();

        // We expect only 0 or 1 from leaf nodes
        if (visible == 2)
        {
          vtkWarningWithObjectMacro(node, "GetBranchVisibility: Unexpected visibility value for node " << displayableNode->GetName());
        }
      }
      // If the current node visibility does not match the found visibility, then set partial visibility
      else if (displayableNode->GetDisplayVisibility() != visible)
      {
        return 2;
      }
    }
  }

  return visible;
}

//---------------------------------------------------------------------------
bool vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel( vtkMRMLNode* node, const char* level )
{
  if (!node || !level)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::IsDicomLevel: Invalid input arguments!" << std::endl;
    return false;
  }

  vtkMRMLHierarchyNode* hnode = NULL;
  if (node->IsA("vtkMRMLHierarchyNode"))
  {
    hnode = vtkMRMLHierarchyNode::SafeDownCast(node);
  }
  else
  {
    hnode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(node->GetScene(), node->GetID());
  }

  if (!hnode)
  {
    return false;
  }

  const char* nodeLevel = hnode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
  if (nodeLevel && !STRCASECMP(level, nodeLevel))
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(vtkMRMLNode *associatedNode, bool reverseCriterion/*=false*/)
{
  if (!associatedNode)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode: associated node is null" << std::endl;
    return NULL;
  }

  if (SlicerRtCommon::IsPatientHierarchyNode(associatedNode))
  {
    return vtkMRMLHierarchyNode::SafeDownCast(associatedNode);
  }

  vtkSmartPointer<vtkCollection> hierarchyNodes = vtkSmartPointer<vtkCollection>::Take( associatedNode->GetScene()->GetNodesByClass("vtkMRMLHierarchyNode") );
  vtkObject* nextObject = NULL;
  for (hierarchyNodes->InitTraversal(); (nextObject = hierarchyNodes->GetNextItemAsObject()); )
  {
    vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(nextObject);
    if (hierarchyNode)
    {
      const char* currentAssociatedNodeId = hierarchyNode->GetAssociatedNodeID();
      if ( currentAssociatedNodeId && !STRCASECMP(currentAssociatedNodeId, associatedNode->GetID())
        && (SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode) & !reverseCriterion) )
      {
        return hierarchyNode;
      }
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
std::string vtkSlicerPatientHierarchyModuleLogic::GetTooltipForPatientHierarchyNode(vtkMRMLHierarchyNode* hierarchyNode)
{
  if (!hierarchyNode)
  {
    return "";
  }
  if (!SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode))
  {
    vtkErrorWithObjectMacro(hierarchyNode, "GetTooltipForPatientHierarchyNode: Attribute node is not a patient hierarchy node!");
    return "";
  }

  std::string tooltipString;
  vtkMRMLNode* associatedNode = hierarchyNode->GetAssociatedNode();
  if (associatedNode)
  {
    tooltipString.append(associatedNode->GetNodeTagName());
    tooltipString.append(" (");
  }

  tooltipString.append(hierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME));

  hierarchyNode = hierarchyNode->GetParentNode();
  while (SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode))
  {
    tooltipString.append("; ");
    tooltipString.append(hierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME));
    tooltipString.append(":");
    tooltipString.append(hierarchyNode->GetName());
    hierarchyNode = hierarchyNode->GetParentNode();
  }

  if (associatedNode)
  {
    tooltipString.append(")");
  }

  return tooltipString;
}

//---------------------------------------------------------------------------
const char* vtkSlicerPatientHierarchyModuleLogic::GetChildDicomLevel(const char* parentLevel)
{
  if (!parentLevel)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetChildDicomLevel: Invalid parent level!" << std::endl;
    return NULL;
  }

  const char* childLevel;
  if (!STRCASECMP(parentLevel, PATIENTHIERARCHY_LEVEL_PATIENT))
  {
    childLevel = PATIENTHIERARCHY_LEVEL_STUDY;
  }
  else if (!STRCASECMP(parentLevel, PATIENTHIERARCHY_LEVEL_STUDY))
  {
    childLevel = PATIENTHIERARCHY_LEVEL_SERIES;
  }
  else if (!STRCASECMP(parentLevel, PATIENTHIERARCHY_LEVEL_SERIES))
  {
    childLevel = PATIENTHIERARCHY_LEVEL_SUBSERIES;
  }
  else if (!STRCASECMP(parentLevel, PATIENTHIERARCHY_LEVEL_SUBSERIES))
  {
    childLevel = PATIENTHIERARCHY_LEVEL_SUBSERIES;
  }
  else
  {
    //TODO: Report error so that it is logged (no vtk class, no Qt)
    return NULL;
  }

  return childLevel;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::CreateGenericChildNodeForPatientHierarchyNode(vtkMRMLNode* parentNode, vtkMRMLScene* scene)
{
  if (!scene)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::CreateGenericChildNodeForPatientHierarchyNode: Invalid scene!" << std::endl;
    return NULL;
  }

  std::string childLevel("");

  // Create new patient if there is no current node (the user right-clicked on the scene)
  if (!parentNode)
  {
    childLevel = std::string(vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_PATIENT);
  }
  // Otherwise determine the level of the current (parent) node and its child level
  else
  {
    if (!SlicerRtCommon::IsPatientHierarchyNode(parentNode))
    {
      vtkErrorWithObjectMacro(parentNode, "CreateChildNodeForPatientHierarchyNode: Attribute node is not a patient hierarchy node!");
      return NULL;
    }
    vtkMRMLHierarchyNode* parentHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(parentNode);
    const char* parentLevel = parentHierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
    childLevel = vtkSlicerPatientHierarchyModuleLogic::GetChildDicomLevel(parentLevel);
    if (childLevel.empty())
    {
      vtkErrorWithObjectMacro(parentNode, "CreateChildNodeForPatientHierarchyNode: Invalid DICOM level for specified parent node '" << (parentLevel?parentLevel:"") << "'");
      return NULL;
    }
  }

  // Create patient hierarchy entry
  vtkSmartPointer<vtkMRMLHierarchyNode> childPatientHierarchyNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  childPatientHierarchyNode->HideFromEditorsOff();
  childPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  childPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    childLevel.c_str());
  childPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
    "");
  std::string childNodeName = SlicerRtCommon::PATIENTHIERARCHY_NEW_NODE_NAME_PREFIX + childLevel + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  childPatientHierarchyNode->SetName(childNodeName.c_str());
  if (parentNode)
  {
    childPatientHierarchyNode->SetParentNodeID(parentNode->GetID());
  }
  scene->AddNode(childPatientHierarchyNode);

  return childPatientHierarchyNode;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::CreateChildStructureSetNodeForPatientHierarchyNode(vtkMRMLNode* parentNode)
{
  if (!parentNode)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::CreateChildStructureSetNodeForPatientHierarchyNode: Invalid parent node!" << std::endl;
    return NULL;
  }
  if (!parentNode->GetScene())
  {
    vtkErrorWithObjectMacro(parentNode, "CreateChildStructureSetNodeForPatientHierarchyNode: Invalid MRML scene!");
    return NULL;
  }
  if (!SlicerRtCommon::IsPatientHierarchyNode(parentNode))
  {
    vtkErrorWithObjectMacro(parentNode, "CreateChildStructureSetNodeForPatientHierarchyNode: Attribute node is not a patient hierarchy node!");
    return NULL;
  }
  vtkMRMLHierarchyNode* parentHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(parentNode);
  const char* parentLevel = parentHierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
  if (!parentLevel || STRCASECMP(parentLevel, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY))
  {
    vtkErrorWithObjectMacro(parentNode, "CreateChildStructureSetNodeForPatientHierarchyNode: Structure set can only be child of a study node, but parent node's level is '" << (parentLevel?parentLevel:"") << "'");
    return NULL;
  }

  // Create structure set node
  vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> structureSetPatientHierarchyNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
  std::string structureSetPatientHierarchyNodeName = SlicerRtCommon::PATIENTHIERARCHY_NEW_STRUCTURE_SET_NAME + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  structureSetPatientHierarchyNode->SetName(structureSetPatientHierarchyNodeName.c_str());
  structureSetPatientHierarchyNode->AllowMultipleChildrenOn();
  structureSetPatientHierarchyNode->HideFromEditorsOff();
  structureSetPatientHierarchyNode->SetSaveWithScene(0);
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME, SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  std::string defaultStructureSetDicomUid = SlicerRtCommon::PATIENTHIERARCHY_NEW_STRUCTURE_SET_NAME + SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME;
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME, defaultStructureSetDicomUid.c_str());
  structureSetPatientHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  structureSetPatientHierarchyNode->SetParentNodeID(parentNode->GetID());
  parentNode->GetScene()->AddNode(structureSetPatientHierarchyNode);

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> structureSetPatientHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  std::string contourHierarchyDisplayNodeName = std::string(SlicerRtCommon::PATIENTHIERARCHY_NEW_STRUCTURE_SET_NAME) + "_Display";
  structureSetPatientHierarchyDisplayNode->SetName(contourHierarchyDisplayNodeName.c_str());
  structureSetPatientHierarchyDisplayNode->SetVisibility(1);

  parentNode->GetScene()->AddNode(structureSetPatientHierarchyDisplayNode);
  structureSetPatientHierarchyNode->SetAndObserveDisplayNodeID(structureSetPatientHierarchyDisplayNode->GetID());

  // Add color table node and default colors
  vtkSmartPointer<vtkMRMLColorTableNode> structureSetColorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string structureSetColorTableNodeName;
  structureSetColorTableNodeName = SlicerRtCommon::PATIENTHIERARCHY_NEW_STRUCTURE_SET_NAME + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  structureSetColorTableNodeName = parentNode->GetScene()->GenerateUniqueName(structureSetColorTableNodeName);
  structureSetColorTableNode->SetName(structureSetColorTableNodeName.c_str());
  structureSetColorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  structureSetColorTableNode->HideFromEditorsOff();
  structureSetColorTableNode->SetSaveWithScene(0);
  structureSetColorTableNode->SetTypeToUser();
  parentNode->GetScene()->AddNode(structureSetColorTableNode);

  structureSetColorTableNode->SetNumberOfColors(2);
  structureSetColorTableNode->GetLookupTable()->SetTableRange(0,1);
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_BACKGROUND, 0.0, 0.0, 0.0, 0.0); // Black background
  structureSetColorTableNode->AddColor(SlicerRtCommon::COLOR_NAME_INVALID,
    SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1],
    SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3] ); // Color indicating invalid index

  // Add color table in patient hierarchy
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchyColorTableNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  std::string phColorTableNodeName;
  phColorTableNodeName = structureSetColorTableNodeName + SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX;
  phColorTableNodeName = parentNode->GetScene()->GenerateUniqueName(phColorTableNodeName);
  patientHierarchyColorTableNode->SetName(phColorTableNodeName.c_str());
  patientHierarchyColorTableNode->HideFromEditorsOff();
  patientHierarchyColorTableNode->SetSaveWithScene(0);
  patientHierarchyColorTableNode->SetAssociatedNodeID(structureSetColorTableNode->GetID());
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME, SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchyColorTableNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SUBSERIES);
  patientHierarchyColorTableNode->SetParentNodeID(structureSetPatientHierarchyNode->GetID());
  parentNode->GetScene()->AddNode(patientHierarchyColorTableNode);

  return structureSetPatientHierarchyNode;
}

//---------------------------------------------------------------------------
const char* vtkSlicerPatientHierarchyModuleLogic::GetAttributeFromAncestor(vtkMRMLNode* sourceNode, const char* attributeName, const char* dicomLevel/*=NULL*/)
{
  if (!sourceNode)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetAttributeFromAncestor: source node is null" << std::endl;
    return NULL;
  }
  if (!attributeName)
  {
    vtkErrorWithObjectMacro(sourceNode, "GetAttributeFromAncestor: Empty attribute name!");
    return NULL;
  }

  const char* attributeValue = NULL;
  vtkMRMLHierarchyNode* hierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(sourceNode);

  while (hierarchyNode && hierarchyNode->GetParentNodeID())
  {
    hierarchyNode = hierarchyNode->GetParentNode();
    if (dicomLevel && STRCASECMP(dicomLevel, hierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME)))
    {
      continue;
    }

    attributeValue = hierarchyNode->GetAttribute(attributeName);
    if (attributeValue)
    {
      return attributeValue;
    }
  }

  return attributeValue;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::GetAncestorAtLevel(vtkMRMLNode* sourceNode, const char* dicomLevel)
{
  if (!sourceNode)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetAttributeFromAncestor: source node is null" << std::endl;
    return NULL;
  }
  if (!dicomLevel)
  {
    vtkErrorWithObjectMacro(sourceNode, "GetAttributeFromAncestor: Empty DICOM level!");
    return NULL;
  }

  vtkMRMLHierarchyNode* hierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(sourceNode);
  while (hierarchyNode && hierarchyNode->GetParentNodeID()) // We do not return source node even if it is at the requested level, we only look in the ancestors
  {
    hierarchyNode = hierarchyNode->GetParentNode();
    if (!STRCASECMP(dicomLevel, hierarchyNode->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME)))
    {
      // Level found
      return hierarchyNode;
    }
  }

  vtkWarningWithObjectMacro(sourceNode, "GetAttributeFromAncestor: No ancestor found for node '" << sourceNode->GetName() << "' at level '" << dicomLevel << "'!");
  return NULL;
}
