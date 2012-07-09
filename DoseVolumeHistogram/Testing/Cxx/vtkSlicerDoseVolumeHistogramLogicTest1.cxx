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
#include "vtkSlicerDoseVolumeHistogramLogic.h"
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

std::string csvSeparatorCharacter(",");

//-----------------------------------------------------------------------------
int CompareCsvDvhTables(std::string dvhMetricsCsvFileName, std::string baselineCsvFileName,
                        double &differenceMeanPercent, double &differenceMaxPercent);

//-----------------------------------------------------------------------------
int vtkSlicerDoseVolumeHistogramLogicTest1( int argc, char * argv[] )
{
  // Get temporary directory
  const char *dataDirectoryPath = NULL;
  if (argc > 2)
  {
    if (stricmp(argv[1], "-DataDirectoryPath") == 0)
    {
      dataDirectoryPath = argv[2];
      std::cout << "Data directory path: " << dataDirectoryPath << std::endl;
    }
    else
    {
      dataDirectoryPath = "";
    }
  }
  const char *temporaryDirectoryPath = NULL;
  if (argc > 4)
  {
    if (stricmp(argv[3], "-TemporaryDirectoryPath") == 0)
    {
      temporaryDirectoryPath = argv[4];
      std::cout << "Temporary directory path: " << temporaryDirectoryPath << std::endl;
    }
    else
    {
      temporaryDirectoryPath = "";
    }
  }
  double toleranceMeanPercent = 0.0;
  if (argc > 6)
  {
    if (stricmp(argv[5], "-ToleranceMeanPercent") == 0)
    {
      toleranceMeanPercent = atof(argv[6]);
      std::cout << "Tolerance mean percent: " << toleranceMeanPercent << std::endl;
    }
  }
  double toleranceMaxPercent = 0.0;
  if (argc > 8)
  {
    if (stricmp(argv[7], "-ToleranceMaxPercent") == 0)
    {
      toleranceMaxPercent = atof(argv[8]);
      std::cout << "Tolerance max percent: " << toleranceMaxPercent << std::endl;
    }
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
  std::cout << "Loading dose attributes from file '" << doseAttributesFileName << "' ("
    << (vtksys::SystemTools::FileExists(doseAttributesFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

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
  }
  attributesStream.close();

  mrmlScene->AddNode(doseScalarVolumeNode);
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load dose volume
  std::string doseVolumeFileName = std::string(dataDirectoryPath) + "/Dose.nrrd";
  std::cout << "Loading dose volume from file '" << doseVolumeFileName << "' ("
    << (vtksys::SystemTools::FileExists(doseVolumeFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

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
  std::cout << "Loading structure names from file '" << structureNamesFileName << "' ("
    << (vtksys::SystemTools::FileExists(structureNamesFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

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

    std::cout << "Loading structure model from file '" << modelFileName << "' ("
      << (vtksys::SystemTools::FileExists(modelFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

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
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramLogic> dvhLogic =
    vtkSmartPointer<vtkSlicerDoseVolumeHistogramLogic>::New();
  dvhLogic->SetMRMLScene(mrmlScene);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode> paramNode =
    vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New();
  paramNode->SetDoseVolumeNodeId(doseScalarVolumeNode->GetID());
  paramNode->SetStructureSetModelNodeId(modelHierarchyRootNode->GetID());
  paramNode->SetChartNodeId(chartNode->GetID());
  mrmlScene->AddNode(paramNode);
  dvhLogic->SetAndObserveDoseVolumeHistogramNode(paramNode);

  // Compute DVH and get result nodes
  dvhLogic->ComputeDvh();
  dvhLogic->RefreshDvhDoubleArrayNodesFromScene();

  std::set<std::string>* dvhNodes = paramNode->GetDvhDoubleArrayNodeIds();
  if (dvhNodes->size() != structureNames.size())
  {
    mrmlScene->Commit();
    std::cerr << "Invalid DVH node list!" << std::endl;
    return EXIT_FAILURE;
  }

  // Add DVH arrays to chart node
  std::set<std::string>::iterator dvhIt;
  for (dvhIt = dvhNodes->begin(); dvhIt != dvhNodes->end(); ++dvhIt)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      mrmlScene->GetNodeByID(dvhIt->c_str()));
    if (!dvhNode)
    {
      std::cerr << "Error: Invalid DVH node!" << std::endl;
      return EXIT_FAILURE;
    }

    chartNode->AddArray( dvhNode->GetName(), dvhNode->GetID() );    
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

  std::string baselineCsvFileName = std::string(dataDirectoryPath) + "/BaselineDvhTable.csv";

  // Compare CSV DVH tables
  double differenceMeanPercent = 100.0;
  double differenceMaxPercent = 100.0;
  if (CompareCsvDvhTables(dvhCsvFileName, baselineCsvFileName,
    differenceMeanPercent, differenceMaxPercent) > 0)
  {
    std::cerr << "Failed to compare DVH table to baseline!" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Mean difference: " << differenceMeanPercent
    << " (tolerance: " << toleranceMeanPercent << ")" << std::endl;
  std::cout << "Max difference: " << differenceMaxPercent
    << " (tolerance: " << toleranceMaxPercent << ")" << std::endl;

  bool differenceBelowTolerance = true;
  if (differenceMeanPercent > toleranceMeanPercent)
  {
    std::cerr << "Mean difference is greater than the input tolerance! " << differenceMeanPercent
      << " > " << toleranceMeanPercent << std::endl;
    differenceBelowTolerance = false;
  }
  if (differenceMaxPercent > toleranceMaxPercent)
  {
    std::cerr << "Max difference is greater than the input tolerance! " << differenceMaxPercent
      << " > " << toleranceMaxPercent << std::endl;
    differenceBelowTolerance = false;
  }
  if (!differenceBelowTolerance)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
int CompareCsvDvhTables(std::string dvhCsvFileName, std::string baselineCsvFileName,
                        double &differenceMeanPercent, double &differenceMaxPercent)
{
  std::cout << "Loading baseline CSV DVH table from file '" << baselineCsvFileName << "' ("
    << (vtksys::SystemTools::FileExists(baselineCsvFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

  // Vector of structures, each containing a vector of tuples
  std::vector< std::vector<std::pair<double,double>> > currentDvh;
  std::vector< std::vector<std::pair<double,double>> > baselineDvh;
  std::vector< std::vector<std::pair<double,double>> >* dvh;

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
  differenceMaxPercent = 0.0;
  double sumDifferencePercent = 0.0;
  int numberOfDifferences = 0;
  std::vector< std::vector<std::pair<double,double>> >::iterator currentIt;
  std::vector< std::vector<std::pair<double,double>> >::iterator baselineIt;
  for (currentIt = currentDvh.begin(), baselineIt = baselineDvh.begin();
    currentIt != currentDvh.end(); ++currentIt, ++baselineIt)
  {
    vtkNew<vtkPiecewiseFunction> interpolator;
    interpolator->ClampingOn();
    for (std::vector<std::pair<double,double>>::iterator baselineTupleIt = baselineIt->begin();
      baselineTupleIt != baselineIt->end(); ++baselineTupleIt)
    {
      interpolator->AddPoint(baselineTupleIt->first, baselineTupleIt->second);
    }

    double differenceMaxPercentPerStructure = 0.0;
    double sumdifferencePercentPerStructure = 0.0;
    int numberOfDifferencesPerStructure = 0;
    for (std::vector<std::pair<double,double>>::iterator currentTupleIt = currentIt->begin();
      currentTupleIt != currentIt->end(); ++currentTupleIt)
    {
      double differencePercent = fabs( currentTupleIt->second
                                       - interpolator->GetValue(currentTupleIt->first) );
      if (differencePercent > differenceMaxPercent)
      {
        differenceMaxPercent = differencePercent;
      }
      if (differencePercent > differenceMaxPercentPerStructure)
      {
        differenceMaxPercentPerStructure = differencePercent;
      }
      sumDifferencePercent += differencePercent;
      sumdifferencePercentPerStructure += differencePercent;
      numberOfDifferences++;
      numberOfDifferencesPerStructure++;
    }

    std::cout << "Difference per structure: Mean=" << sumdifferencePercentPerStructure/numberOfDifferencesPerStructure
      << ", Max=" << differenceMaxPercentPerStructure << std::endl;
  }

  differenceMeanPercent = sumDifferencePercent / numberOfDifferences;

  return 0;
}
