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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationModuleLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkMatrix4x4.h>
#include <vtkImageMathematics.h>
#ifdef WIN32
  #include <vtkWin32OutputWindow.h>
#endif

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
  const char *testSceneFileName  = NULL;
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
  const char *temporarySceneFileName = NULL;
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
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Direct vtk messages on standard output
  //TODO: Remove when supported by the test driver (http://www.na-mic.org/Bug/view.php?id=3221)
#ifdef WIN32
  vtkWin32OutputWindow* outputWindow = vtkWin32OutputWindow::SafeDownCast(vtkOutputWindow::GetInstance());
  if (outputWindow)
  {
    outputWindow->SendToStdErrOn();
  }
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();

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
  
  paramNode->GetSelectedInputVolumeIds()->insert(doseScalarVolumeNode->GetID());
  paramNode->GetSelectedInputVolumeIds()->insert(doseScalarVolumeNode2->GetID());
  std::map<std::string,double>* volumeNodeIdsToWeightsMap = paramNode->GetVolumeNodeIdsToWeightsMap();
  std::string doseVolumeId(doseScalarVolumeNode->GetID());
  (*volumeNodeIdsToWeightsMap)[doseScalarVolumeNode->GetID()] = 0.5;
  (*volumeNodeIdsToWeightsMap)[doseScalarVolumeNode2->GetID()] = 0.5;
  paramNode->SetAndObserveAccumulatedDoseVolumeNodeId(outputVolumeNode->GetID());
  paramNode->SetAndObserveReferenceDoseVolumeNodeId(doseScalarVolumeNode->GetID());

  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic> doseAccumulationLogic = vtkSmartPointer<vtkSlicerDoseAccumulationModuleLogic>::New();
  doseAccumulationLogic->SetMRMLScene(mrmlScene);
  doseAccumulationLogic->SetAndObserveDoseAccumulationNode(paramNode);

  // Compute DoseAccumulation
  std::string errorMessage;
  doseAccumulationLogic->AccumulateDoseVolumes(errorMessage);

  if (!errorMessage.empty())
  {
    std::cerr << "ERROR: " << errorMessage << std::endl;
    return EXIT_FAILURE;
  }

  // Get output volume
  vtkSmartPointer<vtkMRMLVolumeNode> accumulatedDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(paramNode->GetAccumulatedDoseVolumeNodeId()));  
  if (accumulatedDoseVolumeNode == NULL)
  { 
    mrmlScene->Commit();
    std::cerr << "ERROR: Unable to get accumulated volume!" << std::endl;
    return EXIT_FAILURE;
  }

  mrmlScene->Commit();

  // Check if the output volume is in the study of the reference dose volume
  if (!vtkSlicerPatientHierarchyModuleLogic::AreNodesInSameBranch(doseScalarVolumeNode, accumulatedDoseVolumeNode, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY))
  {
    std::cerr << "ERROR: Accumulated volume is not under the same study as the reference dose volume!" << std::endl;
    return EXIT_FAILURE;
  }

  // Subtract the dose volume from the accumulated volume and check if we get back the original dose volume
  // TODO: Add test that dose the same thing using different weights
  vtkSmartPointer<vtkImageMathematics> math = vtkSmartPointer<vtkImageMathematics>::New();
  math->SetInput1(doseScalarVolumeNode->GetImageData());
  math->SetInput2(accumulatedDoseVolumeNode->GetImageData());
  math->SetOperationToSubtract();
  math->Update();

  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(math->GetOutput());
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

