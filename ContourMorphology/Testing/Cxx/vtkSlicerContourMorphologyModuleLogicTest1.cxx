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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ContourMorphology includes
#include "vtkSlicerContourMorphologyModuleLogic.h"
#include "vtkMRMLContourMorphologyNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"

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
#include <vtkTransform.h>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#define MIN_VOLUME_DIFFERENCE_TOLERANCE_VOXEL 100

//-----------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogicTest1( int argc, char * argv[] )
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

  const char *inputContourALabelmapFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputContourALabelmapFile") == 0)
    {
      inputContourALabelmapFile = argv[argIndex+1];
      std::cout << "Input Contour A labelmap file name: " << inputContourALabelmapFile << std::endl;
      argIndex += 2;
    }
    else
    {
      inputContourALabelmapFile = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputContourBLabelmapFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputContourBLabelmapFile") == 0)
    {
      inputContourBLabelmapFile = argv[argIndex+1];
      std::cout << "Input Contour B labelmap file name: " << inputContourBLabelmapFile << std::endl;
      argIndex += 2;
    }
    else
    {
      inputContourBLabelmapFile = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *referenceVolumeFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ReferenceVolumeFile") == 0)
    {
      referenceVolumeFile = argv[argIndex+1];
      std::cout << "Reference Volume file name: " << referenceVolumeFile << std::endl;
      argIndex += 2;
    }
    else
    {
      referenceVolumeFile = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *baselineContourLabelmapFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineContourLabelmapFile") == 0)
    {
      baselineContourLabelmapFileName = argv[argIndex+1];
      std::cout << "Baseline Contour labelmap file name: " << baselineContourLabelmapFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineContourLabelmapFileName = "";
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

  const char *morphologicalOperation = NULL;
  vtkMRMLContourMorphologyNode::ContourMorphologyOperationType operation = vtkMRMLContourMorphologyNode::None;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-MorphologicalOperation") == 0)
    {
      morphologicalOperation = argv[argIndex+1];
      std::cout << "Morphological Operation: " << morphologicalOperation << std::endl;
      argIndex += 2;
      if (STRCASECMP(morphologicalOperation, "Expand") == 0)
      {
        operation = vtkMRMLContourMorphologyNode::Expand;
      }
      else if (STRCASECMP(morphologicalOperation, "Shrink") == 0)
      {
        operation = vtkMRMLContourMorphologyNode::Shrink;
      }
      else if (STRCASECMP(morphologicalOperation, "Union") == 0)
      {
        operation = vtkMRMLContourMorphologyNode::Union;
      }
      else if (STRCASECMP(morphologicalOperation, "Intersect") == 0)
      {
        operation = vtkMRMLContourMorphologyNode::Intersect;
      }
      else if (STRCASECMP(morphologicalOperation, "Subtract") == 0)
      {
        operation = vtkMRMLContourMorphologyNode::Subtract;
      }
      else 
      {
        std::cerr << "Invalid Morphological Operation!" << std::endl;
        return EXIT_FAILURE;
      }
    }
    else
    {
      morphologicalOperation = "";
    }
  }

  double morphologicalParameter = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-MorphologicalParameter") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      morphologicalParameter = doubleValue;
      std::cout << "Morphological Parameter: " << morphologicalParameter << std::endl;
      argIndex += 2;
    }
  }

  unsigned int applySimpleTransformToInput = 0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ApplySimpleTransformToInput") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      unsigned int intValue;
      ss >> intValue;
      applySimpleTransformToInput = intValue;
      std::cout << "Apply Transform to Input: " << applySimpleTransformToInput << std::endl;
      argIndex += 2;
    }
  }

  double volumeDifferenceToleranceVoxel = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-VolumeDifferenceToleranceVoxel") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      volumeDifferenceToleranceVoxel = doubleValue;
      std::cout << "Volume difference Tolerance(Voxel): " << volumeDifferenceToleranceVoxel << std::endl;
      argIndex += 2;
    }
  }

  // Constraint the criteria to be greater than zero
  if (volumeDifferenceToleranceVoxel == 0.0)
  {
    volumeDifferenceToleranceVoxel = MIN_VOLUME_DIFFERENCE_TOLERANCE_VOXEL;
  }

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

  // Create input labelmap A volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputLabelmapAScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(inputLabelmapAScalarVolumeNode);
  inputLabelmapAScalarVolumeNode->SetName("Input_Labelmap_A");
  inputLabelmapAScalarVolumeNode->LabelMapOn();

  // Load input labelmap A volume
  std::string inputLabelmapAVolumeFileName = std::string(dataDirectoryPath) + std::string(inputContourALabelmapFile);
  std::cerr << "file name is " << inputLabelmapAVolumeFileName << std::endl;
  if (!vtksys::SystemTools::FileExists(inputLabelmapAVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << inputLabelmapAVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> inputLabelmapAVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(inputLabelmapAVolumeArchetypeStorageNode);
  inputLabelmapAVolumeArchetypeStorageNode->SetFileName(inputLabelmapAVolumeFileName.c_str());

  inputLabelmapAScalarVolumeNode->SetAndObserveStorageNodeID(inputLabelmapAVolumeArchetypeStorageNode->GetID());

  if (! inputLabelmapAVolumeArchetypeStorageNode->ReadData(inputLabelmapAScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << inputLabelmapAVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create input contour A node
  vtkSmartPointer<vtkMRMLContourNode> inputContourANode = vtkSmartPointer<vtkMRMLContourNode>::New();
  inputContourANode->SetName("Input_Contour_A");
  mrmlScene->AddNode(inputContourANode);
  inputContourANode->SetAndObserveIndexedLabelmapVolumeNodeId(inputLabelmapAScalarVolumeNode->GetID());
  inputContourANode->SetActiveRepresentationByNode(inputLabelmapAScalarVolumeNode);

  // Create input labelmap B volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputLabelmapBScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(inputLabelmapBScalarVolumeNode);
  inputLabelmapBScalarVolumeNode->SetName("Input_Labelmap_B");
  inputLabelmapBScalarVolumeNode->LabelMapOn();

  // Load input labelmap volume
  std::string inputLabelmapBVolumeFileName = std::string(dataDirectoryPath) + std::string(inputContourBLabelmapFile);
  if (!vtksys::SystemTools::FileExists(inputLabelmapBVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << inputLabelmapBVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> inputLabelmapBVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(inputLabelmapBVolumeArchetypeStorageNode);
  inputLabelmapBVolumeArchetypeStorageNode->SetFileName(inputLabelmapBVolumeFileName.c_str());

  inputLabelmapBScalarVolumeNode->SetAndObserveStorageNodeID(inputLabelmapBVolumeArchetypeStorageNode->GetID());

  if (! inputLabelmapBVolumeArchetypeStorageNode->ReadData(inputLabelmapBScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << inputLabelmapBVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create input contour B node
  vtkSmartPointer<vtkMRMLContourNode> inputContourBNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  inputContourBNode->SetName("Input_Contour_B");
  mrmlScene->AddNode(inputContourBNode);
  inputContourBNode->SetAndObserveIndexedLabelmapVolumeNodeId(inputLabelmapBScalarVolumeNode->GetID());
  inputContourBNode->SetActiveRepresentationByNode(inputLabelmapBScalarVolumeNode);

  // Create reference volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> referenceVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(referenceVolumeNode);
  referenceVolumeNode->SetName("Reference_Volume");
  referenceVolumeNode->LabelMapOn();

  // Load reference volume node
  std::string referenceVolumeFileName = std::string(dataDirectoryPath) + std::string(referenceVolumeFile);
  if (!vtksys::SystemTools::FileExists(referenceVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << referenceVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> referenceVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(referenceVolumeArchetypeStorageNode);
  referenceVolumeArchetypeStorageNode->SetFileName(referenceVolumeFileName.c_str());

  referenceVolumeNode->SetAndObserveStorageNodeID(referenceVolumeArchetypeStorageNode->GetID());

  if (! referenceVolumeArchetypeStorageNode->ReadData(referenceVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << referenceVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  if (applySimpleTransformToInput == 1)
  {
    vtkSmartPointer<vtkTransform> inputCompareTransform = vtkSmartPointer<vtkTransform>::New();
    inputCompareTransform->Identity();
    inputCompareTransform->Translate(0.0, 0.0, 5.0);

    vtkSmartPointer<vtkMRMLLinearTransformNode> inputCompareTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    inputCompareTransformNode->ApplyTransformMatrix(inputCompareTransform->GetMatrix());
    mrmlScene->AddNode(inputCompareTransformNode);

    inputContourANode->SetAndObserveTransformNodeID(inputCompareTransformNode->GetID());
  }

  // Create output contour node
  vtkSmartPointer<vtkMRMLContourNode> outputContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  outputContourNode->SetName("Output_Contour");
  mrmlScene->AddNode(outputContourNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLContourMorphologyNode> paramNode = vtkSmartPointer<vtkMRMLContourMorphologyNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveContourANodeId(inputContourANode->GetID());
  paramNode->SetAndObserveContourBNodeId(inputContourBNode->GetID());
  paramNode->SetAndObserveReferenceVolumeNodeId(referenceVolumeNode->GetID());
  paramNode->SetAndObserveOutputContourNodeId(outputContourNode->GetID());
  paramNode->SetOperation(operation);
  paramNode->SetXSize(morphologicalParameter);
  paramNode->SetYSize(morphologicalParameter);
  paramNode->SetZSize(morphologicalParameter);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerContourMorphologyModuleLogic> contourMorphologyLogic = vtkSmartPointer<vtkSlicerContourMorphologyModuleLogic>::New();
  contourMorphologyLogic->SetMRMLScene(mrmlScene);
  contourMorphologyLogic->SetAndObserveContourMorphologyNode(paramNode);

  // Compute ContourMorphology
  contourMorphologyLogic->MorphContour();

  outputContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputContourNodeId()));  
  if (outputContourNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid output contour node!" << std::endl;
    return EXIT_FAILURE;
  }
  
  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputLabelmapNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(outputContourNode->GetIndexedLabelmapVolumeNodeId()));  

  mrmlScene->Commit();
  
  // Create baseline labelmap node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> baselineLabelmapScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(baselineLabelmapScalarVolumeNode);
  baselineLabelmapScalarVolumeNode->SetName("Baseline_Labelmap");

  // Load baseline labelmap volume
  std::string baselineLabelmapVolumeFileName = std::string(dataDirectoryPath) + std::string(baselineContourLabelmapFileName);
  if (!vtksys::SystemTools::FileExists(baselineLabelmapVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << baselineLabelmapVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> baselineLabelmapVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(baselineLabelmapVolumeArchetypeStorageNode);
  baselineLabelmapVolumeArchetypeStorageNode->SetFileName(baselineLabelmapVolumeFileName.c_str());
  baselineLabelmapScalarVolumeNode->SetAndObserveStorageNodeID(baselineLabelmapVolumeArchetypeStorageNode->GetID());

  if (! baselineLabelmapVolumeArchetypeStorageNode->ReadData(baselineLabelmapScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << baselineLabelmapVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();

  vtkSmartPointer<vtkImageMathematics> difference = vtkSmartPointer<vtkImageMathematics>::New();
  difference->SetInput1(outputLabelmapNode->GetImageData());
  difference->SetInput2(baselineLabelmapScalarVolumeNode->GetImageData());
  difference->SetOperationToSubtract();
  difference->Update();

  // Compute histogram
  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(difference->GetOutput());
  histogram->IgnoreZeroOn();
  histogram->Update();
  
  if (histogram->GetVoxelCount() > volumeDifferenceToleranceVoxel)
  {
    std::cerr << "Contour Morphology Test: Volume difference Tolerance " << histogram->GetVoxelCount() << "(Voxel) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

