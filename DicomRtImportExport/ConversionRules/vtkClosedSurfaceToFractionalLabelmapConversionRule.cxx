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

==============================================================================*/

// SegmentationCore includes
#include "vtkOrientedImageData.h"

// Segmentations includes
#include "vtkMRMLSegmentationsDisplayableManager2D.h"

// DicomRtImportExport includes
#include "vtkClosedSurfaceToFractionalLabelmapConversionRule.h"

// Slicer includes
#include "vtkLoggingMacros.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>

// VTK includes
#include <vtkNew.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageCast.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataNormals.h>
#include <vtkStripper.h>
#include <vtkTriangleFilter.h>
#include <vtkFractionalPolyDataToImageStencil.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkFieldData.h>

// STD includes
#include <sstream>
#include <queue>

//----------------------------------------------------------------------------
vtkSegmentationConverterRuleNewMacro(vtkClosedSurfaceToFractionalLabelmapConversionRule);

//----------------------------------------------------------------------------
vtkClosedSurfaceToFractionalLabelmapConversionRule::vtkClosedSurfaceToFractionalLabelmapConversionRule()
{
  this->NumberOfOffsets = 6;
  this->UseOutputImageDataGeometry = true;
}

//----------------------------------------------------------------------------
vtkClosedSurfaceToFractionalLabelmapConversionRule::~vtkClosedSurfaceToFractionalLabelmapConversionRule()
{
}

