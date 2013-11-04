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
    return 
      extentsA[0] == extentsB [0] &&
      extentsA[1] == extentsB [1] &&
      extentsA[2] == extentsB [2] &&
      extentsA[3] == extentsB [3] &&
      extentsA[4] == extentsB [4] &&
      extentsA[5] == extentsB [5];
  }

  double roundLarger(double value)
  {
    if( value < 0.0 )
    {
      return floor(value);
    }
    else
    {
      return ceil(value);
    }
  }
}

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
  this->SetBackgroundValue(0.0);

  this->UseReferenceValuesOn();
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
  normalFilter->SetInput(this->InputPolyData);
  normalFilter->ConsistencyOn();

  // Make sure that we have a clean triangle polydata
  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputConnection(normalFilter->GetOutputPort());

  // Convert to triangle strip
  vtkSmartPointer<vtkStripper> stripper=vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(triangle->GetOutputPort());

  int calculatedExtents[6] = {0, 0, 0, 0, 0, 0};
  double polydataExtents[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  this->InputPolyData->GetPoints()->ComputeBounds();
  this->InputPolyData->GetPoints()->GetBounds(polydataExtents);
  int referenceExtents[6] = {0, 0, 0, 0, 0, 0};
  this->ReferenceImageData->GetExtent(referenceExtents);

  calculatedExtents[0] = std::min<int>(roundLarger(polydataExtents[0]), referenceExtents[0]);
  calculatedExtents[1] = std::max<int>(roundLarger(polydataExtents[1]), referenceExtents[1]);
  calculatedExtents[2] = std::min<int>(roundLarger(polydataExtents[2]), referenceExtents[2]);
  calculatedExtents[3] = std::max<int>(roundLarger(polydataExtents[3]), referenceExtents[3]);
  calculatedExtents[4] = std::min<int>(roundLarger(polydataExtents[4]), referenceExtents[4]);
  calculatedExtents[5] = std::max<int>(roundLarger(polydataExtents[5]), referenceExtents[5]);

  if( !areExtentsEqual(calculatedExtents, referenceExtents) )
  {
    vtkWarningMacro("Update: Extents of computed labelmap are not the same as the reference volume.");
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
    referenceImage->SetExtent(calculatedExtents);
    referenceImage->SetSpacing(this->ReferenceImageData->GetSpacing());
    referenceImage->SetOrigin(this->ReferenceImageData->GetOrigin());
    referenceImage->SetScalarType(VTK_UNSIGNED_CHAR);
    referenceImage->SetNumberOfScalarComponents(1);
    referenceImage->AllocateScalars();
    void *referenceImagePixelsPointer = referenceImage->GetScalarPointerForExtent(calculatedExtents);
    if (referenceImagePixelsPointer==NULL)
    {
      std::cerr << "ERROR: Cannot allocate memory for accumulation image";
      return;
    }
    else
    {
      memset(referenceImagePixelsPointer,0,((calculatedExtents[1]-calculatedExtents[0]+1)*(calculatedExtents[3]-calculatedExtents[2]+1)*(calculatedExtents[5]-calculatedExtents[4]+1)*referenceImage->GetScalarSize()*referenceImage->GetNumberOfScalarComponents()));
    }
  }

  // Convert polydata to stencil
  vtkNew<vtkPolyDataToImageStencil> polyToImage;
  polyToImage->SetInputConnection(stripper->GetOutputPort());
  polyToImage->SetOutputSpacing(this->ReferenceImageData->GetSpacing());
  polyToImage->SetOutputOrigin(this->ReferenceImageData->GetOrigin());
  polyToImage->SetOutputWholeExtent(calculatedExtents);
  polyToImage->Update();  

  // Convert stencil to image
  vtkNew<vtkImageStencil> stencil;
  stencil->SetInput(referenceImage);
  stencil->SetStencil(polyToImage->GetOutput());
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
