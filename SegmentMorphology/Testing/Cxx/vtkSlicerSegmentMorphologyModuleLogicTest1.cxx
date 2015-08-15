/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

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

// SegmentMorphology includes
#include "vtkSlicerSegmentMorphologyModuleLogic.h"
#include "vtkMRMLSegmentMorphologyNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkRibbonModelToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToRibbonModelConversionRule.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScene.h>

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// SegmentationCore includes
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSegmentationConverterFactory.h"

// VTK includes
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>
#include <vtkImageMathematics.h>
#include <vtkNew.h>
#include <vtkTransform.h>

// ITK includes
#include "itkFactoryRegistration.h"

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#define MIN_VOLUME_DIFFERENCE_TOLERANCE_VOXEL 100

//-----------------------------------------------------------------------------
int vtkSlicerSegmentMorphologyModuleLogicTest1( int argc, char * argv[] )
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

  const char *inputSegmentationAFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputSegmentationAFile") == 0)
    {
      inputSegmentationAFile = argv[argIndex+1];
      std::cout << "Input segmentation A labelmap file name: " << inputSegmentationAFile << std::endl;
      argIndex += 2;
    }
    else
    {
      inputSegmentationAFile = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *inputSegmentationBFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-InputSegmentationBFile") == 0)
    {
      inputSegmentationBFile = argv[argIndex+1];
      std::cout << "Input segmentation B labelmap file name: " << inputSegmentationBFile << std::endl;
      argIndex += 2;
    }
    else
    {
      inputSegmentationBFile = "";
    }
  }
  else
  {
    std::cerr << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *baselineSegmentationFile = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineSegmentationFile") == 0)
    {
      baselineSegmentationFile = argv[argIndex+1];
      std::cout << "Baseline segmentation file name: " << baselineSegmentationFile << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineSegmentationFile = "";
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
  vtkMRMLSegmentMorphologyNode::SegmentMorphologyOperationType operation = vtkMRMLSegmentMorphologyNode::None;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-MorphologicalOperation") == 0)
    {
      morphologicalOperation = argv[argIndex+1];
      std::cout << "Morphological Operation: " << morphologicalOperation << std::endl;
      argIndex += 2;
      if (STRCASECMP(morphologicalOperation, "Expand") == 0)
      {
        operation = vtkMRMLSegmentMorphologyNode::Expand;
      }
      else if (STRCASECMP(morphologicalOperation, "Shrink") == 0)
      {
        operation = vtkMRMLSegmentMorphologyNode::Shrink;
      }
      else if (STRCASECMP(morphologicalOperation, "Union") == 0)
      {
        operation = vtkMRMLSegmentMorphologyNode::Union;
      }
      else if (STRCASECMP(morphologicalOperation, "Intersect") == 0)
      {
        operation = vtkMRMLSegmentMorphologyNode::Intersect;
      }
      else if (STRCASECMP(morphologicalOperation, "Subtract") == 0)
      {
        operation = vtkMRMLSegmentMorphologyNode::Subtract;
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
  itk::itkFactoryRegistration();

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create Segmentations logic
  vtkSmartPointer<vtkSlicerSegmentationsModuleLogic> segmentationsLogic = vtkSmartPointer<vtkSlicerSegmentationsModuleLogic>::New();
  segmentationsLogic->SetMRMLScene(mrmlScene);

  // Register converters to use ribbon models. Will be unnecessary when having resolved issues regarding
  // direct planar contours to closed surface conversion https://www.assembla.com/spaces/slicerrt/tickets/751
  // (vtkSlicerDicomRtImportExportConversionRules can also be removed from CMake link targets)
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(vtkSmartPointer<vtkRibbonModelToBinaryLabelmapConversionRule>::New());
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(vtkSmartPointer<vtkPlanarContourToRibbonModelConversionRule>::New());
  // Disable closed surface representation so that ribbon model is used for labelmap conversion instead of direct closed surface
  vtkSegmentationConverterFactory::GetInstance()->DisableRepresentation(vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());

  // Save scene to the temporary directory
  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Load input segmentation A node
  std::string inputSegmentationAFileName = std::string(dataDirectoryPath) + std::string(inputSegmentationAFile);
  if (!vtksys::SystemTools::FileExists(inputSegmentationAFileName.c_str()))
  {
    std::cerr << "Loading segmentation from file '" << inputSegmentationAFileName << "' failed - the file does not exist!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLSegmentationNode* inputSegmentationANode = segmentationsLogic->LoadSegmentationFromFile(inputSegmentationAFileName.c_str());
  if (!inputSegmentationANode)
  {
    std::cerr << "Loading segmentation from existing file '" << inputSegmentationAFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  // Make sure binary labelmap representation is available
  if (!inputSegmentationANode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    std::cerr << "Failed to create binary labelmap representation in input segmentation A!" << std::endl;
    return EXIT_FAILURE;
  }
  // Get ID of only segment
  if (inputSegmentationANode->GetSegmentation()->GetNumberOfSegments() > 1)
  {
    std::cerr << "Input segmentation A should contain only one segment!" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> inputSegmentAIDs;
  inputSegmentationANode->GetSegmentation()->GetSegmentIDs(inputSegmentAIDs);
  std::string inputSegmentAID = inputSegmentAIDs[0];

  // Load input segmentation B node if specified
  vtkMRMLSegmentationNode* inputSegmentationBNode = NULL;
  std::string inputSegmentBID("");
  if (strlen(inputSegmentationBFile) > 0)
  {
    std::string inputSegmentationBFileName = std::string(dataDirectoryPath) + std::string(inputSegmentationBFile);
    if (!vtksys::SystemTools::FileExists(inputSegmentationBFileName.c_str()))
    {
      std::cerr << "Loading segmentation from file '" << inputSegmentationBFileName << "' failed - the file does not exist!" << std::endl;
      return EXIT_FAILURE;
    }
    inputSegmentationBNode = segmentationsLogic->LoadSegmentationFromFile(inputSegmentationBFileName.c_str());
    if (!inputSegmentationBNode)
    {
      std::cerr << "Loading segmentation from existing file '" << inputSegmentationBFileName << "' failed!" << std::endl;
      return EXIT_FAILURE;
    }
    // Make sure binary labelmap representation is available
    if (!inputSegmentationBNode->GetSegmentation()->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      std::cerr << "Failed to create binary labelmap representation in input segmentation B!" << std::endl;
      return EXIT_FAILURE;
    }
    // Get ID of only segment
    if (inputSegmentationBNode->GetSegmentation()->GetNumberOfSegments() > 1)
    {
      std::cerr << "Input segmentation B should contain only one segment!" << std::endl;
      return EXIT_FAILURE;
    }
    std::vector<std::string> inputSegmentBIDs;
    inputSegmentationBNode->GetSegmentation()->GetSegmentIDs(inputSegmentBIDs);
    inputSegmentBID = inputSegmentBIDs[0];
  }
  else if (operation != vtkMRMLSegmentMorphologyNode::Expand && operation != vtkMRMLSegmentMorphologyNode::Shrink)
  {
    std::cerr << "Segmentation B file is not specified, but operation is neither Expand nor Shrink!" << std::endl;
    return EXIT_FAILURE;
  }

  // Apply simple transform to 
  if (applySimpleTransformToInput == 1)
  {
    vtkSmartPointer<vtkTransform> simpleTransform = vtkSmartPointer<vtkTransform>::New();
    simpleTransform->Identity();
    simpleTransform->Translate(0.0, 0.0, -10.0);

    vtkSmartPointer<vtkMRMLLinearTransformNode> simpleTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    simpleTransformNode->ApplyTransformMatrix(simpleTransform->GetMatrix());
    mrmlScene->AddNode(simpleTransformNode);

    inputSegmentationANode->SetAndObserveTransformNodeID(simpleTransformNode->GetID());
  }

  // Create output segmentation node
  vtkSmartPointer<vtkMRMLSegmentationNode> outputSegmentationNode = vtkSmartPointer<vtkMRMLSegmentationNode>::New();
  outputSegmentationNode->SetName("Output_Segmentation");
  mrmlScene->AddNode(outputSegmentationNode);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLSegmentMorphologyNode> paramNode = vtkSmartPointer<vtkMRMLSegmentMorphologyNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveSegmentationANode(inputSegmentationANode);
  paramNode->SetSegmentAID(inputSegmentAID.c_str());
  paramNode->SetAndObserveSegmentationBNode(inputSegmentationBNode);
  paramNode->SetSegmentBID(inputSegmentBID.c_str());
  paramNode->SetAndObserveOutputSegmentationNode(outputSegmentationNode);
  paramNode->SetOperation(operation);
  paramNode->SetXSize(morphologicalParameter);
  paramNode->SetYSize(morphologicalParameter);
  paramNode->SetZSize(morphologicalParameter);

  // Create and set up logic
  vtkSmartPointer<vtkSlicerSegmentMorphologyModuleLogic> segmentMorphologyLogic = vtkSmartPointer<vtkSlicerSegmentMorphologyModuleLogic>::New();
  segmentMorphologyLogic->SetMRMLScene(mrmlScene);
  segmentMorphologyLogic->SetAndObserveSegmentMorphologyNode(paramNode);

  // Compute SegmentMorphology
  segmentMorphologyLogic->ApplyMorphologyOperation();

  // Check output
  outputSegmentationNode = paramNode->GetOutputSegmentationNode();  
  if (outputSegmentationNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid output segmentation node!" << std::endl;
    return EXIT_FAILURE;
  }
  // Get ID of only segment
  if (outputSegmentationNode->GetSegmentation()->GetNumberOfSegments() > 1)
  {
    std::cerr << "Output segmentation should contain only one segment!" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> outputSegmentIDs;
  outputSegmentationNode->GetSegmentation()->GetSegmentIDs(outputSegmentIDs);
  std::string outputSegmentID = outputSegmentIDs[0];

  mrmlScene->Commit();

  // Load baseline segmentation for comparison
  std::string baselineSegmentationFileName = std::string(dataDirectoryPath) + std::string(baselineSegmentationFile);
  if (!vtksys::SystemTools::FileExists(baselineSegmentationFileName.c_str()))
  {
    std::cerr << "Loading segmentation from file '" << baselineSegmentationFileName << "' failed - the file does not exist!" << std::endl;
  }
  vtkMRMLSegmentationNode* baselineSegmentationNode = segmentationsLogic->LoadSegmentationFromFile(baselineSegmentationFileName.c_str());
  if (!baselineSegmentationNode)
  {
    std::cerr << "Loading segmentation from existing file '" << baselineSegmentationFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }
  // Get ID of only segment
  if (baselineSegmentationNode->GetSegmentation()->GetNumberOfSegments() > 1)
  {
    std::cerr << "Baseline segmentation should contain only one segment!" << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<std::string> baselineSegmentIDs;
  baselineSegmentationNode->GetSegmentation()->GetSegmentIDs(baselineSegmentIDs);
  std::string baselineSegmentID = baselineSegmentIDs[0];

  mrmlScene->Commit();

  // Compare output to baseline
  vtkOrientedImageData* baselineImageData = vtkOrientedImageData::SafeDownCast(
    baselineSegmentationNode->GetSegmentation()->GetSegment(baselineSegmentID)->GetRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) );
  vtkOrientedImageData* outputImageData = vtkOrientedImageData::SafeDownCast(
    outputSegmentationNode->GetSegmentation()->GetSegment(outputSegmentID)->GetRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) );
  if (!baselineImageData || !outputImageData)
  {
    std::cerr << "Failed to retrieve binary labelmap representation from the baseline or the output segmentation!" << std::endl;
    return EXIT_FAILURE;
  }

  // Check geometries and extents
  if (!vtkOrientedImageDataResample::DoGeometriesMatch(baselineImageData, outputImageData))
  {
    std::cerr << "Baseline and output image data have different geometries!" << std::endl;
    return EXIT_FAILURE;
  }
  if (!vtkOrientedImageDataResample::DoExtentsMatch(baselineImageData, outputImageData))
  {
    std::cerr << "Baseline and output image data have different extents!" << std::endl;
    return EXIT_FAILURE;
  }

  unsigned char* baselineImagePtr = (unsigned char*)baselineImageData->GetScalarPointer();
  unsigned char* outputImagePtr = (unsigned char*)outputImageData->GetScalarPointer();

  if (!baselineImageData || baselineImageData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    std::cerr << "Invalid image data! Scalar type has to be unsigned char instead of '" << (baselineImageData?baselineImageData->GetScalarTypeAsString():"None") << "'" << std::endl;
    return EXIT_FAILURE;
  }
  if (!outputImageData || outputImageData->GetScalarType() != VTK_UNSIGNED_CHAR)
  {
    std::cerr << "Invalid image data! Scalar type has to be unsigned char instead of '" << (outputImageData?outputImageData->GetScalarTypeAsString():"None") << "'" << std::endl;
    return EXIT_FAILURE;
  }
  if (baselineImageData->GetNumberOfPoints() != outputImageData->GetNumberOfPoints())
  {
    std::cerr << "Number of points do not match. " << baselineImageData->GetNumberOfPoints() << " != " << outputImageData->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }
  int mismatches(0);
  for (long i=0; i<baselineImageData->GetNumberOfPoints(); ++i)
  {
    if ( ((*baselineImagePtr) != 0 && (*outputImagePtr) == 0) ||
      ((*baselineImagePtr) == 0 && (*outputImagePtr) != 0) )
    {
      mismatches++;
    }
    ++baselineImagePtr;
    ++outputImagePtr;
  }

  if (mismatches > volumeDifferenceToleranceVoxel)
  {
    std::cerr << "Segment Morphology Test: Volume difference Tolerance " << mismatches << "(Voxel) exceeds threshold!" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

