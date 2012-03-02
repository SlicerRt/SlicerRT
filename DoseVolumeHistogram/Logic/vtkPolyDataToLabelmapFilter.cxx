#include "vtkPolyDataToLabelmapFilter.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkTriangleFilter.h>
#include <vtkStripper.h>
#include <vtkPolyDataNormals.h>
#include <vtkImageCast.h>
#include <vtkDataSetWriter.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyDataToLabelmapFilter);

//----------------------------------------------------------------------------
vtkPolyDataToLabelmapFilter::vtkPolyDataToLabelmapFilter()
{
  this->InputPolyData = NULL;
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  this->SetInputPolyData(inputPolyData);

  this->OutputLabelmap = NULL;
  vtkSmartPointer<vtkImageData> outputLabelmap = vtkSmartPointer<vtkImageData>::New();
  this->SetOutputLabelmap(outputLabelmap);

  this->ReferenceImageData = NULL;
  vtkSmartPointer<vtkImageData> referenceImageData = vtkSmartPointer<vtkImageData>::New();
  this->SetReferenceImageData(referenceImageData);

  this->SetLabelValue(2);
}

//----------------------------------------------------------------------------
vtkPolyDataToLabelmapFilter::~vtkPolyDataToLabelmapFilter()
{
  this->SetInputPolyData(NULL);
  this->SetOutputLabelmap(NULL);
  this->SetReferenceImageData(NULL);
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::SetReferenceImage(vtkImageData* reference)
{
  this->ReferenceImageData->ShallowCopy(reference);
}

//----------------------------------------------------------------------------
vtkImageData* vtkPolyDataToLabelmapFilter::GetOutput()
{
  return this->OutputLabelmap;
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::Update()
{
  vtkNew<vtkPolyDataNormals> normalFilter;
  normalFilter->SetInput(this->InputPolyData);
  normalFilter->ConsistencyOn();

  // Make sure that we have a clean triangle polydata
  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputConnection(normalFilter->GetOutputPort());

  // Convert to triangle strip
  vtkSmartPointer<vtkStripper> stripper=vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(triangle->GetOutputPort());

  // Blank reference image
  vtkSmartPointer<vtkImageData> refImg=vtkSmartPointer<vtkImageData>::New();
  refImg->SetExtent(this->ReferenceImageData->GetExtent());
  refImg->SetSpacing(this->ReferenceImageData->GetSpacing());
  refImg->SetOrigin(this->ReferenceImageData->GetOrigin());
  refImg->SetScalarType(VTK_UNSIGNED_CHAR);
  refImg->SetNumberOfScalarComponents(1);
  refImg->AllocateScalars();
  void *refImgPixelsPtr = refImg->GetScalarPointerForExtent(this->ReferenceImageData->GetExtent());
  if (refImgPixelsPtr==NULL)
  {
    std::cerr << "ERROR: Cannot allocate memory for accumulation image";
    return;
  }
  else
  {
    int* extent = this->ReferenceImageData->GetExtent();
    memset(refImgPixelsPtr,0,((extent[1]-extent[0]+1)*(extent[3]-extent[2]+1)*(extent[5]-extent[4]+1)*refImg->GetScalarSize()*refImg->GetNumberOfScalarComponents()));
  }

  // Convert polydata to stencil
  vtkNew<vtkPolyDataToImageStencil> polyToImage;
  polyToImage->SetInputConnection(stripper->GetOutputPort());
  polyToImage->SetOutputSpacing(this->ReferenceImageData->GetSpacing());
  polyToImage->SetOutputOrigin(this->ReferenceImageData->GetOrigin());
  polyToImage->SetOutputWholeExtent(this->ReferenceImageData->GetExtent());
  polyToImage->Update();  

  // Convert stencil to image
  vtkNew<vtkImageStencil> stencil;
  //stencil->SetInput(this->ReferenceImageData);
  stencil->SetInput(refImg);
  stencil->SetStencil(polyToImage->GetOutput());
  stencil->ReverseStencilOn();
  stencil->SetBackgroundValue(this->LabelValue);
  stencil->Update();

  // Save result to output
  vtkNew<vtkImageCast> imageCast;
  imageCast->SetInputConnection(stencil->GetOutputPort());
  imageCast->SetOutputScalarTypeToUnsignedChar();
  imageCast->Update();
  this->OutputLabelmap->ShallowCopy(imageCast->GetOutput());
}
