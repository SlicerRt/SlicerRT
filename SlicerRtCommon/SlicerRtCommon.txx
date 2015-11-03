/*==============================================================================

  Program: 3D Slicer

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

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageExport.h>
#include <vtkImageThreshold.h>
#include <vtkTransform.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// Segmentations includes
#include "vtkOrientedImageData.h"

//---------------------------------------------------------------------------
namespace
{
  template<typename T> bool AreEqualWithTolerance(const T& lhs, const T& rhs)
  {
    T val = lhs - rhs;
    if( val < 0 )
    {
      val = -val;
    }
    return val < EPSILON;
  }
}

//---------------------------------------------------------------------------
template<typename T> bool SlicerRtCommon::IsEqual( const vtkVector3<T>& lhs, const vtkVector3<T>& rhs )
{
  return AreEqualWithTolerance<T>(lhs.GetX(), rhs.GetX()) &&
    AreEqualWithTolerance<T>(lhs.GetY(), rhs.GetY()) &&
    AreEqualWithTolerance<T>(lhs.GetZ(), rhs.GetZ());
}

//----------------------------------------------------------------------------
template<typename T> bool SlicerRtCommon::ConvertVolumeNodeToItkImage(vtkMRMLScalarVolumeNode* inVolumeNode, typename itk::Image<T, 3>::Pointer outItkImage, bool applyRasToWorldConversion, bool applyRasToLpsConversion/*=true*/)
{
  if ( inVolumeNode == NULL )
  {
    std::cerr << "SlicerRtCommon::ConvertVolumeNodeToItkImage: Failed to convert volume node to itk image - input MRML volume node is NULL!" << std::endl;
    return false; 
  }
  vtkImageData* inVolume = inVolumeNode->GetImageData();
  if ( inVolume == NULL )
  {
    vtkErrorWithObjectMacro(inVolumeNode, "ConvertVolumeNodeToItkImage: Failed to convert volume node to itk image - image in input MRML volume node is NULL!");
    return false; 
  }
  if ( outItkImage.IsNull() )
  {
    vtkErrorWithObjectMacro(inVolumeNode, "ConvertVolumeNodeToItkImage: Failed to convert volume node to itk image - output image is NULL!");
    return false; 
  }
  if (sizeof(T) != inVolume->GetScalarSize())
  {
    vtkErrorWithObjectMacro(inVolumeNode, "ConvertVolumeNodeToItkImage: Requested type has a different scalar size than input type - output image is NULL!");
    return false; 
  }

  // Convert vtkImageData to itkImage 
  vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New(); 
#if (VTK_MAJOR_VERSION <= 5)
  imageExport->SetInput(inVolume);
#else
  imageExport->SetInputData(inVolume);
#endif
  imageExport->Update(); 

  // Determine input volume to world transform
  vtkSmartPointer<vtkMatrix4x4> rasToWorldTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMRMLTransformNode* inTransformNode=inVolumeNode->GetParentTransformNode();
  if (inTransformNode!=NULL)
  {
    if (inTransformNode->IsTransformToWorldLinear() == 0)
    {
      vtkErrorWithObjectMacro(inVolumeNode, "ConvertVolumeNodeToItkImage: There is a non-linear transform assigned to an input dose volume. Only linear transforms are supported!");
      return false;
    }
    inTransformNode->GetMatrixTransformToWorld(rasToWorldTransformMatrix);
  }

  vtkSmartPointer<vtkMatrix4x4> inVolumeToRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeNode->GetIJKToRASMatrix(inVolumeToRasTransformMatrix);

  // RAS (Slicer) to LPS (ITK) transform matrix
  vtkSmartPointer<vtkMatrix4x4> ras2LpsTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  ras2LpsTransformMatrix->SetElement(0,0,-1.0);
  ras2LpsTransformMatrix->SetElement(1,1,-1.0);
  ras2LpsTransformMatrix->SetElement(2,2, 1.0);
  ras2LpsTransformMatrix->SetElement(3,3, 1.0);
  
  vtkSmartPointer<vtkTransform> inVolumeToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldTransform->Identity();
  inVolumeToWorldTransform->PostMultiply();
  inVolumeToWorldTransform->Concatenate(inVolumeToRasTransformMatrix);
  if (applyRasToWorldConversion)
  {
    inVolumeToWorldTransform->Concatenate(rasToWorldTransformMatrix);
  }
  if (applyRasToLpsConversion)
  {
    inVolumeToWorldTransform->Concatenate(ras2LpsTransformMatrix);
  }

  // Set ITK image properties: spacing
  double outputSpacing[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetScale(outputSpacing);
  if (applyRasToLpsConversion)
  {
    outputSpacing[0] = outputSpacing[0] < 0 ? -outputSpacing[0] : outputSpacing[0];
    outputSpacing[1] = outputSpacing[1] < 0 ? -outputSpacing[1] : outputSpacing[1];
    outputSpacing[2] = outputSpacing[2] < 0 ? -outputSpacing[2] : outputSpacing[2];
  }
  outItkImage->SetSpacing(outputSpacing);

  // Set ITK image properties: origin
  double outputOrigin[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetPosition(outputOrigin);
  outItkImage->SetOrigin(outputOrigin);

  // Set ITK image properties: orientation
  vtkSmartPointer<vtkMatrix4x4> inVolumeToWorldTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeToWorldTransform->GetMatrix(inVolumeToWorldTransformMatrix);

  // normalize direction vectors
  itk::Matrix<double,3,3> outputDirectionMatrix;
  unsigned int col = 0;
  for (col=0; col<3; col++)
  {
    double len = 0;
    unsigned int row = 0;
    for (row=0; row<3; row++)
    {
      len += inVolumeToWorldTransformMatrix->GetElement(row, col) * inVolumeToWorldTransformMatrix->GetElement(row, col);
    }
    if (len == 0.0)
    {
      len = 1.0;
    }
    len = sqrt(len);
    for (row=0; row<3; row++)
    {
      outputDirectionMatrix[row][col] = inVolumeToWorldTransformMatrix->GetElement(row, col)/len;
    }
  }

  outItkImage->SetDirection(outputDirectionMatrix);

  // Set ITK image properties: regions
  int inputExtent[6]={0,0,0,0,0,0}; 
  inVolume->GetExtent(inputExtent); 
  typename itk::Image<T, 3>::SizeType inputSize;
  inputSize[0] = inputExtent[1] - inputExtent[0] + 1;
  inputSize[1] = inputExtent[3] - inputExtent[2] + 1;
  inputSize[2] = inputExtent[5] - inputExtent[4] + 1;

  typename itk::Image<T, 3>::IndexType start;
  start[0]=start[1]=start[2]=0;

  typename itk::Image<T, 3>::RegionType region;
  region.SetSize(inputSize);
  region.SetIndex(start);
  outItkImage->SetRegions(region);

  // Create and export ITK image
  try
  {
    outItkImage->Allocate();
  }
  catch(itk::ExceptionObject & err)
  {
    vtkErrorWithObjectMacro(inVolumeNode, "ConvertVolumeNodeToItkImage: Failed to allocate memory for the image conversion: " << err.GetDescription())
    return false;
  }

  imageExport->Export( outItkImage->GetBufferPointer() );

  return true;
}

//----------------------------------------------------------------------------
template<typename T> bool SlicerRtCommon::ConvertVtkOrientedImageDataToItkImage(vtkOrientedImageData* inImageData, typename itk::Image<T, 3>::Pointer outItkImage, bool applyRasToLpsConversion/*=true*/)
{
  if ( inImageData == NULL )
  {
    std::cerr << "SlicerRtCommon::ConvertVtkOrientedImageDataToItkImage: Failed to convert oriented image data to itk image - input MRML image data is NULL!" << std::endl;
    return false; 
  }
  if ( outItkImage.IsNull() )
  {
    vtkErrorWithObjectMacro(inImageData, "ConvertVtkOrientedImageDataToItkImage: Failed to convert oriented image data to itk image - output image is NULL!");
    return false; 
  }
  if (sizeof(T) != inImageData->GetScalarSize())
  {
    vtkErrorWithObjectMacro(inImageData, "ConvertVtkOrientedImageDataToItkImage: Requested type has a different scalar size than input type - output image is NULL!");
    return false; 
  }

  // Convert vtkOrientedImageData to itkImage 
  vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New(); 
#if (VTK_MAJOR_VERSION <= 5)
  imageExport->SetInput(inImageData);
#else
  imageExport->SetInputData(inImageData);
#endif
  imageExport->Update(); 

  // Determine input image to world transform
  vtkSmartPointer<vtkMatrix4x4> inImageToWorldRasMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  inImageData->GetImageToWorldMatrix(inImageToWorldRasMatrix);

  // RAS (Slicer) to LPS (ITK) transform matrix
  vtkSmartPointer<vtkMatrix4x4> ras2LpsTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  ras2LpsTransformMatrix->SetElement(0,0,-1.0);
  ras2LpsTransformMatrix->SetElement(1,1,-1.0);
  ras2LpsTransformMatrix->SetElement(2,2, 1.0);
  ras2LpsTransformMatrix->SetElement(3,3, 1.0);
  
  vtkSmartPointer<vtkTransform> inImageToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  inImageToWorldTransform->Identity();
  inImageToWorldTransform->PostMultiply();
  inImageToWorldTransform->Concatenate(inImageToWorldRasMatrix);
  if (applyRasToLpsConversion)
  {
    inImageToWorldTransform->Concatenate(ras2LpsTransformMatrix);
  }

  // Set ITK image properties: spacing
  double outputSpacing[3] = {0.0, 0.0, 0.0};
  inImageToWorldTransform->GetScale(outputSpacing);
  if (applyRasToLpsConversion)
  {
    outputSpacing[0] = outputSpacing[0] < 0 ? -outputSpacing[0] : outputSpacing[0];
    outputSpacing[1] = outputSpacing[1] < 0 ? -outputSpacing[1] : outputSpacing[1];
    outputSpacing[2] = outputSpacing[2] < 0 ? -outputSpacing[2] : outputSpacing[2];
  }
  outItkImage->SetSpacing(outputSpacing);

  // Set ITK image properties: origin
  double outputOrigin[3] = {0.0, 0.0, 0.0};
  inImageToWorldTransform->GetPosition(outputOrigin);
  outItkImage->SetOrigin(outputOrigin);

  // Set ITK image properties: orientation
  vtkSmartPointer<vtkMatrix4x4> inImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inImageToWorldTransform->GetMatrix(inImageToWorldMatrix);

  // normalize direction vectors
  itk::Matrix<double,3,3> outputDirectionMatrix;
  unsigned int col = 0;
  for (col=0; col<3; col++)
  {
    double len = 0;
    unsigned int row = 0;
    for (row=0; row<3; row++)
    {
      len += inImageToWorldMatrix->GetElement(row, col) * inImageToWorldMatrix->GetElement(row, col);
    }
    if (len == 0.0)
    {
      len = 1.0;
    }
    len = sqrt(len);
    for (row=0; row<3; row++)
    {
      outputDirectionMatrix[row][col] = inImageToWorldMatrix->GetElement(row, col)/len;
    }
  }

  outItkImage->SetDirection(outputDirectionMatrix);

  // Set ITK image properties: regions
  int inputExtent[6]={0,0,0,0,0,0}; 
  inImageData->GetExtent(inputExtent); 
  typename itk::Image<T, 3>::SizeType inputSize;
  inputSize[0] = inputExtent[1] - inputExtent[0] + 1;
  inputSize[1] = inputExtent[3] - inputExtent[2] + 1;
  inputSize[2] = inputExtent[5] - inputExtent[4] + 1;

  typename itk::Image<T, 3>::IndexType start;
  start[0]=inputExtent[0];
  start[1]=inputExtent[2];
  start[2]=inputExtent[4];

  typename itk::Image<T, 3>::RegionType region;
  region.SetSize(inputSize);
  region.SetIndex(start);
  outItkImage->SetRegions(region);

  // Create and export ITK image
  try
  {
    outItkImage->Allocate();
  }
  catch(itk::ExceptionObject & err)
  {
    vtkErrorWithObjectMacro(inImageData, "ConvertVtkOrientedImageDataToItkImage: Failed to allocate memory for the image conversion: " << err.GetDescription())
    return false;
  }

  imageExport->Export( outItkImage->GetBufferPointer() );

  return true;
}

//----------------------------------------------------------------------------
template<typename T> bool SlicerRtCommon::ConvertItkImageToVtkImageData(typename itk::Image<T, 3>::Pointer inItkImage, vtkImageData* outVtkImageData, int vtkType)
{
  if ( outVtkImageData == NULL )
  {
    std::cerr << "SlicerRtCommon::ConvertItkImageToVtkImageData: Output VTK image data is NULL!" << std::endl;
    return false; 
  }

  if ( inItkImage.IsNull() )
  {
    vtkErrorWithObjectMacro(outVtkImageData, "ConvertItkImageToVtkImageData: Input ITK image is invalid!");
    return false; 
  }

  typename itk::Image<T, 3>::RegionType region = inItkImage->GetBufferedRegion();
  typename itk::Image<T, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, (int) imageSize[0]-1, 0, (int) imageSize[1]-1, 0, (int) imageSize[2]-1};
  outVtkImageData->SetExtent(extent);
#if (VTK_MAJOR_VERSION <= 5)
  outVtkImageData->SetScalarType(vtkType);
  outVtkImageData->SetNumberOfScalarComponents(1);
  outVtkImageData->AllocateScalars();
#else
  outVtkImageData->AllocateScalars(vtkType, 1);
#endif

  T* outVtkImageDataPtr = (T*)outVtkImageData->GetScalarPointer();
  typename itk::ImageRegionIteratorWithIndex< itk::Image<T, 3> > itInItkImage(
    inItkImage, inItkImage->GetLargestPossibleRegion() );
  for ( itInItkImage.GoToBegin(); !itInItkImage.IsAtEnd(); ++itInItkImage )
  {
    typename itk::Image<T, 3>::IndexType i = itInItkImage.GetIndex();
    (*outVtkImageDataPtr) = inItkImage->GetPixel(i);
    outVtkImageDataPtr++;
  }

  return true;
}

//----------------------------------------------------------------------------
template<typename T> bool SlicerRtCommon::ConvertItkImageToVolumeNode(typename itk::Image<T, 3>::Pointer inItkImage, vtkMRMLScalarVolumeNode* outVolumeNode, int vtkType, bool applyLpsToRasConversion/*=true*/)
{
  if (outVolumeNode == NULL)
  {
    std::cerr << "SlicerRtCommon::ConvertItkImageToVolumeNode: Failed to convert itk image to volume node - output MRML volume node is NULL!" << std::endl;
    return false; 
  }
  if (inItkImage.IsNull())
  {
    vtkErrorWithObjectMacro(outVolumeNode, "ConvertItkImageToVolumeNode: Failed to convert itk image to volume node - input image is NULL!");
    return false; 
  }

  // Create image data if does not exist
  vtkImageData* outImageData = outVolumeNode->GetImageData();
  if (outImageData == NULL)
  {
    vtkSmartPointer<vtkImageData> newImageData = vtkSmartPointer<vtkImageData>::New();
	outVolumeNode->SetAndObserveImageData(newImageData);
	outImageData = newImageData.GetPointer();
  }

  // Get input image properties
  typename itk::Image<T, 3>::RegionType itkRegion = inItkImage->GetLargestPossibleRegion();
  typename itk::Image<T, 3>::PointType itkOrigin = inItkImage->GetOrigin();
  typename itk::Image<T, 3>::SpacingType itkSpacing = inItkImage->GetSpacing();
  typename itk::Image<T, 3>::DirectionType itkDirections = inItkImage->GetDirection();
  // Make image properties accessible for VTK
  double origin[3] = {itkOrigin[0], itkOrigin[1], itkOrigin[2]};
  double spacing[3] = {itkSpacing[0], itkSpacing[1], itkSpacing[2]};
  double directions[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  for (unsigned int col=0; col<3; col++)
  {
    for (unsigned int row=0; row<3; row++)
    {
      directions[row][col] = itkDirections[row][col];
    }
  }
  
  // Convert ITK image to the VTK image data member of the output volume node
  if (!SlicerRtCommon::ConvertItkImageToVtkImageData<T>(inItkImage, outImageData, vtkType))
  {
    vtkErrorWithObjectMacro(outVolumeNode, "ConvertItkImageToVolumeNode: Failed to convert ITK image to VTK image data");
    return false; 
  }
  
  // Apply ITK geometry to volume node
  outVolumeNode->SetOrigin(origin);
  outVolumeNode->SetSpacing(spacing);
  outVolumeNode->SetIJKToRASDirections(directions);

  // Apply LPS to RAS conversion if requested
  if (applyLpsToRasConversion)
  {
	//  LPS (ITK)to RAS (Slicer) transform matrix
	vtkSmartPointer<vtkMatrix4x4> lps2RasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
	lps2RasTransformMatrix->SetElement(0,0,-1.0);
	lps2RasTransformMatrix->SetElement(1,1,-1.0);
	lps2RasTransformMatrix->SetElement(2,2, 1.0);
	lps2RasTransformMatrix->SetElement(3,3, 1.0);

	vtkSmartPointer<vtkMatrix4x4> outVolumeImageToLpsWorldTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
	outVolumeNode->GetIJKToRASMatrix(outVolumeImageToLpsWorldTransformMatrix);

	vtkSmartPointer<vtkTransform> imageToWorldTransform = vtkSmartPointer<vtkTransform>::New();
	imageToWorldTransform->Identity();
	imageToWorldTransform->PostMultiply();
	imageToWorldTransform->Concatenate(outVolumeImageToLpsWorldTransformMatrix);
	imageToWorldTransform->Concatenate(lps2RasTransformMatrix);

	outVolumeNode->SetIJKToRASMatrix(imageToWorldTransform->GetMatrix());
  }

  return true;
}
