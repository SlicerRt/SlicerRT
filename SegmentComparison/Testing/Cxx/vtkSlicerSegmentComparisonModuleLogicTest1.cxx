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

// SegmentComparison includes
#include "vtkSlicerSegmentComparisonModuleLogic.h"
#include "vtkMRMLSegmentComparisonNode.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkRibbonModelToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToRibbonModelConversionRule.h"

// SegmentationCore includes
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSegmentationConverterFactory.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScene.h>

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
int vtkSlicerSegmentComparisonModuleLogicTest1( int argc, char * argv[] )
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

  const char *inputSegmentationReferenceFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputSegmentationReferenceFile") == 0)
    {
      inputSegmentationReferenceFileName = argv[argIndex+1];
      std::cout << "Reference input segmentation file name: " << inputSegmentationReferenceFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputSegmentationReferenceFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputSegmentationCompareFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputSegmentationCompareFile") == 0)
    {
      inputSegmentationCompareFileName = argv[argIndex+1];
      std::cout << "Compare input segmentation file name: " << inputSegmentationCompareFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputSegmentationCompareFileName = "";
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

  // Create Segmentations logic
  vtkSmartPointer<vtkSlicerSegmentationsModuleLogic> segmentationsLogic = vtkSmartPointer<vtkSlicerSegmentationsModuleLogic>::New();
  segmentationsLogic->SetMRMLScene(mrmlScene);

  // Register converters to use ribbon models. Will be unnecessary when having resolved issues regarding
  // direct planar contours to closed surface conversion https://www.assembla.com/spaces/slicerrt/tickets/751
  // (vtkSlicerDicomRtImportExportConversionRules can also be removed from CMake link targets)
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(vtkSmartPointer<vtkRibbonModelToBinaryLabelmapConversionRule>::New());
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(vtkSmartPointer<vtkPlanarContourToRibbonModelConversionRule>::New());
  // Disable closed surface representation so that ribbon model is used for labelmap conversion instead of direct closed surface
  vtkSegmentationConverterFactory::GetInstance()->DisableRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());

  // Save scene to the temporary directory
  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Load reference segmentation node
  std::string referenceSegmentationFile = std::string(dataDirectoryPath) + std::string(inputSegmentationReferenceFileName);
  if (!vtksys::SystemTools::FileExists(referenceSegmentationFile.c_str()))
  {
    std::cerr << "Loading segmentation from file '" << referenceSegmentationFile << "' failed - the file does not exist!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLSegmentationNode* referenceSegmentationNode = segmentationsLogic->LoadSegmentationFromFile(referenceSegmentationFile.c_str());
  if (!referenceSegmentationNode)
  {
    std::cerr << "Loading segmentation from existing file '" << referenceSegmentationFile << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  // Make sure binary labelmap representation is available
  if (!referenceSegmentationNode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    std::cerr << "Failed to create binary labelmap representation in reference segmentation!" << std::endl;
    return EXIT_FAILURE;
  }
  // Get ID of only segment
  if (referenceSegmentationNode->GetSegmentation()->GetNumberOfSegments() > 1)
  {
    std::cerr << "Reference segmentation should contain only one segment!" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> referenceSegmentIDs;
  referenceSegmentationNode->GetSegmentation()->GetSegmentIDs(referenceSegmentIDs);
  std::string referenceSegmentID = referenceSegmentIDs[0];

  // Load compare segmentation node
  std::string compareSegmentationFile = std::string(dataDirectoryPath) + std::string(inputSegmentationCompareFileName);
  if (!vtksys::SystemTools::FileExists(compareSegmentationFile.c_str()))
  {
    std::cerr << "Loading segmentation from file '" << compareSegmentationFile << "' failed - the file does not exist!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLSegmentationNode* compareSegmentationNode = segmentationsLogic->LoadSegmentationFromFile(compareSegmentationFile.c_str());
  if (!compareSegmentationNode)
  {
    std::cerr << "Loading segmentation from existing file '" << compareSegmentationFile << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  // Make sure binary labelmap representation is available
  if (!compareSegmentationNode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    std::cerr << "Failed to create binary labelmap representation in compare segmentation!" << std::endl;
    return EXIT_FAILURE;
  }
  // Get ID of only segment
  if (compareSegmentationNode->GetSegmentation()->GetNumberOfSegments() > 1)
  {
    std::cerr << "Compare segmentation should contain only one segment!" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> compareSegmentIDs;
  compareSegmentationNode->GetSegmentation()->GetSegmentIDs(compareSegmentIDs);
  std::string compareSegmentID = compareSegmentIDs[0];

  // Create transform if necessary
  if (applySimpleTransformToInputCompare)
  {
    vtkSmartPointer<vtkTransform> inputCompareTransform = vtkSmartPointer<vtkTransform>::New();
    inputCompareTransform->Identity();
    inputCompareTransform->Translate(5.0, 0.0, 0.0);

    vtkSmartPointer<vtkMRMLLinearTransformNode> inputCompareTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    inputCompareTransformNode->ApplyTransformMatrix(inputCompareTransform->GetMatrix());
    mrmlScene->AddNode(inputCompareTransformNode);

    compareSegmentationNode->SetAndObserveTransformNodeID(inputCompareTransformNode->GetID());
  }

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLSegmentComparisonNode> paramNode = vtkSmartPointer<vtkMRMLSegmentComparisonNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveReferenceSegmentationNode(referenceSegmentationNode);
  paramNode->SetReferenceSegmentID(referenceSegmentID.c_str());
  paramNode->SetAndObserveCompareSegmentationNode(compareSegmentationNode);
  paramNode->SetCompareSegmentID(compareSegmentID.c_str());

  // Create and set up logic
  vtkSmartPointer<vtkSlicerSegmentComparisonModuleLogic> segmentComparisonLogic = vtkSmartPointer<vtkSlicerSegmentComparisonModuleLogic>::New();
  segmentComparisonLogic->SetMRMLScene(mrmlScene);
  segmentComparisonLogic->SetAndObserveSegmentComparisonNode(paramNode);

  // Compute Dice and Hausdorff
  std::string errorMessageDice = segmentComparisonLogic->ComputeDiceStatistics();
  std::string errorMessageHausdorff = segmentComparisonLogic->ComputeHausdorffDistances();

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
  double resultTrueNegativesPercent = paramNode->GetTrueNegativesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultTrueNegativesPercent, trueNegativesPercent))
  {
    std::cerr << "True negatives (%) mismatch: " << resultTrueNegativesPercent << " instead of " << trueNegativesPercent << std::endl;
    result = EXIT_FAILURE;
  }
  double resultFalsePositivesPercent = paramNode->GetFalsePositivesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultFalsePositivesPercent, falsePositivesPercent))
  {
    std::cerr << "False positives (%) mismatch: " << resultFalsePositivesPercent << " instead of " << falsePositivesPercent << std::endl;
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
