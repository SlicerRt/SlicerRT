
//DeformationFieldVisualizer logic
#include "vtkSlicerDeformationFieldVisualizerLogic.h"
#include "vtkMRMLDeformationFieldVisualizerNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelStorageNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLTransformStorageNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLBSplineTransformNode.h>
#include <vtkMRMLGridTransformNode.h>

// VTK includes
#include <vtkPolyDataWriter.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkCollection.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
int vtkSlicerDeformationFieldVisualizerTest1(int argc, char *argv[])
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
  
  const char *transformFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-Transform") == 0)
    {
      transformFileName = argv[argIndex+1];
      std::cout << "Transform file name: " << transformFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      transformFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *baselineGridModelFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineGridModel") == 0)
    {
      baselineGridModelFileName = argv[argIndex+1];
      std::cout << "Baseline Grid model file name: " << baselineGridModelFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineGridModelFileName = "";
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
  
  /*
  double volumeDifferenceThreshold = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-VolumeDifferenceThreshold") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      volumeDifferenceThreshold = doubleValue;
      std::cout << "Volume difference Threshold: " << volumeDifferenceThreshold << std::endl;
      argIndex += 2;
    }
  }

  // Constraint the criteria to be greater than zero
  if (volumeDifferenceThreshold == 0.0)
  {
    volumeDifferenceThreshold = EPSILON;
  }
  //*/

  // Make sure NRRD reading works
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif


  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();
  
  // Set up parameter node
  vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode> paramNode = vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode>::New();
  
  //vtkSmartPointer<vtkMRMLTransformNode> transformNode = vtkSmartPointer<vtkMRMLTransformNode>::New();
  //vtkMRMLTransformNode *transformNode = NULL;
  //transformNode->SetName("Transform");
  
  std::string transformFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(transformFileName);
  if (!vtksys::SystemTools::FileExists(transformFileFullPath.c_str()))
  {
    std::cerr << "Loading transform from file '" << transformFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  vtkSmartPointer<vtkMRMLTransformStorageNode> transformStorageNode = vtkSmartPointer<vtkMRMLTransformStorageNode>::New();
  transformStorageNode->SetFileName(transformFileFullPath.c_str());

  transformStorageNode->SetScene(mrmlScene);

  vtkSmartPointer<vtkMRMLLinearTransformNode> linearTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  vtkSmartPointer<vtkMRMLBSplineTransformNode> bsplineTransformNode = vtkSmartPointer<vtkMRMLBSplineTransformNode>::New();
  vtkSmartPointer<vtkMRMLGridTransformNode> gridTransformNode = vtkSmartPointer<vtkMRMLGridTransformNode>::New();

  linearTransformNode->SetScene(mrmlScene);
  bsplineTransformNode->SetScene(mrmlScene);
  gridTransformNode->SetScene(mrmlScene);

  if (transformStorageNode->ReadData(gridTransformNode))
  {
    mrmlScene->AddNode(gridTransformNode);
    mrmlScene->AddNode(transformStorageNode);
	mrmlScene->Commit();
	gridTransformNode->SetAndObserveStorageNodeID(transformStorageNode->GetID());  
	paramNode->SetAndObserveInputVolumeNodeID(gridTransformNode->GetID());
  }
  else if (transformStorageNode->ReadData(bsplineTransformNode))
  {
    mrmlScene->AddNode(bsplineTransformNode);
    mrmlScene->AddNode(transformStorageNode);
	mrmlScene->Commit();
	bsplineTransformNode->SetAndObserveStorageNodeID(transformStorageNode->GetID());
	paramNode->SetAndObserveInputVolumeNodeID(bsplineTransformNode->GetID());
  }  
  else if (transformStorageNode->ReadData(linearTransformNode))
  {
    mrmlScene->AddNode(linearTransformNode);
    mrmlScene->AddNode(transformStorageNode);
	mrmlScene->Commit();
	linearTransformNode->SetAndObserveStorageNodeID(transformStorageNode->GetID());
    paramNode->SetAndObserveInputVolumeNodeID(linearTransformNode->GetID());	
  }
  else
  {
    std::cerr << "Reading transform from file '" << transformFileFullPath << "' failed!" << std::endl;
    return EXIT_FAILURE; 
  }
  
  
  // Create reference volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> referenceVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  referenceVolumeNode->SetName("Reference"); 
  mrmlScene->AddNode(referenceVolumeNode);
  
  // Load reference image
  std::string referenceVolumeFileName = std::string(dataDirectoryPath) + "/Head.nrrd";
  if (!vtksys::SystemTools::FileExists(referenceVolumeFileName.c_str()))
  {
    std::cerr << "Loading reference volume from file '" << referenceVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }
  
  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> referenceVolumeArchetypeStorageNode = vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  referenceVolumeArchetypeStorageNode->SetFileName(referenceVolumeFileName.c_str());
  mrmlScene->AddNode(referenceVolumeArchetypeStorageNode);
  
  referenceVolumeNode->SetAndObserveStorageNodeID(referenceVolumeArchetypeStorageNode->GetID());

  if (! referenceVolumeArchetypeStorageNode->ReadData(referenceVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading reference image from file '" << referenceVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  paramNode->SetAndObserveReferenceVolumeNodeID(referenceVolumeNode->GetID());

  //vtkSmartPointer<vtkMRMLModelStorageNode> modelStorageNode = vtkSmartPointer<vtkMRMLModelStorageNode>::New();
  //modelStorageNode->SetScene(mrmlScene);
  
  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  mrmlScene->AddNode(modelNode);
  paramNode->SetAndObserveOutputModelNodeID(modelNode->GetID());	 

  mrmlScene->AddNode(paramNode);
  
  // Initialize logic node
  vtkSmartPointer<vtkSlicerDeformationFieldVisualizerLogic> deformationFieldVisualizerLogic = vtkSmartPointer<vtkSlicerDeformationFieldVisualizerLogic>::New();
  deformationFieldVisualizerLogic->SetMRMLScene(mrmlScene);
  deformationFieldVisualizerLogic->SetAndObserveDeformationFieldVisualizerNode(paramNode);
  
  // TODO: Add check for successful transform field conversion
  deformationFieldVisualizerLogic->GenerateTransformField();

  // TODO: Add tests for all other visualization options
  // TODO: Expand grid visualization test
  //----------------------------------------------------------------------------
  //Test Grid option
  deformationFieldVisualizerLogic->CreateVisualization(2);
  
  modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputModelNodeID()));  
  if (modelNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model node!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();
  
  std::string baselineGridModelFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(baselineGridModelFileName);
  if (!vtksys::SystemTools::FileExists(baselineGridModelFileFullPath.c_str()))
  {
    std::cerr << "Loading baseline model from file '" << baselineGridModelFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
  reader->SetFileName(baselineGridModelFileFullPath.c_str());
  reader->Update();
  
  std::cout<<"Baseline number of pieces:"<<reader->GetOutput()->GetNumberOfPieces()<<std::endl;
  std::cout<<"Baseline number of lines:"<<reader->GetOutput()->GetNumberOfLines()<<std::endl;
  std::cout<<"Baseline number of polys:"<<reader->GetOutput()->GetNumberOfPolys()<<std::endl;
  
  std::cout<<"Current number of pieces:"<<modelNode->GetPolyData()->GetNumberOfPieces()<<std::endl;
  std::cout<<"Current number of lines::"<<modelNode->GetPolyData()->GetNumberOfLines()<<std::endl;
  std::cout<<"Current number of polys::"<<modelNode->GetPolyData()->GetNumberOfPolys()<<std::endl;

  if (reader->GetOutput()->GetNumberOfLines() != modelNode->GetPolyData()->GetNumberOfLines())
  {
    std::cerr << "Grid visualization: Mismatching number of lines" << std::endl;
    return EXIT_FAILURE;
  }
  
  //----------------------------------------------------------------------------
  //Test Glyph option
  /*
  deformationFieldVisualizerLogic->CreateVisualization(1);
  
  modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputModelNodeID()));  
  if (modelNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model node!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();
  
  std::string baselineGlyphModelFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(baselineGlyphModelFileName);
  if (!vtksys::SystemTools::FileExists(baselineGlyphModelFileFullPath.c_str()))
  {
    std::cerr << "Loading baseline model from file '" << baselineGlyphModelFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
  reader->SetFileName(baselineGlyphModelFileFullPath.c_str());
  reader->Update();
  
  // vtkMassProperties currently only processes triangle meshes
  vtkSmartPointer<vtkTriangleFilter> triangleBaseline = vtkSmartPointer<vtkTriangleFilter>::New();
  triangleBaseline->SetInput(reader->GetOutput());
  triangleBaseline->Update();
  
  vtkSmartPointer<vtkMassProperties> propertiesBaseline = vtkSmartPointer<vtkMassProperties>::New();
  propertiesBaseline->SetInput(triangleBaseline->GetOutput());
  propertiesBaseline->Update();
  //std::cout<<"Baseline volume:"<<propertiesBaseline->GetVolume()<<std::endl;
  
  
  vtkSmartPointer<vtkTriangleFilter> triangleCurrent = vtkSmartPointer<vtkTriangleFilter>::New();
  triangleCurrent->SetInput(modelNode->GetPolyData());
  triangleCurrent->Update();
  
  vtkSmartPointer<vtkMassProperties> propertiesCurrent = vtkSmartPointer<vtkMassProperties>::New();
  propertiesCurrent->SetInput(triangleCurrent->GetOutput());
  propertiesCurrent->Update();
  //std::cout<<"Current volume:"<<propertiesCurrent->GetVolume()<<std::endl;

  if (reader->GetOutput()->GetNumberOfLines() != modelNode->GetPolyData()->GetNumberOfLines())
  {
    std::cerr << "Mismatching number of lines" << std::endl;
    return EXIT_FAILURE;
  }
  
  // TODO: Not the best for comparison since it ignores orientation. Comparing vector field directly may account for this.
  double volumeDifference = abs(propertiesBaseline->GetVolume() - propertiesCurrent->GetVolume());
  std::cout << "Model volume difference: " << volumeDifference << std::endl;
 
  if (volumeDifference > volumeDifferenceThreshold)
  {
    std::cerr << "Volume difference tolerance exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }
  //*/
  
  //----------------------------------------------------------------------------
  return EXIT_SUCCESS;
}
