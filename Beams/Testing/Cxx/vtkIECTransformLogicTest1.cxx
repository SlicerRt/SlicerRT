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

// Beams includes
#include "vtkSlicerIECTransformLogic.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

// SegmentationCore includes
#include "vtkOrientedImageDataResample.h"


//----------------------------------------------------------------------------
int GetNumberOfNonIdentityTransforms(vtkMRMLScene* mrmlScene);

//----------------------------------------------------------------------------
int vtkIECTransformLogicTest1(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create and set up logic
  vtkSmartPointer<vtkSlicerIECTransformLogic> iecLogic = vtkSmartPointer<vtkSlicerIECTransformLogic>::New();
  iecLogic->BuildIECTransformHierarchy(mrmlScene);

  int expectedNumberOfLinearTransformNodes = 19;
  int numberOfLinearTransformNodes = mrmlScene->GetNumberOfNodesByClass("vtkMRMLLinearTransformNode");
  if (numberOfLinearTransformNodes != expectedNumberOfLinearTransformNodes)
  {
    std::cerr << __LINE__ << ": Number of linear transform nodes: " << numberOfLinearTransformNodes << " does not match expected value: " << expectedNumberOfLinearTransformNodes << std::endl;
    return EXIT_FAILURE;
  }

  // Print transform nodes in hierarchy
  std::cout << "Transform hierarchy successfully created:" << std::endl;
  mrmlScene->InitTraversal();
  vtkMRMLNode* node = mrmlScene->GetNextNodeByClass("vtkMRMLLinearTransformNode");
  while (node)
  {
    std::cout << "  " << node->GetName() << std::endl;
    vtkMRMLTransformNode* parentTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(node)->GetParentTransformNode();
    std::cout << "    Parent: " << (parentTransformNode?parentTransformNode->GetName():"(none)") << std::endl;

    node = mrmlScene->GetNextNodeByClass("vtkMRMLLinearTransformNode");
  }
  std::cout << std::endl;

  // Create beam node
  vtkSmartPointer<vtkMRMLRTBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  mrmlScene->AddNode(beamNode);
  // Create parent plan node and set up subject hierarchy (to prevent error messages)
  vtkSmartPointer<vtkMRMLRTPlanNode> planNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
  mrmlScene->AddNode(planNode);
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(mrmlScene);
  vtkIdType planShID = shNode->CreateItem(shNode->GetSceneItemID(), planNode);
  shNode->CreateItem(planShID, beamNode);

  // Test effect of parameter changes on the transform hierarchy

  int numOfNonIdentityTransforms = 0;
  int expectedNumOfNonIdentityTransforms = 0;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  // Gantry angle
  beamNode->SetGantryAngle(1.0);
  iecLogic->UpdateTransformForBeam(beamNode);
  expectedNumOfNonIdentityTransforms = 2;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  //TODO: Check parent transform of beam
  //TODO: Do all beam transform parameters

  std::cout << "Room's eye view logic test passed" << std::endl;
  return EXIT_SUCCESS;
}

//---------------------------------------------------------------------------
bool AreEqualWithTolerance(double a, double b)
{
  return fabs(a - b) < 0.0001;
};

//---------------------------------------------------------------------------
bool IsEqual(vtkMatrix4x4* lhs, vtkMatrix4x4* rhs)
{
  if (!lhs || !rhs)
    {
    return false;
    }
  return  AreEqualWithTolerance(lhs->GetElement(0,0), rhs->GetElement(0,0)) &&
          AreEqualWithTolerance(lhs->GetElement(0,1), rhs->GetElement(0,1)) &&
          AreEqualWithTolerance(lhs->GetElement(0,2), rhs->GetElement(0,2)) &&
          AreEqualWithTolerance(lhs->GetElement(0,3), rhs->GetElement(0,3)) &&
          AreEqualWithTolerance(lhs->GetElement(1,0), rhs->GetElement(1,0)) &&
          AreEqualWithTolerance(lhs->GetElement(1,1), rhs->GetElement(1,1)) &&
          AreEqualWithTolerance(lhs->GetElement(1,2), rhs->GetElement(1,2)) &&
          AreEqualWithTolerance(lhs->GetElement(1,3), rhs->GetElement(1,3)) &&
          AreEqualWithTolerance(lhs->GetElement(2,0), rhs->GetElement(2,0)) &&
          AreEqualWithTolerance(lhs->GetElement(2,1), rhs->GetElement(2,1)) &&
          AreEqualWithTolerance(lhs->GetElement(2,2), rhs->GetElement(2,2)) &&
          AreEqualWithTolerance(lhs->GetElement(2,3), rhs->GetElement(2,3)) &&
          AreEqualWithTolerance(lhs->GetElement(3,0), rhs->GetElement(3,0)) &&
          AreEqualWithTolerance(lhs->GetElement(3,1), rhs->GetElement(3,1)) &&
          AreEqualWithTolerance(lhs->GetElement(3,2), rhs->GetElement(3,2)) &&
          AreEqualWithTolerance(lhs->GetElement(3,3), rhs->GetElement(3,3));
}

//----------------------------------------------------------------------------
int GetNumberOfNonIdentityTransforms(vtkMRMLScene* mrmlScene)
{
  if (!mrmlScene)
  {
    return -1;
  }

  // Create identity matrix for comparison
  vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  identityMatrix->Identity();

  // Count transform nodes with identity transform
  int numberOfNonIdentityTransforms = 0;
  mrmlScene->InitTraversal();
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMRMLNode* node = mrmlScene->GetNextNodeByClass("vtkMRMLLinearTransformNode");
  while (node)
  {
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
    transformNode->GetMatrixTransformToParent(matrix);
    if (!IsEqual(matrix, identityMatrix))
    {
      ++numberOfNonIdentityTransforms;
    }

    node = mrmlScene->GetNextNodeByClass("vtkMRMLLinearTransformNode");
  }

  return numberOfNonIdentityTransforms;
}
