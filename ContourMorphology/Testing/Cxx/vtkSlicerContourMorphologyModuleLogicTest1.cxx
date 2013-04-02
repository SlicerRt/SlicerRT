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

  const char *referenceContourLabelmapFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ReferenceContourLabelmapFile") == 0)
    {
      referenceContourLabelmapFile = argv[argIndex+1];
      std::cout << "Reference Contour labelmap file name: " << referenceContourLabelmapFile << std::endl;
      argIndex += 2;
    }
    else
    {
      referenceContourLabelmapFile = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputContourLabelmapFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputContourLabelmapFile") == 0)
    {
      inputContourLabelmapFileName = argv[argIndex+1];
      std::cout << "Input Contour labelmap file name: " << inputContourLabelmapFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      inputContourLabelmapFileName = "";
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
  unsigned int operation = 0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-MorphologicalOperation") == 0)
    {
      morphologicalOperation = argv[argIndex+1];
      std::cout << "Morphological Operation: " << morphologicalOperation << std::endl;
      argIndex += 2;
      if (STRCASECMP(morphologicalOperation, "Expand") == 0)
      {
        operation = 0;
      }
      else if (STRCASECMP(morphologicalOperation, "Shrink") == 0)
      {
        operation = 1;
      }
      else if (STRCASECMP(morphologicalOperation, "Union") == 0)
      {
        operation = 2;
      }
      else if (STRCASECMP(morphologicalOperation, "Intersect") == 0)
      {
        operation = 3;
      }
      else if (STRCASECMP(morphologicalOperation, "Subtract") == 0)
      {
        operation = 4;
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

  // Create reference labelmap volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> referenceLabelmapScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(referenceLabelmapScalarVolumeNode);
  referenceLabelmapScalarVolumeNode->SetName("Reference_Labelmap");
  referenceLabelmapScalarVolumeNode->LabelMapOn();

  // Load reference labelmap volume
  std::string referenceLabelmapVolumeFileName = std::string(dataDirectoryPath) + std::string(referenceContourLabelmapFile);
  std::cerr << "file name is " << referenceLabelmapVolumeFileName << std::endl;
  if (!vtksys::SystemTools::FileExists(referenceLabelmapVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << referenceLabelmapVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> referenceLabelmapVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(referenceLabelmapVolumeArchetypeStorageNode);
  referenceLabelmapVolumeArchetypeStorageNode->SetFileName(referenceLabelmapVolumeFileName.c_str());

  referenceLabelmapScalarVolumeNode->SetAndObserveStorageNodeID(referenceLabelmapVolumeArchetypeStorageNode->GetID());

  if (! referenceLabelmapVolumeArchetypeStorageNode->ReadData(referenceLabelmapScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << referenceLabelmapVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create reference contour node
  vtkSmartPointer<vtkMRMLContourNode> referenceContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  referenceContourNode->SetName("Reference_contour");
  mrmlScene->AddNode(referenceContourNode);
  referenceContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(referenceLabelmapScalarVolumeNode->GetID());
  referenceContourNode->SetActiveRepresentationByNode(referenceLabelmapScalarVolumeNode);

  // Create input labelmap volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputLabelmapScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(inputLabelmapScalarVolumeNode);
  inputLabelmapScalarVolumeNode->SetName("Input_Labelmap");
  inputLabelmapScalarVolumeNode->LabelMapOn();

  // Load input labelmap volume
  std::string inputLabelmapVolumeFileName = std::string(dataDirectoryPath) + std::string(inputContourLabelmapFileName);
  if (!vtksys::SystemTools::FileExists(inputLabelmapVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << inputLabelmapVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> inputLabelmapVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(inputLabelmapVolumeArchetypeStorageNode);
  inputLabelmapVolumeArchetypeStorageNode->SetFileName(inputLabelmapVolumeFileName.c_str());

  inputLabelmapScalarVolumeNode->SetAndObserveStorageNodeID(inputLabelmapVolumeArchetypeStorageNode->GetID());

  if (! inputLabelmapVolumeArchetypeStorageNode->ReadData(inputLabelmapScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << inputLabelmapVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create input contour node
  vtkSmartPointer<vtkMRMLContourNode> inputContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  inputContourNode->SetName("Input_contour");
  mrmlScene->AddNode(inputContourNode);
  inputContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(inputLabelmapScalarVolumeNode->GetID());
  inputContourNode->SetActiveRepresentationByNode(inputLabelmapScalarVolumeNode);

  if (applySimpleTransformToInput == 1)
  {
    vtkSmartPointer<vtkTransform> inputCompareTransform = vtkSmartPointer<vtkTransform>::New();
    inputCompareTransform->Identity();
    inputCompareTransform->Translate(0.0, 0.0, -5.0);

    vtkSmartPointer<vtkMRMLLinearTransformNode> inputCompareTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    inputCompareTransformNode->ApplyTransformMatrix(inputCompareTransform->GetMatrix());
    mrmlScene->AddNode(inputCompareTransformNode);

    inputContourNode->SetAndObserveTransformNodeID(inputCompareTransformNode->GetID());
  }

  // Create output contour node
  vtkSmartPointer<vtkMRMLContourNode> outputContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  outputContourNode->SetName("Output_Contour");
  mrmlScene->AddNode(outputContourNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLContourMorphologyNode> paramNode = vtkSmartPointer<vtkMRMLContourMorphologyNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveReferenceContourNodeID(referenceContourNode->GetID());
  paramNode->SetAndObserveInputContourNodeID(inputContourNode->GetID());
  paramNode->SetAndObserveOutputContourNodeID(outputContourNode->GetID());
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

  outputContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputContourNodeID()));  
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
    std::cerr << "Contour Expanding: Volume difference Tolerance(Voxel) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

