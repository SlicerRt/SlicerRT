/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// PlastimatchPy Logic includes
#include "vtkPlmpyRegistration.h"

// SlicerRtCommon
#include "PlmCommon.h"
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>

// ITK includes
#include <itkAffineTransform.h>
#include <itkArray.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>

// Plastimatch includes
#include "bspline_interpolate.h"
#include "plm_config.h"
#include "plm_image_header.h"
#include "plm_warp.h"
#include "plmregister.h"
#include "pointset.h"
#include "pointset_warp.h"
#include "xform.h"
#include "raw_pointset.h"
#include "volume.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlmpyRegistration);

//----------------------------------------------------------------------------
vtkPlmpyRegistration::vtkPlmpyRegistration()
{
  this->FixedImageID = NULL;
  this->MovingImageID = NULL;
  this->FixedLandmarksFileName = NULL;
  this->MovingLandmarksFileName = NULL;
  this->InitializationLinearTransformationID = NULL;
  this->OutputVolumeID = NULL;

  this->FixedLandmarks = NULL;
  this->MovingLandmarks = NULL;

  this->WarpedLandmarks = NULL;
  vtkSmartPointer<vtkPoints> warpedLandmarks = vtkSmartPointer<vtkPoints>::New();
  this->SetWarpedLandmarks(warpedLandmarks);

  this->MovingImageToFixedImageVectorField = NULL;

  this->RegistrationParameters = new Registration_parms();
  this->RegistrationData = new Registration_data();

}

//----------------------------------------------------------------------------
vtkPlmpyRegistration::~vtkPlmpyRegistration()
{
  this->SetFixedImageID(NULL);
  this->SetMovingImageID(NULL);
  this->SetFixedLandmarksFileName(NULL);
  this->SetMovingLandmarksFileName(NULL);
  this->SetInitializationLinearTransformationID(NULL);
  this->SetOutputVolumeID(NULL);

  this->SetFixedLandmarks(NULL);
  this->SetMovingLandmarks(NULL);
  this->SetWarpedLandmarks(NULL);

  this->MovingImageToFixedImageVectorField = NULL;

  delete this->RegistrationParameters;
  delete this->RegistrationData;
}

