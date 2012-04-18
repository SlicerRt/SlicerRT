#include "vtkSlicerDoseVolumeHistogramLogic.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>

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
  const char *temporaryDirectoryPath = NULL;
  if (argc > 2)
  {
    if (stricmp(argv[1], "-T") == 0)
    {
      temporaryDirectoryPath = argv[2];
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

  std::string doseVolumeFileName = std::string(temporaryDirectoryPath) + "/Dose.nrrd";
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
    std::string modelFileName = std::string(temporaryDirectoryPath) + "/" + std::string(testModelNames[i]) + ".vtk";
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

  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerDoseVolumeHistogramLogic> dvhLogic = vtkSmartPointer<vtkSlicerDoseVolumeHistogramLogic>::New();
  dvhLogic->SetMRMLScene(mrmlScene);
  dvhLogic->SetDoseVolumeNode(doseScalarVolumeNode);
  dvhLogic->SetStructureSetModelNode(modelHierarchyRootNode);

  // Compute DVH and get result nodes
  dvhLogic->ComputeDvh();
  dvhLogic->RefreshDvhDoubleArrayNodesFromScene();

  mrmlScene->Commit();

  vtkCollection* dvhNodes = dvhLogic->GetDvhDoubleArrayNodes();
  if (dvhNodes->GetNumberOfItems() < 1)
  {
    std::cerr << "No DVH nodes created!" << std::endl;
    return EXIT_FAILURE;
  }

  // Check values
  //char metricListAttributeName[64];
  //sprintf(metricListAttributeName, "%s%s", vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_LIST_ATTRIBUTE_NAME.c_str());
  //QSet<QString> metricSet;
  //for (int i=0; i<dvhNodes->GetNumberOfItems(); ++i)
  //{
  //  vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast( dvhNodes->GetItemAsObject(i) );
  //  if (!dvhNode)
  //  {
  //    continue;
  //  }

  return EXIT_SUCCESS;  
}
