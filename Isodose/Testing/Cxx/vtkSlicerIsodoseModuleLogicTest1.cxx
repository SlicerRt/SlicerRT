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

// Isodose includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkMRMLIsodoseNode.h"

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

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <vtkLookupTable.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#define EPSILON 0.0001

/* Define case insensitive string compare for all supported platforms. */
#if defined( _WIN32 ) && !defined(__CYGWIN__)
#  if defined(__BORLANDC__)
#    define STRCASECMP stricmp
#  else
#    define STRCASECMP _stricmp
#  endif
#else
#  define STRCASECMP strcasecmp
#endif

//-----------------------------------------------------------------------------
int vtkSlicerIsodoseModuleLogicTest1( int argc, char * argv[] )
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

  double volumeDifferenceCriterion = 0.0;
  if (argc > argIndex+1)
  {
    if (STRCASECMP(argv[argIndex], "-VolumeDifferenceCriterion") == 0)
    {
      volumeDifferenceCriterion = atof(argv[argIndex+1]);
      std::cout << "Volume difference criterion: " << volumeDifferenceCriterion << std::endl;
      argIndex += 2;
    }
  }

  // Constraint the criteria to be greater than zero
  if (volumeDifferenceCriterion == 0.0)
  {
    volumeDifferenceCriterion = EPSILON;
  }

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Create dose volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> doseScalarVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  doseScalarVolumeNode->SetName("Dose");

  // Load and set attributes from file
  std::string doseAttributesFileName = std::string(dataDirectoryPath) + "/Dose.attributes";
  if (!vtksys::SystemTools::FileExists(doseAttributesFileName.c_str()))
  {
    std::cerr << "Loading dose attributes from file '" << doseAttributesFileName << "' failed - the file does not exist!" << std::endl;
  }

  std::string doseUnitName = "";
  std::ifstream attributesStream;
  attributesStream.open(doseAttributesFileName.c_str(), std::ifstream::in);
  char attribute[512];
  while (attributesStream.getline(attribute, 512, ';'))
  {
    std::string attributeStr(attribute);
    int colonIndex = attributeStr.find(':');
    std::string name = attributeStr.substr(0, colonIndex);
    std::string value = attributeStr.substr(colonIndex + 1);
    doseScalarVolumeNode->SetAttribute(name.c_str(), value.c_str());

    if (SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.compare(name) == 0)
    {
      doseUnitName = value;
    }
  }
  attributesStream.close();

  mrmlScene->AddNode(doseScalarVolumeNode);
  //EXERCISE_BASIC_DISPLAYABLE_MRML_METHODS(vtkMRMLScalarVolumeNode, doseScalarVolumeNode);

  // Load dose volume
  std::string doseVolumeFileName = std::string(dataDirectoryPath) + "/Dose.nrrd";
  if (!vtksys::SystemTools::FileExists(doseVolumeFileName.c_str()))
  {
    std::cerr << "Loading dose volume from file '" << doseVolumeFileName << "' failed - the file does not exist!" << std::endl;
  }

  vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode> doseVolumeArchetypeStorageNode =
    vtkSmartPointer<vtkMRMLVolumeArchetypeStorageNode>::New();
  doseVolumeArchetypeStorageNode->SetFileName(doseVolumeFileName.c_str());
  mrmlScene->AddNode(doseVolumeArchetypeStorageNode);
  //EXERCISE_BASIC_STORAGE_MRML_METHODS(vtkMRMLVolumeArchetypeStorageNode, doseVolumeArchetypeStorageNode);

  doseScalarVolumeNode->SetAndObserveStorageNodeID(doseVolumeArchetypeStorageNode->GetID());

  if (! doseVolumeArchetypeStorageNode->ReadData(doseScalarVolumeNode))
  {
    mrmlScene->Commit();
    std::cerr << "Reading dose volume from file '" << doseVolumeFileName << "' failed!" << std::endl;
    return EXIT_FAILURE;
  }

  // Determine maximum dose
  vtkNew<vtkImageAccumulate> doseStat;
  doseStat->SetInput(doseScalarVolumeNode->GetImageData());
  doseStat->Update();
  double maxDose = doseStat->GetMax()[0];

  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  std::cout << "before logic node" << std::endl;

  // Create and set up parameter set MRML node
  vtkSmartPointer<vtkMRMLIsodoseNode> paramNode = vtkSmartPointer<vtkMRMLIsodoseNode>::New();
  mrmlScene->AddNode(paramNode);
  paramNode->SetAndObserveDoseVolumeNodeId(doseScalarVolumeNode->GetID());

  std::cout << "before logic node2" << std::endl;

  // Create and set up logic
  vtkSmartPointer<vtkSlicerIsodoseModuleLogic> isodoseLogic = vtkSmartPointer<vtkSlicerIsodoseModuleLogic>::New();
  isodoseLogic->SetMRMLScene(mrmlScene);
  isodoseLogic->SetAndObserveIsodoseNode(paramNode);
  isodoseLogic->GetDefaultLabelMapColorTableNodeId();

  std::cout << "before logic node3" << std::endl;

  // Compute isodose
  isodoseLogic->ComputeIsodose();

  std::cout << "before logic node4" << std::endl;

  vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyRootNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    mrmlScene->GetNodeByID(paramNode->GetOutputHierarchyNodeId()));  
  if (modelHierarchyRootNode == NULL)
  {
    mrmlScene->Commit();
    std::cerr << "Invalid model hierarchy node!" << std::endl;
    return EXIT_FAILURE;
  }

  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);
  mrmlScene->Commit();

  bool returnWithSuccess = true;

  // Compare isodose surfaces
  double agreementAcceptancePercentage = -1.0;
  if (vtksys::SystemTools::FileExists(baselineIsodoseSurfaceFileName))
  {
    //if (CompareIsodoseSurfaces(temporaryDvhTableCsvFileName, baselineDvhTableCsvFileName, maxDose,
    //  volumeDifferenceCriterion, doseToAgreementCriterion, agreementAcceptancePercentage) > 0)
    //{
    //  std::cerr << "Failed to compare DVH table to baseline!" << std::endl;
    //  returnWithSuccess = false;
    //}
  }
  else
  {
    //std::cerr << "Failed to open baseline DVH table: " << baselineDvhTableCsvFileName << std::endl;
    //returnWithSuccess = false;
  }

  if (!returnWithSuccess)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

