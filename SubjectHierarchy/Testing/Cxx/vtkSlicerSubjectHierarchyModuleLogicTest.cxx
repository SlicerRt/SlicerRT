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

// Subject Hierarchy includes
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkPolyData.h>
#include <vtksys/SystemTools.hxx>


// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLModelHierarchyNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

// STD includes
#include <cassert>
#include <sstream>

namespace
{
  void PopulateScene(vtkMRMLScene* scene);

  bool TestExpand();
  bool TestNodeAccess();
  bool TestNodeAssociations();
  bool TestTreeOperations();
  bool TestInsertDicomSeriesEmpty();
  bool TestInsertDicomSeriesPopulated();
  bool TestVisibilityOperations();
  bool TestTransformBranch();

  const char* STUDY_ATTRIBUTE_NAME = "TestStudyAttribute";
  const char* STUDY_ATTRIBUTE_VALUE = "1";
  const char* UID_NAME = "DICOM";
  const char* PATIENT_UID_VALUE = "PATIENT";
  const char* STUDY1_UID_VALUE = "STUDY1";
  const char* STUDY2_UID_VALUE = "STUDY2";
  const char* VOLUME1_UID_VALUE = "VOLUME1";
  const char* VOLUME2_UID_VALUE = "VOLUME2";
  const char* MODEL2_UID_VALUE = "MODEL2";

} // end of anonymous namespace

