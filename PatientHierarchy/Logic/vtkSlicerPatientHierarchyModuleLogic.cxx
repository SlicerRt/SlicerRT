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
#include "vtkMRMLPatientHierarchyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes

// VTK includes
#include <vtkNew.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// STD includes
#include <cassert>

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

//-----------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::RegisterNodes()
{
  //vtkMRMLScene* scene = this->GetMRMLScene(); 
  //if (!scene)
  //{
  //  return;
  //}
  //scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLPatientHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode*
vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUid(
  vtkMRMLScene *scene, const char* dicomDatabaseFileName, const char* uid )
{
  if (!scene || !dicomDatabaseFileName || !uid)
  {
    return NULL;
  }

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  for (unsigned int i=0; i<numberOfNodes; i++)
  {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if (node && node->GetUid()
      && !strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName())
      && !strcmp(uid, node->GetUid()) )
    {
      return node;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
void vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy(
  vtkMRMLScene *scene, const char* dicomDatabaseFileName, 
  const char* patientId, const char* studyInstanceUid, const char* seriesInstanceUid )
{
  if ( !scene || !dicomDatabaseFileName
    || !patientId || !studyInstanceUid || !seriesInstanceUid )
  {
    return;
  }

  vtkMRMLPatientHierarchyNode* patientNode = NULL;
  vtkMRMLPatientHierarchyNode* studyNode = NULL;
  vtkMRMLPatientHierarchyNode* seriesNode = NULL;

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  // Find referenced nodes
  for (unsigned int i=0; i<numberOfNodes; i++)
  {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if ( node && node->GetUid() && !strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName()) )
    {
      if (!strcmp(patientId, node->GetUid()))
      {
        patientNode = node;
      }
      else if (!strcmp(studyInstanceUid, node->GetUid()))
      {
        studyNode = node;
      }
      else if (!strcmp(seriesInstanceUid, node->GetUid()))
      {
        seriesNode = node;
      }
    }
  }

  if (!seriesNode)
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::InsertDicomSeriesInHierarchy: Patient hierarchy node with ID="
      << patientId << " cannot be found!");
    return;
  }

  // Create patient and study nodes if they do not exist yet
  if (!patientNode)
  {
    patientNode = vtkMRMLPatientHierarchyNode::New();
    patientNode->AllowMultipleChildrenOn();
    patientNode->HideFromEditorsOff();
    patientNode->SetUid(patientId);
    patientNode->SetDicomDatabaseFileName(dicomDatabaseFileName);
    patientNode->AddTag(vtkMRMLPatientHierarchyNode::PATIENTHIERARCHY_LEVEL_PATIENT);
    scene->AddNode(patientNode);
  }

  if (!studyNode)
  {
    studyNode = vtkMRMLPatientHierarchyNode::New();
    studyNode->AllowMultipleChildrenOn();
    studyNode->HideFromEditorsOff();
    studyNode->SetUid(studyInstanceUid);
    studyNode->SetDicomDatabaseFileName(dicomDatabaseFileName);
    studyNode->AddTag(vtkMRMLPatientHierarchyNode::PATIENTHIERARCHY_LEVEL_STUDY);
    studyNode->SetParentNodeID(patientNode->GetID());
    scene->AddNode(studyNode);
  }

  seriesNode->SetParentNodeID(studyNode->GetID());
}

//---------------------------------------------------------------------------
bool vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch(vtkMRMLScene *scene,
                                                                const char* nodeId1, const char* nodeId2,
                                                                int lowestCommonLevel)
{
  if ( !scene || !nodeId1 || !nodeId2 )
  {
    return false;
  }

  if (lowestCommonLevel < 0)
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Invalid lowest common level!");
    return false;
  }

  // Get input nodes
  vtkMRMLPatientHierarchyNode* node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId1));
  vtkMRMLPatientHierarchyNode* node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId2));

  // If not hierarchy nodes, check if they have an associated patient hierarchy node
  if (!node1)
  {
    node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId1) );
  }
  if (!node2)
  {
    node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId2) );
  }

  // Check if valid nodes are found
  if (!node1 || !node2)
  {
    return false;
  }

  // If either input node is already higher level than the lowest common level then return with false
  if (node1->GetLevel() < lowestCommonLevel || node2->GetLevel() < lowestCommonLevel)
  {
    return false;
  }

  // Walk the hierarchy up until we reach the lowest common level
  while (node1->GetLevel() > lowestCommonLevel)
  {
    node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(node1->GetParentNode());
  }
  while (node2->GetLevel() > lowestCommonLevel)
  {
    node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(node2->GetParentNode());
  }

  // Sanity check
  if (!node1 || !node2)
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Invalid patient hierarchy tree - level inconsistency!");
    return false;
  }
  if ( !node1->GetUid() || !node1->GetDicomDatabaseFileName()
    || !node2->GetUid() || !node2->GetDicomDatabaseFileName() )
  {
    vtkErrorWithObjectMacro(scene,
      "vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch: Found ancestor node contains empty Instance UID or DICOM database file name!");
    return false;
  }

  // Check if the found nodes match
  // (handling the case when two patient hierarchy nodes point to the same DICOM object)
  if ( !strcmp(node1->GetUid(), node2->GetUid())
    && !strcmp(node1->GetDicomDatabaseFileName(), node2->GetDicomDatabaseFileName()) )
  {
    return true;
  }

  return false;
}
