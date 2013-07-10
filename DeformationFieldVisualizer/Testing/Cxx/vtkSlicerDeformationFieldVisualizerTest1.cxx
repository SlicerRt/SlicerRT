/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  This file was originally developed by Franklin King, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

//DeformationFieldVisualizer logic
#include "vtkSlicerDeformationFieldVisualizerLogic.h"
#include "vtkMRMLDeformationFieldVisualizerNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLTransformStorageNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLBSplineTransformNode.h>
#include <vtkMRMLGridTransformNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
#ifdef WIN32
  #include <vtkWin32OutputWindow.h>
#endif

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

  const char *baselineGlyphModelFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineGlyphModel") == 0)
    {
      baselineGlyphModelFileName = argv[argIndex+1];
      std::cout << "Baseline Glyph model file name: " << baselineGlyphModelFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineGlyphModelFileName = "";
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
  
  const char *baselineBlockModelFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineBlockModel") == 0)
    {
      baselineBlockModelFileName = argv[argIndex+1];
      std::cout << "Baseline Block model file name: " << baselineBlockModelFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineBlockModelFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }  
  
  const char *baselineContourModelFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineContourModel") == 0)
    {
      baselineContourModelFileName = argv[argIndex+1];
      std::cout << "Baseline Contour model file name: " << baselineContourModelFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineContourModelFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  
  const char *baselineGlyphSliceModelFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineGlyphSliceModel") == 0)
    {
      baselineGlyphSliceModelFileName = argv[argIndex+1];
      std::cout << "Baseline Glyph Slice model file name: " << baselineGlyphSliceModelFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineGlyphSliceModelFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *baselineGridSliceModelFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineGridSliceModel") == 0)
    {
      baselineGridSliceModelFileName = argv[argIndex+1];
      std::cout << "Baseline Grid Slice model file name: " << baselineGridSliceModelFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineGridSliceModelFileName = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }
  
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
      std::cout << "Volume difference threshold: " << volumeDifferenceThreshold << std::endl;
      argIndex += 2;
    }
  }
  
  // Constraint the criteria to be greater than zero
  if (volumeDifferenceThreshold == 0.0)
  {
    volumeDifferenceThreshold = EPSILON;
  }
  
  double surfaceAreaDifferenceThreshold = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-SurfaceAreaDifferenceThreshold") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      surfaceAreaDifferenceThreshold = doubleValue;
      std::cout << "Surface area difference threshold: " << surfaceAreaDifferenceThreshold << std::endl;
      argIndex += 2;
    }
  }
  
  // Constraint the criteria to be greater than zero
  if (surfaceAreaDifferenceThreshold == 0.0)
  {
    surfaceAreaDifferenceThreshold = EPSILON;
  }
  

  // Make sure NRRD reading works
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Direct vtk messages on standard output
  //TODO: Remove when supported by the test driver (http://www.na-mic.org/Bug/view.php?id=3221)
