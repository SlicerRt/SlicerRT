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

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
int vtkSlicerDoseVolumeHistogramLogicTest1( int argc, char * argv[] )
{
  // Get temporary directory
  const char *sourceDirectoryPath = NULL;
  if (argc > 2)
  {
    if (stricmp(argv[1], "-D") == 0)
    {
      sourceDirectoryPath = argv[2];
      std::cout << "Source directory: " << sourceDirectoryPath << std::endl;
    }
    else
    {
      sourceDirectoryPath = "";
    }
  }
  const char *temporaryDirectoryPath = NULL;
  if (argc > 4)
  {
    if (stricmp(argv[3], "-T") == 0)
    {
      temporaryDirectoryPath = argv[4];
      std::cout << "Temporary directory: " << temporaryDirectoryPath << std::endl;
    }
    else
    {
      temporaryDirectoryPath = "";
    }
  }

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  std::string sceneFileName = std::string(temporaryDirectoryPath) + "/DvhTestScene.mrml";
  vtksys::SystemTools::RemoveFile(sceneFileName.c_str());
  mrmlScene->SetRootDirectory(temporaryDirectoryPath);
  mrmlScene->SetURL(sceneFileName.c_str());
  mrmlScene->Commit();

  // Load dose volume
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  doseScalarVolumeNode->SetName("Dose");
  doseScalarVolumeNode->SetAttribute("DoseUnitName", "GY");
  doseScalarVolumeNode->SetAttribute("DoseUnitValue", "4.4812099e-5");
  doseScalarVolumeNode->SetAttribute("LabelMap", "0");
  mrmlScene->AddNode(doseScalarVolumeNode);
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(doseScalarVolumeNode);

  std::string doseVolumeFileName = std::string(sourceDirectoryPath) + "/Data/Dose.nrrd";
  std::cout << "Loading dose volume from file '" << doseVolumeFileName << "' ("
    << (vtksys::SystemTools::FileExists(doseVolumeFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> doseVolumeArchetypeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  doseVolumeArchetypeStorageNode->SetFileName(doseVolumeFileName.c_str());
  mrmlScene->AddNode(doseVolumeArchetypeStorageNode);
  //EXERCISE_BASIC_STORAGE_MRML_METHODS(doseVolumeArchetypeStorageNode);

  doseScalarVolumeNode->SetAndObserveStorageNodeID(doseVolumeArchetypeStorageNode->GetID());

  if (! doseVolumeArchetypeStorageNode->ReadData(doseScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading dose volume from file '" << doseVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create model hierarchy root node
  std::string hierarchyNodeName = "RTSTRUCT: PROS - all structures";
  vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();  
  modelHierarchyRootNode->SetName(hierarchyNodeName.c_str());
  modelHierarchyRootNode->AllowMultipleChildrenOn();
  modelHierarchyRootNode->HideFromEditorsOff();
  mrmlScene->AddNode(modelHierarchyRootNode);
  //EXERCISE_BASIC_MRML_METHODS(vtkMRMLModelHierarchyNode, modelHierarchyNode)

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> modelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  hierarchyNodeName.append("Display");
  modelDisplayNode->SetName(hierarchyNodeName.c_str());
  modelDisplayNode->SetVisibility(1);
  mrmlScene->AddNode(modelDisplayNode);
  modelHierarchyRootNode->SetAndObserveDisplayNodeID( modelDisplayNode->GetID() );
  //EXERCISE_BASIC_DISPLAY_MRML_METHODS(vtkMRMLModelDisplayNode, displayNode) TODO: uncomment these and try if they work after the node in question is fully set

  // Load models and create nodes
  const char* testModelNames[5] = {"Bladder", "FemoralHeadLt", "FemoralHeadRT", "PTV", "Rectum"};
  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  for (int i=0; i<5; ++i)
  {
    // Read polydata
    vtkSmartPointer<vtkPolyDataReader> modelPolyDataReader = vtkSmartPointer<vtkPolyDataReader>::New();
    std::string modelFileName = std::string(sourceDirectoryPath) + "/Data/" + std::string(testModelNames[i]) + ".vtk";
    modelPolyDataReader->SetFileName(modelFileName.c_str());

    std::cout << "Loading structure model from file '" << modelFileName << "' ("
      << (vtksys::SystemTools::FileExists(modelFileName.c_str()) ? "Exists" : "Does not exist!") << ")" << std::endl;

    // Create display node
    vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mrmlScene->AddNode(displayNode));
    //EXERCISE_BASIC_DISPLAY_MRML_METHODS(vtkMRMLModelDisplayNode, displayNode) TODO: uncomment these and try if they work after the node in question is fully set
    displayNode->SetModifiedSinceRead(1);
    displayNode->SliceIntersectionVisibilityOn();
    displayNode->VisibilityOn();

    // Create model node
    vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
    modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->AddNode(modelNode));
    //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLModelNode, modelNode)
    modelNode->SetName(testModelNames[i]);
    modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
    modelNode->SetAndObservePolyData( modelPolyDataReader->GetOutput() );
    modelNode->SetModifiedSinceRead(1);
    modelNode->SetHideFromEditors(0);
    modelNode->SetSelectable(1);

    // Create model hierarchy node
    vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
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
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramLogic> dvhLogic = vtkSmartPointer<vtkSlicerDoseVolumeHistogramLogic>::New();
  dvhLogic->SetMRMLScene(mrmlScene);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode> paramNode = vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New();
  paramNode->SetDoseVolumeNodeId(doseScalarVolumeNode->GetID());
  paramNode->SetStructureSetModelNodeId(modelHierarchyRootNode->GetID());
  paramNode->SetChartNodeId(chartNode->GetID());
  mrmlScene->AddNode(paramNode);
  dvhLogic->SetAndObserveDoseVolumeHistogramNode(paramNode);

  // Compute DVH and get result nodes
  dvhLogic->ComputeDvh();
  dvhLogic->RefreshDvhDoubleArrayNodesFromScene();

  std::set<std::string>* dvhNodes = paramNode->GetDvhDoubleArrayNodeIds();
  if (dvhNodes->size() < 1)
  {
    mrmlScene->Commit();
    std::cerr << "No DVH nodes created!" << std::endl;
    return EXIT_FAILURE;
  }

  // Add DVH arrays to chart node
  std::set<std::string>::iterator dvhIt;
  int i;
  for (i=0, dvhIt = dvhNodes->begin(); dvhIt != dvhNodes->end(); ++dvhIt, ++i)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(mrmlScene->GetNodeByID(dvhIt->c_str()));
    if (!dvhNode)
    {
      std::cerr << "Error: Item " << i << " in DVH node list is not valid!" << std::endl;
      continue;
    }

    chartNode->AddArray( testModelNames[i], dvhNode->GetID() );    
  }

  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);
  mrmlScene->Commit();

  // Export DVH to CSV
  std::string dvhCsvFileName = std::string(temporaryDirectoryPath) + "/DvhTestTable.csv";
  vtksys::SystemTools::RemoveFile(dvhCsvFileName.c_str());
  dvhLogic->ExportDvhToCsv(dvhCsvFileName.c_str());

  // Export DVH metrics to CSV
  static const double vDoseValuesCcArr[] = {5, 20};
  std::vector<double> vDoseValuesCc( vDoseValuesCcArr, vDoseValuesCcArr + sizeof(vDoseValuesCcArr) / sizeof(vDoseValuesCcArr[0]) );
  static const double vDoseValuesPercentArr[] = {5, 20};
  std::vector<double> vDoseValuesPercent( vDoseValuesPercentArr, vDoseValuesPercentArr + sizeof(vDoseValuesPercentArr) / sizeof(vDoseValuesPercentArr[0]) );
  static const double dVolumeValuesArr[] = {5, 10};
  std::vector<double> dVolumeValues( dVolumeValuesArr, dVolumeValuesArr + sizeof(dVolumeValuesArr) / sizeof(dVolumeValuesArr[0]) );

  std::string dvhMetricsCsvFileName = std::string(temporaryDirectoryPath) + "/DvhTestMetrics.csv";
  vtksys::SystemTools::RemoveFile(dvhMetricsCsvFileName.c_str());
  dvhLogic->ExportDvhMetricsToCsv(dvhMetricsCsvFileName.c_str(), vDoseValuesCc, vDoseValuesPercent, dVolumeValues);

  return EXIT_SUCCESS;  
}
