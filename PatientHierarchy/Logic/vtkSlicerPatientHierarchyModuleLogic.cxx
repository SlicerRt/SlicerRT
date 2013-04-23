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
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"

// MRML includes
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkStringArray.h>
#include <vtkSmartPointer.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// STD includes
#include <cassert>

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
void vtkSlicerPatientHierarchyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::RegisterNodes()
{
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->CreateDefaultStructureSetNode();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::OnMRMLSceneEndClose()
{
  assert(this->GetMRMLScene() != 0);

  this->CreateDefaultStructureSetNode();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::CreateDefaultStructureSetNode()
{
  assert(this->GetMRMLScene() != 0);

  vtkSmartPointer<vtkCollection> defaultStructureSetNodes = vtkSmartPointer<vtkCollection>::Take(
    this->GetMRMLScene()->GetNodesByClassByName("vtkMRMLContourHierarchyNode", SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME) );
  if (defaultStructureSetNodes->GetNumberOfItems() > 0)
  {
    vtkWarningMacro("CreateDefaultStructureSetNode: Default structure set node already exists");
    return;
  }

  vtkSmartPointer<vtkMRMLContourHierarchyNode> contourHierarchyNode = vtkSmartPointer<vtkMRMLContourHierarchyNode>::New();
  contourHierarchyNode->SetName(SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME);
  contourHierarchyNode->AllowMultipleChildrenOn();
  contourHierarchyNode->HideFromEditorsOff();
  contourHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  contourHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  contourHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME.c_str(), SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME);
  this->GetMRMLScene()->AddNode(contourHierarchyNode);

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> contourHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  std::string contourHierarchyDisplayNodeName = std::string(SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME) + "Display";
  contourHierarchyDisplayNode->SetName(contourHierarchyDisplayNodeName.c_str());
  contourHierarchyDisplayNode->SetVisibility(1);

  this->GetMRMLScene()->AddNode(contourHierarchyDisplayNode);
  contourHierarchyNode->SetAndObserveDisplayNodeID(contourHierarchyDisplayNode->GetID());
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode*
vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(vtkMRMLScene* scene, const char* uid)
{
  if (!scene || !uid)
  {
    return NULL;
  }

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLHierarchyNode", patientHierarchyNodes);

  for (unsigned int i=0; i<numberOfNodes; i++)
  {
    vtkMRMLHierarchyNode* node = vtkMRMLHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
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
void vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
  vtkMRMLScene *scene, const char* patientId, const char* studyInstanceUID, const char* seriesInstanceUID )
{
  if ( !scene || !patientId || !studyInstanceUID || !seriesInstanceUID )
  {
    return;
  }

  vtkSmartPointer<vtkMRMLHierarchyNode> patientNode;
  vtkSmartPointer<vtkMRMLHierarchyNode> studyNode;
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
        patientNode = vtkSmartPointer<vtkMRMLHierarchyNode>::Take(node);
        patientNode->Register(NULL);
      }
      else if (!STRCASECMP(studyInstanceUID, nodeUID))
      {
        studyNode = vtkSmartPointer<vtkMRMLHierarchyNode>::Take(node);
        studyNode->Register(NULL);
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
    return;
  }

  // Create patient and study nodes if they do not exist yet
  if (!patientNode)
  {
    patientNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
    patientNode->AllowMultipleChildrenOn();
    patientNode->HideFromEditorsOff();
    patientNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
      SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
    patientNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_PATIENT);
    patientNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
      patientId);
    scene->AddNode(patientNode);
  }

  if (!studyNode)
  {
    studyNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
    studyNode->AllowMultipleChildrenOn();
    studyNode->HideFromEditorsOff();
    studyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
      SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
    studyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
      vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY);
    studyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
      studyInstanceUID);
    studyNode->SetParentNodeID(patientNode->GetID());
    scene->AddNode(studyNode);
  }

  seriesNode->SetParentNodeID(studyNode->GetID());
}

