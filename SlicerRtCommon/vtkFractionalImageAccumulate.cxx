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


#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSegmentationConverter.h"

// Change the value of FRACTIONAL_DATA_TYPE, FRACTIONAL_MIN, and FRACTIONAL_MAX based on the value of VTK_FRACTIONAL_DATA_TYPE
#define VTK_FRACTIONAL_DATA_TYPE VTK_UNSIGNED_CHAR

#if VTK_FRACTIONAL_DATA_TYPE == VTK_UNSIGNED_CHAR
  #define FRACTIONAL_DATA_TYPE unsigned char
  #define FRACTIONAL_MIN 0
  #define FRACTIONAL_MAX 216
#elif VTK_FRACTIONAL_DATA_TYPE == VTK_CHAR
  #define FRACTIONAL_DATA_TYPE char
  #define FRACTIONAL_MIN -108
  #define FRACTIONAL_MAX 108
#endif

#include <math.h>

vtkStandardNewMacro(vtkFractionalImageAccumulate);

//----------------------------------------------------------------------------
vtkFractionalImageAccumulate::vtkFractionalImageAccumulate()
{
}

//----------------------------------------------------------------------------
vtkFractionalImageAccumulate::~vtkFractionalImageAccumulate()
{
}

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
// This templated function executes the filter for any type of data.
template <class T>
int vtkFractionalImageAccumulateExecute(vtkFractionalImageAccumulate *self,
                              vtkImageData *inData, T *,
                              vtkImageData *outData, double *outPtr,
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

  vtkImageStencilIterator<T> inIter(inData, stencil, updateExtent, self);

vtkImageData* fractionalLabelmap = self->GetFractionalLabelmap();
vtkImageStencilIterator<FRACTIONAL_DATA_TYPE> fractionalIter(fractionalLabelmap, stencil, updateExtent, self);
  while (!inIter.IsAtEnd())
    {
    if (inIter.IsInStencil() ^ reverseStencil)
      {
      T *inPtr = inIter.BeginSpan();
      T *spanEndPtr = inIter.EndSpan();

      FRACTIONAL_DATA_TYPE* fractionalPtr = (FRACTIONAL_DATA_TYPE*)fractionalIter.BeginSpan();

      while (inPtr != spanEndPtr)
        {
        // find the bin for this pixel.
        bool outOfBounds = false;
        double *outPtrC = outPtr;
        double total  = 0.0;

        for (int idxC = 0; idxC < numC; ++idxC)
          {

          double v = static_cast<double>(*inPtr++);
          double f;

          if (self->GetUseFractionalLabelmap())
          {
            f = ( static_cast<FRACTIONAL_DATA_TYPE>(*fractionalPtr++) - (double)FRACTIONAL_MIN ) / (double)(FRACTIONAL_MAX - FRACTIONAL_MIN);
          }
          else
          {
            f = static_cast<FRACTIONAL_DATA_TYPE>(*fractionalPtr++);
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
  void *inPtr;
  void *outPtr;

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

  vtkDataArray *inArray = this->GetInputArrayToProcess(0,inputVector);
  inPtr = inData->GetArrayPointerForExtent(inArray, uExt);
  outPtr = outData->GetScalarPointer();

  // Components turned into x, y and z
  if (inData->GetNumberOfScalarComponents() > 3)
    {
    vtkErrorMacro("This filter can handle up to 3 components");
    return 1;
    }

  // this filter expects that output is type int.
  if (outData->GetScalarType() != VTK_DOUBLE)
    {
    vtkErrorMacro(<< "Execute: out ScalarType " << outData->GetScalarType()
                  << " must be double\n");
    return 1;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkFractionalImageAccumulateExecute( this,
                                                inData,
                                                static_cast<VTK_TT *>(inPtr),
                                                outData,
                                                static_cast<double *>(outPtr),
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