//---------------------------------------------------------------------------
int vtkSlicerSubjectHierarchyModuleLogicTest(int vtkNotUsed(argc),
                                              char * vtkNotUsed(argv)[] )
{
  if (!TestExpand())
  {
    std::cerr << "'TestExpand' call not successful." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

namespace
{
  //---------------------------------------------------------------------------
  // Populate a sample subject hierarchy scene
  // Scene
  //  + SubjectHierarchyNode
  //     |    (Patient)
  //     +- SubjectHierarchyNode
  //     |   |    (Study)
  //     |   +- SubjectHierarchyNode -- ScalarVolumeNode
  //     |            (Series)                  > DisplayNode
  //     +- SubjectHierarchyNode
  //         |    (Study)
  //         +- SubjectHierarchyNode -- ScalarVolumeNode
  //         |        (Series)                  > DisplayNode
  //         +- ModelHierarchyNode -- SubjectHierarchyNode -- ModelNode
  //           (nested association)         (Series)              > DisplayNode
  //
  void PopulateScene(vtkMRMLScene* scene)
  {
    // Create patient and studies
    vtkMRMLSubjectHierarchyNode* patientShNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, NULL, vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT, "Patient");
    patientShNode->AddUID(UID_NAME, PATIENT_UID_VALUE);

    vtkMRMLSubjectHierarchyNode* study1ShNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, patientShNode, vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY, "Study1");
    study1ShNode->AddUID(UID_NAME, STUDY1_UID_VALUE);

    vtkMRMLSubjectHierarchyNode* study2ShNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, patientShNode, vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY, "Study2");
    study2ShNode->AddUID(UID_NAME, STUDY2_UID_VALUE);
    study2ShNode->SetAttribute(STUDY_ATTRIBUTE_NAME, STUDY_ATTRIBUTE_VALUE);

    // Create volume series in study 1
    vtkSmartPointer<vtkMRMLScalarVolumeNode> volume1Node =
      vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    volume1Node->SetName("Volume1");
    scene->AddNode(volume1Node);

    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volume1DisplayNode =
      vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    scene->AddNode(volume1DisplayNode);
    volume1Node->SetAndObserveDisplayNodeID(volume1DisplayNode->GetID());

    vtkMRMLSubjectHierarchyNode* volume1SeriesShNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, study1ShNode, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES,
      volume1Node->GetName(), volume1Node);
    volume1SeriesShNode->AddUID(UID_NAME, VOLUME1_UID_VALUE);

    // Create volume series in study 2
    vtkSmartPointer<vtkMRMLScalarVolumeNode> volume2Node =
      vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    volume2Node->SetName("Volume2");
    scene->AddNode(volume2Node);

    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> volume2DisplayNode =
      vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    scene->AddNode(volume2DisplayNode);
    volume2Node->SetAndObserveDisplayNodeID(volume2DisplayNode->GetID());

    vtkMRMLSubjectHierarchyNode* volume2SeriesShNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, study2ShNode, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES,
      volume2Node->GetName(), volume2Node);
    volume2SeriesShNode->AddUID(UID_NAME, VOLUME2_UID_VALUE);

    // Create model series in study 2 with nested association
    vtkSmartPointer<vtkMRMLModelNode> model2Node =
      vtkSmartPointer<vtkMRMLModelNode>::New();
    model2Node->SetName("Model2");
    scene->AddNode(model2Node);

    vtkSmartPointer<vtkMRMLModelDisplayNode> model2DisplayNode =
      vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    scene->AddNode(model2DisplayNode);
    model2Node->SetAndObserveDisplayNodeID(model2DisplayNode->GetID());

    vtkSmartPointer<vtkMRMLModelHierarchyNode> model2ModelHierarchyNode =
      vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
    model2ModelHierarchyNode->SetName("Model2_Hierarchy");
    model2ModelHierarchyNode->SetDisplayableNodeID(model2Node->GetID());
    //model2ModelHierarchyNode->SetParentNodeID(modelHierarchyRootNode->GetID()); // No parent node needed to test nested associations
    scene->AddNode(model2ModelHierarchyNode);

    vtkMRMLSubjectHierarchyNode* model2SeriesShNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      scene, study2ShNode, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES,
      model2Node->GetName(), model2ModelHierarchyNode);
    model2SeriesShNode->AddUID(UID_NAME, MODEL2_UID_VALUE);

    assert(scene->GetNumberOfNodes() == 13);
  }

  //---------------------------------------------------------------------------
  bool TestExpand()
  {
    if (!TestNodeAccess())
    {
      std::cerr << "'TestNodeAccess' call not successful." << std::endl;
      return false;
    }
    if (!TestNodeAssociations())
    {
      std::cerr << "'TestNodeAssociations' call not successful." << std::endl;
      return false;
    }
    if (!TestTreeOperations())
    {
      std::cerr << "'TestTreeOperations' call not successful." << std::endl;
      return false;
    }
    if (!TestInsertDicomSeriesEmpty())
    {
      std::cerr << "'TestInsertDicomSeriesEmpty' call not successful." << std::endl;
      return false;
    }
    if (!TestInsertDicomSeriesPopulated())
    {
      std::cerr << "'TestInsertDicomSeriesPopulated' call not successful." << std::endl;
      return false;
    }
    if (!TestVisibilityOperations())
    {
      std::cerr << "'TestVisibilityOperations' call not successful." << std::endl;
      return false;
    }
    if (!TestTransformBranch())
    {
      std::cerr << "'TestTransformBranch' call not successful." << std::endl;
      return false;
    }
    return true;
  }

  //---------------------------------------------------------------------------
  bool TestNodeAccess()
  {
    vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
    PopulateScene(scene);

    // Get node by UID
    vtkMRMLSubjectHierarchyNode* patientNode =
      vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(scene, UID_NAME, PATIENT_UID_VALUE);
    if (!patientNode)
    {
      std::cout << "Failed to get patient by UID" << std::endl;
      return false;
    }
    // Check level
    if (strcmp(patientNode->GetLevel(), vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT))
    {
      std::cout << "Wrong level of found node!" << std::endl;
      return false;
    }

    return true;
  }

  //---------------------------------------------------------------------------
  bool TestNodeAssociations()
  {
    vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
    PopulateScene(scene);

    // Get volume node for testing simple association
    vtkMRMLSubjectHierarchyNode* volume1ShNode =
      vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(scene, UID_NAME, VOLUME1_UID_VALUE);
    if (!volume1ShNode)
    {
      std::cout << "Failed to get volume by UID" << std::endl;
      return false;
    }

    // Get associated data node simple case
    vtkMRMLScalarVolumeNode* volume1Node = vtkMRMLScalarVolumeNode::SafeDownCast(
      volume1ShNode->GetAssociatedDataNode() );
    if (!volume1Node)
    {
      std::cout << "Failed to get associated volume node (simple association)" << std::endl;
      return false;
    }

    // Get associated subject hierarchy node simple case
    vtkMRMLSubjectHierarchyNode* volume1ShNodeTest =
      vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(volume1Node);
    if (volume1ShNodeTest != volume1ShNode)
    {
      std::cout << "Failed to get associated subject hierarchy node for volume (simple association)" << std::endl;
      return false;
    }

    // Get model node for testing nested association
    vtkMRMLSubjectHierarchyNode* model2ShNode =
      vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(scene, UID_NAME, MODEL2_UID_VALUE);
    if (!model2ShNode)
    {
      std::cout << "Failed to get model by UID" << std::endl;
      return false;
    }

    // Get associated data node nested case
    vtkMRMLModelNode* model2Node = vtkMRMLModelNode::SafeDownCast(
      model2ShNode->GetAssociatedDataNode() );
    if (!model2Node)
    {
      std::cout << "Failed to get associated model node (nested association)" << std::endl;
      return false;
    }

    // Get associated subject hierarchy node nested case
    vtkMRMLSubjectHierarchyNode* model2ShNodeTest =
      vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(model2Node);
    if (model2ShNodeTest != model2ShNode)
    {
      std::cout << "Failed to get associated subject hierarchy node for model (nested association)" << std::endl;
      return false;
    }

    // Get simple hierarchy node for nested association
    vtkMRMLHierarchyNode* model2ModelHierarchyNode =
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, model2Node->GetID());
    if (!model2ModelHierarchyNode || !model2ModelHierarchyNode->IsA("vtkMRMLModelHierarchyNode"))
    {
      std::cout << "Failed to get associated model hierarchy node for model (directly associated hierarchy node in nested association)" << std::endl;
      return false;
    }

    return true;
  }

  //---------------------------------------------------------------------------
  bool TestTreeOperations()
  {
    vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
    PopulateScene(scene);

    // vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch
    // GetChildWithName
    // GetAssociatedChildrenNodes
    // GetAttributeFromAncestor
    // GetAncestorAtLevel
    return true;
  }

  //---------------------------------------------------------------------------
  bool TestInsertDicomSeriesEmpty()
  {
    return true;
  }

  //---------------------------------------------------------------------------
  bool TestInsertDicomSeriesPopulated()
  {
    vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
    PopulateScene(scene);

    return true;
  }

  //---------------------------------------------------------------------------
  bool TestVisibilityOperations()
  {
    vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
    PopulateScene(scene);

    return true;
  }

  //---------------------------------------------------------------------------
  bool TestTransformBranch()
  {
    vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
    PopulateScene(scene);

    // IsAnyNodeInBranchTransformed
    // TransformBranch
    // HardenTransformOnBranch
    return true;
  }

} // end of anonymous namespace