//----------------------------------------------------------------------------
void vtkPlmpyRegistration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkPlmpyRegistration::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML Scene!");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML Scene!");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::AddStage()
{
  this->RegistrationParameters->append_stage();
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::SetPar(char* key, char* value)
{
  printf ("Setting parameter %s %s\n", key, value);
  this->RegistrationParameters->set_key_value("STAGE", key, value);
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::RunRegistration()
{
#if defined (commentout)
  // Set input images
  vtkMRMLScalarVolumeNode* fixedVtkImage = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  itk::Image<float, 3>::Pointer fixedItkImage = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(fixedVtkImage, fixedItkImage, true);

  vtkMRMLScalarVolumeNode* movingVtkImage = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->MovingImageID));
  itk::Image<float, 3>::Pointer movingItkImage = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(movingVtkImage, movingItkImage, true);

  this->RegistrationData->fixed_image = Plm_image::New (
    new Plm_image(fixedItkImage));
  this->RegistrationData->moving_image = Plm_image::New(
    new Plm_image(movingItkImage));
#endif

  this->RegistrationData->fixed_image = 
    PlmCommon::ConvertVolumeNodeToPlmImage(
      this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  this->RegistrationData->moving_image = 
    PlmCommon::ConvertVolumeNodeToPlmImage(
      this->GetMRMLScene()->GetNodeByID(this->MovingImageID));

  /* A little debugging information */
  printf ("Fixed image\n");
  this->RegistrationData->fixed_image->print ();
  printf ("Moving image\n");
  this->RegistrationData->moving_image->print ();

  // Set landmarks 
  if (this->FixedLandmarks && this->MovingLandmarks)
  {
    this->SetLandmarksFromSlicer();
  }

  // Set initial affine transformation
  if (this->InitializationLinearTransformationID)
  {
    this->ApplyInitialLinearTransformation();
  }

  // Run registration and warp image
  Xform::Pointer outputXform = 
    do_registration_pure (this->RegistrationData, this->RegistrationParameters);
  printf ("do_registration_pure is complete.\n");

  Plm_image* warpedImage = new Plm_image();
  this->ApplyWarp(
    warpedImage, this->MovingImageToFixedImageVectorField, 
    outputXform, this->RegistrationData->fixed_image.get(), 
    this->RegistrationData->moving_image.get(), -1200, 0, 1);
  printf ("ApplyWarp() is complete.\n");

  this->SetWarpedImageInVolumeNode(warpedImage);
  //this->SetWarpedImageInVolumeNode(this->RegistrationData->fixed_image.get());

  delete warpedImage;
}

//------------------------------------------------------------------------------
void vtkPlmpyRegistration::WarpLandmarks()
{
  Labeled_pointset warpedPointset;
  pointset_warp(&warpedPointset, this->RegistrationData->moving_landmarks, this->MovingImageToFixedImageVectorField);
  
  // Clear warped landmarks
  this->WarpedLandmarks->Initialize();

  for (int i=0; i < (int)warpedPointset.count(); ++i)
  {
    vtkDebugMacro("[RTN] "
            << warpedPointset.point_list[i].p[0] << " "
            << warpedPointset.point_list[i].p[1] << " "
            << warpedPointset.point_list[i].p[2] << " -> "
            << -warpedPointset.point_list[i].p[0] << " "
            << -warpedPointset.point_list[i].p[1] << " "
            << warpedPointset.point_list[i].p[2]);
    this->WarpedLandmarks->InsertPoint(i,
      - warpedPointset.point_list[i].p[0],
      - warpedPointset.point_list[i].p[1],
      warpedPointset.point_list[i].p[2]);
    }
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::SetLandmarksFromSlicer()
{
  if (!this->FixedLandmarks || !this->MovingLandmarks)
    {
    vtkErrorMacro("SetLandmarksFromSlicer: Landmark point lists are not valid!");
    return;
    }

  Labeled_pointset* fixedLandmarksSet = new Labeled_pointset();
  Labeled_pointset* movingLandmarksSet = new Labeled_pointset();
  
  for (int i = 0; i < this->FixedLandmarks->GetNumberOfPoints(); i++)
    {

// NSh Debug
    printf("C code fixed landmark %d is (%.3f %.3f %.3f)\n", i,
      - this->FixedLandmarks->GetPoint(i)[0],
      - this->FixedLandmarks->GetPoint(i)[1],
      this->FixedLandmarks->GetPoint(i)[2]
	);

    Labeled_point* fixedLandmark = new Labeled_point("point",
      - this->FixedLandmarks->GetPoint(i)[0],
      - this->FixedLandmarks->GetPoint(i)[1],
      this->FixedLandmarks->GetPoint(i)[2]);

    Labeled_point* movingLandmark = new Labeled_point("point",
      - this->MovingLandmarks->GetPoint(i)[0],
      - this->MovingLandmarks->GetPoint(i)[1],
      this->MovingLandmarks->GetPoint(i)[2]);
   
    fixedLandmarksSet->point_list.push_back(*fixedLandmark);
    movingLandmarksSet->point_list.push_back(*movingLandmark);
    }

  this->RegistrationData->fixed_landmarks = fixedLandmarksSet;
  this->RegistrationData->moving_landmarks = movingLandmarksSet;
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::SetLandmarksFromFiles()
{
  if (!this->FixedLandmarksFileName || !this->MovingLandmarksFileName)
    {
    vtkErrorMacro("SetLandmarksFromFiles: Unable to read landmarks from files as at least one of the filenames is invalid!");
    return;
    }

  Labeled_pointset* fixedLandmarksFromFile = new Labeled_pointset();
  fixedLandmarksFromFile->load(this->FixedLandmarksFileName);
  this->RegistrationData->fixed_landmarks = fixedLandmarksFromFile;

  Labeled_pointset* movingLandmarksFromFile = new Labeled_pointset();
  movingLandmarksFromFile->load(this->MovingLandmarksFileName);
  this->RegistrationData->moving_landmarks = movingLandmarksFromFile;
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::ApplyInitialLinearTransformation()
{
  if (!this->InitializationLinearTransformationID)
    {
    vtkErrorMacro("ApplyInitialLinearTransformation: Invalid input transformation ID!");
    return;
    }

  // Get transformation as 4x4 matrix
  vtkMRMLLinearTransformNode* initializationLinearTransformationNode =
    vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->InitializationLinearTransformationID));
  if (!initializationLinearTransformationNode)
    {
    vtkErrorMacro("ApplyInitialLinearTransformation: Failed to retrieve input transformation!");
    return;
    }
  vtkMatrix4x4* initializationTransformationAsVtkTransformationMatrix = 
    initializationLinearTransformationNode->GetMatrixTransformToParent();

  // Create ITK array to store the parameters
  itk::Array<double> initializationTransformationAsAffineParameters;
  initializationTransformationAsAffineParameters.SetSize(12);

  // Set rotations
  int affineParameterIndex=0;
  for (int column=0; column < 3; column++)
    {
    for (int row=0; row < 3; row++)
      {
      initializationTransformationAsAffineParameters.SetElement(
        affineParameterIndex, initializationTransformationAsVtkTransformationMatrix->GetElement(row,column));
      affineParameterIndex++;
      }
    }

  // Set translations
  initializationTransformationAsAffineParameters.SetElement(
    9, initializationTransformationAsVtkTransformationMatrix->GetElement(0,3));
  initializationTransformationAsAffineParameters.SetElement(
    10, initializationTransformationAsVtkTransformationMatrix->GetElement(1,3));
  initializationTransformationAsAffineParameters.SetElement(
    11, initializationTransformationAsVtkTransformationMatrix->GetElement(2,3));

  // Create ITK affine transformation
  itk::AffineTransform<double, 3>::Pointer initializationTransformationAsItkTransformation = 
    itk::AffineTransform<double, 3>::New();
  initializationTransformationAsItkTransformation->SetParameters(initializationTransformationAsAffineParameters);

  // Set transformation
  Xform::Pointer plmInitializationXform = Xform::New ();
  plmInitializationXform->set_aff(initializationTransformationAsItkTransformation);

  // Warp image using the input transformation
  Plm_image::Pointer prealignedImage = Plm_image::New();
  this->ApplyWarp(prealignedImage.get(), NULL, 
    plmInitializationXform, 
    this->RegistrationData->fixed_image.get(), this->RegistrationData->moving_image.get(), -1200, 0, 1);

  // Update moving image
  this->RegistrationData->moving_image = prealignedImage;
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::ApplyWarp(
  Plm_image* warpedImage,
  DeformationFieldType::Pointer vectorFieldFromTransformation, 
  const Xform::Pointer inputTransformation, 
  Plm_image* fixedImage, 
  Plm_image* imageToWarp, 
  float defaultValue, 
  int useItk, 
  int interpolationLinear)
{
  Plm_image_header* plastimatchImageHeader = new Plm_image_header(fixedImage);
  plm_warp(warpedImage, &vectorFieldFromTransformation, inputTransformation, plastimatchImageHeader,
    imageToWarp, defaultValue, useItk, interpolationLinear);
  this->MovingImageToFixedImageVectorField = vectorFieldFromTransformation;
}

//---------------------------------------------------------------------------
void vtkPlmpyRegistration::SetWarpedImageInVolumeNode(Plm_image* warpedPlastimatchImage)
{
  if (!warpedPlastimatchImage || !warpedPlastimatchImage->itk_float())
    {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Invalid warped image!");
    return;
    }

  itk::Image<float, 3>::Pointer outputImageItk = warpedPlastimatchImage->itk_float();    

  vtkSmartPointer<vtkImageData> outputImageVtk = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(outputImageItk, outputImageVtk, VTK_FLOAT);
  
  // Read fixed image to get the geometrical information
  vtkMRMLScalarVolumeNode* fixedVolumeNode 
    = vtkMRMLScalarVolumeNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  if (!fixedVolumeNode)
    {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Node containing the fixed image cannot be retrieved!");
    return;
    }

  // Create new image node
  vtkMRMLScalarVolumeNode* warpedImageNode 
    = vtkMRMLScalarVolumeNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(this->OutputVolumeID));
  if (!warpedImageNode)
    {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Node containing the warped image cannot be retrieved!");
    return;
    }

  // Set warped image to a Slicer node
  warpedImageNode->CopyOrientation(fixedVolumeNode);
  warpedImageNode->SetAndObserveImageData(outputImageVtk);
}

