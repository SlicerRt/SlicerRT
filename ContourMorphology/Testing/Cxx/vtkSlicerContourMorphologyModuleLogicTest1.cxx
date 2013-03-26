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

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

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

  // Create labelmap volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> labelmapScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(labelmapScalarVolumeNode);
  labelmapScalarVolumeNode->SetName("PTV_Labelmap");
  labelmapScalarVolumeNode->LabelMapOn();
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load labelmap volume
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

  // Create contour node
  vtkSmartPointer<vtkMRMLContourNode> contourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  contourNode->SetName("PTV_contour");
  mrmlScene->AddNode(contourNode);
  contourNode->SetAndObserveIndexedLabelmapVolumeNodeId(labelmapScalarVolumeNode->GetID());
  contourNode->SetActiveRepresentationByNode(labelmapScalarVolumeNode);

  // Create output contour node
  vtkSmartPointer<vtkMRMLContourNode> outputContourNode = vtkSmartPointer<vtkMRMLContourNode>::New();
  outputContourNode->SetName("OutputContour");
  mrmlScene->AddNode(outputContourNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLContourMorphologyNode> paramNode = vtkSmartPointer<vtkMRMLContourMorphologyNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveReferenceContourNodeID(contourNode->GetID());
  paramNode->SetAndObserveInputContourNodeID(contourNode->GetID());
  paramNode->SetAndObserveOutputContourNodeID(outputContourNode->GetID());
  paramNode->SetOperationToExpand();
  paramNode->SetXSize(5);
  paramNode->SetYSize(5);
  paramNode->SetZSize(5);

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
  
  // Create baseline expand labelmap node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> labelmapExpandBaselineScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(labelmapExpandBaselineScalarVolumeNode);
  labelmapExpandBaselineScalarVolumeNode->SetName("PTV_Labelmap_Exp");
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load baseline labelmap volume
  std::string labelmapExpandBaselineVolumeFileName = std::string(dataDirectoryPath) + "/PTV_Contour_Labelmap_Exp.nrrd";
  if (!vtksys::SystemTools::FileExists(labelmapExpandBaselineVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << labelmapExpandBaselineVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> labelmapExpandBaselineVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(labelmapExpandBaselineVolumeArchetypeStorageNode);
  labelmapExpandBaselineVolumeArchetypeStorageNode->SetFileName(labelmapExpandBaselineVolumeFileName.c_str());
  labelmapExpandBaselineScalarVolumeNode->SetAndObserveStorageNodeID(labelmapExpandBaselineVolumeArchetypeStorageNode->GetID());

  if (! labelmapExpandBaselineVolumeArchetypeStorageNode->ReadData(labelmapExpandBaselineScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << labelmapExpandBaselineVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();

  vtkSmartPointer<vtkImageMathematics> difference = vtkSmartPointer<vtkImageMathematics>::New();
  difference->SetInput1(outputLabelmapNode->GetImageData());
  difference->SetInput2(labelmapExpandBaselineScalarVolumeNode->GetImageData());
  difference->SetOperationToSubtract();
  difference->Update();

  // Compute histogram
  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(difference->GetOutput());
  histogram->IgnoreZeroOn();
  histogram->Update();
  
  if (histogram->GetVoxelCount() > volumeDifferenceToleranceCc)
  {
    std::cerr << "Contour Expanding: Volume difference Tolerance(Cc) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  /////////////////
  // Test shrinking
  paramNode->SetOperationToShrink();
  paramNode->SetXSize(5);
  paramNode->SetYSize(5);
  paramNode->SetZSize(5);

  // Morph contour
  contourMorphologyLogic->MorphContour();

  // Create baseline expand labelmap node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> labelmapShrinkBaselineScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  mrmlScene->AddNode(labelmapShrinkBaselineScalarVolumeNode);
  labelmapShrinkBaselineScalarVolumeNode->SetName("PTV_Labelmap_Shrinked");
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load dose volume
  std::string labelmapShrinkBaselineVolumeFileName = std::string(dataDirectoryPath) + "/PTV_Contour_Labelmap_Shrk.nrrd";
  if (!vtksys::SystemTools::FileExists(labelmapShrinkBaselineVolumeFileName.c_str()))
  {
    std::cerr << "Loading labelmap from file '" << labelmapShrinkBaselineVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> labelmapShrinkBaselineVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  mrmlScene->AddNode(labelmapShrinkBaselineVolumeArchetypeStorageNode);
  labelmapShrinkBaselineVolumeArchetypeStorageNode->SetFileName(labelmapShrinkBaselineVolumeFileName.c_str());
  labelmapShrinkBaselineScalarVolumeNode->SetAndObserveStorageNodeID(labelmapShrinkBaselineVolumeArchetypeStorageNode->GetID());

  if (! labelmapShrinkBaselineVolumeArchetypeStorageNode->ReadData(labelmapShrinkBaselineScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading labelmap from file '" << labelmapShrinkBaselineVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  mrmlScene->Commit();

  difference->SetInput1(outputLabelmapNode->GetImageData());
  difference->SetInput2(labelmapShrinkBaselineScalarVolumeNode->GetImageData());
  difference->SetOperationToSubtract();
  difference->Update();

  // Compute histogram
  histogram->SetInput(difference->GetOutput());
  histogram->IgnoreZeroOn();
  histogram->Update();
  
  if (histogram->GetVoxelCount() > volumeDifferenceToleranceCc)
  {
    std::cerr << "Contour Shrinking: Volume difference Tolerance(Cc) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  // Create dose volume node
  vtkSmartPointer<vtkMRMLContourNode> contourNode2 = vtkSmartPointer<vtkMRMLContourNode>::New();
  contourNode2->SetName("PTV_Labelmap_Expanded");
  mrmlScene->AddNode(contourNode2);
  contourNode2->SetAndObserveIndexedLabelmapVolumeNodeId(labelmapExpandBaselineScalarVolumeNode->GetID());
  contourNode2->SetActiveRepresentationByNode(labelmapExpandBaselineScalarVolumeNode);

  /////////////
  // Test union
  paramNode->SetAndObserveReferenceContourNodeID(contourNode2->GetID());
  paramNode->SetAndObserveInputContourNodeID(contourNode->GetID());
  paramNode->SetOperationToUnion();

  // Compute ContourMorphology
  contourMorphologyLogic->MorphContour();

  mrmlScene->Commit();

  difference->SetInput1(labelmapExpandBaselineScalarVolumeNode->GetImageData());
  difference->SetInput2(outputLabelmapNode->GetImageData());
  difference->SetOperationToSubtract();
  difference->Update();

  // Compute histogram
  histogram->SetInput(difference->GetOutput());
  histogram->IgnoreZeroOn();
  histogram->Update();
  
  if (histogram->GetVoxelCount() > volumeDifferenceToleranceCc)
  {
    std::cerr << "Contour Union: Volume difference Tolerance(Cc):" << histogram->GetVoxelCount() << " exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  ////////////////////
  // Test intersection
  paramNode->SetAndObserveReferenceContourNodeID(contourNode2->GetID());
  paramNode->SetAndObserveInputContourNodeID(contourNode->GetID());
  paramNode->SetOperationToIntersect();

  // Compute ContourMorphology
  contourMorphologyLogic->MorphContour();

  mrmlScene->Commit();

  difference->SetInput1(outputLabelmapNode->GetImageData());
  difference->SetInput2(labelmapScalarVolumeNode->GetImageData());
  difference->SetOperationToSubtract();
  difference->Update();

  // Compute histogram
  histogram->SetInput(difference->GetOutput());
  histogram->IgnoreZeroOn();
  histogram->Update();
  
  if (histogram->GetVoxelCount() > volumeDifferenceToleranceCc)
  {
    std::cerr << "Contour Intersection: Volume difference Tolerance(Cc) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

