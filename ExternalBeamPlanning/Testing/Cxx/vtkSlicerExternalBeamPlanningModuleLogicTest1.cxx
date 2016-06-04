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

  This file was originally developed by Csaba Pinter, Perk Lab, Queen's University 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ExternalBeamPlanning includes
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTProtonBeamNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Subject hierarchy includes
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkSlicerSubjectHierarchyModuleLogic.h>

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkImageMathematics.h>
#include <vtkTimerLog.h>

// ITK includes
#include "itkFactoryRegistration.h"

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
bool IsEqualWithTolerance(double a, double b) { return fabs(a - b) < 0.0001; };

//-----------------------------------------------------------------------------
int vtkSlicerExternalBeamPlanningModuleLogicTest1( int argc, char * argv[] )
{
  int argIndex = 1;
  std::ostream& outputStream = std::cout;
  std::ostream& errorStream = std::cerr;

  // TestSceneFile
  const char *testSceneFileName  = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TestSceneFile") == 0)
    {
      testSceneFileName = argv[argIndex+1];
      outputStream << "Test MRML scene file name: " << testSceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      testSceneFileName = "";
    }
  }
  else
  {
    errorStream << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // TemporarySceneFile
  const char *temporarySceneFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporarySceneFile") == 0)
    {
      temporarySceneFileName = argv[argIndex+1];
      outputStream << "Temporary scene file name: " << temporarySceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporarySceneFileName = "";
    }
  }
  else
  {
    errorStream << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> subjectHierarchyLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  subjectHierarchyLogic->SetMRMLScene(mrmlScene);

  vtkSmartPointer<vtkSlicerSegmentationsModuleLogic> segmentationsLogic =
    vtkSmartPointer<vtkSlicerSegmentationsModuleLogic>::New();
  segmentationsLogic->SetMRMLScene(mrmlScene);

  vtkSmartPointer<vtkSlicerExternalBeamPlanningModuleLogic> ebpLogic = 
    vtkSmartPointer<vtkSlicerExternalBeamPlanningModuleLogic>::New();
  ebpLogic->SetMRMLScene(mrmlScene);

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();

  // Save it to the temporary directory
  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Get CT, segmentation, dose volume
  vtkSmartPointer<vtkCollection> ctVolumeNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("303: Unnamed Series") );
  vtkSmartPointer<vtkCollection> doseVolumeNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("RTDOSE [1]") );
  vtkSmartPointer<vtkCollection> segmentationNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("103: RTSTRUCT: AutoSS") );
  if (ctVolumeNodes->GetNumberOfItems() != 1 || doseVolumeNodes->GetNumberOfItems() != 1 || segmentationNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get input nodes!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* ctVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(ctVolumeNodes->GetItemAsObject(0));
  vtkMRMLScalarVolumeNode* doseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes->GetItemAsObject(0));

  // Create plan
  vtkSmartPointer<vtkMRMLRTPlanNode> planNode = vtkSmartPointer<vtkMRMLRTPlanNode>::New();
  planNode->SetName("TestProtonPlan");
  mrmlScene->AddNode(planNode);

  // Set plan parameters
  planNode->SetAndObserveReferenceVolumeNode(ctVolumeNode);
  planNode->SetAndObserveSegmentationNode(segmentationNode);
  planNode->SetAndObserveOutputTotalDoseVolumeNode(doseVolumeNode);
  planNode->SetDoseEngine(vtkMRMLRTPlanNode::Plastimatch);
  planNode->SetTargetSegmentID("Tumor_Contour");
  planNode->SetIsocenterToTargetCenter();

  // Add first beam
  vtkSmartPointer<vtkMRMLRTProtonBeamNode> firstBeamNode = vtkSmartPointer<vtkMRMLRTProtonBeamNode>::New();
  firstBeamNode->SetName("FirstBeam");
  mrmlScene->AddNode(firstBeamNode);
  firstBeamNode->CreateDefaultBeamModel();
  planNode->AddBeam(firstBeamNode);

  // Set first beam parameters
  firstBeamNode->SetX1Jaw(-50.0);
  firstBeamNode->SetX2Jaw(50.0);
  firstBeamNode->SetY1Jaw(-50.0);
  firstBeamNode->SetY2Jaw(75.0);
  firstBeamNode->SetEnergyResolution(4.0);

  // Add second beam copying the first
  //TODO:
  // Check if parameters have been copied from the first one properly
  //TODO:
  // Change geometry of second beam
  //TODO:

  // Setup time measurement
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Calculate dose
  std::string errorMessage("");
  //errorMessage = ebpLogic->InitializeAccumulatedDose(planNode);
  //if (!errorMessage.empty())
  //{
  //  mrmlScene->Commit();
  //  errorStream << "ERROR: Failed to initialize accumulated dose!" << std::endl;
  //  return EXIT_FAILURE;
  //}

  errorMessage = ebpLogic->CalculateDose(firstBeamNode);
  if (!errorMessage.empty())
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to calculate dose for beam " << firstBeamNode->GetName() << std::endl;
    return EXIT_FAILURE;
  }

  errorMessage = ebpLogic->CreateAccumulatedDose(planNode);
  if (!errorMessage.empty())
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to finalize accumulated dose!" << std::endl;
    return EXIT_FAILURE;
  }

  // Report time measurement
  double checkpointEnd = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
  std::cout << "Dose computation time: " << checkpointEnd-checkpointStart << " s" << std::endl;

  // Check computed output
  vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();
  imageAccumulate->SetInputConnection(doseVolumeNode->GetImageDataConnection());
  imageAccumulate->Update();
  double doseMax = imageAccumulate->GetMax()[0];
  double doseMean = imageAccumulate->GetMean()[0];
  double doseStdDev = imageAccumulate->GetStandardDeviation()[0];
  double doseVoxelCount = imageAccumulate->GetVoxelCount();

  outputStream << "Dose volume properties:" << std::endl << "  Max=" << doseMax << ", Mean=" << doseMean << ", StdDev=" << doseStdDev << ", NumberOfVoxels=" << doseVoxelCount << std::endl;
  if ( !IsEqualWithTolerance(doseMax, 1.05797)
    || !IsEqualWithTolerance(doseMean, 0.0251127)
    || !IsEqualWithTolerance(doseStdDev, 0.144932)
    || !IsEqualWithTolerance(doseVoxelCount, 1000) )
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Output dose volume properties don't match the baselines!" << std::endl;
    return EXIT_FAILURE;
  }

  mrmlScene->Commit();

  return EXIT_SUCCESS;
}
