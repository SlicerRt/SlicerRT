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

  This file was originally developed by Adam Rankin, Perk Lab, Queen's University 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// DoseAccumulation includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>

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
int vtkSlicerDoseComparisonModuleLogicTest1( int argc, char * argv[] )
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
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
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

  // Get day 1 dose volume
  vtkSmartPointer<vtkCollection> doseVolumeNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("EclipseEnt_Dose") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get day 1 dose volume!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* day1DoseScalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));

  // Get day 2 dose volume
  doseVolumeNodes = vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("EclipseEnt_Dose_Day2") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get day 2 dose volume!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* day2DoseScalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));

  // Create output dose volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputGammaVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  outputGammaVolumeNode->SetName("OutputDose");
  mrmlScene->AddNode(outputGammaVolumeNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLDoseComparisonNode> paramNode = vtkSmartPointer<vtkMRMLDoseComparisonNode>::New();
  mrmlScene->AddNode(paramNode);

  paramNode->SetAndObserveReferenceDoseVolumeNodeId(day1DoseScalarVolumeNode->GetID());
  paramNode->SetAndObserveCompareDoseVolumeNodeId(day2DoseScalarVolumeNode->GetID());
  paramNode->SetAndObserveGammaVolumeNodeId(outputGammaVolumeNode->GetID());

  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseComparisonModuleLogic> doseComparisonLogic = vtkSmartPointer<vtkSlicerDoseComparisonModuleLogic>::New();
  doseComparisonLogic->SetMRMLScene(mrmlScene);
  doseComparisonLogic->SetAndObserveDoseComparisonNode(paramNode);

  // Compute DoseAccumulation
  doseComparisonLogic->ComputeGammaDoseDifference();

  // Get saved volume
  vtkSmartPointer<vtkCollection> gammaVolumeNodes = vtkSmartPointer<vtkCollection>::Take(
    mrmlScene->GetNodesByName("GammaVolume_EclipseEnt_Day1Day2_Baseline") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get baseline gamma volume!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* baselineGammaVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(gammaVolumeNodes->GetItemAsObject(0));

  mrmlScene->Commit();

  // Subtract the baseline gamma volume from the resultant gamma volume to see if we end up with a zero result. If not, dose comparison has changed (bad!)
  vtkSmartPointer<vtkImageMathematics> math = vtkSmartPointer<vtkImageMathematics>::New();
  math->SetInput1(outputGammaVolumeNode->GetImageData());
  math->SetInput2(baselineGammaVolumeNode->GetImageData());
  math->SetOperationToSubtract();
  math->Update();

  vtkImageData* comparison = math->GetOutput();
  double range[2];
  comparison->GetScalarRange(range);
  if( range[0] != 0.0 || range[1] != 0.0 )
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

