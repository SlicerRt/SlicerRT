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
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkSlicerIECTransformLogic.h"
#include "vtkSlicerBeamsModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>


//----------------------------------------------------------------------------
/// Get all linear transforms from the scene that are not identity, and are not beam transform nodes
bool GetLinearTransformNodes(
  vtkMRMLScene* mrmlScene, std::vector<vtkMRMLLinearTransformNode*> &transformNodes,
  bool includeIdentity=true, bool includeBeamTransforms=true );
/// Determine whether a transform node is a beam transform
bool IsBeamTransformNode(vtkMRMLTransformNode* node);

int GetNumberOfNonIdentityIECTransforms(vtkMRMLScene* mrmlScene);
void PrintLinearTransformNodeMatrices( vtkMRMLScene* mrmlScene,
  bool includeIdentity=true, bool includeBeamTransforms=true );

bool IsTransformMatrixEqualTo(vtkMRMLScene* mrmlScene, vtkMRMLLinearTransformNode* transformNode, double baselineElements[16]);
bool AreEqualWithTolerance(double a, double b);
bool IsEqual(vtkMatrix4x4* lhs, vtkMatrix4x4* rhs);

//----------------------------------------------------------------------------
int vtkSlicerIECTransformLogicTest1(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create and set up logic classes
  vtkSmartPointer<vtkSlicerIECTransformLogic> iecLogic = vtkSmartPointer<vtkSlicerIECTransformLogic>::New();
  iecLogic->SetMRMLScene(mrmlScene);
  iecLogic->BuildIECTransformHierarchy();
  vtkSmartPointer<vtkSlicerBeamsModuleLogic> beamsLogic = vtkSmartPointer<vtkSlicerBeamsModuleLogic>::New();
  beamsLogic->SetMRMLScene(mrmlScene);

  int expectedNumberOfLinearTransformNodes = 13;
  int numberOfLinearTransformNodes = mrmlScene->GetNumberOfNodesByClass("vtkMRMLLinearTransformNode");
  if (numberOfLinearTransformNodes != expectedNumberOfLinearTransformNodes)
  {
    std::cerr << __LINE__ << ": Number of linear transform nodes: " << numberOfLinearTransformNodes << " does not match expected value: " << expectedNumberOfLinearTransformNodes << std::endl;
    return EXIT_FAILURE;
  }

  // Print transform nodes in hierarchy
  std::cout << "Transform hierarchy successfully created (" << numberOfLinearTransformNodes << " transforms):" << std::endl;
  std::vector<vtkMRMLLinearTransformNode*> transformNodes;
  GetLinearTransformNodes(mrmlScene, transformNodes);
  std::vector<vtkMRMLLinearTransformNode*>::iterator trIt;
  for (trIt=transformNodes.begin(); trIt!=transformNodes.end(); ++trIt)
  {
    vtkMRMLLinearTransformNode* node = (*trIt);
    std::cout << "  " << node->GetName() << std::endl;
    vtkMRMLTransformNode* parentTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(node)->GetParentTransformNode();
    std::cout << "    Parent: " << (parentTransformNode?parentTransformNode->GetName():"(none)") << std::endl;
  }
  std::cout << std::endl;

  // Create beam node
  vtkSmartPointer<vtkMRMLRTBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  mrmlScene->AddNode(beamNode);
  // Create parent plan node and add beam to it (setup subject hierarchy)
  vtkSmartPointer<vtkMRMLRTPlanNode> planNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
  mrmlScene->AddNode(planNode);
  planNode->AddBeam(beamNode);

  vtkMRMLLinearTransformNode* beamTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(beamNode->GetParentTransformNode());
  if (!beamTransformNode)
  {
    std::cerr << __LINE__ << ": Beam node does not have a valid beam transform node" << std::endl;
    return EXIT_FAILURE;
  }

  //
  // Test effect of parameter changes on the transform hierarchy

  // Isocenter position, origin
  int numOfNonIdentityTransforms = 0;
  int expectedNumOfNonIdentityTransforms = 2;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityIECTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  double expectedFixedReferenceToRasTransform_Origin_MatrixElements[16] =
    {  -1, 0, 0, 0,   0, 0, 1, 0,   0, 1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      iecLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::FixedReference, vtkSlicerIECTransformLogic::RAS),
      expectedFixedReferenceToRasTransform_Origin_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": FixedReferenceToRasTransform transform does not match baseline for origin" << std::endl;
    return EXIT_FAILURE;
    }

  double expectedBeamTransform_IsocenterOrigin_MatrixElements[16] =
    {  -1, 0, 0, 0,   0, 0, 1, 0,   0, 1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      beamTransformNode, expectedBeamTransform_IsocenterOrigin_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": Beam transform does not match baseline for isocenter at origin" << std::endl;
    return EXIT_FAILURE;
    }

  // Isocenter position, translated
  double isocenter[3] = {1000.0, 200.0, 0.0};
  planNode->SetIsocenterPosition(isocenter);
  iecLogic->UpdateBeamTransform(beamNode);

  double expectedFixedReferenceToRasTransform_Translated_MatrixElements[16] =
    {  -1, 0, 0, 1000,   0, 0, 1, 200,   0, 1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      iecLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::FixedReference, vtkSlicerIECTransformLogic::RAS),
      expectedFixedReferenceToRasTransform_Translated_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": FixedReferenceToRasTransform transform does not match baseline for translated isocenter" << std::endl;
    return EXIT_FAILURE;
    }

  double expectedBeamTransform_IsocenterTranslated_MatrixElements[16] =
    {  -1, 0, 0, 1000,   0, 0, 1, 200,   0, 1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      beamTransformNode, expectedBeamTransform_IsocenterTranslated_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": Beam transform does not match baseline for translated isocenter" << std::endl;
    return EXIT_FAILURE;
    }

  // Gantry angle, 1 degree
  beamNode->SetGantryAngle(1.0);
  iecLogic->UpdateBeamTransform(beamNode);
  expectedNumOfNonIdentityTransforms = 3;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityIECTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  double expectedGantryToFixedReferenceTransform_1_MatrixElements[16] =
    {  0.999848, 0, 0.0174524, 0,   0, 1, 0, 0,   -0.0174524, 0, 0.999848, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      iecLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference),
      expectedGantryToFixedReferenceTransform_1_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": GantryToFixedReferenceTransform does not match baseline for 1 degree angle" << std::endl;
    return EXIT_FAILURE;
    }

  double expectedBeamTransform_Gantry1_MatrixElements[16] =
    {  -0.999848, 0, -0.0174524, 1000,   -0.0174524, 0, 0.999848, 200,   0, 1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      beamTransformNode, expectedBeamTransform_Gantry1_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": Beam transform does not match baseline for gantry 1 degree angle" << std::endl;
    return EXIT_FAILURE;
    }

  // Gantry angle, 90 degrees
  beamNode->SetGantryAngle(90.0);
  iecLogic->UpdateBeamTransform(beamNode);
  double expectedGantryToFixedReference_90_MatrixElements[16] =
    {  0, 0, 1, 0,   0, 1, 0, 0,   -1, 0, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      iecLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Gantry, vtkSlicerIECTransformLogic::FixedReference),
      expectedGantryToFixedReference_90_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": GantryToFixedReferenceTransform does not match baseline for 90 degrees angle" << std::endl;
    return EXIT_FAILURE;
    }

  double expectedBeamTransform_Gantry90_MatrixElements[16] =
    {  0, 0, -1, 1000,   -1, 0, 0, 200,   0, 1, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      beamTransformNode, expectedBeamTransform_Gantry90_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": Beam transform does not match baseline for gantry 90 degree angle" << std::endl;
    return EXIT_FAILURE;
    }

  // Collimator angle, 1 degree
  //TODO: Different transform after consolidation
  beamNode->SetCollimatorAngle(1.0);
  iecLogic->UpdateBeamTransform(beamNode);
  expectedNumOfNonIdentityTransforms = 4;
  if ((numOfNonIdentityTransforms = GetNumberOfNonIdentityIECTransforms(mrmlScene)) != expectedNumOfNonIdentityTransforms)
    {
    std::cerr << __LINE__ << ": Number of non-identity linear transforms: " << numOfNonIdentityTransforms << " does not match expected value: " << expectedNumOfNonIdentityTransforms << std::endl;
    return EXIT_FAILURE;
    }

  double expectedCollimatorToGantryTransform_1_MatrixElements[16] =
    {  0.999848, -0.0174524, 0, 0,   0.0174524, 0.999848, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      iecLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry),
      expectedCollimatorToGantryTransform_1_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": CollimatorToGantry does not match baseline" << std::endl;
    return EXIT_FAILURE;
    }

  double expectedBeamTransform_Collimator1_MatrixElements[16] =
    {  0, 0, -1, 1000,   -0.999848, 0.0174524, 0, 200,   0.0174524, 0.999848, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      beamTransformNode, expectedBeamTransform_Collimator1_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": Beam transform does not match baseline for collimator 1 degree angle" << std::endl;
    return EXIT_FAILURE;
    }

  // Collimator angle, 90 degrees
  beamNode->SetCollimatorAngle(90.0);
  iecLogic->UpdateBeamTransform(beamNode);
  double expectedCollimatorToGantryTransform_90_MatrixElements[16] =
    {  0, -1, 0, 0,   1, 0, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      iecLogic->GetTransformNodeBetween(vtkSlicerIECTransformLogic::Collimator, vtkSlicerIECTransformLogic::Gantry),
      expectedCollimatorToGantryTransform_90_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": CollimatorToGantry does not match baseline" << std::endl;
    return EXIT_FAILURE;
    }

  double expectedBeamTransform_Collimator90_MatrixElements[16] =
    {  0, 0, -1, 1000,   0, 1, 0, 200,   1, 0, 0, 0,   0, 0, 0, 1  };
  if ( !IsTransformMatrixEqualTo(mrmlScene,
      beamTransformNode, expectedBeamTransform_Collimator90_MatrixElements ) )
    {
    std::cerr << __LINE__ << ": Beam transform does not match baseline for collimator 90 degree angle" << std::endl;
    return EXIT_FAILURE;
    }

  //TODO: Test code to print all non-identity transforms (useful to add more test cases)
  //std::cout << "ZZZ after collimator angle 90:" << std::endl;
  //PrintLinearTransformNodeMatrices(mrmlScene, false, true);

  std::cout << "IEC logic test passed" << std::endl;
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int GetNumberOfNonIdentityIECTransforms(vtkMRMLScene* mrmlScene)
{
  std::vector<vtkMRMLLinearTransformNode*> nonIdentityIECTransforms;
  if (!GetLinearTransformNodes(mrmlScene, nonIdentityIECTransforms, false, false))
  {
    return -1;
  }

  return nonIdentityIECTransforms.size();
}

