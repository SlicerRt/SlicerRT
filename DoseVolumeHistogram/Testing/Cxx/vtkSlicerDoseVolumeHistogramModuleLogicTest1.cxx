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

// DoseVolumeHistogram includes
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLChartNode.h>

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkLookupTable.h>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

std::string csvSeparatorCharacter(",");

//-----------------------------------------------------------------------------
int CompareCsvDvhTables(std::string dvhMetricsCsvFileName, std::string baselineCsvFileName,
                        double maxDose, double volumeDifferenceCriterion, double doseToAgreementCriterion,
                        double &agreementAcceptancePercentage);

double GetAgreementForDvhPlotPoint(std::vector<std::pair<double,double> >& referenceDvhPlot,
                                   std::vector<std::pair<double,double> >& compareDvhPlot,
                                   unsigned int compareIndex, double totalVolume, double maxDose,
                                   double volumeDifferenceCriterion, double doseToAgreementCriterion);

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
  // RasterizationOversamplingFactor
  double rasterizationOversamplingFactor = 2.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-RasterizationOversamplingFactor") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      rasterizationOversamplingFactor = doubleValue;
      std::cout << "Rasterization oversampling factor: " << rasterizationOversamplingFactor << std::endl;
      argIndex += 2;
    }
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
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create Contours logic
  vtkSmartPointer<vtkSlicerContoursModuleLogic> contoursLogic =
    vtkSmartPointer<vtkSlicerContoursModuleLogic>::New();
  contoursLogic->SetMRMLScene(mrmlScene);

  // TODO: Remove when subject hierarchy is integrated into Slicer core
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> subjectHierarchyLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  subjectHierarchyLogic->SetMRMLScene(mrmlScene);

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

  // Get contour set subject hierarchy node
  vtkMRMLSubjectHierarchyNode* contourHierarchyNode = NULL;
  mrmlScene->InitTraversal();
  vtkMRMLNode *node = mrmlScene->GetNextNodeByClass("vtkMRMLSubjectHierarchyNode");
  while (node != NULL)
  {
    vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(node);
    if (shNode)
    {
      const char* contourHierarchyIdentifier = shNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (contourHierarchyIdentifier != NULL)
      {
        contourHierarchyNode = shNode;
        break;
      }
    }
    node = mrmlScene->GetNextNodeByClass("vtkMRMLSubjectHierarchyNode");
  }
  if (!contourHierarchyNode)
  {
    mrmlScene->Commit();
    std::cerr << "ERROR: Failed to get contour hierarchy node!" << std::endl;
    return EXIT_FAILURE;
  }

  // Determine maximum dose
  vtkNew<vtkImageAccumulate> doseStat;
  doseStat->SetInput(doseScalarVolumeNode->GetImageData());
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
  paramNode->SetAndObserveContourSetContourNode(contourHierarchyNode);
  paramNode->SetAndObserveChartNode(chartNode);
  mrmlScene->AddNode(paramNode);
  dvhLogic->SetAndObserveDoseVolumeHistogramNode(paramNode);

  // Set start value and step size if specified
  if (dvhStartValue != 0.0 && dvhStepSize != 0.0)
  {
    dvhLogic->SetStartValue(dvhStartValue);
    dvhLogic->SetStepSize(dvhStepSize);
  }

  // Get list of contours from the contour hierarchy node
  std::vector<vtkMRMLContourNode*> selectedContourNodes;
  vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(contourHierarchyNode, selectedContourNodes);

  // Compute DVH and get result nodes
  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = selectedContourNodes.begin(); contourIt != selectedContourNodes.end(); ++contourIt)
  {
    std::cout << "Computing DVH for contour '" << (*contourIt)->GetName() << "'" << std::endl;

    std::string errorMessage;
    dvhLogic->ComputeDvh((*contourIt), errorMessage);

    if (!errorMessage.empty())
    {
      std::cerr << errorMessage << std::endl;
      return EXIT_FAILURE;
    }
  }

  dvhLogic->RefreshDvhDoubleArrayNodesFromScene();

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

  // Vector of structures, each containing a vector of tuples
  std::vector< std::vector<std::pair<double,double> > > currentDvh;
  std::vector< std::vector<std::pair<double,double> > > baselineDvh;
  std::vector< std::vector<std::pair<double,double> > >* dvh;
  std::vector<std::string> structureNames;
  std::vector<double> structureVolumeCCs;

  char line[16384];
  for (int i=0; i<2; ++i)
  {
    std::ifstream dvhStream;
    if (i==0)
    {
      // Load current DVH from CSV
      dvhStream.open(dvhCsvFileName.c_str(), std::ifstream::in);
      dvh = &currentDvh;
    }
    else
    {
      // Load baseline DVH from CSV
      dvhStream.open(baselineCsvFileName.c_str(), std::ifstream::in);
      dvh = &baselineDvh;
    }

    bool firstLine = true;
    int fieldCount = 0;
    while (dvhStream.getline(line, 16384, '\n'))
    {
      std::string lineStr(line);
      size_t commaPosition = lineStr.find(csvSeparatorCharacter);

      // Determine number of fields (twice the number of structures)
      if (firstLine)
      {
        while (commaPosition != std::string::npos)
        {
          // Parse structure name and total volume
          if (i==0 && fieldCount%2==1)
          {
            std::string field = lineStr.substr(0, commaPosition);
            size_t middlePosition = field.find(SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE);
            structureNames.push_back(field.substr(0, middlePosition - SlicerRtCommon::DVH_ARRAY_NODE_NAME_POSTFIX.size()));

            std::string structureVolumeString = field.substr( middlePosition + SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size(), 
              field.size() - middlePosition - SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size() - SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_END.size() );

            std::stringstream ss;
            ss << structureVolumeString;
            double doubleValue;
            ss >> doubleValue;
            double structureVolume = doubleValue;
            if (structureVolume == 0)
            {
              std::cerr << "Invalid structure volume in CSV header field " << field << std::endl;
            }

            structureVolumeCCs.push_back(structureVolume);
          }

          fieldCount++;
          lineStr = lineStr.substr(commaPosition+1);
          commaPosition = lineStr.find(csvSeparatorCharacter);
        }
        if (! lineStr.empty() )
        {
          fieldCount++;
        }
        dvh->resize((int)fieldCount/2);
        firstLine = false;
        continue;
      }

      // Read all tuples from the current line
      int structureNumber = 0;
      while (commaPosition != std::string::npos)
      {
        double doubleValue;
        {
          std::stringstream ss;
          ss << lineStr.substr(0, commaPosition);
          ss >> doubleValue;
        }
        double dose = doubleValue;

        lineStr = lineStr.substr(commaPosition+1);
        commaPosition = lineStr.find(csvSeparatorCharacter);
        {
          std::stringstream ss;
          ss << lineStr.substr(0, commaPosition);
          ss >> doubleValue;
        }
        double percent = doubleValue;

        if ((dose != 0.0 || percent != 0.0) && (commaPosition > 0))
        {
          std::pair<double,double> tuple(dose, percent);
          dvh->at(structureNumber).push_back(tuple);
        }

        lineStr = lineStr.substr(commaPosition+1);
        commaPosition = lineStr.find(csvSeparatorCharacter);
        structureNumber++;
      }
    }
    dvhStream.close();
  }

  if (currentDvh.size() != baselineDvh.size())
  {
    std::cerr << "Number of structures in the current and the baseline DVH tables do not match (" << currentDvh.size() << "<>" << baselineDvh.size() << ")!" << std::endl;
    return 1;
  }

  // Compare the current DVH to the baseline and determine mean and maximum difference
  agreementAcceptancePercentage = 0.0;
  int totalNumberOfBins = 0;
  int totalNumberOfAcceptedAgreements = 0;
  int numberOfAcceptedStructuresWith90 = 0;
  int numberOfAcceptedStructuresWith95 = 0;
  std::vector< std::vector<std::pair<double,double> > >::iterator currentIt;
  std::vector< std::vector<std::pair<double,double> > >::iterator baselineIt;

  int structureIndex = 0;
  for (currentIt = currentDvh.begin(), baselineIt = baselineDvh.begin();
    currentIt != currentDvh.end(); ++currentIt, ++baselineIt, ++structureIndex)
  {
    int numberOfBinsPerStructure = 0;
    int numberOfAcceptedAgreementsPerStructure = 0;

    // Compute gamma for each bin in the current structure
    for (unsigned int i=0; i<baselineIt->size(); ++i)
    {
      double agreement = GetAgreementForDvhPlotPoint(*currentIt, *baselineIt, i, structureVolumeCCs[structureIndex],
        maxDose, volumeDifferenceCriterion, doseToAgreementCriterion);

      //ofstream test;
      //std::string testFilename = "d:\\gamma_" + structureNames[structureIndex] + ".log";
      //test.open(testFilename.c_str(), ios::app);
      //test << baselineIt->at(i).first << "," << baselineIt->at(i).second << "," << agreement << std::endl;
      //test.close();

      if (agreement == -1.0)
      {
        std::cerr << "Invalid agreement, skipped!" << std::endl;
        continue;
      }
      if (agreement <= 1.0)
      {
        numberOfAcceptedAgreementsPerStructure++;
        totalNumberOfAcceptedAgreements++;
      }

      numberOfBinsPerStructure++;
      totalNumberOfBins++;
    }

    double acceptedBinsRatio = 100.0 * (double)numberOfAcceptedAgreementsPerStructure / (double)numberOfBinsPerStructure;

    if (acceptedBinsRatio > 90)
    {
      numberOfAcceptedStructuresWith90++;

      if (acceptedBinsRatio > 95)
      {
        numberOfAcceptedStructuresWith95++;
      }
    }
      
    std::cout << "Accepted agreements per structure (" << structureNames[structureIndex] << ", " << structureVolumeCCs[structureIndex] << " cc): " << numberOfAcceptedAgreementsPerStructure
      << " out of " << numberOfBinsPerStructure << " (" << std::fixed << std::setprecision(2) << acceptedBinsRatio << "%)" << std::endl;
  } // For all structures

  std::cout << "Accepted structures with threshold of 90%: " << std::fixed << std::setprecision(2) << (double)numberOfAcceptedStructuresWith90 / (double)currentDvh.size() * 100.0 << std::endl;
  std::cout << "Accepted structures with threshold of 95%: " << std::fixed << std::setprecision(2) << (double)numberOfAcceptedStructuresWith95 / (double)currentDvh.size() * 100.0 << std::endl;

  agreementAcceptancePercentage = 100.0 * (double)totalNumberOfAcceptedAgreements / (double)totalNumberOfBins;

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
