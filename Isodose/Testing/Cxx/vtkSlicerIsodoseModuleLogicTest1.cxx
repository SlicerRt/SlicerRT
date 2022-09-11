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
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkMassProperties.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>

// ITK includes
#include "itkFactoryRegistration.h"

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
int vtkSlicerIsodoseModuleLogicTest1( int argc, char * argv[] )
{
  int argIndex = 1;

  // TestSceneFile
  const char *testSceneFileName  = nullptr;
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
  const char *temporarySceneFileName = nullptr;
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
  const char *baselineIsodoseSurfaceFileName = nullptr;
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
  vtkNew<vtkMRMLScene> mrmlScene;

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
  doseScalarVolumeNode->CreateDefaultDisplayNodes();

  // Create and set up logic
  vtkNew<vtkSlicerIsodoseModuleLogic> isodoseLogic;
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
  vtkNew<vtkMRMLIsodoseNode> paramNode;
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveDoseVolumeNode(doseScalarVolumeNode);
  paramNode->SetAndObserveColorTableNode(isodoseColorNode);

  // Compute isosurface for dose volume
  bool result = isodoseLogic->CreateIsodoseSurfaces(paramNode);
  mrmlScene->Commit();
  // Check the result and model
  if (result && paramNode->GetIsosurfacesModelNode())
  {
    // Get calculated model
    vtkMRMLModelNode* modelNode = paramNode->GetIsosurfacesModelNode();

    vtkNew<vtkPolyDataReader> reader;
    reader->SetFileName(baselineIsodoseSurfaceFileName);
    reader->Update();

    vtkNew<vtkPolyData> baselinePolyData;
    vtkNew<vtkMassProperties> propertiesBaseline;
    propertiesBaseline->SetInputData(reader->GetOutput());
    propertiesBaseline->Update();

    vtkNew<vtkMassProperties> propertiesCurrent;
    propertiesCurrent->SetInputData(modelNode->GetPolyData());
    propertiesCurrent->Update();

    double baselineVolumeCc = propertiesBaseline->GetVolume();
    double currentVolumeCc = propertiesCurrent->GetVolume();
    double volumeDifferenceCc = fabs(baselineVolumeCc - currentVolumeCc);
    if (volumeDifferenceCc > volumeDifferenceToleranceCc)
    {
      std::cerr << "Volume difference Tolerance(Cc) exceeds threshold" << std::endl;
      std::cerr << "Baseline volume (Cc):\t" << baselineVolumeCc << std::endl;
      std::cerr << "Current volume (Cc):\t" << currentVolumeCc << std::endl;
      std::cerr << "Volume difference (Cc):\t" << volumeDifferenceCc << std::endl;
      return EXIT_FAILURE;
    }
  }
  else
  {
    std::cerr << "Unable to compute isosurfaces model" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
