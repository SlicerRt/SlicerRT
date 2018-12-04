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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkCollection.h>
#include <vtkMassProperties.h>

// ITK includes
#include "itkFactoryRegistration.h"

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
int vtkSlicerIsodoseModuleLogicTest1( int argc, char * argv[] )
{
  int argIndex = 1;

  // TestSceneFile
  const char *testSceneFileName  = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-TestSceneFile") == 0)
    {
      testSceneFileName = argv[argIndex+1];
      std::cout << "Test MRML scene file name: " << testSceneFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      testSceneFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments" << std::endl;
    return EXIT_FAILURE;
  }
  // TemporarySceneFile
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
    std::cerr << "Invalid arguments" << std::endl;
    return EXIT_FAILURE;
  }
  // BaselineIsodoseSurfaceFile
  const char *baselineIsodoseSurfaceFileName = NULL;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-BaselineIsodoseSurfaceFile") == 0)
    {
      baselineIsodoseSurfaceFileName = argv[argIndex+1];
      std::cout << "Baseline isodose surface file name: " << baselineIsodoseSurfaceFileName << std::endl;
      argIndex += 2;
    }
    else
    {
      baselineIsodoseSurfaceFileName = "";
    }
  }
  else
  {
    std::cerr << "Invalid arguments" << std::endl;
    return EXIT_FAILURE;
  }
  // VolumeDifferenceToleranceCc
  double volumeDifferenceToleranceCc = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-VolumeDifferenceToleranceCc") == 0)
    {
      volumeDifferenceToleranceCc = vtkVariant(argv[argIndex+1]).ToDouble();
      std::cout << "Volume difference Tolerance(Cc): " << volumeDifferenceToleranceCc << std::endl;
      argIndex += 2;
    }
  }
  else
  {
    std::cerr << "Invalid arguments" << std::endl;
    return EXIT_FAILURE;
  }

  // Constraint the criteria to be greater than zero
  if (volumeDifferenceToleranceCc == 0.0)
  {
    volumeDifferenceToleranceCc = EPSILON;
  }

  // Make sure NRRD reading works
  itk::itkFactoryRegistration();

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();
  // Trigger resolving subject hierarchies after import (merging the imported one with the pseudo-singleton one).
  // Normally this is done by the plugin logic, but it is a Qt class, so we need to trigger it manually from a VTK-only environment.
  vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(mrmlScene);

  // Save it to the temporary directory
  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Get dose volume
  vtkSmartPointer<vtkCollection> doseVolumeNodes = vtkSmartPointer<vtkCollection>::Take(
    mrmlScene->GetNodesByName("Dose") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    std::cerr << "ERROR: Failed to get dose volume" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* doseScalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));

  // Create and set up logic
  vtkSmartPointer<vtkSlicerIsodoseModuleLogic> isodoseLogic =
    vtkSmartPointer<vtkSlicerIsodoseModuleLogic>::New();
  isodoseLogic->SetMRMLScene(mrmlScene);

  // Set the number of Isodose level to 1 by setting number of color to 1
  vtkMRMLColorTableNode* isodoseColorNode = vtkSlicerIsodoseModuleLogic::GetDefaultIsodoseColorTable(mrmlScene);
  if (!isodoseColorNode)
  {
    mrmlScene->Commit();
    std::cerr << "ERROR: Failed to create default isodose color table" << std::endl;
    return EXIT_FAILURE;
  }
  isodoseColorNode->SetNumberOfColors(1);

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLIsodoseNode> paramNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveDoseVolumeNode(doseScalarVolumeNode);
  paramNode->SetAndObserveColorTableNode(isodoseColorNode);

  // Compute isodose
  isodoseLogic->CreateIsodoseSurfaces(paramNode);

  vtkIdType isodoseFolderitemID = isodoseLogic->GetIsodoseFolderItemID(paramNode);
  if (!isodoseFolderitemID)
  {
    mrmlScene->Commit();
    std::cerr << "No isodose subject hierarchy folder created" << std::endl;
    return EXIT_FAILURE;
  }
  
  mrmlScene->Commit();

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(mrmlScene);
  if (!shNode)
  {
    std::cerr << "Failed to access subject hierarchy node";
    return EXIT_FAILURE;
  }
  std::vector<vtkIdType> isodoseChildItemIDs;
  shNode->GetItemChildren(isodoseFolderitemID, isodoseChildItemIDs, false);
  if (isodoseChildItemIDs.size() == 0)
  {
    std::cerr << "No items in isodose folder" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(shNode->GetItemDataNode(isodoseChildItemIDs[0]));

  vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
  reader->SetFileName(baselineIsodoseSurfaceFileName);
  reader->Update();

  vtkSmartPointer<vtkPolyData> baselinePolyData = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkMassProperties> propertiesBaseline = vtkSmartPointer<vtkMassProperties>::New();
  propertiesBaseline->SetInputData(reader->GetOutput());
  propertiesBaseline->Update();

  vtkSmartPointer<vtkMassProperties> propertiesCurrent = vtkSmartPointer<vtkMassProperties>::New();
  propertiesCurrent->SetInputData(modelNode->GetPolyData());
  propertiesCurrent->Update();

  if (fabs(propertiesBaseline->GetVolume() - propertiesCurrent->GetVolume()) > volumeDifferenceToleranceCc)
  {
    std::cerr << "Volume difference Tolerance(Cc) exceeds threshold" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
