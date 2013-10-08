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

#include "SlicerRtCommon.h"
#include "vtkConvertContourRepresentations.h"
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


  // Make sure NRRD reading works
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  // Create scene
  vtkSmartPointer<vtkMRMLScene> mrmlScene = vtkSmartPointer<vtkMRMLScene>::New();

  // Load test scene into temporary scene
  mrmlScene->SetURL(testSceneFileName);
  mrmlScene->Import();

  vtksys::SystemTools::RemoveFile(temporarySceneFileName);
  mrmlScene->SetRootDirectory( vtksys::SystemTools::GetParentDirectory(temporarySceneFileName).c_str() );
  mrmlScene->SetURL(temporarySceneFileName);
  mrmlScene->Commit();

  // Get CT volume
  vtkSmartPointer<vtkCollection> doseVolumeNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("5: RTDOSE: PROS") );
  if (doseVolumeNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get CT volume!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLScalarVolumeNode* doseScalarVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(doseVolumeNodes->GetItemAsObject(0));

  // Get the body contour
  vtkSmartPointer<vtkCollection> bodyContourNodes = 
    vtkSmartPointer<vtkCollection>::Take( mrmlScene->GetNodesByName("BODY_Contour") );
  if (bodyContourNodes->GetNumberOfItems() != 1)
  {
    mrmlScene->Commit();
    errorStream << "ERROR: Failed to get body contour!" << std::endl;
    return EXIT_FAILURE;
  }
  vtkMRMLContourNode* bodyContourNode = vtkMRMLContourNode::SafeDownCast(bodyContourNodes->GetItemAsObject(0));

  bodyContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(doseScalarVolumeNode->GetID());
  bodyContourNode->SetRasterizationOversamplingFactor(2.0);

  vtkMRMLScalarVolumeNode* indexedLabelmapNode(NULL);
  {
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(bodyContourNode);
    converter->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);

    indexedLabelmapNode = bodyContourNode->GetIndexedLabelmapVolumeNode();
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
