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

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

  This file is a modified version of vtkImageAccumulate.cxx from VTK

==============================================================================*/

#include "vtkFractionalImageAccumulate.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkImageStencilIterator.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkFieldData.h>
#include <vtkMath.h>

vtkStandardNewMacro(vtkFractionalImageAccumulate);

//----------------------------------------------------------------------------
vtkFractionalImageAccumulate::vtkFractionalImageAccumulate()
{
  this->MinimumFractionalValue = 0;
  this->MaximumFractionalValue = 1.0;
}

//----------------------------------------------------------------------------
vtkFractionalImageAccumulate::~vtkFractionalImageAccumulate() = default;

//----------------------------------------------------------------------------
int vtkFractionalImageAccumulate::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->ComponentExtent,6);
  outInfo->Set(vtkDataObject::ORIGIN(),this->ComponentOrigin,3);
  outInfo->Set(vtkDataObject::SPACING(),this->ComponentSpacing,3);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_DOUBLE, 1);
  return 1;
}

//----------------------------------------------------------------------------
template<class BaseImageScalarType>
int vtkFractionalImageAccumulateExecute(vtkFractionalImageAccumulate *self,
                              vtkImageData *inData,
                              vtkImageData *outData,
                              double min[3], double max[3],
                              double mean[3],
                              double standardDeviation[3],
                              vtkIdType *voxelCount,
                              double *fractionalVoxelCount,
                              int* updateExtent)
{
    switch (self->GetFractionalLabelmap()->GetScalarType())
    {
    vtkTemplateMacro( vtkFractionalImageAccumulateExecute2( self,
                                                (BaseImageScalarType*) nullptr,
                                                (VTK_TT*) nullptr,
                                                inData,
                                                outData,
                                                min, max,
                                                mean,
                                                standardDeviation,
                                                voxelCount,
                                                fractionalVoxelCount,
                                                updateExtent ) );
    default:
      //vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }

    return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class BaseImageScalarType, class FractionalImageScalarType>
int vtkFractionalImageAccumulateExecute2(vtkFractionalImageAccumulate *self,
                              BaseImageScalarType* vtkNotUsed(baseTypePtr),
                              FractionalImageScalarType* vtkNotUsed(fractionalTypePtr),
                              vtkImageData *inData,
                              vtkImageData *outData,
                              double min[3], double max[3],
                              double mean[3],
                              double standardDeviation[3],
                              vtkIdType *voxelCount,
                              double *fractionalVoxelCount,
                              int* updateExtent)
{
  // variables used to compute statistics (filter handles max 3 components)
  double sum[3];
  sum[0] = sum[1] = sum[2] = 0.0;
  double sumSqr[3];
  sumSqr[0] = sumSqr[1] = sumSqr[2] = 0.0;
  min[0] = min[1] = min[2] = VTK_DOUBLE_MAX;
  max[0] = max[1] = max[2] = VTK_DOUBLE_MIN;
  standardDeviation[0] = standardDeviation[1] = standardDeviation[2] = 0.0;
  *voxelCount = 0;
  *fractionalVoxelCount = 0;
  double *outPtr = static_cast<double *>(outData->GetScalarPointer());
  if (!outPtr)
    {
      return 0;
    }

  // input's number of components is used as output dimensionality
  int numC = inData->GetNumberOfScalarComponents();
  if (numC > 3)
    {
    return 0;
    }

  // get information for output data
  int outExtent[6];
  outData->GetExtent(outExtent);
  vtkIdType outIncs[3];
  outData->GetIncrements(outIncs);
  double origin[3];
  outData->GetOrigin(origin);
  double spacing[3];
  outData->GetSpacing(spacing);

  // zero count in every bin
  vtkIdType size = 1;
  size *= (outExtent[1] - outExtent[0] + 1);
  size *= (outExtent[3] - outExtent[2] + 1);
  size *= (outExtent[5] - outExtent[4] + 1);
  for (vtkIdType j = 0; j < size; j++)
    {
    outPtr[j] = 0;
    }

  vtkImageStencilData *stencil = self->GetStencil();
  bool reverseStencil = (self->GetReverseStencil() != 0);
  bool ignoreZero = (self->GetIgnoreZero() != 0);

  vtkImageStencilIterator<BaseImageScalarType> inIter(inData, stencil, updateExtent, self);

  vtkImageData* fractionalLabelmap = self->GetFractionalLabelmap();
  vtkImageStencilIterator<FractionalImageScalarType> fractionalIter(fractionalLabelmap, stencil, updateExtent, self);

  while (!inIter.IsAtEnd())
    {
    if (inIter.IsInStencil() ^ reverseStencil)
      {
      BaseImageScalarType *inPtr = inIter.BeginSpan();
      BaseImageScalarType *spanEndPtr = inIter.EndSpan();

      FractionalImageScalarType* fractionalPtr = (FractionalImageScalarType*)fractionalIter.BeginSpan();

      while (inPtr != spanEndPtr)
        {
        // find the bin for this pixel.
        bool outOfBounds = false;
        double *outPtrC = outPtr;
        double total  = 0.0;

        for (int idxC = 0; idxC < numC; ++idxC)
          {

          double v = static_cast<double>(*inPtr++);
          double f = 1.0;

          if (self->GetUseFractionalLabelmap())
          {
            f = ( (*fractionalPtr++) - self->GetMinimumFractionalValue() ) / (self->GetMaximumFractionalValue() - self->GetMinimumFractionalValue());
          }

          if (!ignoreZero || v != 0)
            {
            // gather statistics
            sum[idxC] += v*f;
            sumSqr[idxC] += v*v*f*f;
            if (v > max[idxC])
              {
              max[idxC] = v;
              }
            if (v < min[idxC])
              {
              min[idxC] = v;
              }
            (*voxelCount)++;
            (*fractionalVoxelCount)+=f;
            total+=f;
            }

          // compute the index
          int outIdx = vtkMath::Floor((v - origin[idxC]) / spacing[idxC]);

          // verify that it is in range
          if (outIdx >= outExtent[idxC*2] && outIdx <= outExtent[idxC*2+1])
            {
            outPtrC += (outIdx - outExtent[idxC*2]) * outIncs[idxC];
            }
          else
            {
              outOfBounds = true;
            }

          }

        // increment the bin
        if (!outOfBounds)
          {
            (*outPtrC) += total;
          }
        }
      }
    fractionalIter.NextSpan();
    inIter.NextSpan();
    }

  // initialize the statistics
  mean[0] = 0;
  mean[1] = 0;
  mean[2] = 0;

  standardDeviation[0] = 0;
  standardDeviation[1] = 0;
  standardDeviation[2] = 0;

  if (*fractionalVoxelCount != 0) // avoid the div0
    {
    double n = static_cast<double>(*fractionalVoxelCount);
    mean[0] = sum[0]/n;
    mean[1] = sum[1]/n;
    mean[2] = sum[2]/n;

    if (*fractionalVoxelCount - 1 != 0) // avoid the div0
      {
      double m = static_cast<double>(*fractionalVoxelCount - 1);
      standardDeviation[0] = sqrt((sumSqr[0] - mean[0]*mean[0]*n)/m);
      standardDeviation[1] = sqrt((sumSqr[1] - mean[1]*mean[1]*n)/m);
      standardDeviation[2] = sqrt((sumSqr[2] - mean[2]*mean[2]*n)/m);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
int vtkFractionalImageAccumulate::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{

  // get the input
  vtkInformation* in1Info = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    in1Info->Get(vtkDataObject::DATA_OBJECT()));
  int *uExt = in1Info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // get the output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Executing image accumulate");

  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  outData->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  outData->AllocateScalars(outInfo);

  // Components turned into x, y and z
  if (inData->GetNumberOfScalarComponents() > 3)
    {
    vtkErrorMacro("This filter can handle up to 3 components");
    return 1;
    }

  // this filter expects that output is type double.
  if (outData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: out ScalarType " << outData->GetScalarType()
                  << " must be double\n");
    return 1;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkFractionalImageAccumulateExecute<VTK_TT>( this,
                                                inData,
                                                outData,
                                                this->Min, this->Max,
                                                this->Mean,
                                                this->StandardDeviation,
                                                &this->VoxelCount,
                                                &this->FractionalVoxelCount,
                                                uExt ));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkFractionalImageAccumulate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}