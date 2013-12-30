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

// ContourComparison includes
#include "vtkSlicerContourComparisonModuleLogic.h"
#include "vtkMRMLContourComparisonNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLContourNode.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkImageMathematics.h>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

bool CheckIfResultIsWithinOneTenthPercentFromBaseline(double result, double baseline);

//-----------------------------------------------------------------------------
int vtkSlicerContourComparisonModuleLogicTest1( int argc, char * argv[] )
{
  int argIndex = 1;

  const char *dataDirectoryPath = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-DataDirectoryPath") == 0)
    {
      dataDirectoryPath = argv[argIndex+1];
      std::cout << "Data directory path: " << dataDirectoryPath << std::endl;
      argIndex += 2;
    }
    else
    {
      dataDirectoryPath = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputLabelmapReferenceFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputLabelmapReference") == 0)
    {
      inputLabelmapReferenceFileName = argv[argIndex+1];
      std::cout << "Reference input labelmap file name: " << inputLabelmapReferenceFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputLabelmapReferenceFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputLabelmapCompareFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputLabelmapCompare") == 0)
    {
      inputLabelmapCompareFileName = argv[argIndex+1];
      std::cout << "Compare input labelmap file name: " << inputLabelmapCompareFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputLabelmapCompareFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

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
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  bool applySimpleTransformToInputCompare = false;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ApplySimpleTransformToInputCompare") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      int intValue;
      ss >> intValue;
      applySimpleTransformToInputCompare = (intValue == 1 ? true : false);
      std::cout << "Apply simple transform to input compare: " << (applySimpleTransformToInputCompare ? "true" : "false") << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double hausdorffMaximumMm = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-HausdorffMaximumMm") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      hausdorffMaximumMm = doubleValue;
      std::cout << "Hausdorff maximum (mm): " << hausdorffMaximumMm << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double hausdorffAverageMm = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-HausdorffAverageMm") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      hausdorffAverageMm = doubleValue;
      std::cout << "Hausdorff average (mm): " << hausdorffAverageMm << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double hausdorff95PercentMm = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-Hausdorff95PercentMm") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      hausdorff95PercentMm = doubleValue;
      std::cout << "Hausdorff 95% (mm): " << hausdorff95PercentMm << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double diceCoefficient = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-DiceCoefficient") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      diceCoefficient = doubleValue;
      std::cout << "Dice coefficient: " << diceCoefficient << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double truePositivesPercent = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TruePositivesPercent") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      truePositivesPercent = doubleValue;
      std::cout << "True positives (%): " << truePositivesPercent << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double trueNegativesPercent = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TrueNegativesPercent") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      trueNegativesPercent = doubleValue;
      std::cout << "True Negatives (%): " << trueNegativesPercent << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double falsePositivesPercent = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-FalsePositivesPercent") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      falsePositivesPercent = doubleValue;
      std::cout << "False positives (%): " << falsePositivesPercent << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  double falseNegativesPercent = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-FalseNegativesPercent") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      falseNegativesPercent = doubleValue;
      std::cout << "False negatives (%): " << falseNegativesPercent << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }


  // Make sure NRRD reading works
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // TODO: Remove when subject hierarchy is integrated into Slicer core
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> subjectHierarchyLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  subjectHierarchyLogic->SetMRMLScene(mrmlScene);

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Create reference input labelmap node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputLabelmapReferenceScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(inputLabelmapReferenceScalarVolumeNode);
  inputLabelmapReferenceScalarVolumeNode->SetName("Input_Reference_Labelmap");
  inputLabelmapReferenceScalarVolumeNode->LabelMapOn();
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, inputLabelmapReferenceScalarVolumeNode);

  // Load reference input labelmap
  std::string inputLabelmapReferenceFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(inputLabelmapReferenceFileName);
  if (!vtksys::SystemTools::FileExists(inputLabelmapReferenceFileFullPath.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << inputLabelmapReferenceFileFullPath << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> inputLabelmapReferenceArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(inputLabelmapReferenceArchetypeStorageNode);
  inputLabelmapReferenceArchetypeStorageNode->SetFileName(inputLabelmapReferenceFileFullPath.c_str());
  //EXERCISE_BASIC_STORAGE_MRML_METHODS(vtkMRMLVolumeArchetypeStorageNode, inputLabelmapReferenceArchetypeStorageNode);

  inputLabelmapReferenceScalarVolumeNode->SetAndObserveStorageNodeID(inputLabelmapReferenceArchetypeStorageNode->GetID());

  if (! inputLabelmapReferenceArchetypeStorageNode->ReadData(inputLabelmapReferenceScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << inputLabelmapReferenceFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create compare input labelmap node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputLabelmapCompareScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(inputLabelmapCompareScalarVolumeNode);
  inputLabelmapCompareScalarVolumeNode->SetName("Input_Compare_Labelmap");
  inputLabelmapCompareScalarVolumeNode->LabelMapOn();
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, inputLabelmapCompareScalarVolumeNode);

  // Load compare input labelmap
  std::string inputLabelmapCompareFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(inputLabelmapCompareFileName);
  if (!vtksys::SystemTools::FileExists(inputLabelmapCompareFileFullPath.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << inputLabelmapCompareFileFullPath << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> inputLabelmapCompareArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(inputLabelmapCompareArchetypeStorageNode);
  inputLabelmapCompareArchetypeStorageNode->SetFileName(inputLabelmapCompareFileFullPath.c_str());
  //EXERCISE_BASIC_STORAGE_MRML_METHODS(vtkMRMLVolumeArchetypeStorageNode, inputLabelmapCompareArchetypeStorageNode);

  inputLabelmapCompareScalarVolumeNode->SetAndObserveStorageNodeID(inputLabelmapCompareArchetypeStorageNode->GetID());

  if (! inputLabelmapCompareArchetypeStorageNode->ReadData(inputLabelmapCompareScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << inputLabelmapCompareFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create transform if necessary
  if (applySimpleTransformToInputCompare)
  {
    vtkSmartPointer<vtkTransform> inputCompareTransform = vtkSmartPointer<vtkTransform>::New();
    inputCompareTransform->Identity();
    inputCompareTransform->Translate(5.0, 0.0, 0.0);

    vtkSmartPointer<vtkMRMLLinearTransformNode> inputCompareTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    inputCompareTransformNode->ApplyTransformMatrix(inputCompareTransform->GetMatrix());
    mrmlScene->AddNode(inputCompareTransformNode);

    inputLabelmapCompareScalarVolumeNode->SetAndObserveTransformNodeID(inputCompareTransformNode->GetID());
  }

  // Create contour nodes from the input labelmaps
  vtkSmartPointer<vtkMRMLContourNode> referenceContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  referenceContourNode->SetName("Reference_Contour");
  mrmlScene->AddNode(referenceContourNode);
  referenceContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(inputLabelmapReferenceScalarVolumeNode->GetID());

  vtkSmartPointer<vtkMRMLContourNode> compareContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  compareContourNode->SetName("Compare_Contour");
  mrmlScene->AddNode(compareContourNode);
  compareContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(inputLabelmapCompareScalarVolumeNode->GetID());

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLContourComparisonNode> paramNode = vtkSmartPointer<vtkMRMLContourComparisonNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveReferenceContourNode(referenceContourNode);
  paramNode->SetAndObserveCompareContourNode(compareContourNode);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerContourComparisonModuleLogic> contourComparisonLogic = vtkSmartPointer<vtkSlicerContourComparisonModuleLogic>::New();

  contourComparisonLogic->SetMRMLScene(mrmlScene);
  contourComparisonLogic->SetAndObserveContourComparisonNode(paramNode);

  // Compute Dice and Hausdorff
  std::string errorMessageDice;
  contourComparisonLogic->ComputeDiceStatistics(errorMessageDice);
  std::string errorMessageHausdorff;
  contourComparisonLogic->ComputeHausdorffDistances(errorMessageHausdorff);

  if (!paramNode->GetHausdorffResultsValid() || !paramNode->GetDiceResultsValid())
  {
    mrmlScene->Commit();
    std::cerr << "Failed to compute results!" << std::endl;
    return EXIT_FAILURE;
  }

  mrmlScene->Commit();

  // Compare results to baseline
  double resultHausdorffMaximumMm = paramNode->GetMaximumHausdorffDistanceForBoundaryMm();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultHausdorffMaximumMm, hausdorffMaximumMm))
  {
    std::cerr << "Hausdorff maximum (mm) mismatch: " << resultHausdorffMaximumMm << " instead of " << hausdorffMaximumMm << std::endl;
    return EXIT_FAILURE;
  }
  double resultHausdorffAverageMm = paramNode->GetAverageHausdorffDistanceForBoundaryMm();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultHausdorffAverageMm, hausdorffAverageMm))
  {
    std::cerr << "Hausdorff average (mm) mismatch: " << resultHausdorffAverageMm << " instead of " << hausdorffAverageMm << std::endl;
    return EXIT_FAILURE;
  }
  double resultHausdorff95PercentMm = paramNode->GetPercent95HausdorffDistanceForBoundaryMm();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultHausdorff95PercentMm, hausdorff95PercentMm))
  {
    std::cerr << "Hausdorff 95% mismatch: " << resultHausdorff95PercentMm << " instead of " << hausdorff95PercentMm << std::endl;
    return EXIT_FAILURE;
  }

  double resultDiceCoefficient = paramNode->GetDiceCoefficient();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultDiceCoefficient, diceCoefficient))
  {
    std::cerr << "Dice coefficient mismatch: " << resultDiceCoefficient << " instead of " << diceCoefficient << std::endl;
    return EXIT_FAILURE;
  }
  double resultTruePositivesPercent = paramNode->GetTruePositivesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultTruePositivesPercent, truePositivesPercent))
  {
    std::cerr << "True positives (%) mismatch: " << resultTruePositivesPercent << " instead of " << truePositivesPercent << std::endl;
    return EXIT_FAILURE;
  }
  double resultFalsePositivesPercent = paramNode->GetFalsePositivesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultFalsePositivesPercent, falsePositivesPercent))
  {
    std::cerr << "False positives (%) mismatch: " << resultFalsePositivesPercent << " instead of " << falsePositivesPercent << std::endl;
    return EXIT_FAILURE;
  }
  double resultTrueNegativesPercent = paramNode->GetTrueNegativesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultTrueNegativesPercent, trueNegativesPercent))
  {
    std::cerr << "True negatives (%) mismatch: " << resultTrueNegativesPercent << " instead of " << trueNegativesPercent << std::endl;
    return EXIT_FAILURE;
  }
  double resultFalseNegativesPercent = paramNode->GetFalseNegativesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultFalseNegativesPercent, falseNegativesPercent))
  {
    std::cerr << "False negatives (%) mismatch: " << resultFalseNegativesPercent << " instead of " << falseNegativesPercent << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
bool CheckIfResultIsWithinOneTenthPercentFromBaseline(double result, double baseline)
{
  if (baseline == 0.0)
  {
    return (fabs(result - baseline) < 0.0001);
  }

  double ratio = result / baseline;
  double absoluteDifferencePercent = fabs(ratio - 1.0) * 100.0;

  return absoluteDifferencePercent < 0.1;
}
