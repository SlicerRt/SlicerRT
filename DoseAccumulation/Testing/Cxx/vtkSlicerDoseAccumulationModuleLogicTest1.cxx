/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationModuleLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// Subject hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkMatrix4x4.h>
#include <vtkImageMathematics.h>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
int vtkSlicerDoseAccumulationModuleLogicTest1( int argc, char * argv[] )
{
  int argIndex = 1;

  // TestSceneFile
  const char *testSceneFileName  = nullptr;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TestSceneFile") == 0)
    {
      testSceneFileName = argv[argIndex+1];
      std::cout << "Test MRML scene file name: " << testSceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      testSceneFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // TemporarySceneFile
  const char *temporarySceneFileName = nullptr;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporarySceneFile") == 0)
    {
      temporarySceneFileName = argv[argIndex+1];
      std::cout << "Temporary scene file name: " << temporarySceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporarySceneFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // DoseDifferenceCriterion
  double doseDifferenceCriterion = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-DoseDifferenceCriterion") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      doseDifferenceCriterion = doubleValue;
      std::cout << "Dose difference criterion: " << doseDifferenceCriterion << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  // Constraint the criteria to be greater than zero
  if (doseDifferenceCriterion == 0.0)
  {
    doseDifferenceCriterion = EPSILON;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();
  // Trigger resolving subject hierarchies after import (merging the imported one with the pseudo-singleton one).
  // Normally this is done by the plugin logic, but it is a Qt class, so we need to trigger it manually from a VTK-only environment.
  vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(mrmlScene);

  // Save it to the temporary directory
  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Get dose volume
  vtkSmartPointer<vtkCollection> doseVolumeNodes = vtkSmartPointer<vtkCollection>::Take(
    mrmlScene->GetNodesByName("Dose") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    std::cerr << "ERROR: Failed to get dose volume!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* doseScalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));

  // Create second dose volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseScalarVolumeNode2 = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  doseScalarVolumeNode2->SetName("Dose2");
  doseScalarVolumeNode2->Copy(doseScalarVolumeNode);
  mrmlScene->AddNode(doseScalarVolumeNode2);

  // Create output dose volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  outputVolumeNode->SetName("OutputDose");

  mrmlScene->AddNode(outputVolumeNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLDoseAccumulationNode> paramNode = vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New();
  mrmlScene->AddNode(paramNode);
  
  paramNode->AddSelectedInputVolumeNode(doseScalarVolumeNode, 0.5);
  paramNode->AddSelectedInputVolumeNode(doseScalarVolumeNode2, 0.5);
  paramNode->SetAndObserveAccumulatedDoseVolumeNode(outputVolumeNode);
  paramNode->SetAndObserveReferenceDoseVolumeNode(doseScalarVolumeNode);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic> doseAccumulationLogic = vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic>::New();
  doseAccumulationLogic->SetMRMLScene(mrmlScene);

  // Compute DoseAccumulation
  std::string errorMessage = doseAccumulationLogic->AccumulateDoseVolumes(paramNode);
  if (!errorMessage.empty())
  {
    std::cerr << "ERROR: " << errorMessage << std::endl;
    return EXIT_FAILURE;
  }

  // Get output volume
  vtkMRMLScalarVolumeNode* accumulatedDoseVolumeNode = paramNode->GetAccumulatedDoseVolumeNode();
  if (accumulatedDoseVolumeNode == nullptr)
  {
    mrmlScene->Commit();
    std::cerr << "ERROR: Unable to get accumulated volume!" << std::endl;
    return EXIT_FAILURE;
  }

  mrmlScene->Commit();

  // Check if the output volume is in the study of the reference dose volume
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(mrmlScene);
  if (!shNode)
  {
    std::cerr << "ERROR: Failed to access subject hierarchy node" << std::endl;
    return EXIT_FAILURE;
  }
  if ( vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch(
         doseScalarVolumeNode, accumulatedDoseVolumeNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy())
       == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID )
  {
    std::cerr << "ERROR: Accumulated volume is not under the same study as the reference dose volume" << std::endl;
    return EXIT_FAILURE;
  }

  // Subtract the dose volume from the accumulated volume and check if we get back the original dose volume
  // TODO: Add test that dose the same thing using different weights
  vtkSmartPointer<vtkImageMathematics> math = vtkSmartPointer<vtkImageMathematics>::New();
  math->SetInput1Data(doseScalarVolumeNode->GetImageData());
  math->SetInput2Data(accumulatedDoseVolumeNode->GetImageData());
  math->SetOperationToSubtract();
  math->Update();

  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInputData(math->GetOutput());
  histogram->Update();
  double maxDiff = histogram->GetMax()[0];
  double minDiff = histogram->GetMin()[0];

  if (maxDiff > doseDifferenceCriterion || minDiff < -doseDifferenceCriterion)
  {
    std::cerr << "ERROR: Difference between baseline and accumulated dose exceeds threshold" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