//----------------------------------------------------------------------------
unsigned int vtkClosedSurfaceToFractionalLabelmapConversionRule::GetConversionCost(
  vtkDataObject* vtkNotUsed(sourceRepresentation)/*=NULL*/,
  vtkDataObject* vtkNotUsed(targetRepresentation)/*=NULL*/)
{
  // Rough input-independent guess (ms)
  return 500;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkClosedSurfaceToFractionalLabelmapConversionRule::ConstructRepresentationObjectByRepresentation(std::string representationName)
{
  if ( !representationName.compare(this->GetSourceRepresentationName()) )
  {
    return (vtkDataObject*)vtkPolyData::New();
  }
  else if ( !representationName.compare(this->GetTargetRepresentationName()) )
  {
    return (vtkDataObject*)vtkOrientedImageData::New();
  }
  else
  {
    return NULL;
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkClosedSurfaceToFractionalLabelmapConversionRule::ConstructRepresentationObjectByClass(std::string className)
{
  if (!className.compare("vtkPolyData"))
  {
    return (vtkDataObject*)vtkPolyData::New();
  }
  else if (!className.compare("vtkOrientedImageData"))
  {
    return (vtkDataObject*)vtkOrientedImageData::New();
  }
  else
  {
    return NULL;
  }
}

//----------------------------------------------------------------------------
bool vtkClosedSurfaceToFractionalLabelmapConversionRule::Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation)
{

  // Check validity of source and target representation objects
  vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(sourceRepresentation);
  if (!closedSurfacePolyData)
  {
    vtkErrorMacro("Convert: Source representation is not a poly data!");
    return false;
  }
  vtkOrientedImageData* fractionalLabelMap = vtkOrientedImageData::SafeDownCast(targetRepresentation);
  if (!fractionalLabelMap)
  {
    vtkErrorMacro("Convert: Target representation is not an oriented image data!");
    return false;
  }
  if (closedSurfacePolyData->GetNumberOfPoints() < 2 || closedSurfacePolyData->GetNumberOfCells() < 2)
  {
    vtkErrorMacro("Convert: Cannot create binary labelmap from surface with number of points: " << closedSurfacePolyData->GetNumberOfPoints() << " and number of cells: " << closedSurfacePolyData->GetNumberOfCells());
    return false;
  }

  // Compute output labelmap geometry based on poly data, an reference image
  // geometry, and store the calculated geometry in output labelmap image data
  if (!this->CalculateOutputGeometry(closedSurfacePolyData, fractionalLabelMap))
  {
    vtkErrorMacro("Convert: Failed to calculate output image geometry!");
    return false;
  }

  // Pad the extent of the fractional labelmap
  int extent[6] = {0,-1,0,-1,0,-1};
  fractionalLabelMap->GetExtent(extent);
  for (int i=0; i<2; ++i)
  {
    --extent[2*i];
    ++extent[2*i+1];
  }
  fractionalLabelMap->SetExtent(extent);

  // Allocate output image data
  fractionalLabelMap->AllocateScalars(VTK_FRACTIONAL_DATA_TYPE, 1);

  // Set-up fractional labelmap
  void* fractionalLabelMapVoxelsPointer = fractionalLabelMap->GetScalarPointerForExtent(fractionalLabelMap->GetExtent());
  if (!fractionalLabelMapVoxelsPointer)
  {
    vtkErrorMacro("Convert: Failed to allocate memory for output labelmap image!");
    return false;
  }
  else
  {
    memset(fractionalLabelMapVoxelsPointer, FRACTIONAL_MIN, ((extent[1]-extent[0]+1)*(extent[3]-extent[2]+1)*(extent[5]-extent[4]+1) * fractionalLabelMap->GetScalarSize() * fractionalLabelMap->GetNumberOfScalarComponents()));
  }

  vtkSmartPointer<vtkMatrix4x4> outputLabelmapImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  fractionalLabelMap->GetImageToWorldMatrix(outputLabelmapImageToWorldMatrix);

  vtkSmartPointer<vtkTransform> inverseOutputLabelmapGeometryTransform = vtkSmartPointer<vtkTransform>::New();
  inverseOutputLabelmapGeometryTransform->SetMatrix(outputLabelmapImageToWorldMatrix);
  inverseOutputLabelmapGeometryTransform->Inverse();

  // Set geometry to identity for the volume so that we can perform the stencil operation in IJK space
  vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  identityMatrix->Identity();

  // Transform the polydata from RAS to IJK space
  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformPolyDataFilter->SetInputData(closedSurfacePolyData);
  transformPolyDataFilter->SetTransform(inverseOutputLabelmapGeometryTransform);

  // Compute polydata normals
  vtkNew<vtkPolyDataNormals> normalFilter;
  normalFilter->SetInputConnection(transformPolyDataFilter->GetOutputPort());
  normalFilter->ConsistencyOn();

  // Make sure that we have a clean triangle polydata
  vtkNew<vtkTriangleFilter> triangle;
  triangle->SetInputConnection(normalFilter->GetOutputPort());

  // Convert to triangle strip
  vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(triangle->GetOutputPort());
  stripper->Update();

  // PolyData of the closed surface in IJK space
  vtkSmartPointer<vtkPolyData> transformedClosedSurface = stripper->GetOutput();

  // The magnitude of the offset step size ( n-1 / 2n )
  double offsetStepSize = (double)(this->NumberOfOffsets-1.0)/(2 * this->NumberOfOffsets);

  vtkSmartPointer<vtkFractionalPolyDataToImageStencil> polyDataToImageStencil = vtkSmartPointer<vtkFractionalPolyDataToImageStencil>::New();
  polyDataToImageStencil->SetInputData(transformedClosedSurface);
  polyDataToImageStencil->SetOutputSpacing(1.0,1.0,1.0);
  polyDataToImageStencil->SetOutputOrigin(0,0,0);
  polyDataToImageStencil->SetOutputWholeExtent(fractionalLabelMap->GetExtent());

  // Iterate through "NumberOfOffsets" in each of the dimensions and calculate
  for (int k = 0; k < this->NumberOfOffsets; ++k)
  {
    double kOffset = ( (double) k / this->NumberOfOffsets - offsetStepSize );

    for (int j = 0; j < this->NumberOfOffsets; ++j)
    {
      double jOffset = ( (double) j / this->NumberOfOffsets - offsetStepSize );

      for (int i = 0; i < this->NumberOfOffsets; ++i)
      {
        double iOffset = ( (double) i / this->NumberOfOffsets - offsetStepSize );
        
        vtkSmartPointer<vtkOrientedImageData> binaryLabelMap = vtkSmartPointer<vtkOrientedImageData>::New();
        binaryLabelMap->SetOrigin(iOffset, jOffset, kOffset);
        binaryLabelMap->SetExtent(fractionalLabelMap->GetExtent());
        binaryLabelMap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

        this->CreateBinaryLabelMap(transformedClosedSurface, binaryLabelMap, polyDataToImageStencil);
        this->AddBinaryLabelMapToFractionalLabelMap(binaryLabelMap, fractionalLabelMap);

      } // i
    } // j
  } // k

  vtkSmartPointer<vtkDoubleArray> scalarRange = vtkSmartPointer<vtkDoubleArray>::New();
  scalarRange->SetName(vtkMRMLSegmentationsDisplayableManager2D::GetScalarRangeFieldName());
  scalarRange->InsertNextValue(FRACTIONAL_MIN);
  scalarRange->InsertNextValue(FRACTIONAL_MAX);
  fractionalLabelMap->GetFieldData()->AddArray(scalarRange);

  vtkSmartPointer<vtkDoubleArray> thresholdValue = vtkSmartPointer<vtkDoubleArray>::New();
  thresholdValue->SetName(vtkMRMLSegmentationsDisplayableManager2D::GetThresholdValueFieldName());
  thresholdValue->InsertNextValue((FRACTIONAL_MIN+FRACTIONAL_MAX)/2.0);
  fractionalLabelMap->GetFieldData()->AddArray(thresholdValue);

  vtkSmartPointer<vtkIntArray> interpolationType = vtkSmartPointer<vtkIntArray>::New();
  interpolationType->SetName(vtkMRMLSegmentationsDisplayableManager2D::GetInterpolationTypeFieldName());
  interpolationType->InsertNextValue(VTK_LINEAR_INTERPOLATION);
  fractionalLabelMap->GetFieldData()->AddArray(interpolationType);

  return true;
}

//----------------------------------------------------------------------------
void vtkClosedSurfaceToFractionalLabelmapConversionRule::CreateBinaryLabelMap(vtkPolyData* closedSurface, vtkOrientedImageData* binaryLabelMap, vtkFractionalPolyDataToImageStencil* polyDataToImageStencil)
{

  if (!closedSurface)
  {
    vtkErrorMacro("CreateBinaryLabelMap: Invalid vtkPolyData!");
    return;
  }

  if (!binaryLabelMap)
  {
    vtkErrorMacro("CreateBinaryLabelMap: Invalid vtkOrientedImageData!");
    return;
  }

  if (!polyDataToImageStencil)
  {
    vtkErrorMacro("CreateBinaryLabelMap: Invalid vtkFractionalPolyDataToImageStencil!");
  }

  // Set-up binary labelmap
  void* binaryLabelMapPointer = binaryLabelMap->GetScalarPointerForExtent(binaryLabelMap->GetExtent());
  if (!binaryLabelMapPointer)
  {
    vtkErrorMacro("Convert: Failed to allocate memory for output labelmap image!");
    return;
  }
  else
  {
    int extent[6] = {-1,0,-1,0,-1,0};
    binaryLabelMap->GetExtent(extent);
    memset(binaryLabelMapPointer, 0, ((extent[1]-extent[0]+1)*(extent[3]-extent[2]+1)*(extent[5]-extent[4]+1) * binaryLabelMap->GetScalarSize() * binaryLabelMap->GetNumberOfScalarComponents()));
  }

  polyDataToImageStencil->SetOutputOrigin(binaryLabelMap->GetOrigin());

  // Convert stencil to image
  vtkNew<vtkImageStencil> stencil;
  stencil->SetInputData(binaryLabelMap);
  stencil->SetStencilConnection(polyDataToImageStencil->GetOutputPort());
  stencil->ReverseStencilOn();
  stencil->SetBackgroundValue(1); // General foreground value is 1 (background value because of reverse stencil)

  // Save result to output
  vtkNew<vtkImageCast> imageCast;
  imageCast->SetInputConnection(stencil->GetOutputPort());
  imageCast->SetOutputScalarTypeToUnsignedChar();
  imageCast->Update();
  binaryLabelMap->ShallowCopy(imageCast->GetOutput());

  return;
}

//----------------------------------------------------------------------------
void vtkClosedSurfaceToFractionalLabelmapConversionRule::AddBinaryLabelMapToFractionalLabelMap(vtkOrientedImageData* binaryLabelMap, vtkOrientedImageData* fractionalLabelMap)
{

  if (!binaryLabelMap)
  {
    vtkErrorMacro("CreateBinaryLabelMap: Invalid vtkOrientedImageData!");
    return;
  }

  if (!fractionalLabelMap)
  {
    vtkErrorMacro("CreateBinaryLabelMap: Invalid vtkOrientedImageData!");
    return;
  }

  int binaryExtent[6] = {0,-1,0,-1,0,-1};
  binaryLabelMap->GetExtent(binaryExtent);

  int fractionalExtent[6] = {0,-1,0,-1,0,-1};
  fractionalLabelMap->GetExtent(fractionalExtent);

  // Get points to the extent in both the binary and fractional labelmaps
  char* binaryLabelMapPointer = (char*)binaryLabelMap->GetScalarPointerForExtent(binaryExtent);
  FRACTIONAL_DATA_TYPE* fractionalLabelMapPointer = (FRACTIONAL_DATA_TYPE*)fractionalLabelMap->GetScalarPointerForExtent(fractionalExtent);

  // Loop through each of the voxels in the current extent of the binary labelmap
  for (int k=binaryExtent[4]; k <= binaryExtent[5]; ++k)
  {
    for (int j=binaryExtent[2]; j <= binaryExtent[3]; ++j)
    {
      for (int i=binaryExtent[0]; i <= binaryExtent[1]; ++i)
      {

        // If the binary voxel is not empty
        if ( (*binaryLabelMapPointer) > 0 )
        {
          ++(*fractionalLabelMapPointer);
        }

        // Increment the pointer to the next binary voxel
        ++binaryLabelMapPointer;
        ++fractionalLabelMapPointer;
      }
    }
  }

}