//----------------------------------------------------------------------------
void PrintLinearTransformNodeMatrices(vtkMRMLScene* mrmlScene,
  bool includeIdentity/*=true*/, bool includeBeamTransforms/*=true*/ )
{
  std::vector<vtkMRMLLinearTransformNode*> transformNodes;
  GetLinearTransformNodes(mrmlScene, transformNodes, includeIdentity, includeBeamTransforms);
  if (!transformNodes.size())
  {
    std::cout << "There are no non-identity IEC transform in the scene" << std::endl;
    return;
  }
  std::cout << "Printing " << transformNodes.size() << " linear transform nodes in the scene (include identity transforms:"
    << (includeIdentity?"true":"false") << ", include beam transforms:" << (includeBeamTransforms?"true":"false") << std::endl;

  // Print linear transform node matrices for nodes that fulfill the conditions
  std::vector<vtkMRMLLinearTransformNode*>::iterator trIt;
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (trIt=transformNodes.begin(); trIt!=transformNodes.end(); ++trIt)
  {
    vtkMRMLLinearTransformNode* transformNode = (*trIt);
    if (!transformNode)
    {
      continue;
    }

    std::cout << "- " << transformNode->GetName() << std::endl << "    {";
    transformNode->GetMatrixTransformToParent(matrix);
    for (int i = 0; i < 4; i++)
    {
      std::cout << "  ";
      for (int j = 0; j < 4; j++)
      {
        std::cout << matrix->GetElement(i,j) << ((i==3&&j==3)?"  ":", ");
      }
    }
    std::cout << "}" << std::endl;
  }
}

