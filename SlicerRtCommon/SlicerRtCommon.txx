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

//----------------------------------------------------------------------------
template<typename T> bool SlicerRtCommon::ConvertVolumeNodeToItkImage(vtkMRMLScalarVolumeNode* inVolumeNode, typename itk::Image<T, 3>::Pointer outItkImage, bool applyRasToLpsConversion/*=false*/)
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
  imageExport->SetInput(inVolume);
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

  vtkSmartPointer<vtkMatrix4x4> ras2LpsTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  ras2LpsTransformMatrix->SetElement(0,0,-1.0);
  ras2LpsTransformMatrix->SetElement(1,1,-1.0);
  ras2LpsTransformMatrix->SetElement(2,2, 1.0);
  ras2LpsTransformMatrix->SetElement(3,3, 1.0);
  
  vtkSmartPointer<vtkTransform> inVolumeToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldTransform->Identity();
  inVolumeToWorldTransform->PostMultiply();
  inVolumeToWorldTransform->Concatenate(inVolumeToRasTransformMatrix);
  inVolumeToWorldTransform->Concatenate(rasToWorldTransformMatrix);
  if (applyRasToLpsConversion)
  {
    inVolumeToWorldTransform->Concatenate(ras2LpsTransformMatrix);
  }

  // Set ITK image properties
  double outputSpacing[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetScale(outputSpacing);
  if (applyRasToLpsConversion)
  {
    outputSpacing[0] = outputSpacing[0] < 0 ? -outputSpacing[0] : outputSpacing[0];
    outputSpacing[1] = outputSpacing[1] < 0 ? -outputSpacing[1] : outputSpacing[1];
    outputSpacing[2] = outputSpacing[2] < 0 ? -outputSpacing[2] : outputSpacing[2];
  }
  outItkImage->SetSpacing(outputSpacing);

  double outputOrigin[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetPosition(outputOrigin);
  outItkImage->SetOrigin(outputOrigin);

  double outputOrienationAngles[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetOrientation(outputOrienationAngles);
  vtkSmartPointer<vtkTransform> inVolumeToWorldOrientationTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldOrientationTransform->Identity();
  inVolumeToWorldOrientationTransform->RotateX(outputOrienationAngles[0]);
  inVolumeToWorldOrientationTransform->RotateY(outputOrienationAngles[1]);
  inVolumeToWorldOrientationTransform->RotateZ(outputOrienationAngles[2]);
  vtkSmartPointer<vtkMatrix4x4> inVolumeToWorldOrientationTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeToWorldOrientationTransform->GetMatrix(inVolumeToWorldOrientationTransformMatrix);
  itk::Matrix<double,3,3> outputDirectionMatrix;
  for(int i=0; i<3; i++)
  {
    for(int j=0; j<3; j++)
    {
      outputDirectionMatrix[i][j] = inVolumeToWorldOrientationTransformMatrix->GetElement(i,j);
    }
  }
  outItkImage->SetDirection(outputDirectionMatrix);

  int inputExtent[6]={0,0,0,0,0,0}; 
  inVolume->GetExtent(inputExtent); 
  typename itk::Image<T, 3>::SizeType inputSize;
  inputSize[0] = inputExtent[1] - inputExtent[0] + 1;
  inputSize[1] = inputExtent[3] - inputExtent[2] + 1;
  inputSize[2] = inputExtent[5] - inputExtent[4] + 1;

  typename itk::Image<T, 3>::IndexType start;
  start[0]=start[1]=start[2]=0.0;

  typename itk::Image<T, 3>::RegionType region;
  region.SetSize(inputSize);
  region.SetIndex(start);
  outItkImage->SetRegions(region);

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
  outVtkImageData->SetScalarType(vtkType);
  outVtkImageData->SetNumberOfScalarComponents(1);
  outVtkImageData->AllocateScalars();

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