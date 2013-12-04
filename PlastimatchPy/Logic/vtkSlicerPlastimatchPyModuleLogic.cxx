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

#include "SlicerRtCommon.h"

// PlastimatchPy Logic includes
#include "vtkSlicerPlastimatchPyModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>

// ITK includes
#include <itkAffineTransform.h>
#include <itkArray.h>
#include <itkImageRegionIteratorWithIndex.h>

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
vtkStandardNewMacro(vtkSlicerPlastimatchPyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerPlastimatchPyModuleLogic::vtkSlicerPlastimatchPyModuleLogic()
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
vtkSlicerPlastimatchPyModuleLogic::~vtkSlicerPlastimatchPyModuleLogic()
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

  this->RegistrationParameters = NULL;
  this->RegistrationData = NULL;
}

//----------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RegisterNodes: Invalid MRML Scene!");
    return;
    }
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML Scene!");
    return;
    }
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::AddStage()
{
  this->RegistrationParameters->append_stage();
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::SetPar(char* key, char* value)
{        
  this->RegistrationParameters->set_key_val(key, value, 1);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::RunRegistration()
{
  // Set input images
  vtkMRMLScalarVolumeNode* fixedVtkImage = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  itk::Image<float, 3>::Pointer fixedItkImage = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImageInLPS<float>(fixedVtkImage, fixedItkImage);

  vtkMRMLScalarVolumeNode* movingVtkImage = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->MovingImageID));
  itk::Image<float, 3>::Pointer movingItkImage = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImageInLPS<float>(movingVtkImage, movingItkImage);

  //this->RegistrationData->fixed_image = new Plm_image(fixedItkImage);
  //this->RegistrationData->moving_image = new Plm_image(movingItkImage);
  this->RegistrationData->fixed_image = Plm_image::New (
    new Plm_image(fixedItkImage));
  this->RegistrationData->moving_image = Plm_image::New(
    new Plm_image(movingItkImage));
  
  // Set landmarks 
  if (this->FixedLandmarks && this->MovingLandmarks)
    {
    // From Slicer
    this->SetLandmarksFromSlicer();
    }
  else if (this->FixedLandmarksFileName && this->FixedLandmarksFileName)
    {
    // From Files
    this->SetLandmarksFromFiles();
    }
  else
    {
    vtkErrorMacro("RunRegistration: Unable to retrieve fixed and moving landmarks!");
    return;
    }
  
  // Set initial affine transformation
  if (this->InitializationLinearTransformationID)
    {
    this->ApplyInitialLinearTransformation(); 
    } 

  // Run registration and warp image
  Xform* movingImageToFixedImageTransformation = NULL; // Transformation (linear or deformable) computed by Plastimatch
  do_registration_pure(&movingImageToFixedImageTransformation, this->RegistrationData ,this->RegistrationParameters);

  Plm_image* warpedImage = new Plm_image();
  this->ApplyWarp(warpedImage, this->MovingImageToFixedImageVectorField, movingImageToFixedImageTransformation,
    this->RegistrationData->fixed_image.get(), this->RegistrationData->moving_image.get(), -1200, 0, 1);

  this->SetWarpedImageInVolumeNode(warpedImage);

  delete warpedImage;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::WarpLandmarks()
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
void vtkSlicerPlastimatchPyModuleLogic::SetLandmarksFromSlicer()
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
void vtkSlicerPlastimatchPyModuleLogic::SetLandmarksFromFiles()
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
void vtkSlicerPlastimatchPyModuleLogic::ApplyInitialLinearTransformation()
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
  Xform* initializationTransformationAsPlastimatchTransformation = new Xform();
  initializationTransformationAsPlastimatchTransformation->set_aff(initializationTransformationAsItkTransformation);

  // Warp image using the input transformation
  Plm_image::Pointer prealignedImage = Plm_image::New();
  this->ApplyWarp(prealignedImage.get(), NULL, initializationTransformationAsPlastimatchTransformation,
    this->RegistrationData->fixed_image.get(), this->RegistrationData->moving_image.get(), -1200, 0, 1);

  // Update moving image
  this->RegistrationData->moving_image = prealignedImage;

  delete initializationTransformationAsPlastimatchTransformation;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::ApplyWarp(Plm_image* warpedImage,
  DeformationFieldType::Pointer vectorFieldFromTransformation, Xform* inputTransformation, 
  Plm_image* fixedImage, Plm_image* imageToWarp, float defaultValue, int useItk, int interpolationLinear)
{
  Plm_image_header* plastimatchImageHeader = new Plm_image_header(fixedImage);
  plm_warp(warpedImage, &vectorFieldFromTransformation, inputTransformation, plastimatchImageHeader,
    imageToWarp, defaultValue, useItk, interpolationLinear);
  this->MovingImageToFixedImageVectorField = vectorFieldFromTransformation;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchPyModuleLogic::SetWarpedImageInVolumeNode(Plm_image* warpedPlastimatchImage)
{
  if (!warpedPlastimatchImage || !warpedPlastimatchImage->itk_float())
    {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Invalid warped image!");
    return;
    }

  itk::Image<float, 3>::Pointer outputImageItk = warpedPlastimatchImage->itk_float();    

  vtkSmartPointer<vtkImageData> outputImageVtk = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::RegionType region = outputImageItk->GetBufferedRegion();
  itk::Image<float, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, (int) imageSize[0]-1, 0, (int) imageSize[1]-1, 0, (int) imageSize[2]-1};
  outputImageVtk->SetExtent(extent);
  outputImageVtk->SetScalarType(VTK_FLOAT);
  outputImageVtk->SetNumberOfScalarComponents(1);
  outputImageVtk->AllocateScalars();
  
  float* outputImagePtr = (float*)outputImageVtk->GetScalarPointer();
  itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > outputImageItkIterator(
    outputImageItk, outputImageItk->GetLargestPossibleRegion() );
  
  for ( outputImageItkIterator.GoToBegin(); !outputImageItkIterator.IsAtEnd(); ++outputImageItkIterator)
    {
    itk::Image<float, 3>::IndexType i = outputImageItkIterator.GetIndex();
    (*outputImagePtr) = outputImageItk->GetPixel(i);
    outputImagePtr++;
    }
  
  // Read fixed image to get the geometrical information
  vtkMRMLScalarVolumeNode* fixedVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(\
    this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  if (!fixedVolumeNode)
    {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Node containing the fixed image cannot be retrieved!");
    return;
    }

  // Create new image node
  vtkMRMLScalarVolumeNode* warpedImageNode = vtkMRMLScalarVolumeNode::SafeDownCast(
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
