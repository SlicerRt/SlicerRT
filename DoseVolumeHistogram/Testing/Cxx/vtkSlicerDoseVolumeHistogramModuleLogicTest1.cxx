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
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#define EPSILON 0.0001

/* Define case insensitive string compare for all supported platforms. */
#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP strcasecmp
#endif

std::string csvSeparatorCharacter(",");

//-----------------------------------------------------------------------------
int CompareCsvDvhTables(std::string dvhMetricsCsvFileName, std::string baselineCsvFileName, std::string doseUnitName,
                        std::vector<vtkMRMLDoubleArrayNode*> dvhNodes,
                        double volumeDifferenceCriterion, double doseToAgreementCriterion,
                        double &agreementAcceptancePercentage);

double GetAgreementForDvhPlotPoint(std::vector<std::pair<double,double> >& referenceDvhPlot,
                                   std::vector<std::pair<double,double> >& compareDvhPlot,
                                   int compareIndex, double totalVolume, double maxDose,
                                   double volumeDifferenceCriterion, double doseToAgreementCriterion);

int CompareCsvDvhMetrics(std::string dvhMetricsCsvFileName, std::string baselineDvhMetricCsvFileName, double metricDifferenceThreshold);

//-----------------------------------------------------------------------------
int vtkSlicerDoseVolumeHistogramModuleLogicTest1( int argc, char * argv[] )
{
  // Get temporary directory
  const char *dataDirectoryPath = NULL;
  if (argc > 2)
  {
    if (STRCASECMP(argv[1], "-DataDirectoryPath") == 0)
    {
      dataDirectoryPath = argv[2];
      std::cout << "Data directory path: " << dataDirectoryPath << std::endl;
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
  const char *baselineDirectoryPath = NULL;
  if (argc > 4)
  {
    if (STRCASECMP(argv[3], "-BaselineDirectoryPath") == 0)
    {
      baselineDirectoryPath = argv[4];
      std::cout << "Baseline directory path: " << baselineDirectoryPath << std::endl;
    }
    else
    {
      baselineDirectoryPath = "";
    }
  }
  const char *temporaryDirectoryPath = NULL;
  if (argc > 6)
  {
    if (STRCASECMP(argv[5], "-TemporaryDirectoryPath") == 0)
    {
      temporaryDirectoryPath = argv[6];
      std::cout << "Temporary directory path: " << temporaryDirectoryPath << std::endl;
    }
    else
    {
      temporaryDirectoryPath = "";
    }
  }
  double volumeDifferenceCriterion = 0.0;
  if (argc > 8)
  {
    if (STRCASECMP(argv[7], "-VolumeDifferenceCriterion") == 0)
    {
      volumeDifferenceCriterion = atof(argv[8]);
      std::cout << "Volume difference criterion: " << volumeDifferenceCriterion << std::endl;
    }
  }
  double doseToAgreementCriterion = 0.0;
  if (argc > 10)
  {
    if (STRCASECMP(argv[9], "-DoseToAgreementCriterion") == 0)
    {
      doseToAgreementCriterion = atof(argv[10]);
      std::cout << "Dose-to-agreement criterion: " << doseToAgreementCriterion << std::endl;
    }
  }
  double agreementAcceptancePercentageThreshold = 0.0;
  if (argc > 12)
  {
    if (STRCASECMP(argv[11], "-AgreementAcceptancePercentageThreshold") == 0)
    {
      agreementAcceptancePercentageThreshold = atof(argv[12]);
      std::cout << "Agreement acceptance percentage threshold: " << agreementAcceptancePercentageThreshold << std::endl;
    }
  }
  double metricDifferenceThreshold = 0.0;
  if (argc > 14)
  {
    if (STRCASECMP(argv[13], "-DvhStartValue") == 0)
    {
      metricDifferenceThreshold = atof(argv[14]);
      std::cout << "Metric difference threshold: " << metricDifferenceThreshold << std::endl;
    }
  }
  double dvhStartValue = 0.0;
  if (argc > 16)
  {
    if (STRCASECMP(argv[15], "-DvhStartValue") == 0)
    {
      dvhStartValue = atof(argv[16]);
      std::cout << "DVH start value: " << dvhStartValue << std::endl;
    }
  }
  double dvhStepSize = 0.0;
  if (argc > 18)
  {
    if (STRCASECMP(argv[15], "-DvhStepSize") == 0)
    {
      dvhStepSize = atof(argv[18]);
      std::cout << "DVH step size: " << dvhStepSize << std::endl;
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

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  std::string sceneFileName = std::string(temporaryDirectoryPath) + "/DvhTestScene.mrml";
  vtksys::SystemTools::RemoveFile(sceneFileName.c_str());
  mrmlScene->SetRootDirectory(temporaryDirectoryPath);
  mrmlScene->SetURL(sceneFileName.c_str());
  mrmlScene->Commit();

  // Create dose volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseScalarVolumeNode =
    vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  doseScalarVolumeNode->SetName("Dose");

  // Load and set attributes from file
  std::string doseAttributesFileName = std::string(dataDirectoryPath) + "/Dose.attributes";
  if (!vtksys::SystemTools::FileExists(doseAttributesFileName.c_str()))
  {
    std::cerr << "Loading dose attributes from file '" << doseAttributesFileName << "' failed - the file does not exist!" << std::endl;
  }

  std::string doseUnitName = "";
  std::ifstream attributesStream;
  attributesStream.open(doseAttributesFileName.c_str(), std::ifstream::in);
  char attribute[512];
  while (attributesStream.getline(attribute, 512, ';'))
  {
    std::string attributeStr(attribute);
    int colonIndex = attributeStr.find(':');
    std::string name = attributeStr.substr(0, colonIndex);
    std::string value = attributeStr.substr(colonIndex + 1);
    doseScalarVolumeNode->SetAttribute(name.c_str(), value.c_str());

    if (vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_UNIT_NAME_ATTRIBUTE_NAME.compare(name) == 0)
    {
      doseUnitName = value;
    }
  }
  attributesStream.close();

  mrmlScene->AddNode(doseScalarVolumeNode);
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load dose volume
  std::string doseVolumeFileName = std::string(dataDirectoryPath) + "/Dose.nrrd";
  if (!vtksys::SystemTools::FileExists(doseVolumeFileName.c_str()))
  {
    std::cerr << "Loading dose volume from file '" << doseVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> doseVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  doseVolumeArchetypeStorageNode->SetFileName(doseVolumeFileName.c_str());
  mrmlScene->AddNode(doseVolumeArchetypeStorageNode);
  //EXERCISE_BASIC_STORAGE_MRML_METHODS(vtkMRMLVolumeArchetypeStorageNode, doseVolumeArchetypeStorageNode);

  doseScalarVolumeNode->SetAndObserveStorageNodeID(doseVolumeArchetypeStorageNode->GetID());

  if (! doseVolumeArchetypeStorageNode->ReadData(doseScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading dose volume from file '" << doseVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create model hierarchy root node
  std::string hierarchyNodeName = "All structures";
  vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode =
    vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();  
  modelHierarchyRootNode->SetName(hierarchyNodeName.c_str());
  modelHierarchyRootNode->AllowMultipleChildrenOn();
  modelHierarchyRootNode->HideFromEditorsOff();
  mrmlScene->AddNode(modelHierarchyRootNode);
  //EXERCISE_BASIC_MRML_METHODS(vtkMRMLModelHierarchyNode, modelHierarchyNode)

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode =
    vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  hierarchyNodeName.append("Display");
  modelDisplayNode->SetName(hierarchyNodeName.c_str());
  modelDisplayNode->SetVisibility(1);
  mrmlScene->AddNode(modelDisplayNode);
  modelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );
  //EXERCISE_BASIC_DISPLAY_MRML_METHODS(vtkMRMLModelDisplayNode, displayNode) TODO: uncomment these and try if they work after the node in question is fully set

  // Load models and create nodes
  std::string structureNamesFileName = std::string(dataDirectoryPath) + "/Structure.names";
  if (!vtksys::SystemTools::FileExists(structureNamesFileName.c_str()))
  {
    std::cerr << "Loading structure names from file '" << structureNamesFileName << "' failed - the file does not exist!" << std::endl;
  }

  std::vector<std::string> structureNames;
  std::ifstream structureNamesStream;
  structureNamesStream.open(structureNamesFileName.c_str(), std::ifstream::in);
  char structureName[512];
  while (structureNamesStream.getline(structureName, 512, ';'))
  {
    structureNames.push_back(structureName);
  }
  structureNamesStream.close();

  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);
  std::vector<std::string>::iterator it;
  for (it = structureNames.begin(); it != structureNames.end(); ++it)
  {
    // Read polydata
    vtkSmartPointer<vtkPolyDataReader> modelPolyDataReader = vtkSmartPointer<vtkPolyDataReader>::New();
    std::string modelFileName = std::string(dataDirectoryPath) + "/" + (*it) + ".vtk";
    modelPolyDataReader->SetFileName(modelFileName.c_str());

    if (!vtksys::SystemTools::FileExists(modelFileName.c_str()))
    {
      std::cerr << "Loading structure model from file '" << modelFileName << "' failed - the file does not exist!" << std::endl;
    }

    // Create display node
    vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode =
      vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mrmlScene->AddNode(displayNode));
    //EXERCISE_BASIC_DISPLAY_MRML_METHODS(vtkMRMLModelDisplayNode, displayNode)
    displayNode->SliceIntersectionVisibilityOn();
    displayNode->VisibilityOn();

    // Create model node
    vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
    modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->AddNode(modelNode));
    //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLModelNode, modelNode)
    modelNode->SetName(it->c_str());
    modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
    modelNode->SetAndObservePolyData( modelPolyDataReader->GetOutput() );
    modelNode->SetHideFromEditors(0);
    modelNode->SetSelectable(1);

    // Create model hierarchy node
    vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode =
      vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
    mrmlScene->AddNode(modelHierarchyNode);
    //EXERCISE_BASIC_MRML_METHODS(vtkMRMLModelHierarchyNode, modelHierarchyNode)
    modelHierarchyNode->SetParentNodeID( modelHierarchyRootNode->GetID() );
    modelHierarchyNode->SetModelNodeID( modelNode->GetID() );
  }

  // Create chart node
  vtkSmartPointer<vtkMRMLChartNode> chartNode = vtkSmartPointer<vtkMRMLChartNode>::New();
  chartNode->SetProperty("default", "title", "Dose Volume Histogram");
  chartNode->SetProperty("default", "xAxisLabel", "Dose [Gy]");
  chartNode->SetProperty("default", "yAxisLabel", "Fractional volume [%]");
  chartNode->SetProperty("default", "type", "Line");
  mrmlScene->AddNode(chartNode);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramModuleLogic> dvhLogic =
    vtkSmartPointer<vtkSlicerDoseVolumeHistogramModuleLogic>::New();
  dvhLogic->SetMRMLScene(mrmlScene);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode> paramNode =
    vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New();
  paramNode->SetDoseVolumeNodeId(doseScalarVolumeNode->GetID());
  paramNode->SetStructureSetModelNodeId(modelHierarchyRootNode->GetID());
  paramNode->SetChartNodeId(chartNode->GetID());
  mrmlScene->AddNode(paramNode);
  dvhLogic->SetAndObserveDoseVolumeHistogramNode(paramNode);

  // Set start value and step size if specified
  if (dvhStartValue != 0.0 && dvhStepSize != 0.0)
  {
    dvhLogic->SetStartValue(dvhStartValue);
    dvhLogic->SetStepSize(dvhStepSize);
  }

  // Compute DVH and get result nodes
  dvhLogic->ComputeDvh();
  dvhLogic->RefreshDvhDoubleArrayNodesFromScene();

  std::set<std::string>* dvhNodeIDs = paramNode->GetDvhDoubleArrayNodeIds();
  if (dvhNodeIDs->size() != structureNames.size())
  {
    mrmlScene->Commit();
    std::cerr << "Invalid DVH node list!" << std::endl;
    return EXIT_FAILURE;
  }

  // Add DVH arrays to chart node
  std::vector<vtkMRMLDoubleArrayNode*> dvhNodes;
  std::set<std::string>::iterator dvhIt;
  for (dvhIt = dvhNodeIDs->begin(); dvhIt != dvhNodeIDs->end(); ++dvhIt)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      mrmlScene->GetNodeByID(dvhIt->c_str()));
    if (!dvhNode)
    {
      std::cerr << "Error: Invalid DVH node!" << std::endl;
      return EXIT_FAILURE;
    }

    chartNode->AddArray( dvhNode->GetName(), dvhNode->GetID() );    
    dvhNodes.push_back(dvhNode);
  }

  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);
  mrmlScene->Commit();

  // Export DVH to CSV
  std::string dvhCsvFileName = std::string(temporaryDirectoryPath) + "/DvhTestTable.csv";
  vtksys::SystemTools::RemoveFile(dvhCsvFileName.c_str());
  dvhLogic->ExportDvhToCsv(dvhCsvFileName.c_str());

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

  std::string dvhMetricsCsvFileName = std::string(temporaryDirectoryPath) + "/DvhTestMetrics.csv";
  vtksys::SystemTools::RemoveFile(dvhMetricsCsvFileName.c_str());
  dvhLogic->ExportDvhMetricsToCsv(dvhMetricsCsvFileName.c_str(),
    vDoseValuesCc, vDoseValuesPercent, dVolumeValuesCc, dVolumeValuesPercent);

  bool returnWithSuccess = true;

  // Compare CSV DVH tables
  std::string baselineDvhTableCsvFileName = std::string(baselineDirectoryPath) + "/BaselineDvhTable.csv";
  double agreementAcceptancePercentage = -1.0;
  if (CompareCsvDvhTables(dvhCsvFileName, baselineDvhTableCsvFileName, doseUnitName, dvhNodes, 
    volumeDifferenceCriterion, doseToAgreementCriterion, agreementAcceptancePercentage) > 0)
  {
    std::cerr << "Failed to compare DVH table to baseline!" << std::endl;
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
  std::string baselineDvhMetricCsvFileName = std::string(baselineDirectoryPath) + "/BaselineDvhMetrics.csv";
  if (vtksys::SystemTools::FileExists(baselineDvhMetricCsvFileName.c_str())) // TODO: remove when all the metric tables can be compared
  {
    if (CompareCsvDvhMetrics(dvhMetricsCsvFileName, baselineDvhMetricCsvFileName, metricDifferenceThreshold) > 0)
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
int CompareCsvDvhTables(std::string dvhCsvFileName, std::string baselineCsvFileName, std::string doseUnitName,
                        std::vector<vtkMRMLDoubleArrayNode*> dvhNodes,
                        double volumeDifferenceCriterion, double doseToAgreementCriterion,
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

  char line[1024];
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
    while (dvhStream.getline(line, 1024, '\n'))
    {
      std::string lineStr(line);
      size_t commaPosition = lineStr.find(csvSeparatorCharacter);

      // Determine number of fields (twice the number of structures)
      if (firstLine)
      {
        while (commaPosition != std::string::npos)
        {
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
        double dose = atof(lineStr.substr(0, commaPosition).c_str());

        lineStr = lineStr.substr(commaPosition+1);
        commaPosition = lineStr.find(csvSeparatorCharacter);
        double percent = atof(lineStr.substr(0, commaPosition).c_str());

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
    std::cerr << "Number of structures in the current and the baseline DVH tables do not match!" << std::endl;
    return 1;
  }

  // Compare the current DVH to the baseline and determine mean and maximum difference
  agreementAcceptancePercentage = 0.0;
  int totalNumberOfBins = 0;
  int totalNumberOfAcceptedAgreements = 0;
  std::vector< std::vector<std::pair<double,double> > >::iterator currentIt;
  std::vector< std::vector<std::pair<double,double> > >::iterator baselineIt;
  std::vector<vtkMRMLDoubleArrayNode*>::iterator dvhNodeIt;

  for (currentIt = currentDvh.begin(), baselineIt = baselineDvh.begin(), dvhNodeIt = dvhNodes.begin();
    currentIt != currentDvh.end(); ++currentIt, ++baselineIt, ++dvhNodeIt)
  {
    int numberOfBinsPerStructure = 0;
    int numberOfAcceptedAgreementsPerStructure = 0;

    const char* structureName = (*dvhNodeIt)->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());

    std::string totalVolumeAttributeName = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX + vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
    const char* totalVolumeStr = (*dvhNodeIt)->GetAttribute(totalVolumeAttributeName.c_str());
    double totalVolume = atof(totalVolumeStr);

    double maxDose = 0.0;
    const char* maxDoseStr = NULL;
    char maxDoseAttributeName[64];
    vtkSlicerDoseVolumeHistogramModuleLogic::AssembleDoseMetricAttributeName(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName.c_str(), maxDoseAttributeName);
    maxDoseStr = (*dvhNodeIt)->GetAttribute(maxDoseAttributeName);
    if (maxDoseStr)
    {
      maxDose = atof(maxDoseStr);
    }

    if (totalVolume == 0.0 || maxDose == 0.0)
    {
      std::cerr << "Invalid attribute in DVH node " << (*dvhNodeIt)->GetName() << ": " << totalVolumeAttributeName << "=" << totalVolumeStr << ", " << maxDoseAttributeName << "=" << (maxDoseStr?maxDoseStr:"NULL") << std::endl;
      continue;
    }

    for (int i=0; i<currentIt->size(); ++i)
    {
      double agreement = GetAgreementForDvhPlotPoint(*baselineIt, *currentIt, i, totalVolume, maxDose, volumeDifferenceCriterion, doseToAgreementCriterion);

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

    std::cout << "Accepted agreements per structure (" << (structureName?structureName:"NULL(error!)") << "): " << numberOfAcceptedAgreementsPerStructure
      << " out of " << numberOfBinsPerStructure << " (" << std::fixed << std::setprecision(2) << 100.0 * (double)numberOfAcceptedAgreementsPerStructure / (double)numberOfBinsPerStructure << "%)" << std::endl;
  }

  agreementAcceptancePercentage = 100.0 * (double)totalNumberOfAcceptedAgreements / (double)totalNumberOfBins;

  return 0;
}

//-----------------------------------------------------------------------------
double GetAgreementForDvhPlotPoint(std::vector<std::pair<double,double> >& referenceDvhPlot, std::vector<std::pair<double,double> >& compareDvhPlot,
                               int compareIndex, double totalVolume, double maxDose,
                               double volumeDifferenceCriterion, double doseToAgreementCriterion)
{
  // Formula is (based on the article Elbert2010):
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
    double Gamma = sqrt( pow((100.0*(vr-vi))/(volumeDifferenceCriterion*totalVolume),2) + pow((100.0*(dr-di))/(doseToAgreementCriterion*maxDose),2) );
    if (Gamma < gamma)
    {
      gamma = Gamma;
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
          i++;
          continue;
        }

        double currentMetric = atof(currentLineStr.substr(0, currentCommaPosition).c_str());
        double baselineMetric = atof(baselineLineStr.substr(0, baselineCommaPosition).c_str());

        currentLineStr = currentLineStr.substr(currentCommaPosition+1);
        baselineLineStr = baselineLineStr.substr(baselineCommaPosition+1);

        currentCommaPosition = currentLineStr.find(csvSeparatorCharacter);
        baselineCommaPosition = baselineLineStr.find(csvSeparatorCharacter);

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
          std::cerr << "Difference of metric '" << fieldNames[i] << "' for structure '" << structureName << "' is too high! Current=" << currentMetric << "Baseline=" << baselineMetric;
          returnWithSuccess = false;
        }

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
