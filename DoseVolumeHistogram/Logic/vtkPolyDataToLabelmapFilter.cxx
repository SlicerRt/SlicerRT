#include "vtkPolyDataToLabelmapFilter.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataPointSampler.h>
#include <vtkImageImport.h>

// ITK includes
#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryThresholdImageFunction.h>
#include <itkFloodFilledImageFunctionConditionalIterator.h>
#include <itkVTKImageExport.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyDataToLabelmapFilter);

//----------------------------------------------------------------------------
vtkPolyDataToLabelmapFilter::vtkPolyDataToLabelmapFilter()
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  this->SetInputPolyData(inputPolyData);

  this->SetOutputLabelmap(NULL);

  this->SetSampleDistance(1.1);
  this->SetLabelValue(2);
  this->OutputLabelmapSize[0] = this->OutputLabelmapSize[1] = this->OutputLabelmapSize[2] = 256;
}

//----------------------------------------------------------------------------
vtkPolyDataToLabelmapFilter::~vtkPolyDataToLabelmapFilter()
{
  this->SetInputPolyData(NULL);
  this->SetOutputLabelmap(NULL);
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::SetInput(vtkDataSet *input)
{
  this->InputPolyData->DeepCopy(input);
}

//----------------------------------------------------------------------------
vtkImageData* vtkPolyDataToLabelmapFilter::GetOutput()
{
  return this->OutputLabelmap;
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::Update()
{
  // Convert from RAS (Slicer) to LPS (ITK, DICOM)
  vtkPoints* allPoints = this->InputPolyData->GetPoints();
  for( int k = 0; k < allPoints->GetNumberOfPoints(); k++ )
  {
    double* point = this->InputPolyData->GetPoint( k );
    point[0] = -point[0];
    point[1] = -point[1];
    allPoints->SetPoint( k, point[0], point[1], point[2] );
  }

  // Create output label map
  itk::Size<3> size;
  size.SetSize(this->OutputLabelmapSize);
  LabelImageType::Pointer labelmapItk = LabelImageType::New();
  labelmapItk->SetRegions( size );
  labelmapItk->Allocate();
  labelmapItk->FillBuffer( 0 );

  int result = EXIT_FAILURE;
  result = ConvertModelToLabelUsingFloodFill(this->InputPolyData, labelmapItk, this->SampleDistance, this->LabelValue);       

  if (result != EXIT_SUCCESS)
  {
    // Conversion failed
    std::cerr << "Conversion from poly data to labelmap failed!" << std::endl;
    return;
  }

  // Convert output labelmap to vtk image data
  itk::VTKImageExport<LabelImageType>::Pointer itkExporter = itk::VTKImageExport<LabelImageType>::New();
  itkExporter->SetInput( labelmapItk );
  
  vtkSmartPointer<vtkImageImport> importer = vtkSmartPointer<vtkImageImport>::New();

  importer->SetUpdateInformationCallback(itkExporter->GetUpdateInformationCallback());
  importer->SetPipelineModifiedCallback(itkExporter->GetPipelineModifiedCallback());
  importer->SetWholeExtentCallback(itkExporter->GetWholeExtentCallback());
  importer->SetSpacingCallback(itkExporter->GetSpacingCallback());
  importer->SetOriginCallback(itkExporter->GetOriginCallback());
  importer->SetScalarTypeCallback(itkExporter->GetScalarTypeCallback());
  importer->SetNumberOfComponentsCallback(itkExporter->GetNumberOfComponentsCallback());
  importer->SetPropagateUpdateExtentCallback(itkExporter->GetPropagateUpdateExtentCallback());
  importer->SetUpdateDataCallback(itkExporter->GetUpdateDataCallback());
  importer->SetDataExtentCallback(itkExporter->GetDataExtentCallback());
  importer->SetBufferPointerCallback(itkExporter->GetBufferPointerCallback());
  importer->SetCallbackUserData(itkExporter->GetCallbackUserData());

  this->OutputLabelmap->DeepCopy( importer->GetOutput() );
}

//----------------------------------------------------------------------------
LabelImageType::Pointer vtkPolyDataToLabelmapFilter::BinaryErodeFilter3D( LabelImageType::Pointer & img, unsigned int ballsize )
{
  typedef itk::BinaryBallStructuringElement<unsigned char, 3>                     KernalType;
  typedef itk::BinaryErodeImageFilter<LabelImageType, LabelImageType, KernalType> ErodeFilterType;
  ErodeFilterType::Pointer erodeFilter = ErodeFilterType::New();
  erodeFilter->SetInput( img );

  KernalType           ball;
  KernalType::SizeType ballSize;
  for( int k = 0; k < 3; k++ )
  {
    ballSize[k] = ballsize;
  }
  ball.SetRadius(ballSize);
  ball.CreateStructuringElement();
  erodeFilter->SetKernel( ball );
  erodeFilter->Update();
  return erodeFilter->GetOutput();

}

//----------------------------------------------------------------------------
LabelImageType::Pointer vtkPolyDataToLabelmapFilter::BinaryDilateFilter3D( LabelImageType::Pointer & img, unsigned int ballsize )
{
  typedef itk::BinaryBallStructuringElement<unsigned char, 3>                      KernalType;
  typedef itk::BinaryDilateImageFilter<LabelImageType, LabelImageType, KernalType> DilateFilterType;
  DilateFilterType::Pointer dilateFilter = DilateFilterType::New();
  dilateFilter->SetInput( img );
  KernalType           ball;
  KernalType::SizeType ballSize;
  for( int k = 0; k < 3; k++ )
  {
    ballSize[k] = ballsize;
  }
  ball.SetRadius(ballSize);
  ball.CreateStructuringElement();
  dilateFilter->SetKernel( ball );
  dilateFilter->Update();
  return dilateFilter->GetOutput();
}

//----------------------------------------------------------------------------
LabelImageType::Pointer vtkPolyDataToLabelmapFilter::BinaryClosingFilter3D( LabelImageType::Pointer & img, unsigned int ballsize )
{
  LabelImageType::Pointer imgDilate = BinaryDilateFilter3D( img, ballsize );

  return BinaryErodeFilter3D( imgDilate, ballsize );
}

//----------------------------------------------------------------------------
// Return value:
//  EXIT_SUCCESS if the conversion is successful
//  EXIT_FAILURE if there was an error
int vtkPolyDataToLabelmapFilter::ConvertModelToLabelUsingFloodFill(vtkPolyData* inputPolyData, LabelImageType::Pointer outputLabel, double sampleDistance, unsigned char labelValue)
{
  vtkSmartPointer<vtkPolyDataPointSampler> sampler = vtkSmartPointer<vtkPolyDataPointSampler>::New();

  sampler->SetInput( inputPolyData );
  sampler->SetDistance( sampleDistance );
  sampler->GenerateEdgePointsOn();
  sampler->GenerateInteriorPointsOn();
  sampler->GenerateVertexPointsOn();
  sampler->Update();

  std::cout << inputPolyData->GetNumberOfPoints() << std::endl;
  std::cout << sampler->GetOutput()->GetNumberOfPoints() << std::endl;
  for( int k = 0; k < sampler->GetOutput()->GetNumberOfPoints(); k++ )
  {
    double *pt = sampler->GetOutput()->GetPoint( k );
    LabelImageType::PointType pitk;
    pitk[0] = pt[0];
    pitk[1] = pt[1];
    pitk[2] = pt[2];
    LabelImageType::IndexType idx;
    outputLabel->TransformPhysicalPointToIndex( pitk, idx );

    if( outputLabel->GetLargestPossibleRegion().IsInside(idx) )
    {
      outputLabel->SetPixel( idx, labelValue );
    }
  }

  // do morphological closing
  LabelImageType::Pointer closedLabel = BinaryClosingFilter3D( outputLabel, 2);
  itk::ImageRegionIteratorWithIndex<LabelImageType> itLabel(closedLabel, closedLabel->GetLargestPossibleRegion() );

  // do flood fill using binary threshold image function
  typedef itk::BinaryThresholdImageFunction<LabelImageType> ImageFunctionType;
  ImageFunctionType::Pointer func = ImageFunctionType::New();
  func->SetInputImage( closedLabel );
  func->ThresholdBelow(1);

  LabelImageType::IndexType idx;
  LabelImageType::PointType COG;

  // set the centre of gravity
  // double *bounds = polyData->GetBounds();
  COG.Fill(0.0);
  for( vtkIdType k = 0; k < inputPolyData->GetNumberOfPoints(); k++ )
  {
    double *pt = inputPolyData->GetPoint( k );
    for( int m = 0; m < 3; m++ )
    {
      COG[m] += pt[m];
    }
  }
  for( int m = 0; m < 3; m++ )
  {
    COG[m] /= static_cast<float>( inputPolyData->GetNumberOfPoints() );
  }

  outputLabel->TransformPhysicalPointToIndex( COG, idx );

  itk::FloodFilledImageFunctionConditionalIterator<LabelImageType, ImageFunctionType> floodFill( closedLabel, func, idx );
  for( floodFill.GoToBegin(); !floodFill.IsAtEnd(); ++floodFill )
  {
    LabelImageType::IndexType i = floodFill.GetIndex();
    closedLabel->SetPixel( i, 255 );
  }
  LabelImageType::Pointer finalLabel = BinaryClosingFilter3D( closedLabel, 2);
  for( itLabel.GoToBegin(); !itLabel.IsAtEnd(); ++itLabel )
  {
    LabelImageType::IndexType i = itLabel.GetIndex();
    outputLabel->SetPixel( i, finalLabel->GetPixel(i) );
  }

  return EXIT_SUCCESS;
}
