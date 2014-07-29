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

// ContourComparison includes
#include "vtkSlicerContourComparisonModuleLogic.h"
#include "vtkMRMLContourComparisonNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourStorageNode.h"
#include "vtkSlicerContoursModuleLogic.h"
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
#include "itkFactoryRegistration.h"

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

  const char *inputContourReferenceFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputContourReferenceFile") == 0)
    {
      inputContourReferenceFileName = argv[argIndex+1];
      std::cout << "Reference input contour file name: " << inputContourReferenceFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputContourReferenceFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputContourCompareFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputContourCompareFile") == 0)
    {
      inputContourCompareFileName = argv[argIndex+1];
      std::cout << "Compare input contour file name: " << inputContourCompareFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputContourCompareFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *referenceVolumeFilename = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ReferenceVolumeFile") == 0)
    {
      referenceVolumeFilename = argv[argIndex+1];
      std::cout << "Reference volume file name: " << referenceVolumeFilename << std::endl;
      argIndex += 2;
    }
    else
    {
      referenceVolumeFilename = "";
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
      std::cout << "True negatives (%): " << trueNegativesPercent << std::endl;
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
  itk::itkFactoryRegistration();

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

  // Create reference contour node
  std::string inputContourReferenceFile = std::string(dataDirectoryPath) + std::string(inputContourReferenceFileName);
  if (!vtksys::SystemTools::FileExists(inputContourReferenceFile.c_str()))
  {
    std::cerr << "Loading contour from file '" << inputContourReferenceFile << "' failed - the file does not exist!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkSmartPointer<vtkMRMLContourNode> referenceContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  mrmlScene->AddNode(referenceContourNode);
  referenceContourNode->SetName("Reference_Contour");
  vtkMRMLContourStorageNode* referenceContourStorageNode = vtkSlicerContoursModuleLogic::CreateContourStorageNode(referenceContourNode);
  referenceContourStorageNode->SetFileName(inputContourReferenceFile.c_str());
  if (!referenceContourStorageNode->ReadData(referenceContourNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading contour from file '" << inputContourReferenceFile << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create compare contour node
  std::string compareContourFile = std::string(dataDirectoryPath) + std::string(inputContourCompareFileName);
  if (!vtksys::SystemTools::FileExists(compareContourFile.c_str()))
  {
    std::cerr << "Loading contour from file '" << compareContourFile << "' failed - the file does not exist!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkSmartPointer<vtkMRMLContourNode> compareContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  mrmlScene->AddNode(compareContourNode);
  compareContourNode->SetName("Reference_Contour");
  vtkMRMLContourStorageNode* compareContourStorageNode = vtkSlicerContoursModuleLogic::CreateContourStorageNode(compareContourNode);
  compareContourStorageNode->SetFileName(compareContourFile.c_str());
  if (!compareContourStorageNode->ReadData(compareContourNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading contour from file '" << compareContourFile << "' failed!" << std::endl;
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

    compareContourNode->SetAndObserveTransformNodeID(inputCompareTransformNode->GetID());
  }

  // Create reference volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> referenceVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(referenceVolumeNode);
  referenceVolumeNode->SetName("Reference_Volume");
  referenceVolumeNode->LabelMapOn();

  // Load reference volume node
  std::string referenceVolumeFileName = std::string(dataDirectoryPath) + std::string(referenceVolumeFilename);
  if (!vtksys::SystemTools::FileExists(referenceVolumeFileName.c_str()))
  {
    std::cerr << "Loading volume from file '" << referenceVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> referenceVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(referenceVolumeArchetypeStorageNode);
  referenceVolumeArchetypeStorageNode->SetFileName(referenceVolumeFileName.c_str());

  referenceVolumeNode->SetAndObserveStorageNodeID(referenceVolumeArchetypeStorageNode->GetID());

  if (! referenceVolumeArchetypeStorageNode->ReadData(referenceVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading volume from file '" << referenceVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLContourComparisonNode> paramNode = vtkSmartPointer<vtkMRMLContourComparisonNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveReferenceContourNode(referenceContourNode);
  paramNode->SetAndObserveCompareContourNode(compareContourNode);
  paramNode->SetAndObserveRasterizationReferenceVolumeNode(referenceVolumeNode);

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
  int result(EXIT_SUCCESS);
  double resultHausdorffMaximumMm = paramNode->GetMaximumHausdorffDistanceForBoundaryMm();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultHausdorffMaximumMm, hausdorffMaximumMm))
  {
    std::cerr << "Hausdorff maximum (mm) mismatch: " << resultHausdorffMaximumMm << " instead of " << hausdorffMaximumMm << std::endl;
    result = EXIT_FAILURE;
  }
  double resultHausdorffAverageMm = paramNode->GetAverageHausdorffDistanceForBoundaryMm();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultHausdorffAverageMm, hausdorffAverageMm))
  {
    std::cerr << "Hausdorff average (mm) mismatch: " << resultHausdorffAverageMm << " instead of " << hausdorffAverageMm << std::endl;
    result = EXIT_FAILURE;
  }
  double resultHausdorff95PercentMm = paramNode->GetPercent95HausdorffDistanceForBoundaryMm();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultHausdorff95PercentMm, hausdorff95PercentMm))
  {
    std::cerr << "Hausdorff 95% mismatch: " << resultHausdorff95PercentMm << " instead of " << hausdorff95PercentMm << std::endl;
    result = EXIT_FAILURE;
  }

  double resultDiceCoefficient = paramNode->GetDiceCoefficient();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultDiceCoefficient, diceCoefficient))
  {
    std::cerr << "Dice coefficient mismatch: " << resultDiceCoefficient << " instead of " << diceCoefficient << std::endl;
    result = EXIT_FAILURE;
  }
  double resultTruePositivesPercent = paramNode->GetTruePositivesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultTruePositivesPercent, truePositivesPercent))
  {
    std::cerr << "True positives (%) mismatch: " << resultTruePositivesPercent << " instead of " << truePositivesPercent << std::endl;
    result = EXIT_FAILURE;
  }
  double resultFalsePositivesPercent = paramNode->GetFalsePositivesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultFalsePositivesPercent, falsePositivesPercent))
  {
    std::cerr << "False positives (%) mismatch: " << resultFalsePositivesPercent << " instead of " << falsePositivesPercent << std::endl;
    result = EXIT_FAILURE;
  }
  double resultTrueNegativesPercent = paramNode->GetTrueNegativesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultTrueNegativesPercent, trueNegativesPercent))
  {
    std::cerr << "True negatives (%) mismatch: " << resultTrueNegativesPercent << " instead of " << trueNegativesPercent << std::endl;
    result = EXIT_FAILURE;
  }
  double resultFalseNegativesPercent = paramNode->GetFalseNegativesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultFalseNegativesPercent, falseNegativesPercent))
  {
    std::cerr << "False negatives (%) mismatch: " << resultFalseNegativesPercent << " instead of " << falseNegativesPercent << std::endl;
    result = EXIT_FAILURE;
  }

  return result;
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
