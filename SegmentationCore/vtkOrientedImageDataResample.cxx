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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// SegmentationCore includes
#include "vtkOrientedImageDataResample.h"

#include "vtkOrientedImageData.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVersionMacros.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkImageReslice.h>
#include <vtkImageConstantPad.h>
#include <vtkGeneralTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPlaneSource.h>
#include <vtkAppendPolyData.h>

vtkStandardNewMacro(vtkOrientedImageDataResample);

//----------------------------------------------------------------------------
vtkOrientedImageDataResample::vtkOrientedImageDataResample()
{
}

//----------------------------------------------------------------------------
vtkOrientedImageDataResample::~vtkOrientedImageDataResample()
{
}

//-----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(vtkOrientedImageData* inputImage, vtkOrientedImageData* referenceImage, vtkOrientedImageData* outputImage, bool linearInterpolation/*=false*/, bool padImage/*=false*/)
{
  if (!inputImage || !referenceImage || !outputImage)
  {
    return false;
  }

  // Get transform between input and reference
  vtkSmartPointer<vtkTransform> inputImageToReferenceImageTransform = vtkSmartPointer<vtkTransform>::New();
  vtkOrientedImageDataResample::GetTransformBetweenOrientedImages(inputImage, referenceImage, inputImageToReferenceImageTransform);

  // Calculate output extent in reference frame for padding if requested. Use all bounding box corners
  int inputExtentInReferenceFrame[6] = {0,-1,0,-1,0,-1};
  if (padImage)
  {
    vtkOrientedImageDataResample::TransformExtent(inputImage->GetExtent(), inputImageToReferenceImageTransform, inputExtentInReferenceFrame);
  }
  else
  {
    referenceImage->GetExtent(inputExtentInReferenceFrame);
  }

  // Return with failure if output extent is empty
  if ( inputExtentInReferenceFrame[0] == inputExtentInReferenceFrame[1] || inputExtentInReferenceFrame[2] == inputExtentInReferenceFrame[3] || inputExtentInReferenceFrame[4] == inputExtentInReferenceFrame[5] )
  {
    return false;
  }

  // Make sure input image data fits into the extent. If padding is disabled, then union extent is the reference extent
  int referenceExtent[6] = {0,-1,0,-1,0,-1};
  referenceImage->GetExtent(referenceExtent);
  int unionExtent[6] = { std::min(inputExtentInReferenceFrame[0],referenceExtent[0]), std::max(inputExtentInReferenceFrame[1],referenceExtent[1]),
                         std::min(inputExtentInReferenceFrame[2],referenceExtent[2]), std::max(inputExtentInReferenceFrame[3],referenceExtent[3]),
                         std::min(inputExtentInReferenceFrame[4],referenceExtent[4]), std::max(inputExtentInReferenceFrame[5],referenceExtent[5]) };

  // Invert transform for the resampling
  vtkSmartPointer<vtkTransform> referenceImageToInputImageTransform = vtkSmartPointer<vtkTransform>::New();
  referenceImageToInputImageTransform->Concatenate(inputImageToReferenceImageTransform);
  referenceImageToInputImageTransform->Inverse();

  // Create clone for input image that has an identity geometry
  //TODO: Creating a new vtkOrientedImageReslice class would be a better solution on the long run
  vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  identityMatrix->Identity();
  vtkSmartPointer<vtkOrientedImageData> identityInputImage = vtkSmartPointer<vtkOrientedImageData>::New();
  identityInputImage->ShallowCopy(inputImage);
  identityInputImage->SetGeometryFromImageToWorldMatrix(identityMatrix);

  // Perform resampling
  vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
#if (VTK_MAJOR_VERSION <= 5)
  resliceFilter->SetInput(identityInputImage);
#else
  resliceFilter->SetInputData(identityInputImage);
#endif
  resliceFilter->SetOutputOrigin(0, 0, 0);
  resliceFilter->SetOutputSpacing(1, 1, 1);
  resliceFilter->SetOutputExtent(unionExtent);

  resliceFilter->SetResliceTransform(referenceImageToInputImageTransform);

  // Set interpolation mode
  if (linearInterpolation)
  {
    resliceFilter->SetInterpolationModeToLinear();
  }
  else
  {
    resliceFilter->SetInterpolationModeToNearestNeighbor();
  }
  resliceFilter->Update();

  // Get reference geometry to set after copying result into output
  vtkSmartPointer<vtkMatrix4x4> referenceImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceImage->GetImageToWorldMatrix(referenceImageToWorldMatrix);

  // Set output
  outputImage->DeepCopy(resliceFilter->GetOutput());
  outputImage->SetGeometryFromImageToWorldMatrix(referenceImageToWorldMatrix);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::ResampleOrientedImageToReferenceGeometry(vtkOrientedImageData* inputImage, vtkMatrix4x4* referenceToWorldMatrix, vtkOrientedImageData* outputImage, bool linearInterpolation/*=false*/)
{
  if (!inputImage || !referenceToWorldMatrix || !outputImage)
  {
    return false;
  }

  // Only support the following scalar types
  int inputImageScalarType = inputImage->GetScalarType();
  if ( inputImageScalarType != VTK_UNSIGNED_CHAR
    && inputImageScalarType != VTK_UNSIGNED_SHORT
    && inputImageScalarType != VTK_SHORT )
  {
    vtkErrorWithObjectMacro(inputImage, "ResampleOrientedImageToReferenceGeometry: Input image scalar type must be unsigned char, unsighed short, or short!");
    return false;
  }

  // Determine IJK extent of contained data (non-zero voxels) in the input image
  int inputExtent[6] = {0,-1,0,-1,0,-1};
  inputImage->GetExtent(inputExtent);
  int inputDimensions[3] = {0, 0, 0};
  inputImage->GetDimensions(inputDimensions);
  int effectiveInputExtent[6] = {inputExtent[1], inputExtent[0], inputExtent[3], inputExtent[2], inputExtent[5], inputExtent[4]};
  // Handle three scalar types
  unsigned char* imagePtrUChar = (unsigned char*)inputImage->GetScalarPointerForExtent(inputExtent);
  unsigned short* imagePtrUShort = (unsigned short*)inputImage->GetScalarPointerForExtent(inputExtent);
  short* imagePtrShort = (short*)inputImage->GetScalarPointerForExtent(inputExtent);

  for (int i=0; i<inputDimensions[0]; ++i)
  {
    for (int j=0; j<inputDimensions[1]; ++j)
    {
      for (int k=0; k<inputDimensions[2]; ++k)
      {
        int voxelValue = 0;
        if (inputImageScalarType == VTK_UNSIGNED_CHAR)
        {
          voxelValue = (*(imagePtrUChar + i + j*inputDimensions[0] + k*inputDimensions[0]*inputDimensions[1]));
        }
        else if (inputImageScalarType == VTK_UNSIGNED_SHORT)
        {
          voxelValue = (*(imagePtrUShort + i + j*inputDimensions[0] + k*inputDimensions[0]*inputDimensions[1]));
        }
        else if (inputImageScalarType == VTK_SHORT)
        {
          voxelValue = (*(imagePtrShort + i + j*inputDimensions[0] + k*inputDimensions[0]*inputDimensions[1]));
        }

        if (voxelValue != 0)
        {
          if (i < effectiveInputExtent[0])
          {
            effectiveInputExtent[0] = i;
          }
          if (i > effectiveInputExtent[1])
          {
            effectiveInputExtent[1] = i;
          }
          if (j < effectiveInputExtent[2])
          {
            effectiveInputExtent[2] = j;
          }
          if (j > effectiveInputExtent[3])
          {
            effectiveInputExtent[3] = j;
          }
          if (k < effectiveInputExtent[4])
          {
            effectiveInputExtent[4] = k;
          }
          if (k > effectiveInputExtent[5])
          {
            effectiveInputExtent[5] = k;
          }
        }
      }
    }
  }

  // Return with failure if effective input extent is empty
  if ( effectiveInputExtent[0] == effectiveInputExtent[1] || effectiveInputExtent[2] == effectiveInputExtent[3] || effectiveInputExtent[4] == effectiveInputExtent[5] )
  {
    return false;
  }

  // Apply extent offset on calculated effective extent
  effectiveInputExtent[0] = effectiveInputExtent[0] + inputExtent[0];
  effectiveInputExtent[1] = effectiveInputExtent[1] + inputExtent[0];
  effectiveInputExtent[2] = effectiveInputExtent[2] + inputExtent[2];
  effectiveInputExtent[3] = effectiveInputExtent[3] + inputExtent[2];
  effectiveInputExtent[4] = effectiveInputExtent[4] + inputExtent[4];
  effectiveInputExtent[5] = effectiveInputExtent[5] + inputExtent[4];

  // Assemble transform
  vtkSmartPointer<vtkTransform> referenceImageToInputImageTransform = vtkSmartPointer<vtkTransform>::New();
  referenceImageToInputImageTransform->Identity();
  referenceImageToInputImageTransform->PostMultiply();

  vtkSmartPointer<vtkMatrix4x4> inputImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inputImage->GetImageToWorldMatrix(inputImageToWorldMatrix);
  referenceImageToInputImageTransform->Concatenate(inputImageToWorldMatrix);

  vtkSmartPointer<vtkMatrix4x4> worldToReferenceMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  worldToReferenceMatrix->DeepCopy(referenceToWorldMatrix);
  worldToReferenceMatrix->Invert();
  referenceImageToInputImageTransform->Concatenate(worldToReferenceMatrix);

  vtkSmartPointer<vtkTransform> inputImageToReferenceImageTransform = vtkSmartPointer<vtkTransform>::New();
  inputImageToReferenceImageTransform->DeepCopy(referenceImageToInputImageTransform);
  inputImageToReferenceImageTransform->Inverse();

  // Determine output origin and spacing using vtkOrientedImageData function
  vtkSmartPointer<vtkOrientedImageData> utilityImageData = vtkSmartPointer<vtkOrientedImageData>::New();
  utilityImageData->SetGeometryFromImageToWorldMatrix(referenceToWorldMatrix);
  double outputOrigin[3] = {0.0, 0.0, 0.0};
  utilityImageData->GetOrigin(outputOrigin);
  double outputSpacing[3] = {0.0, 0.0, 0.0};
  utilityImageData->GetSpacing(outputSpacing);

  // Calculate output extent in reference frame. Use all bounding box corners
  int outputExtent[6] = {0,-1,0,-1,0,-1};
  vtkOrientedImageDataResample::TransformExtent(effectiveInputExtent, referenceImageToInputImageTransform, outputExtent);

  // Return with failure if effective output extent is empty
  if ( outputExtent[0] == outputExtent[1]
    || outputExtent[2] == outputExtent[3]
    || outputExtent[4] == outputExtent[5] )
  {
    return false;
  }

  // Create clone for input image that has an identity geometry
  //TODO: vtkOrientedImageReslice would be a better solution on the long run
  vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  identityMatrix->Identity();
  vtkSmartPointer<vtkOrientedImageData> identityInputImage = vtkSmartPointer<vtkOrientedImageData>::New();
  identityInputImage->ShallowCopy(inputImage);
  identityInputImage->SetGeometryFromImageToWorldMatrix(identityMatrix);

  // Perform resampling
  vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
#if (VTK_MAJOR_VERSION <= 5)
  resliceFilter->SetInput(identityInputImage);
#else
  resliceFilter->SetInputData(identityInputImage);
#endif
  resliceFilter->SetOutputOrigin(0, 0, 0);
  resliceFilter->SetOutputSpacing(1, 1, 1);
  resliceFilter->SetOutputExtent(outputExtent);

  resliceFilter->SetResliceTransform(inputImageToReferenceImageTransform);

  // Set interpolation mode
  if (linearInterpolation)
  {
    resliceFilter->SetInterpolationModeToLinear();
  }
  else
  {
    resliceFilter->SetInterpolationModeToNearestNeighbor();
  }
  resliceFilter->Update();

  // Set output
  outputImage->DeepCopy(resliceFilter->GetOutput());
  outputImage->SetGeometryFromImageToWorldMatrix(referenceToWorldMatrix);

  return true;
}

//---------------------------------------------------------------------------
bool vtkOrientedImageDataResample::IsEqual( const vtkMatrix4x4& lhs, const vtkMatrix4x4& rhs )
{
  return  AreEqualWithTolerance(lhs.GetElement(0,0), rhs.GetElement(0,0)) &&
          AreEqualWithTolerance(lhs.GetElement(0,1), rhs.GetElement(0,1)) &&
          AreEqualWithTolerance(lhs.GetElement(0,2), rhs.GetElement(0,2)) &&
          AreEqualWithTolerance(lhs.GetElement(0,3), rhs.GetElement(0,3)) &&
          AreEqualWithTolerance(lhs.GetElement(1,0), rhs.GetElement(1,0)) &&
          AreEqualWithTolerance(lhs.GetElement(1,1), rhs.GetElement(1,1)) &&
          AreEqualWithTolerance(lhs.GetElement(1,2), rhs.GetElement(1,2)) &&
          AreEqualWithTolerance(lhs.GetElement(1,3), rhs.GetElement(1,3)) &&
          AreEqualWithTolerance(lhs.GetElement(2,0), rhs.GetElement(2,0)) &&
          AreEqualWithTolerance(lhs.GetElement(2,1), rhs.GetElement(2,1)) &&
          AreEqualWithTolerance(lhs.GetElement(2,2), rhs.GetElement(2,2)) &&
          AreEqualWithTolerance(lhs.GetElement(2,3), rhs.GetElement(2,3)) &&
          AreEqualWithTolerance(lhs.GetElement(3,0), rhs.GetElement(3,0)) &&
          AreEqualWithTolerance(lhs.GetElement(3,1), rhs.GetElement(3,1)) &&
          AreEqualWithTolerance(lhs.GetElement(3,2), rhs.GetElement(3,2)) &&
          AreEqualWithTolerance(lhs.GetElement(3,3), rhs.GetElement(3,3));
}

//----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::DoGeometriesMatch(vtkOrientedImageData* image1, vtkOrientedImageData* image2)
{
  if (!image1 || !image2)
  {
    return false;
  }

  vtkSmartPointer<vtkMatrix4x4> image1ToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image1->GetImageToWorldMatrix(image1ToWorldMatrix);

  vtkSmartPointer<vtkMatrix4x4> image2ToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image2->GetImageToWorldMatrix(image2ToWorldMatrix);

  return vtkOrientedImageDataResample::IsEqual(*image1ToWorldMatrix, *image2ToWorldMatrix);
}

//----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::DoExtentsMatch(vtkOrientedImageData* image1, vtkOrientedImageData* image2)
{
  if (!image1 || !image2)
  {
    return false;
  }

  int image1Extent[6] = {0,-1,0,-1,0,-1};
  image1->GetExtent(image1Extent);
  int image2Extent[6] = {0,-1,0,-1,0,-1};
  image2->GetExtent(image2Extent);
  if ( image1Extent[0] != image2Extent[0] || image1Extent[1] != image2Extent[1] || image1Extent[2] != image2Extent[2]
    || image1Extent[3] != image2Extent[3] || image1Extent[4] != image2Extent[4] || image1Extent[5] != image2Extent[5] )
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::DoGeometriesMatchIgnoreOrigin(vtkOrientedImageData* image1, vtkOrientedImageData* image2)
{
  if (!image1 || !image2)
  {
    return false;
  }

  // Create geometry matrices with no origin so that comparison for only directions and spacing is possible
  vtkSmartPointer<vtkMatrix4x4> image1ToWorldMatrixWithoutOrigin = vtkSmartPointer<vtkMatrix4x4>::New();
  image1->GetImageToWorldMatrix(image1ToWorldMatrixWithoutOrigin);
  image1ToWorldMatrixWithoutOrigin->SetElement(0,3,0.0);
  image1ToWorldMatrixWithoutOrigin->SetElement(1,3,0.0);
  image1ToWorldMatrixWithoutOrigin->SetElement(2,3,0.0);

  vtkSmartPointer<vtkMatrix4x4> image2ToWorldMatrixWithoutOrigin = vtkSmartPointer<vtkMatrix4x4>::New();
  image2->GetImageToWorldMatrix(image2ToWorldMatrixWithoutOrigin);
  image2ToWorldMatrixWithoutOrigin->SetElement(0,3,0.0);
  image2ToWorldMatrixWithoutOrigin->SetElement(1,3,0.0);
  image2ToWorldMatrixWithoutOrigin->SetElement(2,3,0.0);

  return vtkOrientedImageDataResample::IsEqual(*image1ToWorldMatrixWithoutOrigin, *image2ToWorldMatrixWithoutOrigin);
}

//----------------------------------------------------------------------------
void vtkOrientedImageDataResample::TransformExtent(int inputExtent[6], vtkTransform* inputToOutputTransform, int outputExtent[6])
{
  if (!inputToOutputTransform)
  {
    return;
  }

  // Apply transform on all eight corners and determine output extent based on these transformed corners
  double outputIjkExtentCorner[3] = {0.0, 0.0, 0.0};
  double outputExtentDouble[6] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  for (int i=0; i<2; ++i)
  {
    for (int j=0; j<2; ++j)
    {
      for (int k=0; k<2; ++k)
      {
        double inputBoxCorner[3] = {inputExtent[i], inputExtent[2+j], inputExtent[4+k]};
        inputToOutputTransform->TransformPoint(inputBoxCorner, outputIjkExtentCorner);
        if (outputIjkExtentCorner[0] < outputExtentDouble[0])
        {
          outputExtentDouble[0] = outputIjkExtentCorner[0];
        }
        if (outputIjkExtentCorner[0] > outputExtentDouble[1])
        {
          outputExtentDouble[1] = outputIjkExtentCorner[0];
        }
        if (outputIjkExtentCorner[1] < outputExtentDouble[2])
        {
          outputExtentDouble[2] = outputIjkExtentCorner[1];
        }
        if (outputIjkExtentCorner[1] > outputExtentDouble[3])
        {
          outputExtentDouble[3] = outputIjkExtentCorner[1];
        }
        if (outputIjkExtentCorner[2] < outputExtentDouble[4])
        {
          outputExtentDouble[4] = outputIjkExtentCorner[2];
        }
        if (outputIjkExtentCorner[2] > outputExtentDouble[5])
        {
          outputExtentDouble[5] = outputIjkExtentCorner[2];
        }
      }
    }
  }

  outputExtent[0] = (int)floor(outputExtentDouble[0]);
  outputExtent[1] = (int)ceil(outputExtentDouble[1]);
  outputExtent[2] = (int)floor(outputExtentDouble[2]);
  outputExtent[3] = (int)ceil(outputExtentDouble[3]);
  outputExtent[4] = (int)floor(outputExtentDouble[4]);
  outputExtent[5] = (int)ceil(outputExtentDouble[5]);
}

//----------------------------------------------------------------------------
void vtkOrientedImageDataResample::TransformOrientedImageDataBounds(vtkOrientedImageData* image, vtkAbstractTransform* transform, double transformedBounds[6])
{
  if (!image || !transform)
  {
    return;
  }

  // Get input image properties
  vtkSmartPointer<vtkMatrix4x4> imageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image->GetImageToWorldMatrix(imageToWorldMatrix);
  int* imageExtent = image->GetExtent();

  // Append transformed side planes poly data to one model and get bounds
  vtkNew<vtkAppendPolyData> appendPolyData;
  for (int i=0; i<6; i++)
  {
    int normalAxis = i/2; // Axis along which the plane is constant
    double currentPlaneOriginImage[4] = {0.0, 0.0, 0.0, 1.0};
    currentPlaneOriginImage[normalAxis] += imageExtent[i];
    double currentPlaneOriginWorld[4] = {0.0, 0.0, 0.0, 1.0};
    imageToWorldMatrix->MultiplyPoint(currentPlaneOriginImage, currentPlaneOriginWorld);

    double currentPlanePoint1Image[4] = {currentPlaneOriginImage[0], currentPlaneOriginImage[1], currentPlaneOriginImage[2], 1.0};
    int point1Axis = (normalAxis + 1) % 3; // Axis different from normal axis
    currentPlanePoint1Image[point1Axis] = imageExtent[point1Axis * 2 + 1];
    double currentPlanePoint1World[4] = {0.0, 0.0, 0.0, 1.0};
    imageToWorldMatrix->MultiplyPoint(currentPlanePoint1Image, currentPlanePoint1World);

    double currentPlanePoint2Image[4] = {currentPlaneOriginImage[0], currentPlaneOriginImage[1], currentPlaneOriginImage[2], 1.0};
    int point2Axis = 3 - normalAxis - point1Axis; // Axis different from both normal axis and point 1 axis
    currentPlanePoint2Image[point2Axis] = imageExtent[point2Axis * 2 + 1];
    double currentPlanePoint2World[4] = {0.0, 0.0, 0.0, 1.0};
    imageToWorldMatrix->MultiplyPoint(currentPlanePoint2Image, currentPlanePoint2World);

    vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
    planeSource->SetOrigin(currentPlaneOriginWorld);
    planeSource->SetPoint1(currentPlanePoint1World);
    planeSource->SetPoint2(currentPlanePoint2World);
    planeSource->SetResolution(5,5); // Use only three subdivision points along each axis
    planeSource->Update();

#if (VTK_MAJOR_VERSION <= 5)
    appendPolyData->AddInput(planeSource->GetOutput());
#else
    appendPolyData->AddInputData(planeSource->GetOutput());
#endif
  }

  // Transform boundary poly data
  vtkNew<vtkTransformPolyDataFilter> transformFilter;
  transformFilter->SetInputConnection(appendPolyData->GetOutputPort());
  transformFilter->SetTransform(transform);
  transformFilter->Update();

  // Get bounds of transformed boundary poly data
  transformFilter->GetOutput()->ComputeBounds();
  transformFilter->GetOutput()->GetBounds(transformedBounds);
}

//----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::GetTransformBetweenOrientedImages(vtkOrientedImageData* image1, vtkOrientedImageData* image2, vtkTransform* image1ToImage2Transform)
{
  if (!image1 || !image2 || !image1ToImage2Transform)
  {
    return false;
  }

  // Assemble transform
  image1ToImage2Transform->Identity();
  image1ToImage2Transform->PostMultiply();

  vtkSmartPointer<vtkMatrix4x4> image1ToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image1->GetImageToWorldMatrix(image1ToWorldMatrix);
  image1ToImage2Transform->Concatenate(image1ToWorldMatrix);

  vtkSmartPointer<vtkMatrix4x4> image2ToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  image2->GetImageToWorldMatrix(image2ToWorldMatrix);

  vtkSmartPointer<vtkMatrix4x4> worldToReferenceImageMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  worldToReferenceImageMatrix->DeepCopy(image2ToWorldMatrix);
  worldToReferenceImageMatrix->Invert();
  image1ToImage2Transform->Concatenate(worldToReferenceImageMatrix);

  return true;
}

//----------------------------------------------------------------------------
bool vtkOrientedImageDataResample::PadImageToContainImage(vtkOrientedImageData* inputImage, vtkOrientedImageData* containedImage, vtkOrientedImageData* outputImage)
{
  if (!inputImage || !containedImage || !outputImage)
  {
    return false;
  }

  // Get transform between input and contained
  vtkSmartPointer<vtkTransform> inputImageToContainedImageTransform = vtkSmartPointer<vtkTransform>::New();
  vtkOrientedImageDataResample::GetTransformBetweenOrientedImages(inputImage, containedImage, inputImageToContainedImageTransform);

  // Calculate output extent in reference frame for padding if requested. Use all bounding box corners
  int inputExtentInContainedFrame[6] = {0,-1,0,-1,0,-1};
  vtkOrientedImageDataResample::TransformExtent(inputImage->GetExtent(), inputImageToContainedImageTransform, inputExtentInContainedFrame);

  // Return with failure if output extent is empty
  if ( inputExtentInContainedFrame[0] == inputExtentInContainedFrame[1] || inputExtentInContainedFrame[2] == inputExtentInContainedFrame[3] || inputExtentInContainedFrame[4] == inputExtentInContainedFrame[5] )
  {
    return false;
  }

  // Make sure input image data fits into the extent. If padding is disabled, then union extent is the reference extent
  int containedExtent[6] = {0,-1,0,-1,0,-1};
  containedImage->GetExtent(containedExtent);
  int unionExtent[6] = { std::min(inputExtentInContainedFrame[0],containedExtent[0]), std::max(inputExtentInContainedFrame[1],containedExtent[1]),
                         std::min(inputExtentInContainedFrame[2],containedExtent[2]), std::max(inputExtentInContainedFrame[3],containedExtent[3]),
                         std::min(inputExtentInContainedFrame[4],containedExtent[4]), std::max(inputExtentInContainedFrame[5],containedExtent[5]) };

  // Pad image by expansion extent (extents are fitted to the structure, dilate will reach the edge of the image)
  vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
#if (VTK_MAJOR_VERSION <= 5)
  padder->SetInput(inputImage);
#else
  padder->SetInputData(inputImage);
#endif
  padder->SetOutputWholeExtent(unionExtent);
  padder->Update();

  // Set output
  outputImage->DeepCopy(padder->GetOutput());

  vtkSmartPointer<vtkMatrix4x4> inputImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inputImage->GetImageToWorldMatrix(inputImageToWorldMatrix);
  outputImage->SetGeometryFromImageToWorldMatrix(inputImageToWorldMatrix);

  return true;
}

//----------------------------------------------------------------------------
void vtkOrientedImageDataResample::TransformOrientedImage(vtkOrientedImageData* image, vtkAbstractTransform* transform, bool geometryOnly/* = false*/)
{
  if (!image || !transform)
  {
    return;
  }

  // Linear: simply multiply the geometry matrix with the applied matrix, extent stays the same
  vtkLinearTransform* worldToTransformedWorldLinearTransform = vtkLinearTransform::SafeDownCast(transform);
  if (worldToTransformedWorldLinearTransform)
  {
    vtkSmartPointer<vtkMatrix4x4> imageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    image->GetImageToWorldMatrix(imageToWorldMatrix);

    vtkSmartPointer<vtkTransform> imageToTransformedWorldTransform = vtkSmartPointer<vtkTransform>::New();
    imageToTransformedWorldTransform->Concatenate(worldToTransformedWorldLinearTransform);
    imageToTransformedWorldTransform->Concatenate(imageToWorldMatrix);

    image->SetGeometryFromImageToWorldMatrix(imageToTransformedWorldTransform->GetMatrix());
  }
  // Non-linear: calculate new extents and change only the extents when applying deformable transform
  else
  {
    // Get geometry transform and its inverse
    vtkNew<vtkMatrix4x4> imageToWorldMatrix;
    image->GetImageToWorldMatrix(imageToWorldMatrix.GetPointer());

    vtkNew<vtkMatrix4x4> worldToImageMatrix;
    worldToImageMatrix->DeepCopy(imageToWorldMatrix.GetPointer());
    worldToImageMatrix->Invert();

    // Calculate output extent
    double transformedBoundsWorld[6] = {0.0, -1.0, 0.0, -1.0, 0.0, -1.0};
    vtkOrientedImageDataResample::TransformOrientedImageDataBounds(image, transform, transformedBoundsWorld);
    double transformedBoundsWorldCorner1[4] = {transformedBoundsWorld[0], transformedBoundsWorld[2], transformedBoundsWorld[4], 1.0};
    double transformedBoundsWorldCorner2[4] = {transformedBoundsWorld[1], transformedBoundsWorld[3], transformedBoundsWorld[5], 1.0};
    double transformedBoundsImageCorner1[4] = {0.0, 0.0, 0.0, 1.0};
    double transformedBoundsImageCorner2[4] = {0.0, 0.0, 0.0, 1.0};
    worldToImageMatrix->MultiplyPoint(transformedBoundsWorldCorner1, transformedBoundsImageCorner1);
    worldToImageMatrix->MultiplyPoint(transformedBoundsWorldCorner2, transformedBoundsImageCorner2);
    int outputExtent[6] = { // Bounds and extent might be in different order if transform also mirrors (it usually does due to LPS->RAS mapping)
      (int)floor( std::min(transformedBoundsImageCorner1[0], transformedBoundsImageCorner2[0]) ),
      (int)ceil( std::max(transformedBoundsImageCorner1[0], transformedBoundsImageCorner2[0]) ),
      (int)floor( std::min(transformedBoundsImageCorner1[1], transformedBoundsImageCorner2[1]) ),
      (int)ceil( std::max(transformedBoundsImageCorner1[1], transformedBoundsImageCorner2[1]) ),
      (int)floor( std::min(transformedBoundsImageCorner1[2], transformedBoundsImageCorner2[2]) ),
      (int)ceil( std::max(transformedBoundsImageCorner1[2], transformedBoundsImageCorner2[2]) )
    };

    // It becomes transformedWorldToWorld transform
    transform->Inverse();

    // Create reslice transform
    vtkNew<vtkGeneralTransform> resliceTransform;
    resliceTransform->Identity();
    resliceTransform->PostMultiply();
    resliceTransform->Concatenate(imageToWorldMatrix.GetPointer());
    resliceTransform->Concatenate(transform);
    resliceTransform->Concatenate(worldToImageMatrix.GetPointer());

    // Perform resampling
    vtkNew<vtkImageReslice> reslice;
    reslice->GenerateStencilOutputOn();
    reslice->SetResliceTransform(resliceTransform.GetPointer());
#if (VTK_MAJOR_VERSION <= 5)
    reslice->SetInput(image);
#else
    reslice->SetInputData(image);
#endif
    reslice->SetInterpolationModeToLinear();
    reslice->SetBackgroundColor(0, 0, 0, 0);
    reslice->AutoCropOutputOff();
    reslice->SetOptimization(1);
    reslice->SetOutputOrigin(image->GetOrigin());
    reslice->SetOutputSpacing(image->GetSpacing());
    reslice->SetOutputDimensionality(3);
    reslice->SetOutputExtent(image->GetExtent());//outputExtent);
    reslice->Update();
#if (VTK_MAJOR_VERSION <= 5)
    if (reslice->GetOutput(1))
    {
      reslice->GetOutput(1)->SetUpdateExtentToWholeExtent();
    }
#endif
    image->DeepCopy(reslice->GetOutput());
    image->SetGeometryFromImageToWorldMatrix(imageToWorldMatrix.GetPointer());
  }
}