#ifdef WIN32
  vtkWin32OutputWindow* outputWindow = vtkWin32OutputWindow::SafeDownCast(vtkOutputWindow::GetInstance());
  if (outputWindow)
  {
    outputWindow->SendToStdErrOn();
  }
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();
  
  // Set up parameter node
  vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode> paramNode = vtkSmartPointer<vtkMRMLDeformationFieldVisualizerNode>::New();
  
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

  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  mrmlScene->AddNode(modelNode);
  paramNode->SetAndObserveOutputModelNodeID(modelNode->GetID());	 

  mrmlScene->AddNode(paramNode);

  // Initialize logic node
  vtkSmartPointer<vtkSlicerDeformationFieldVisualizerLogic> deformationFieldVisualizerLogic = vtkSmartPointer<vtkSlicerDeformationFieldVisualizerLogic>::New();
  deformationFieldVisualizerLogic->SetMRMLScene(mrmlScene);
  deformationFieldVisualizerLogic->SetAndObserveDeformationFieldVisualizerNode(paramNode);
  
  // TODO: Add check for successful transform field conversion
  deformationFieldVisualizerLogic->GenerateDeformationField();
  
  //Emulate parameter adjustments that are done in qSlicerDeformationFieldVisualizerModuleWidget whenever an input volume is selected
  double* range = deformationFieldVisualizerLogic->GetFieldRange();
  range[0] = floor(range[0]*10000)/10000; //truncate to 4 decimal digits due to decimal precision setting in the UI
  range[1] = ceil(range[1]*10000)/10000;
  paramNode->SetGlyphThresholdMin(range[0]);
  paramNode->SetGlyphThresholdMax(range[1]);
  paramNode->SetContourMin(range[0]);
  paramNode->SetContourMax(range[1]);
  paramNode->SetGlyphSliceThresholdMin(range[0]);
  paramNode->SetGlyphSliceThresholdMax(range[1]);

  //Initialize
  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
  
  // vtkMassProperties currently only processes triangle meshes
  vtkSmartPointer<vtkTriangleFilter> triangleBaseline = vtkSmartPointer<vtkTriangleFilter>::New();
  vtkSmartPointer<vtkMassProperties> propertiesBaseline = vtkSmartPointer<vtkMassProperties>::New();  
  
  vtkSmartPointer<vtkTriangleFilter> triangleCurrent = vtkSmartPointer<vtkTriangleFilter>::New();
  vtkSmartPointer<vtkMassProperties> propertiesCurrent = vtkSmartPointer<vtkMassProperties>::New();
  
  double volumeDifference = 10;
  double surfaceAreaDifference = 10;
  
  // TODO: Expand tests
  //----------------------------------------------------------------------------
  // Test Glyph option
  deformationFieldVisualizerLogic->CreateVisualization(deformationFieldVisualizerLogic->VIS_MODE_GLYPH_3D);
  
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
  
  reader->SetFileName(baselineGlyphModelFileFullPath.c_str());
  reader->Update();
  
  // vtkMassProperties currently only processes triangle meshes
  triangleBaseline->SetInput(reader->GetOutput());
  triangleBaseline->Update();
  
  propertiesBaseline->SetInputConnection(triangleBaseline->GetOutputPort());
  propertiesBaseline->Update();
  
  triangleCurrent->SetInput(modelNode->GetPolyData());
  triangleCurrent->Update();
  
  propertiesCurrent->SetInputConnection(triangleCurrent->GetOutputPort());
  propertiesCurrent->Update();
  
  volumeDifference = abs(propertiesBaseline->GetVolume() - propertiesCurrent->GetVolume());
  //std::cout << "Model volume difference: " << volumeDifference << std::endl;
  
  surfaceAreaDifference = abs(propertiesBaseline->GetSurfaceArea() - propertiesCurrent->GetSurfaceArea());
  //std::cout << "Model surface area difference: " << surfaceAreaDifference << std::endl;  
 
  if (volumeDifference > volumeDifferenceThreshold)
  {
    std::cerr << "Volume difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }
  
  if (surfaceAreaDifference > surfaceAreaDifferenceThreshold)
  {
    std::cerr << "Surface area difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }  
 
  //----------------------------------------------------------------------------
  // Test Grid option
  deformationFieldVisualizerLogic->CreateVisualization(deformationFieldVisualizerLogic->VIS_MODE_GRID_3D);
  
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
  
  reader->SetFileName(baselineGridModelFileFullPath.c_str());
  reader->Update();
  
  if (reader->GetOutput()->GetNumberOfPoints() != modelNode->GetPolyData()->GetNumberOfPoints())
  {
    std::cerr << "Grid visualization: Mismatching number of points" << std::endl;
    return EXIT_FAILURE;
  }  
  
  if (reader->GetOutput()->GetNumberOfLines() != modelNode->GetPolyData()->GetNumberOfLines())
  {
    std::cerr << "Grid visualization: Mismatching number of lines" << std::endl;
    return EXIT_FAILURE;
  }

  //----------------------------------------------------------------------------
  // Test Block mode
  deformationFieldVisualizerLogic->CreateVisualization(deformationFieldVisualizerLogic->VIS_MODE_BLOCK_3D);
  
  modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputModelNodeID()));  
  if (modelNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model node!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();
  
  std::string baselineBlockModelFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(baselineBlockModelFileName);
  if (!vtksys::SystemTools::FileExists(baselineBlockModelFileFullPath.c_str()))
  {
    std::cerr << "Loading baseline model from file '" << baselineBlockModelFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  reader->SetFileName(baselineBlockModelFileFullPath.c_str());
  reader->Update();
  
  // vtkMassProperties currently only processes triangle meshes
  triangleBaseline->SetInput(reader->GetOutput());
  triangleBaseline->Update();
  
  propertiesBaseline->SetInputConnection(triangleBaseline->GetOutputPort());
  propertiesBaseline->Update();
  
  triangleCurrent->SetInput(modelNode->GetPolyData());
  triangleCurrent->Update();
  
  propertiesCurrent->SetInputConnection(triangleCurrent->GetOutputPort());
  propertiesCurrent->Update();
  
  volumeDifference = abs(propertiesBaseline->GetVolume() - propertiesCurrent->GetVolume());
  //std::cout << "Model volume difference: " << volumeDifference << std::endl;
  
  surfaceAreaDifference = abs(propertiesBaseline->GetSurfaceArea() - propertiesCurrent->GetSurfaceArea());
  //std::cout << "Model surface area difference: " << surfaceAreaDifference << std::endl;  
 
  if (volumeDifference > volumeDifferenceThreshold)
  {
    std::cerr << "Volume difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }
  
  if (surfaceAreaDifference > surfaceAreaDifferenceThreshold)
  {
    std::cerr << "Surface area difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }
  
  //----------------------------------------------------------------------------
  // Test Contour mode
  deformationFieldVisualizerLogic->CreateVisualization(deformationFieldVisualizerLogic->VIS_MODE_CONTOUR_3D);
  
  modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputModelNodeID()));  
  if (modelNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model node!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();
  
  std::string baselineContourModelFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(baselineContourModelFileName);
  if (!vtksys::SystemTools::FileExists(baselineContourModelFileFullPath.c_str()))
  {
    std::cerr << "Loading baseline model from file '" << baselineContourModelFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  reader->SetFileName(baselineContourModelFileFullPath.c_str());
  reader->Update();
  
  triangleBaseline->SetInput(reader->GetOutput());
  triangleBaseline->Update();
  
  propertiesBaseline->SetInputConnection(triangleBaseline->GetOutputPort());
  propertiesBaseline->Update();
  
  triangleCurrent->SetInput(modelNode->GetPolyData());
  triangleCurrent->Update();
  
  propertiesCurrent->SetInputConnection(triangleCurrent->GetOutputPort());
  propertiesCurrent->Update();
  
  surfaceAreaDifference = abs(propertiesBaseline->GetSurfaceArea() - propertiesCurrent->GetSurfaceArea());
  //std::cout << "Model surface area difference: " << surfaceAreaDifference << std::endl;  
  
  if (surfaceAreaDifference > surfaceAreaDifferenceThreshold)
  {
    std::cerr << "Surface area difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }
  
  //----------------------------------------------------------------------------
  // Test Glyph Slice mode
  vtkSmartPointer<vtkMRMLSliceNode> axialSliceNode = vtkSmartPointer<vtkMRMLSliceNode>::New();
  vtkSmartPointer<vtkMatrix4x4> axialSliceToRas = vtkSmartPointer<vtkMatrix4x4>::New();
  axialSliceToRas->SetElement(0,0,-1); axialSliceToRas->SetElement(0,1,0); axialSliceToRas->SetElement(0,2,0); axialSliceToRas->SetElement(0,3,-6.6449);
  axialSliceToRas->SetElement(1,0, 0); axialSliceToRas->SetElement(1,1,1); axialSliceToRas->SetElement(1,2,0); axialSliceToRas->SetElement(1,3, 8.9286);
  axialSliceToRas->SetElement(2,0, 0); axialSliceToRas->SetElement(2,1,0); axialSliceToRas->SetElement(2,2,1); axialSliceToRas->SetElement(2,3,-8.2143);
  axialSliceToRas->SetElement(3,0, 0); axialSliceToRas->SetElement(3,1,0); axialSliceToRas->SetElement(3,2,0); axialSliceToRas->SetElement(3,3, 1);
  axialSliceNode->SetSliceToRAS(axialSliceToRas);
  
  mrmlScene->AddNode(axialSliceNode);
  
  paramNode->SetGlyphSliceNodeID(axialSliceNode->GetID());
  
  deformationFieldVisualizerLogic->CreateVisualization(deformationFieldVisualizerLogic->VIS_MODE_GLYPH_2D);
  
  modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputModelNodeID()));  
  if (modelNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model node!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();
  
  std::string baselineGlyphSliceModelFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(baselineGlyphSliceModelFileName);
  if (!vtksys::SystemTools::FileExists(baselineGlyphSliceModelFileFullPath.c_str()))
  {
    std::cerr << "Loading baseline model from file '" << baselineGlyphSliceModelFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  reader->SetFileName(baselineGlyphSliceModelFileFullPath.c_str());
  reader->Update();
  
  // vtkMassProperties currently only processes triangle meshes
  triangleBaseline->SetInput(reader->GetOutput());
  triangleBaseline->Update();
  
  propertiesBaseline->SetInput(triangleBaseline->GetOutput());
  propertiesBaseline->Update();
  
  triangleCurrent->SetInput(modelNode->GetPolyData());
  triangleCurrent->Update();
  
  propertiesCurrent->SetInput(triangleCurrent->GetOutput());
  propertiesCurrent->Update();
  
  surfaceAreaDifference = abs(propertiesBaseline->GetSurfaceArea() - propertiesCurrent->GetSurfaceArea());
  //std::cout << "Model surface area difference: " << surfaceAreaDifference << std::endl;  
  
  if (surfaceAreaDifference > surfaceAreaDifferenceThreshold)
  {
    std::cerr << "Surface area difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }
  
  //----------------------------------------------------------------------------
  // Test Grid Slice mode
  vtkSmartPointer<vtkMRMLSliceNode> sagittalSliceNode = vtkSmartPointer<vtkMRMLSliceNode>::New();
  vtkSmartPointer<vtkMatrix4x4> sagittalSliceToRas = vtkSmartPointer<vtkMatrix4x4>::New();
  sagittalSliceToRas->SetElement(0,0, 0); sagittalSliceToRas->SetElement(0,1,0); sagittalSliceToRas->SetElement(0,2,1); sagittalSliceToRas->SetElement(0,3,-6.6449);
  sagittalSliceToRas->SetElement(1,0,-1); sagittalSliceToRas->SetElement(1,1,0); sagittalSliceToRas->SetElement(1,2,0); sagittalSliceToRas->SetElement(1,3, 8.9286);
  sagittalSliceToRas->SetElement(2,0, 0); sagittalSliceToRas->SetElement(2,1,1); sagittalSliceToRas->SetElement(2,2,0); sagittalSliceToRas->SetElement(2,3,-8.2143);
  sagittalSliceToRas->SetElement(3,0, 0); sagittalSliceToRas->SetElement(3,1,0); sagittalSliceToRas->SetElement(3,2,0); sagittalSliceToRas->SetElement(3,3, 1);
  sagittalSliceNode->SetSliceToRAS(sagittalSliceToRas);
  
  mrmlScene->AddNode(sagittalSliceNode);
  
  paramNode->SetGridSliceNodeID(sagittalSliceNode->GetID());
  
  deformationFieldVisualizerLogic->CreateVisualization(deformationFieldVisualizerLogic->VIS_MODE_GRID_2D);
  
  modelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputModelNodeID()));  
  if (modelNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model node!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();
  
  std::string baselineGridSliceModelFileFullPath = std::string(dataDirectoryPath) + std::string("/") + std::string(baselineGridSliceModelFileName);
  if (!vtksys::SystemTools::FileExists(baselineGridSliceModelFileFullPath.c_str()))
  {
    std::cerr << "Loading baseline model from file '" << baselineGridSliceModelFileFullPath << "' failed - the file does not exist!" << std::endl;
  }
  
  reader->SetFileName(baselineGridSliceModelFileFullPath.c_str());
  reader->Update();
  
  // vtkMassProperties currently only processes triangle meshes
  triangleBaseline->SetInput(reader->GetOutput());
  triangleBaseline->Update();
  
  propertiesBaseline->SetInput(triangleBaseline->GetOutput());
  propertiesBaseline->Update();
  
  triangleCurrent->SetInput(modelNode->GetPolyData());
  triangleCurrent->Update();
  
  propertiesCurrent->SetInput(triangleCurrent->GetOutput());
  propertiesCurrent->Update();
  
  surfaceAreaDifference = abs(propertiesBaseline->GetSurfaceArea() - propertiesCurrent->GetSurfaceArea());
  //std::cout << "Model surface area difference: " << surfaceAreaDifference << std::endl;  
  
  if (surfaceAreaDifference > surfaceAreaDifferenceThreshold)
  {
    std::cerr << "Surface area difference exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }  
 
  //----------------------------------------------------------------------------
  return EXIT_SUCCESS;
}
