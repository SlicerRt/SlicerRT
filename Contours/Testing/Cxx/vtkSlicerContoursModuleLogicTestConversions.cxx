/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkConvertContourRepresentations.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkMRMLContourNode.h"

// MRML includes
#include <vtkMRMLCoreTestingMacros.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkPolyData.h>
#include <vtksys/SystemTools.hxx>

// ITK includes
#include "itkFactoryRegistration.h"

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

  // Labelmap metrics
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

  // Labelmap extents
  int expectedExtents[6];
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-LabelMapXMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected LabelMapXMinExtent: " << arg << std::endl;
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
    if (STRCASECMP(argv[argIndex], "-LabelMapXMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected LabelMapXMaxExtent: " << arg << std::endl;
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
    if (STRCASECMP(argv[argIndex], "-LabelMapYMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected LabelMapYMinExtent: " << arg << std::endl;
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
    if (STRCASECMP(argv[argIndex], "-LabelMapYMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected LabelMapYMaxExtent: " << arg << std::endl;
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
    if (STRCASECMP(argv[argIndex], "-LabelMapZMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected LabelMapZMinExtent: " << arg << std::endl;
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
    if (STRCASECMP(argv[argIndex], "-LabelMapZMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected LabelMapZMaxExtent: " << arg << std::endl;
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

  // Closed surface metrics
  int expectedNumberOfPoints;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ExpectedNumberOfPoints") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected number of points: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedNumberOfPoints;
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  int expectedNumberOfCells;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ExpectedNumberOfCells") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected number of cells: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedNumberOfCells;
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  int expectedNumberOfPolys;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ExpectedNumberOfPolys") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected number of polys: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedNumberOfPolys;
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  // Closed surface bounds
  double expectedBounds[6];
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ClosedSurfaceXMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ClosedSurfaceXMinExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedBounds[0];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ClosedSurfaceXMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ClosedSurfaceXMaxExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedBounds[1];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ClosedSurfaceYMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ClosedSurfaceYMinExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedBounds[2];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ClosedSurfaceYMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ClosedSurfaceYMaxExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedBounds[3];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ClosedSurfaceZMinExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ClosedSurfaceZMinExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedBounds[4];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-ClosedSurfaceZMaxExtent") == 0)
    {
      char* arg = argv[argIndex+1];
      outputStream << "Expected ClosedSurfaceZMaxExtent: " << arg << std::endl;
      argIndex += 2;
      std::stringstream ss;
      ss << arg;
      ss >> expectedBounds[5];
    }
  }
  else
  {
    errorStream << "No arguments!" << std::endl;
    return EXIT_FAILURE;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Create logic classes
  vtkSmartPointer<vtkSlicerContoursModuleLogic> contoursLogic =
    vtkSmartPointer<vtkSlicerContoursModuleLogic>::New();
  contoursLogic->SetMRMLScene(mrmlScene);

  // TODO: Remove when subject hierarchy is integrated into Slicer core
  vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic> subjectHierarchyLogic =
    vtkSmartPointer<vtkSlicerSubjectHierarchyModuleLogic>::New();
  subjectHierarchyLogic->SetMRMLScene(mrmlScene);

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
#if (VTK_MAJOR_VERSION <= 5)
  doseScalarVolumeNode->GetImageData()->SetWholeExtent(doseScalarVolumeNode->GetImageData()->GetExtent());
#else
  doseScalarVolumeNode->GetImageData()->SetExtent(doseScalarVolumeNode->GetImageData()->GetExtent());
#endif


  // Get the body contour  
  vtkMRMLContourNode* bodyContourNode = vtkMRMLContourNode::SafeDownCast(mrmlScene->GetNodeByID("vtkMRMLContourNode1"));
  if (bodyContourNode == NULL)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get body contour!" << std::endl;
    return EXIT_FAILURE;
  }

  bodyContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(doseScalarVolumeNode->GetID());
  bodyContourNode->SetRasterizationOversamplingFactor(1.0);

  {
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(bodyContourNode);
    converter->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);
  }

  vtkImageData* image = bodyContourNode->GetLabelmapImageData();
  if (image == NULL)
  {
    errorStream << "Conversion to indexed labelmap failed.";
    return EXIT_FAILURE;
  }
  int result(EXIT_SUCCESS);
  int extents[6];
  image->GetExtent(extents);

  if( extents[0] != expectedExtents[0] || 
    extents[1] != expectedExtents[1] || 
    extents[2] != expectedExtents[2] || 
    extents[3] != expectedExtents[3] || 
    extents[4] != expectedExtents[4] || 
    extents[5] != expectedExtents[5] )
  {
    errorStream << "ERROR: Extents don't match:" << std::endl
      << "  Extent: ( " << extents[0] << "-" << extents[1]
      << ", " << extents[2] << "-" << extents[3]
      << ", " << extents[4] << "-" << extents[5] << " )" << std::endl;
    result = EXIT_FAILURE;
  }

  int voxelCount(0);
  for (int z = extents[4]; z < extents[5]; z++)
  {
    for (int y = extents[2]; y < extents[3]; y++)
    {
      for (int x = extents[0]; x < extents[1]; x++)
      {
        unsigned char* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x,y,z));
        if (*pixel != 0)
        {
          voxelCount++;
        }
      }
    }
  }

  if (voxelCount != nonZeroVoxelCount)
  {
    errorStream << "Non-zero voxel count does not match expected result. Got: " << voxelCount << ". Expected: " << nonZeroVoxelCount << std::endl;
    result = EXIT_FAILURE;
  }

  {
    // Set closed surface model conversion parameters
    bodyContourNode->SetDecimationTargetReductionFactor(0.0);

    // Delete current existing representation and re-convert
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(bodyContourNode);
    converter->ReconvertRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);
  }

  if (!bodyContourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel))
  {
    errorStream << "Conversion to closed surface model failed.";
    return EXIT_FAILURE;
  }

  double bounds[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
  bodyContourNode->GetRASBounds(bounds);

  if( !CheckIfResultIsWithinOneTenthPercentFromBaseline(bounds[0], expectedBounds[0]) || 
    !CheckIfResultIsWithinOneTenthPercentFromBaseline(bounds[1], expectedBounds[1]) || 
    !CheckIfResultIsWithinOneTenthPercentFromBaseline(bounds[2], expectedBounds[2]) || 
    !CheckIfResultIsWithinOneTenthPercentFromBaseline(bounds[3], expectedBounds[3]) || 
    !CheckIfResultIsWithinOneTenthPercentFromBaseline(bounds[4], expectedBounds[4]) || 
    !CheckIfResultIsWithinOneTenthPercentFromBaseline(bounds[5], expectedBounds[5]) )
  {
    errorStream << "Closed surface bounds don't match." << std::endl;
    errorStream << "bounds[0]: " << bounds[0] << std::endl;
    errorStream << "bounds[1]: " << bounds[1] << std::endl;
    errorStream << "bounds[2]: " << bounds[2] << std::endl;
    errorStream << "bounds[3]: " << bounds[3] << std::endl;
    errorStream << "bounds[4]: " << bounds[4] << std::endl;
    errorStream << "bounds[5]: " << bounds[5] << std::endl;
    result = EXIT_FAILURE;
  }

  if(bodyContourNode->GetClosedSurfacePolyData()->GetNumberOfPoints() != expectedNumberOfPoints)
  {
    errorStream << "Number of points mismatch in closed surface model. Expected: " << expectedNumberOfPoints << ". Got: " << bodyContourNode->GetClosedSurfacePolyData()->GetNumberOfPoints() << std::endl;
    result = EXIT_FAILURE;
  }

  if(bodyContourNode->GetClosedSurfacePolyData()->GetNumberOfCells() != expectedNumberOfCells)
  {
    errorStream << "Number of cells mismatch in closed surface model. Expected: " << expectedNumberOfCells << ". Got: " << bodyContourNode->GetClosedSurfacePolyData()->GetNumberOfCells() << std::endl;
    result = EXIT_FAILURE;
  }

  if(bodyContourNode->GetClosedSurfacePolyData()->GetNumberOfPolys() != expectedNumberOfPolys)
  {
    errorStream << "Number of polys mismatch in closed surface model. Expected: " << expectedNumberOfPolys << ". Got: " << bodyContourNode->GetClosedSurfacePolyData()->GetNumberOfPolys() << std::endl;
    result = EXIT_FAILURE;
  }

  return result;
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
