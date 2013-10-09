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

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "SlicerRtCommon.h"
#include "vtkConvertContourRepresentations.h"
#include "vtkSlicerContoursModuleLogic.h"
#include <vtkCollection.h>
#include <vtkMRMLContourNode.h>
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtksys/SystemTools.hxx>

// ITK includes
#if ITK_VERSION_MAJOR > 3
  #include "itkFactoryRegistration.h"
#endif

bool CheckIfResultIsWithinOneTenthPercentFromBaseline(double result, double baseline);

//-----------------------------------------------------------------------------
int vtkSlicerContoursModuleLogicTestConversions ( int argc, char * argv[] )
{
  int argIndex = 1;
  std::ostream& outputStream = std::cout;
  std::ostream& errorStream = std::cerr;

  // TestSceneFile
  const char *testSceneFileName  = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TestSceneFile") == 0)
    {
      testSceneFileName = argv[argIndex+1];
      outputStream << "Test MRML scene file name: " << testSceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      testSceneFileName = "";
    }
  }
  else
  {
    errorStream << "Invalid arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  const char *temporarySceneFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TemporarySceneFile") == 0)
    {
      temporarySceneFileName = argv[argIndex+1];
      outputStream << "Temporary scene file name: " << temporarySceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      temporarySceneFileName = "";
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  int nonZeroVoxelCount(-1);
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-NonZeroVoxelCount") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected non-zero voxel count: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> nonZeroVoxelCount;
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  int expectedExtents[6];
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-XMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected XMinExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedExtents[0];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-XMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected XMaxExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedExtents[1];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-YMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected YMinExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedExtents[2];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-YMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected YMaxExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedExtents[3];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ZMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ZMinExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedExtents[4];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ZMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ZMaxExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedExtents[5];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  // Make sure NRRD reading works
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  vtkSmartPointer<vtkSlicerContoursModuleLogic> logic = vtkSmartPointer<vtkSlicerContoursModuleLogic>::New();
  logic->SetMRMLScene(mrmlScene);

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Get CT volume
  vtkSmartPointer<vtkCollection> doseVolumeNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("Dose") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get CT volume!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* doseScalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));

  // Get the body contour  
  vtkMRMLContourNode* bodyContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->GetNodeByID("vtkMRMLContourNode1"));
  if (bodyContourNode == NULL)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get body contour!" << std::endl;
    return EXIT_FAILURE;
  }

  bodyContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(doseScalarVolumeNode->GetID());
  bodyContourNode->SetRasterizationOversamplingFactor(2.0);

  vtkMRMLScalarVolumeNode* indexedLabelmapNode(NULL);
  {
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(bodyContourNode);
    converter->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);

    indexedLabelmapNode = bodyContourNode->GetIndexedLabelmapVolumeNode();
  }

  vtkImageData* image = indexedLabelmapNode->GetImageData();
  int extents[6];
  image->GetExtent(extents);

  if( extents[0] != expectedExtents[0] || 
    extents[1] != expectedExtents[1] || 
    extents[2] != expectedExtents[2] || 
    extents[3] != expectedExtents[3] || 
    extents[4] != expectedExtents[4] || 
    extents[5] != expectedExtents[5] )
  {
    errorStream << "Extents don't match." << std::endl;
    return EXIT_FAILURE;
  }

  int voxelCount(0);
  for (int z = extents[4]; z < extents[5]; z++)
  {
    for (int y = extents[2]; y < extents[3]; y++)
    {
      for (int x = extents[0]; x < extents[1]; x++)
      {
        unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x,y,z));
        if( *pixel != 0 )
        {
          voxelCount++;
        }
      }
    }
  }

  if( voxelCount != nonZeroVoxelCount )
  {
    errorStream << "Non-zero voxel count does not match expected result. Got: " << voxelCount << ". Expected: " << nonZeroVoxelCount << std::endl;
    return EXIT_FAILURE;
  }

  vtkMRMLModelNode* closedSurfaceModelNode(NULL);
  {
    // Set closed surface model conversion parameters
    bodyContourNode->SetDecimationTargetReductionFactor(0.0);

    // Delete occurrent existing representation and re-convert
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(bodyContourNode);
    converter->ReconvertRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);

    closedSurfaceModelNode = bodyContourNode->GetClosedSurfaceModelNode();
  }

  /*
  double resultFalseNegativesPercent = paramNode->GetFalseNegativesPercent();
  if (!CheckIfResultIsWithinOneTenthPercentFromBaseline(resultFalseNegativesPercent, falseNegativesPercent))
  {
    std::cerr << "False negatives (%) mismatch: " << resultFalseNegativesPercent << " instead of " << falseNegativesPercent << std::endl;
    return EXIT_FAILURE;
  }
  */

  return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
bool CheckIfResultIsWithinOneTenthPercentFromBaseline(double result, double baseline)
{
  if (baseline == 0.0)
  {
    return (fabs(result - baseline) < 0.0001);
  }

  double ratio = result / baseline;
  double absoluteDifferencePercent = fabs(ratio - 1.0) * 100.0;

  return absoluteDifferencePercent < 0.1;
}