//----------------------------------------------------------------------------
bool IsBeamTransformNode(vtkMRMLTransformNode* node)
{
  if (!node)
  {
    return false;
  }

  std::string nodeName(node->GetName());
  std::string postfix = nodeName.substr(nodeName.size() - strlen(vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX), strlen(vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX));
  return !postfix.compare(vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX);
}

//----------------------------------------------------------------------------
bool GetLinearTransformNodes(
  vtkMRMLScene* mrmlScene, std::vector<vtkMRMLLinearTransformNode*> &transformNodes,
  bool includeIdentity/*=true*/, bool includeBeamTransforms/*=true*/ )
{
  if (!mrmlScene)
  {
    return false;
  }

  transformNodes.clear();

  // Create identity matrix for comparison
  vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  identityMatrix->Identity();

  // Collect transform nodes that fulfill the conditions
  std::vector<vtkMRMLNode*> nodes;
  mrmlScene->GetNodesByClass("vtkMRMLLinearTransformNode", nodes);
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (std::vector<vtkMRMLNode*>::iterator nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
  {
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(*nodeIt);
    transformNode->GetMatrixTransformToParent(matrix);
    if ( (includeIdentity || !IsEqual(matrix, identityMatrix))
      && (includeBeamTransforms || !IsBeamTransformNode(transformNode)) )
    {
      transformNodes.push_back(transformNode);
    }
  }

  return true;
}

//---------------------------------------------------------------------------
bool IsTransformMatrixEqualTo(vtkMRMLScene* mrmlScene, vtkMRMLLinearTransformNode* transformNode, double baselineElements[16])
{
  if (!mrmlScene || !transformNode)
  {
    return false;
  }

  // Verify that transform in transform node is linear
  vtkSmartPointer<vtkTransform> linearTransform = vtkSmartPointer<vtkTransform>::New();
  if (!vtkMRMLTransformNode::IsGeneralTransformLinear(transformNode->GetTransformToParent(), linearTransform))
  {
    std::cerr << __LINE__ << ": Non-linear transform found in transform node: " << transformNode->GetName() << std::endl;
    return false;
  }

  // Create matrix from elements
  vtkSmartPointer<vtkMatrix4x4> baselineMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      baselineMatrix->SetElement(i,j, baselineElements[4*i+j]);
    }
  }

  return IsEqual(linearTransform->GetMatrix(), baselineMatrix);
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