//---------------------------------------------------------------------------
bool vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch(vtkMRMLScene *scene,
                                                                const char* nodeId1, const char* nodeId2,
                                                                const char* lowestCommonLevel/*=NULL*/)
{
  if ( !scene || !nodeId1 || !nodeId2 )
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: NULL arguments given!" << std::endl;
    return false;
  }

  if (lowestCommonLevel < 0)
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Invalid lowest common level!");
    return false;
  }

  // Get input nodes
  vtkMRMLHierarchyNode* node1 = vtkMRMLHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId1));
  vtkMRMLHierarchyNode* node2 = vtkMRMLHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId2));

  // If not hierarchy nodes, check if they have an associated patient hierarchy node
  if (!node1)
  {
    node1 = vtkMRMLHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId1) );
  }
  if (!node2)
  {
    node2 = vtkMRMLHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId2) );
  }

  // Check if valid nodes are found
  if ( !node1 || SlicerRtCommon::IsPatientHierarchyNode(node1)
    || !node2 || SlicerRtCommon::IsPatientHierarchyNode(node2) )
  {
    return false;
  }

  // Walk the hierarchy up until we reach the lowest common level
  if (lowestCommonLevel)
  {
    while (true)
    {
      node1 = vtkMRMLHierarchyNode::SafeDownCast(node1->GetParentNode());
      if (!SlicerRtCommon::IsPatientHierarchyNode(node1))
      {
        node1 = NULL;
        vtkWarningWithObjectMacro(scene, "Patient hierarchy node (ID='" << nodeId1 << "') has no ancestor with DICOM level '" << lowestCommonLevel << "'");
        break;
      }
      const char* node1Level = node1->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
      if (!node1Level)
      {
        node1 = NULL;
        vtkWarningWithObjectMacro(scene, "Patient hierarchy node (ID='" << nodeId1 << "') has no DICOM level '" << lowestCommonLevel << "'");
        break;
      }
      if (!STRCASECMP(node1Level, lowestCommonLevel))
      {
        break;
      }
    }

    while (true)
    {
      node2 = vtkMRMLHierarchyNode::SafeDownCast(node2->GetParentNode());
      if (!SlicerRtCommon::IsPatientHierarchyNode(node2))
      {
        node2 = NULL;
        vtkWarningWithObjectMacro(scene, "Patient hierarchy node (ID='" << nodeId2 << "') has no ancestor with DICOM level '" << lowestCommonLevel << "'");
        break;
      }
      const char* node2Level = node1->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME);
      if (!node2Level)
      {
        node2 = NULL;
        vtkWarningWithObjectMacro(scene, "Patient hierarchy node (ID='" << nodeId2 << "') has no DICOM level '" << lowestCommonLevel << "'");
        break;
      }
      if (!STRCASECMP(node2Level, lowestCommonLevel))
      {
        break;
      }
    }
  }

  const char* node1UID = node1->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME);
  const char* node2UID = node2->GetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME);
  if ( !node1UID || !node2UID )
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Found ancestor node contains empty UID!");
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

  node->GetScene()->StartState(vtkMRMLScene::BatchProcessState);

  vtkSmartPointer<vtkCollection> childDisplayableNodes = vtkSmartPointer<vtkCollection>::New();
  node->GetAssociatedChildrendNodes(childDisplayableNodes, "vtkMRMLDisplayableNode");
  childDisplayableNodes->InitTraversal();
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
      // Set Modified flag for all parent nodes so that their icons are refreshed in the tree view
      vtkSlicerPatientHierarchyModuleLogic::SetModifiedToAllAncestors(displayableNode);
    }
  }

  node->GetScene()->EndState(vtkMRMLScene::BatchProcessState);
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::SetModifiedToAllAncestors(vtkMRMLNode* node)
{
  std::set<vtkMRMLHierarchyNode*> parentNodes;
  vtkMRMLHierarchyNode* parentNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode( node->GetScene(), node->GetID() );
  do 
  {
    parentNodes.insert(parentNode);
  }
  while ((parentNode = parentNode->GetParentNode())); // The double parentheses avoids a Linux build warning

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
    if (displayableNode && !displayableNode->IsA("vtkMRMLVolumeNode"))
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
bool vtkSlicerPatientHierarchyModuleLogic::IsPotentialPatientHierarchyNode(vtkMRMLNode* node)
{
  //TODO: Add beam and plan nodes once they are done
  if ( node->IsA("vtkMRMLContourNode")
    || node->IsA("vtkMRMLModelNode") // vtkMRMLModelNode includes annotations too
    || node->IsA("vtkMRMLVolumeNode") )
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(vtkMRMLScene *scene, const char *associatedNodeId, bool reverseCriterion/*=false*/)
{
  if (associatedNodeId == 0)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode: associated node id is null" << std::endl;
    return NULL;
  }
  if (scene == 0)
  {
    std::cerr << "vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode: scene is null" << std::endl;
    return NULL;
  }

  vtkSmartPointer<vtkCollection> hierarchyNodes = vtkSmartPointer<vtkCollection>::Take( scene->GetNodesByClass("vtkMRMLHierarchyNode") );
  vtkObject* nextObject = NULL;
  for (hierarchyNodes->InitTraversal(); (nextObject = hierarchyNodes->GetNextItemAsObject()); )
  {
    vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(nextObject);
    if (hierarchyNode)
    {
      const char* currentAssociatedNodeId = hierarchyNode->GetAssociatedNodeID();
      if ( currentAssociatedNodeId && !STRCASECMP(currentAssociatedNodeId, associatedNodeId)
        && (SlicerRtCommon::IsPatientHierarchyNode(hierarchyNode) & !reverseCriterion) )
      {
        return hierarchyNode;
      }
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerPatientHierarchyModuleLogic::GetAssociatedNonPatientHierarchyNode( vtkMRMLScene *scene, const char *associatedNodeId )
{
  return vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(scene, associatedNodeId, true);
}
