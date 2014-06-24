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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "vtkPolyDataToLabelmapFilter.h"

// VTK includes
#include <algorithm>
#include <math.h>
#include <vtkImageCast.h>
#include <vtkImageStencil.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkSmartPointer.h>
#include <vtkStripper.h>
#include <vtkTriangleFilter.h>

//----------------------------------------------------------------------------

namespace
{
  bool areExtentsEqual(int extentsA[6], int extentsB[6])
  {
    return extentsA[0] == extentsB[0] &&
      extentsA[1] == extentsB[1] &&
      extentsA[2] == extentsB[2] &&
      extentsA[3] == extentsB[3] &&
      extentsA[4] == extentsB[4] &&
      extentsA[5] == extentsB[5];
  }
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyDataToLabelmapFilter);

//----------------------------------------------------------------------------
vtkPolyDataToLabelmapFilter::vtkPolyDataToLabelmapFilter()
: InputPolyData(NULL)
, OutputLabelmap(NULL)
, ReferenceImageData(NULL)
, LabelValue(2)
, BackgroundValue(0.0)
, UseReferenceValues(true)
{
  this->SetInputPolyData(vtkSmartPointer<vtkPolyData>::New());
  this->SetOutputLabelmap(vtkSmartPointer<vtkImageData>::New());
  this->SetReferenceImageData(vtkSmartPointer<vtkImageData>::New());
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
  if (!this->InputPolyData || !this->ReferenceImageData || !this->OutputLabelmap)
  {
    vtkErrorMacro("Update: Input poly data, reference image and output labelmap have to be initialized!");
    return;
  }

  vtkNew<vtkPolyDataNormals> normalFilter;
#if (VTK_MAJOR_VERSION <= 5)
  normalFilter->SetInput(this->InputPolyData);
#else
  normalFilter->SetInputData(this->InputPolyData);
#endif

  normalFilter->ConsistencyOn();

  // Make sure that we have a clean triangle polydata
  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputConnection(normalFilter->GetOutputPort());

  // Convert to triangle strip
  vtkSmartPointer<vtkStripper> stripper=vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(triangle->GetOutputPort());

  int referenceExtents[6] = {0,0,0,0,0,0};
  double origin[3] = {0,0,0};
  std::vector<int> referenceExtentsVector;
  std::vector<double> originVector;
  if( !this->DeterminePolyDataReferenceOverlap(referenceExtentsVector, originVector) )
  {
    vtkErrorMacro("Unable to determine input and reference overlap.");
    return;
  }
  for( int i = 0; i < 6; ++i )
  {
    referenceExtents[i] = referenceExtentsVector[i];
  }
  for( int i = 0; i < 3; ++i )
  {
    origin[i] = originVector[i];
  }

  vtkSmartPointer<vtkImageData> referenceImage = vtkSmartPointer<vtkImageData>::New();
  if (this->UseReferenceValues)
  {
    // Use reference image
    referenceImage->ShallowCopy(this->ReferenceImageData);
  }
  else
  {
    // Blank reference image
    referenceImage->SetExtent(referenceExtents);
    referenceImage->SetSpacing(this->ReferenceImageData->GetSpacing());
    referenceImage->SetOrigin(origin);

#if (VTK_MAJOR_VERSION <= 5)
    referenceImage->SetScalarType(VTK_UNSIGNED_CHAR);
    referenceImage->SetNumberOfScalarComponents(1);
    referenceImage->AllocateScalars();
#else
    referenceImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
#endif

    void *referenceImagePixelsPointer = referenceImage->GetScalarPointerForExtent(this->ReferenceImageData->GetExtent());
    if (referenceImagePixelsPointer==NULL)
    {
      vtkErrorMacro("ERROR: Cannot allocate memory for accumulation image");
      return;
    }
    else
    {
      memset(referenceImagePixelsPointer,0,((referenceExtents[1]-referenceExtents[0]+1)*(referenceExtents[3]-referenceExtents[2]+1)*(referenceExtents[5]-referenceExtents[4]+1)*referenceImage->GetScalarSize()*referenceImage->GetNumberOfScalarComponents()));
    }
  }

  // Convert polydata to stencil
  vtkNew<vtkPolyDataToImageStencil> polyToImage;
  polyToImage->SetInputConnection(stripper->GetOutputPort());
  polyToImage->SetOutputSpacing(this->ReferenceImageData->GetSpacing());
  polyToImage->SetOutputOrigin(origin);
  polyToImage->SetOutputWholeExtent(referenceExtents);
  polyToImage->Update();  

  // Convert stencil to image
  vtkNew<vtkImageStencil> stencil;
#if (VTK_MAJOR_VERSION <= 5)
  stencil->SetInput(referenceImage);
  stencil->SetStencil(polyToImage->GetOutput());
#else
  stencil->SetInputData(referenceImage);
  stencil->SetStencilData(polyToImage->GetOutput());
#endif
  if (this->UseReferenceValues)
  {
    stencil->ReverseStencilOff();
    stencil->SetBackgroundValue(this->BackgroundValue);
    stencil->Update();

    this->OutputLabelmap->ShallowCopy(stencil->GetOutput());
  }
  else
  {
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
}

//----------------------------------------------------------------------------
bool vtkPolyDataToLabelmapFilter::DeterminePolyDataReferenceOverlap(std::vector<int>& referenceExtentsVector, std::vector<double>& originVector)
{
  int referenceExtents[6] = {0,0,0,0,0,0};
  double origin[3] = {0,0,0};
  double expandedBounds[6] = {0,0,0,0,0,0};
  double polydataBounds[6] = {0,0,0,0,0,0};
  double referenceBounds[6] = {0,0,0,0,0,0};
  double spacing[3] = {0,0,0};
  this->ReferenceImageData->GetExtent(referenceExtents);
  this->ReferenceImageData->GetOrigin(origin);

  if( this->InputPolyData == NULL )
  {
    vtkErrorMacro("InputPolyData was null when trying to calculate overlap.");
    return false;
  }
  if( this->InputPolyData->GetPoints() == NULL )
  {
    this->CopyArraysToVectors(referenceExtentsVector, referenceExtents, originVector, origin);
    return true;
  }
  this->InputPolyData->GetPoints()->ComputeBounds();
  this->InputPolyData->GetPoints()->GetBounds(polydataBounds);

  if( this->ReferenceImageData == NULL )
  {
    vtkErrorMacro("ReferenceImageData was null when trying to calcaluate overlap.");
    return false;
  }
  this->ReferenceImageData->ComputeBounds();
  this->ReferenceImageData->GetBounds(referenceBounds);

  expandedBounds[0] = std::min<double>(polydataBounds[0], referenceBounds[0]);
  expandedBounds[1] = std::max<double>(polydataBounds[1], referenceBounds[1]);
  expandedBounds[2] = std::min<double>(polydataBounds[2], referenceBounds[2]);
  expandedBounds[3] = std::max<double>(polydataBounds[3], referenceBounds[3]);
  expandedBounds[4] = std::min<double>(polydataBounds[4], referenceBounds[4]);
  expandedBounds[5] = std::max<double>(polydataBounds[5], referenceBounds[5]);

  // Bounds are now axis aligned with referenceIJK because everything is in that coordinate frame
  // Can compute extents as values derived from bounds
  this->ReferenceImageData->GetSpacing(spacing);
  origin[0] = expandedBounds[0];
  origin[1] = expandedBounds[2];
  origin[2] = expandedBounds[4];

  int calculatedExtents[6] = {0, 0, 0, 0, 0, 0};
  // for each boundary axis, check for necessary expansion of extents
  for( int axis = 0; axis < 3; ++axis )
  {
    // if same as original extent, no problem!
    calculatedExtents[2*axis+1] = ceil( (expandedBounds[2*axis+1] - expandedBounds[2*axis]) * (1/spacing[axis]));
    if( calculatedExtents < 0 )
    {
      vtkErrorMacro("Invalid extent when calculating overlap between input polydata and reference image. Were they in the IJK coordinate system when this was called?");
      return false;
    }
  }

  if( !areExtentsEqual(referenceExtents, calculatedExtents) )
  {
    for( int i = 0; i < 6; ++i )
    {
      referenceExtents[i] = calculatedExtents[i];
    }
    vtkDebugMacro("DeterminePolyDataReferenceOverlap: Extents of computed labelmap are not the same as the reference volume. Expanding labelmap dimensions.");
  }

  this->CopyArraysToVectors(referenceExtentsVector, referenceExtents, originVector, origin);

  return true;
}

//----------------------------------------------------------------------------
void vtkPolyDataToLabelmapFilter::CopyArraysToVectors( std::vector<int> &extentVector, int extents[6], std::vector<double> &originVector, double origin[3] )
{
  for( int i = 0; i < 6; ++i )
  {
    extentVector.push_back(extents[i]);
  }
  for( int i = 0; i < 3; ++i )
  {
    originVector.push_back(origin[i]);
  }
}
