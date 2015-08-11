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

// DoseVolumeHistogram includes
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"
#include "vtkSlicerDoseVolumeHistogramComparisonLogic.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkRibbonModelToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToRibbonModelConversionRule.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// SegmentationCore includes
#include "vtkOrientedImageData.h"
#include "vtkSegmentationConverterFactory.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToClosedSurfaceConversionRule.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLScene.h>

// SubjectHierarchy includes
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkLookupTable.h>
#include <vtkTimerLog.h>

// ITK includes
#include "itkFactoryRegistration.h"

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

std::string csvSeparatorCharacter(",");

//-----------------------------------------------------------------------------
int CompareCsvDvhTables(std::string dvhMetricsCsvFileName, std::string baselineCsvFileName,
                        double maxDose, double volumeDifferenceCriterion, double doseToAgreementCriterion,
                        double &agreementAcceptancePercentage);

int CompareCsvDvhMetrics(std::string dvhMetricsCsvFileName, std::string baselineDvhMetricCsvFileName, double metricDifferenceThreshold);

//-----------------------------------------------------------------------------
int vtkSlicerDoseVolumeHistogramModuleLogicTest1( int argc, char * argv[] )
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
  // BaselineDvhTableCsvFile
  const char *baselineDvhTableCsvFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineDvhTableCsvFile") == 0)
    {
      baselineDvhTableCsvFileName = argv[argIndex+1];
      std::cout << "Baseline DVH table CSV file name: " << baselineDvhTableCsvFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineDvhTableCsvFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // BaselineDvhMetricCsvFile
  const char *baselineDvhMetricCsvFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineDvhMetricCsvFile") == 0)
    {
      baselineDvhMetricCsvFileName = argv[argIndex+1];
      std::cout << "Baseline DVH metric CSV file name: " << baselineDvhMetricCsvFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineDvhMetricCsvFileName = "";
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
  // TemporaryDvhTableCsvFile
  const char *temporaryDvhTableCsvFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporaryDvhTableCsvFile") == 0)
    {
      temporaryDvhTableCsvFileName = argv[argIndex+1];
      std::cout << "Temporary DVH table CSV file name: " << temporaryDvhTableCsvFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporaryDvhTableCsvFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // TemporaryDvhMetricCsvFile
  const char *temporaryDvhMetricCsvFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporaryDvhMetricCsvFile") == 0)
    {
      temporaryDvhMetricCsvFileName = argv[argIndex+1];
      std::cout << "Temporary DVH metric CSV file name: " << temporaryDvhMetricCsvFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporaryDvhMetricCsvFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // AutomaticOversamplingCalculation
  bool automaticOversamplingCalculation = false;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-AutomaticOversamplingCalculation") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      int intValue;
      ss >> intValue;
      automaticOversamplingCalculation = (intValue > 0 ? true : false);
      std::cout << "Automatic oversampling calculation: " << (automaticOversamplingCalculation ? "true" : "false") << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // VolumeDifferenceCriterion
  double volumeDifferenceCriterion = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-VolumeDifferenceCriterion") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      volumeDifferenceCriterion = doubleValue;
      std::cout << "Volume difference criterion: " << volumeDifferenceCriterion << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // DoseToAgreementCriterion
  double doseToAgreementCriterion = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-DoseToAgreementCriterion") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      doseToAgreementCriterion = doubleValue;
      std::cout << "Dose-to-agreement criterion: " << doseToAgreementCriterion << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // AgreementAcceptancePercentageThreshold
  double agreementAcceptancePercentageThreshold = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-AgreementAcceptancePercentageThreshold") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      agreementAcceptancePercentageThreshold = doubleValue;
      std::cout << "Agreement acceptance percentage threshold: " << agreementAcceptancePercentageThreshold << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // MetricDifferenceThreshold
  double metricDifferenceThreshold = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-MetricDifferenceThreshold") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      metricDifferenceThreshold = doubleValue;
      std::cout << "Metric difference threshold: " << metricDifferenceThreshold << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // DvhStartValue
  double dvhStartValue = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-DvhStartValue") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      dvhStartValue = doubleValue;
      std::cout << "DVH start value: " << dvhStartValue << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  // DvhStepSize
  double dvhStepSize = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-DvhStepSize") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      dvhStepSize = doubleValue;
      std::cout << "DVH step size: " << dvhStepSize << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  // Constraint the criteria to be greater than zero
  if (volumeDifferenceCriterion == 0.0)
  {
    volumeDifferenceCriterion = EPSILON;
  }
  if (doseToAgreementCriterion == 0.0)
  {
    doseToAgreementCriterion = EPSILON;
  }
  if (metricDifferenceThreshold == 0.0)
  {
    metricDifferenceThreshold = EPSILON;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create Segmentations logic
  vtkSmartPointer<vtkSlicerSegmentationsModuleLogic> segmentationsLogic = vtkSmartPointer<vtkSlicerSegmentationsModuleLogic>::New();
  segmentationsLogic->SetMRMLScene(mrmlScene);
  // Create Subject hierarchy logic. Needed so that the SH node type is registered. This can be removed
  // once the subject hierarchy node is moved to MRML/Core, and is registered in vtkMRMLScene constructor.
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> subjectHierarchyLogic = vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  subjectHierarchyLogic->SetMRMLScene(mrmlScene);
  // Register converters to use ribbon models. Will be unnecessary when having resolved issues regarding
  // direct planar contours to closed surface conversion https://www.assembla.com/spaces/slicerrt/tickets/751
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(vtkSmartPointer<vtkRibbonModelToBinaryLabelmapConversionRule>::New());
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(vtkSmartPointer<vtkPlanarContourToRibbonModelConversionRule>::New());

  // Load test scene into temporary scene
  //mrmlScene->GetCacheManager()->ClearCache();
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

  // Get segmentation node
  vtkSmartPointer<vtkCollection> segmentationNodes = vtkSmartPointer<vtkCollection>::Take(
    mrmlScene->GetNodesByClass("vtkMRMLSegmentationNode") );
  if (segmentationNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    std::cerr << "ERROR: Failed to get segmentation!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes->GetItemAsObject(0));

  // Create ribbon model representation so that it is used for labelmap conversion instead of direct
  // closed surface. This makes sure that the test works the same way until resolving issues with the
  // direct converter, see https://www.assembla.com/spaces/slicerrt/tickets/751
  segmentationNode->GetSegmentation()->CreateRepresentation(SlicerRtCommon::SEGMENTATION_RIBBON_MODEL_REPRESENTATION_NAME);

  // Determine maximum dose
  vtkNew<vtkImageAccumulate> doseStat;
#if (VTK_MAJOR_VERSION <= 5)
  doseStat->SetInput(doseScalarVolumeNode->GetImageData());
#else
  doseStat->SetInputData(doseScalarVolumeNode->GetImageData());
#endif
  doseStat->Update();
  double maxDose = doseStat->GetMax()[0];

  // Create chart node
  vtkSmartPointer<vtkMRMLChartNode> chartNode = vtkSmartPointer<vtkMRMLChartNode>::New();
  chartNode->SetProperty("default", "title", "Dose Volume Histogram");
  chartNode->SetProperty("default", "xAxisLabel", "Dose [Gy]");
  chartNode->SetProperty("default", "yAxisLabel", "Fractional volume [%]");
  chartNode->SetProperty("default", "type", "Line");
  mrmlScene->AddNode(chartNode);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramModuleLogic> dvhLogic = vtkSmartPointer<vtkSlicerDoseVolumeHistogramModuleLogic>::New();
  dvhLogic->SetMRMLScene(mrmlScene);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode> paramNode = vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New();
  paramNode->SetAndObserveDoseVolumeNode(doseScalarVolumeNode);
  paramNode->SetAndObserveSegmentationNode(segmentationNode);
  paramNode->SetAndObserveChartNode(chartNode);
  paramNode->SetAutomaticOversampling(automaticOversamplingCalculation);
  mrmlScene->AddNode(paramNode);
  dvhLogic->SetAndObserveDoseVolumeHistogramNode(paramNode);

  // Set start value and step size if specified
  if (dvhStartValue != 0.0 && dvhStepSize != 0.0)
  {
    dvhLogic->SetStartValue(dvhStartValue);
    dvhLogic->SetStepSize(dvhStepSize);
  }

  // Setup time measurement
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Compute DVH
  std::string errorMessage = dvhLogic->ComputeDvh();
  if (!errorMessage.empty())
  {
    std::cerr << errorMessage << std::endl;
    return EXIT_FAILURE;
  }

  // Calculate and print oversampling factors if automatically calculated
  if (automaticOversamplingCalculation)
  {
    // Get spacing for dose volume
    double doseSpacing[3] = {0.0,0.0,0.0};
    doseScalarVolumeNode->GetSpacing(doseSpacing);

    // Calculate oversampling factors for all segments (need to calculate as it is not stored per segment)
    vtkSegmentation::SegmentMap segmentMap = segmentationNode->GetSegmentation()->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      vtkSegment* currentSegment = segmentIt->second;
      vtkOrientedImageData* currentBinaryLabelmap = vtkOrientedImageData::SafeDownCast(
        currentSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
      if (!currentBinaryLabelmap)
      {
        std::cerr << "Error: No binary labelmap in segment " << segmentIt->first << std::endl;
        continue;
      }
      double currentSpacing[3] = {0.0,0.0,0.0};
      currentBinaryLabelmap->GetSpacing(currentSpacing);

      double voxelSizeRatio = ((doseSpacing[0]*doseSpacing[1]*doseSpacing[2]) / (currentSpacing[0]*currentSpacing[1]*currentSpacing[2]));
      // Round oversampling to two decimals
      // Note: We need to round to some degree, because e.g. pow(64,1/3) is not exactly 4. It may be debated whether to round to integer or to a certain number of decimals
      double oversamplingFactor = vtkMath::Round( pow( voxelSizeRatio, 1.0/3.0 ) * 100.0 ) / 100.0;
      std::cout << "  Automatic oversampling factor for segment " << segmentIt->first << " calculated to be " << oversamplingFactor << std::endl;
    }
  }

  // Report time measurement
  double checkpointEnd = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
  std::cout << "DVH computation time (including rasterization): " << checkpointEnd-checkpointStart << std::endl;

  std::vector<vtkMRMLNode*> dvhNodes;
  paramNode->GetDvhDoubleArrayNodes(dvhNodes);

  // Add DVH arrays to chart node
  std::vector<vtkMRMLNode*>::iterator dvhIt;
  for (dvhIt = dvhNodes.begin(); dvhIt != dvhNodes.end(); ++dvhIt)
  {
    if (!(*dvhIt))
    {
      std::cerr << "Error: Invalid DVH node!" << std::endl;
      return EXIT_FAILURE;
    }

    chartNode->AddArray( (*dvhIt)->GetName(), (*dvhIt)->GetID() );
  }

  mrmlScene->Commit();

  // Export DVH to CSV
  vtksys::SystemTools::RemoveFile(temporaryDvhTableCsvFileName);
  dvhLogic->ExportDvhToCsv(temporaryDvhTableCsvFileName);

  // Export DVH metrics to CSV
  static const double vDoseValuesCcArr[] = {5, 20};
  std::vector<double> vDoseValuesCc( vDoseValuesCcArr, vDoseValuesCcArr
    + sizeof(vDoseValuesCcArr) / sizeof(vDoseValuesCcArr[0]) );
  static const double vDoseValuesPercentArr[] = {5, 20};
  std::vector<double> vDoseValuesPercent( vDoseValuesPercentArr, vDoseValuesPercentArr
    + sizeof(vDoseValuesPercentArr) / sizeof(vDoseValuesPercentArr[0]) );
  static const double dVolumeValuesCcArr[] = {2, 5};
  std::vector<double> dVolumeValuesCc( dVolumeValuesCcArr, dVolumeValuesCcArr +
    sizeof(dVolumeValuesCcArr) / sizeof(dVolumeValuesCcArr[0]) );
  static const double dVolumeValuesPercentArr[] = {5, 10};
  std::vector<double> dVolumeValuesPercent( dVolumeValuesPercentArr, dVolumeValuesPercentArr +
    sizeof(dVolumeValuesPercentArr) / sizeof(dVolumeValuesPercentArr[0]) );

  vtksys::SystemTools::RemoveFile(temporaryDvhMetricCsvFileName);
  dvhLogic->ExportDvhMetricsToCsv(temporaryDvhMetricCsvFileName,
    vDoseValuesCc, vDoseValuesPercent, dVolumeValuesCc, dVolumeValuesPercent);

  bool returnWithSuccess = true;

  // Compare CSV DVH tables
  double agreementAcceptancePercentage = -1.0;
  if (vtksys::SystemTools::FileExists(baselineDvhTableCsvFileName))
  {
    if (CompareCsvDvhTables(temporaryDvhTableCsvFileName, baselineDvhTableCsvFileName, maxDose,
      volumeDifferenceCriterion, doseToAgreementCriterion, agreementAcceptancePercentage) > 0)
    {
      std::cerr << "Failed to compare DVH table to baseline!" << std::endl;
      returnWithSuccess = false;
    }
  }
  else
  {
    std::cerr << "Failed to open baseline DVH table: " << baselineDvhTableCsvFileName << std::endl;
    returnWithSuccess = false;
  }

  std::cout << "Agreement percentage: " << std::fixed << std::setprecision(2) << agreementAcceptancePercentage << "% (acceptance rate: " << agreementAcceptancePercentageThreshold << "%)" << std::endl;

  if (agreementAcceptancePercentage < agreementAcceptancePercentageThreshold)
  {
    std::cerr << "Agreement acceptance percentage is below threshold: " << std::fixed << std::setprecision(2) << agreementAcceptancePercentage
      << " < " << agreementAcceptancePercentageThreshold << std::endl;
    returnWithSuccess = false;
  }

  // Compare CSV DVH metrics
  if (vtksys::SystemTools::FileExists(baselineDvhMetricCsvFileName)) // TODO: add warning when all the metric tables can be compared
  {
    if (CompareCsvDvhMetrics(temporaryDvhMetricCsvFileName, baselineDvhMetricCsvFileName, metricDifferenceThreshold) > 0)
    {
      std::cerr << "Failed to compare DVH table to baseline!" << std::endl;
      returnWithSuccess = false;
    }
    else
    {
      std::cout << "DVH metrics are within threshold compared to baseline." << std::endl;
    }
  }

  if (!returnWithSuccess)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// IMPORTANT: The baseline table has to be the one with smaller resolution!
int CompareCsvDvhTables(std::string dvhCsvFileName, std::string baselineCsvFileName,
                        double maxDose, double volumeDifferenceCriterion, double doseToAgreementCriterion,
                        double &agreementAcceptancePercentage)
{
  if (!vtksys::SystemTools::FileExists(baselineCsvFileName.c_str()))
  {
    std::cerr << "Loading baseline CSV DVH table from file '" << baselineCsvFileName << "' failed - the file does not exist!" << std::endl;
    return 1;
  }
  
  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramModuleLogic> csvReadLogic = vtkSmartPointer<vtkSlicerDoseVolumeHistogramModuleLogic>::New();

  // Collections of vtkDoubleArrays, with each vtkDoubleArray representing a structure and containing
  // an array of tuples which represent the dose and volume for the bins in that structure.
  vtkCollection* currentDvh = csvReadLogic->ReadCsvToDoubleArrayNode(dvhCsvFileName);
  vtkCollection* baselineDvh = csvReadLogic->ReadCsvToDoubleArrayNode(baselineCsvFileName);
 
  // Compare the current DVH to the baseline and determine mean and maximum difference
  agreementAcceptancePercentage = 0.0;
  int totalNumberOfBins = 0;
  int totalNumberOfAcceptedAgreements = 0;
  int numberOfAcceptedStructuresWith90 = 0;
  int numberOfAcceptedStructuresWith95 = 0;

  // Instantiate logic class for comparing DVH values
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramComparisonLogic> dvhCompareLogic = vtkSmartPointer<vtkSlicerDoseVolumeHistogramComparisonLogic>::New();

  if (currentDvh->GetNumberOfItems() != baselineDvh->GetNumberOfItems())
  {
    std::cerr << "Number of structures in the current and the baseline DVH tables do not match (" << currentDvh->GetNumberOfItems() << "<>" << baselineDvh->GetNumberOfItems() << ")!" << std::endl;
    return 1;
  }

  for (int structureIndex=0; structureIndex < currentDvh->GetNumberOfItems(); structureIndex++)
  {

    vtkMRMLDoubleArrayNode* currentStructure = vtkMRMLDoubleArrayNode::SafeDownCast(currentDvh->GetItemAsObject(structureIndex));
    vtkMRMLDoubleArrayNode* baselineStructure = vtkMRMLDoubleArrayNode::SafeDownCast(baselineDvh->GetItemAsObject(structureIndex));
  
    // Set the logic parameters
    dvhCompareLogic->SetDvh1DoubleArrayNode(currentStructure);
    dvhCompareLogic->SetDvh2DoubleArrayNode(baselineStructure);
    dvhCompareLogic->SetVolumeDifferenceCriterion(volumeDifferenceCriterion);
    dvhCompareLogic->SetDoseToAgreementCriterion(doseToAgreementCriterion);
    dvhCompareLogic->SetDoseMax(maxDose);
    
    // Calculate the agreement percentage for the current structure.
    double acceptedBinsRatio = dvhCompareLogic->CompareDvhTables();

    int numberOfBinsPerStructure = baselineStructure->GetArray()->GetNumberOfTuples();
    totalNumberOfBins += numberOfBinsPerStructure;

    // Calculate the number of accepted bins in the structure based on the percent of accepted bins.
    int numberOfAcceptedAgreementsPerStructure = (int)(0.5 + (acceptedBinsRatio/100) * numberOfBinsPerStructure); 
    totalNumberOfAcceptedAgreements += numberOfAcceptedAgreementsPerStructure;

    if (acceptedBinsRatio > 90)
    {
      ++numberOfAcceptedStructuresWith90;

      if (acceptedBinsRatio > 95)
      {
        ++numberOfAcceptedStructuresWith95;
      }
    }
    
    std::string structureName = currentStructure->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());

    std::ostringstream volumeAttributeNameStream;
    volumeAttributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
    std::string structureVolume = currentStructure->GetAttribute(volumeAttributeNameStream.str().c_str());

    std::cout << "Accepted agreements per structure (" << structureName << ", " << structureVolume << " cc): " << numberOfAcceptedAgreementsPerStructure
      << " out of " << numberOfBinsPerStructure << " (" << std::fixed << std::setprecision(2) << acceptedBinsRatio << "%)" << std::endl;

  } // end for

  std::cout << "Accepted structures with threshold of 90%: " << std::fixed << std::setprecision(2) << (double)numberOfAcceptedStructuresWith90 / (double)currentDvh->GetNumberOfItems() * 100.0 << std::endl;
  std::cout << "Accepted structures with threshold of 95%: " << std::fixed << std::setprecision(2) << (double)numberOfAcceptedStructuresWith95 / (double)currentDvh->GetNumberOfItems() * 100.0 << std::endl;

  agreementAcceptancePercentage = 100.0 * (double)totalNumberOfAcceptedAgreements / (double)totalNumberOfBins;

  currentDvh->Delete();
  baselineDvh->Delete();
  
  return 0;
}

//-----------------------------------------------------------------------------
double GetAgreementForDvhPlotPoint(std::vector<std::pair<double,double> >& referenceDvhPlot, std::vector<std::pair<double,double> >& compareDvhPlot,
                               unsigned int compareIndex, double totalVolume, double maxDose,
                               double volumeDifferenceCriterion, double doseToAgreementCriterion)
{
  // Formula is (based on the article Ebert2010):
  //   gamma(i) = min{ Gamma[(di, vi), (dr, vr)] } for all {r=1..P}, where
  //   compareIndexth DVH point has dose di and volume vi
  //   P is the number of bins in the reference DVH, each rth bin having absolute dose dr and volume vr
  //   Gamma[(di, vi), (dr, vr)] = [ ( (100*(vr-vi)) / (volumeDifferenceCriterion * totalVolume) )^2 + ( (100*(dr-di)) / (doseToAgreementCriterion * maxDose) )^2 ] ^ 1/2
  //   volumeDifferenceCriterion is the volume-difference criterion (% of the total structure volume, totalVolume)
  //   doseToAgreementCriterion is the dose-to-agreement criterion (% of the maximum dose, maxDose)
  // A value of gamma(i) < 1 indicates agreement for the DVH bin compareIndex

  if (compareIndex >= compareDvhPlot.size())
  {
    std::cerr << "Invalid bin index for compare plot! (" << compareIndex << ">=" << compareDvhPlot.size() << ")" << std::endl;
    return -1.0;
  }

  double gamma = DBL_MAX;
  double di = compareDvhPlot[compareIndex].first;
  double vi = compareDvhPlot[compareIndex].second;

  std::vector<std::pair<double,double> >::iterator referenceDvhPlotIt;
  for (referenceDvhPlotIt = referenceDvhPlot.begin(); referenceDvhPlotIt != referenceDvhPlot.end(); ++referenceDvhPlotIt)
  {
    double dr = referenceDvhPlotIt->first;
    double vr = referenceDvhPlotIt->second;
    double currentGamma = sqrt( pow((100.0*(vr-vi))/(volumeDifferenceCriterion*totalVolume),2) + pow((100.0*(dr-di))/(doseToAgreementCriterion*maxDose),2) );
    if (currentGamma < gamma)
    {
      gamma = currentGamma;
    }
  }

  return gamma;
}

//-----------------------------------------------------------------------------
int CompareCsvDvhMetrics(std::string dvhMetricsCsvFileName, std::string baselineDvhMetricCsvFileName, double metricDifferenceThreshold)
{
  if (!vtksys::SystemTools::FileExists(baselineDvhMetricCsvFileName.c_str()))
  {
    std::cerr << "Loading baseline CSV DVH table from file '" << baselineDvhMetricCsvFileName << "' failed - the file does not exist!" << std::endl;
    return 1;
  }

  std::vector<std::string> fieldNames;
  char currentLine[1024];
  char baselineLine[1024];

  std::ifstream currentStream;
  std::ifstream baselineStream;
  currentStream.open(dvhMetricsCsvFileName.c_str(), std::ifstream::in);
  baselineStream.open(baselineDvhMetricCsvFileName.c_str(), std::ifstream::in);

  bool firstLine = true;
  bool returnWithSuccess = true;

  while ( currentStream.getline(currentLine, 1024, '\n')
       && baselineStream.getline(baselineLine, 1024, '\n') )
  {
    std::string currentLineStr(currentLine);
    std::string baselineLineStr(baselineLine);

    size_t currentCommaPosition = currentLineStr.find(csvSeparatorCharacter);
    size_t baselineCommaPosition = baselineLineStr.find(csvSeparatorCharacter);

    // Collect field names
    if (firstLine)
    {
      while (currentCommaPosition != std::string::npos && baselineCommaPosition != std::string::npos)
      {
        fieldNames.push_back(currentLineStr.substr(0, currentCommaPosition));

        currentLineStr = currentLineStr.substr(currentCommaPosition+1);
        baselineLineStr = baselineLineStr.substr(baselineCommaPosition+1);

        currentCommaPosition = currentLineStr.find(csvSeparatorCharacter);
        baselineCommaPosition = baselineLineStr.find(csvSeparatorCharacter);
      }
      firstLine = false;
    }
    else
    {
      // Read all metrics from the current line
      int i=0;
      std::string structureName;
      while (currentCommaPosition != std::string::npos && baselineCommaPosition != std::string::npos)
      {
        if (i==0)
        {
          structureName = currentLineStr.substr(0, currentCommaPosition);
        }
        else
        {
          double doubleValue;
          {
            std::stringstream ss;
            ss << currentLineStr.substr(0, currentCommaPosition);
            ss >> doubleValue;
          }
          double currentMetric = doubleValue;
          {
            std::stringstream ss;
            ss << baselineLineStr.substr(0, baselineCommaPosition);
            ss >> doubleValue;
          }
          double baselineMetric = doubleValue;

          double error = DBL_MAX;
          if (baselineMetric < EPSILON && currentMetric < EPSILON)
          {
            error = 0.0;
          }
          else
          {
            error = currentMetric / baselineMetric - 1.0;
          }

          if (error > metricDifferenceThreshold)
          {
            std::cerr << "Difference of metric '" << fieldNames[i] << "' for structure '" << structureName << "' is too high! Current=" << currentMetric << ", Baseline=" << baselineMetric << std::endl;
            returnWithSuccess = false;
          }
        }

        currentLineStr = currentLineStr.substr(currentCommaPosition+1);
        baselineLineStr = baselineLineStr.substr(baselineCommaPosition+1);
        currentCommaPosition = currentLineStr.find(csvSeparatorCharacter);
        baselineCommaPosition = baselineLineStr.find(csvSeparatorCharacter);

        i++;
      }
    }

    if ( (currentCommaPosition != std::string::npos) != (baselineCommaPosition != std::string::npos) )
    {
      std::cerr << "Number of fields differ in the current and the baseline metric tables!" << std::endl;
      return 1;
    }
  }

  currentStream.close();
  baselineStream.close();

  if (!returnWithSuccess)
  {
    return 1;
  }

  return 0;
}
