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
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLVolumeArchetypeStorageNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLContourNode.h>

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkImageMathematics.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#define MIN_VOLUME_DIFFERENCE_TOLERANCE_CC 100

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

  double volumeDifferenceToleranceCc = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-VolumeDifferenceToleranceCc") == 0)
    {
      std::stringstream ss;
      ss << argv[argIndex+1];
      double doubleValue;
      ss >> doubleValue;
      volumeDifferenceToleranceCc = doubleValue;
      std::cout << "Volume difference Tolerance(Cc): " << volumeDifferenceToleranceCc << std::endl;
      argIndex += 2;
    }
  }

  // Constraint the criteria to be greater than zero
  if (volumeDifferenceToleranceCc == 0.0)
  {
    volumeDifferenceToleranceCc = MIN_VOLUME_DIFFERENCE_TOLERANCE_CC;
  }

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Create dose volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> labelmapScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(labelmapScalarVolumeNode);
  labelmapScalarVolumeNode->SetName("PTV_Labelmap");
  labelmapScalarVolumeNode->LabelMapOn();
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load dose volume
  std::string labelmapVolumeFileName = std::string(dataDirectoryPath) + "/PTV_Contour_Labelmap.nrrd";
  if (!vtksys::SystemTools::FileExists(labelmapVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << labelmapVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> labelmapVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(labelmapVolumeArchetypeStorageNode);
  labelmapVolumeArchetypeStorageNode->SetFileName(labelmapVolumeFileName.c_str());
  //EXERCISE_BASIC_STORAGE_MRML_METHODS(vtkMRMLVolumeArchetypeStorageNode, doseVolumeArchetypeStorageNode);

  labelmapScalarVolumeNode->SetAndObserveStorageNodeID(labelmapVolumeArchetypeStorageNode->GetID());

  if (! labelmapVolumeArchetypeStorageNode->ReadData(labelmapScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << labelmapVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create dose volume node
  vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  contourNode->SetName("PTV_contour");
  mrmlScene->AddNode(contourNode);
  contourNode->SetAndObserveIndexedLabelmapVolumeNodeId(labelmapScalarVolumeNode->GetID());
  contourNode->SetActiveRepresentationByNode(labelmapScalarVolumeNode);

  // Create dose volume node
  vtkSmartPointer<vtkMRMLContourNode> outputContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  outputContourNode->SetName("outputcontour");
  mrmlScene->AddNode(outputContourNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLContourMorphologyNode> paramNode = vtkSmartPointer<vtkMRMLContourMorphologyNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveContourNodeID(contourNode->GetID());
  paramNode->SetAndObserveOutputContourNodeID(outputContourNode->GetID());
  paramNode->SetExpansion(true);
  paramNode->SetXSize(3);
  paramNode->SetYSize(3);
  paramNode->SetZSize(3);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerContourMorphologyModuleLogic> ContourMorphologyLogic = vtkSmartPointer<vtkSlicerContourMorphologyModuleLogic>::New();
  ContourMorphologyLogic->SetMRMLScene(mrmlScene);
  ContourMorphologyLogic->SetAndObserveContourMorphologyNode(paramNode);

  // Compute ContourMorphology
  ContourMorphologyLogic->MorphContour();

  outputContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->GetNodeByID(paramNode->GetOutputContourNodeID()));  
  if (outputContourNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid output contour node!" << std::endl;
    return EXIT_FAILURE;
  }
  
  vtkMRMLScalarVolumeNode* outputLabelmapNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(outputContourNode->GetIndexedLabelmapVolumeNodeId()));  

  mrmlScene->Commit();
  
  // Create baseline labelmap node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> labelmapBaselineScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(labelmapBaselineScalarVolumeNode);
  labelmapBaselineScalarVolumeNode->SetName("PTV_Labelmap_Exp");
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load dose volume
  std::string labelmapBaselineVolumeFileName = std::string(dataDirectoryPath) + "/PTV_Contour_Labelmap_exp.nrrd";
  if (!vtksys::SystemTools::FileExists(labelmapVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << labelmapBaselineVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> labelmapBaselineVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(labelmapBaselineVolumeArchetypeStorageNode);
  labelmapBaselineVolumeArchetypeStorageNode->SetFileName(labelmapBaselineVolumeFileName.c_str());
  labelmapBaselineScalarVolumeNode->SetAndObserveStorageNodeID(labelmapBaselineVolumeArchetypeStorageNode->GetID());

  if (! labelmapBaselineVolumeArchetypeStorageNode->ReadData(labelmapBaselineScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << labelmapBaselineVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();

  // use vtkimagedifferenct
  vtkSmartPointer<vtkImageMathematics> difference = vtkSmartPointer<vtkImageMathematics>::New();
  difference->SetInput1(outputLabelmapNode->GetImageData());
  difference->SetInput2(labelmapBaselineScalarVolumeNode->GetImageData());
  difference->SetOperationToSubtract();
  difference->Update();

  // use vtkimageaccumulate to do histogram
  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(difference->GetOutput());
  histogram->IgnoreZeroOn();
  histogram->Update();
  
  if (histogram->GetVoxelCount() > volumeDifferenceToleranceCc)
  {
    std::cerr << "Volume difference Tolerance(Cc) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

