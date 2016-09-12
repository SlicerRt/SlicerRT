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

  This file is a modified version of vtkPolyDataToImageStencil.cxx from VTK

==============================================================================*/


#include "vtkPolyDataToFractionalLabelMap.h"

// VTK includes
#include <vtkImageStencilData.h>
#include <vtkPolyData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkNew.h>
#include <vtkPolyDataNormals.h>
#include <vtkTriangleFilter.h>
#include <vtkStripper.h>
#include <vtkImageStencil.h>
#include <vtkImageCast.h>

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// std includes
#include <map>

vtkStandardNewMacro(vtkPolyDataToFractionalLabelMap);

//----------------------------------------------------------------------------
vtkPolyDataToFractionalLabelMap::vtkPolyDataToFractionalLabelMap()
{
  this->NumberOfOffsets = 6;

  this->LinesCache = std::map<double, vtkSmartPointer<vtkCellArray> >();
  this->SliceCache = std::map<double, vtkSmartPointer<vtkPolyData> >();
  this->PointIdsCache = std::map<double, vtkIdType*>();
  this->NptsCache = std::map<double, vtkIdType>();
  this->PointNeighborCountsCache = std::map<double,  std::vector<vtkIdType> >();

  this->OutputImageToWorldMatrix = NULL;

  vtkOrientedImageData* output = vtkOrientedImageData::New();

  this->GetExecutive()->SetOutputData(0, output);

  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkPolyDataToFractionalLabelMap::~vtkPolyDataToFractionalLabelMap()
{
  this->OutputImageToWorldMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyDataToFractionalLabelMap::SetOutput(vtkOrientedImageData* output)
{
    this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
vtkOrientedImageData* vtkPolyDataToFractionalLabelMap::GetOutput()
{
  if (this->GetNumberOfOutputPorts() < 1)
    {
    return NULL;
    }

  return vtkOrientedImageData::SafeDownCast(
    this->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
int vtkPolyDataToFractionalLabelMap::FillOutputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkOrientedImageData");
  return 1;
}

//----------------------------------------------------------------------------
vtkOrientedImageData *vtkPolyDataToFractionalLabelMap::AllocateOutputData(
  vtkDataObject *out, int* uExt)
{
  vtkOrientedImageData *res = vtkOrientedImageData::SafeDownCast(out);
  if (!res)
    {
    vtkWarningMacro("Call to AllocateOutputData with non vtkOrientedImageData"
                    " output");
    return NULL;
    }
  res->SetExtent(uExt);
  res->AllocateScalars(VTK_FRACTIONAL_DATA_TYPE, 1);

  // Allocate output image data
  res->AllocateScalars(VTK_FRACTIONAL_DATA_TYPE, 1);

  // Set-up fractional labelmap
  void* fractionalLabelMapVoxelsPointer = res->GetScalarPointerForExtent(res->GetExtent());
  if (!fractionalLabelMapVoxelsPointer)
    {
    vtkErrorMacro("Convert: Failed to allocate memory for output labelmap image!");
    return false;
    }
  else
    {
    int extent[6];
    res->GetExtent(extent);
    memset(fractionalLabelMapVoxelsPointer, FRACTIONAL_MIN, ((extent[1]-extent[0]+1)*(extent[3]-extent[2]+1)*(extent[5]-extent[4]+1) * res->GetScalarSize() * res->GetNumberOfScalarComponents()));
    }

  return res;
}

//----------------------------------------------------------------------------
int vtkPolyDataToFractionalLabelMap::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkOrientedImageData *outputData = vtkOrientedImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
    this->AllocateOutputData(
    outputData,
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));

  vtkInformation *inputInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData *inputData = vtkPolyData::SafeDownCast(
    inputInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->OutputImageToWorldMatrix)
    {
    outputData->SetImageToWorldMatrix(this->OutputImageToWorldMatrix);
    }
  else
    {
    vtkSmartPointer<vtkMatrix4x4> identityMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    identityMatrix->Identity();
    outputData->SetImageToWorldMatrix(identityMatrix);
    }

  if (this->OutputOrigin)
    {
    outputData->SetOrigin(this->OutputOrigin);
    }

  if (this->OutputSpacing)
    {
    outputData->SetSpacing(this->OutputSpacing);
    }
  
  outputData->SetExtent(this->OutputWholeExtent);

  vtkSmartPointer<vtkMatrix4x4> outputLabelmapImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  outputData->GetImageToWorldMatrix(outputLabelmapImageToWorldMatrix);

  vtkSmartPointer<vtkTransform> inverseOutputLabelmapGeometryTransform = vtkSmartPointer<vtkTransform>::New();
  inverseOutputLabelmapGeometryTransform->SetMatrix(outputLabelmapImageToWorldMatrix);
  inverseOutputLabelmapGeometryTransform->Inverse();

  // Transform the polydata from RAS to IJK space
  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformPolyDataFilter->SetInputData(inputData);
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

  int extent[6];
  outputData->GetExtent(extent);

  vtkSmartPointer<vtkOrientedImageData> binaryLabelMap = vtkSmartPointer<vtkOrientedImageData>::New();
  binaryLabelMap->SetExtent(extent);
  binaryLabelMap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  

  // The magnitude of the offset step size ( n-1 / 2n )
  double offsetStepSize = (double)(this->NumberOfOffsets-1.0)/(2 * this->NumberOfOffsets);
  
  // Iterate through "NumberOfOffsets" in each of the dimensions and create a binary labelmap at each offset
  for (int k = 0; k < this->NumberOfOffsets; ++k)
  {
    double kOffset = ( (double) k / this->NumberOfOffsets - offsetStepSize );

    for (int j = 0; j < this->NumberOfOffsets; ++j)
    {
      double jOffset = ( (double) j / this->NumberOfOffsets - offsetStepSize );

      for (int i = 0; i < this->NumberOfOffsets; ++i)
      {
        double iOffset = ( (double) i / this->NumberOfOffsets - offsetStepSize );

        vtkSmartPointer<vtkImageStencilData> imageStencilData = vtkSmartPointer<vtkImageStencilData>::New();
        imageStencilData->SetSpacing(1.0, 1.0, 1.0);
        imageStencilData->SetExtent(extent);
        imageStencilData->AllocateExtents();
        imageStencilData->SetOrigin(iOffset, jOffset, kOffset);

        binaryLabelMap->SetOrigin(iOffset, jOffset, kOffset);
        
        // Create stencil for the current binary labelmap offset
        this->FillImageStencilData(imageStencilData, transformedClosedSurface, extent);

        void* binaryLabelMapVoxelsPointer = binaryLabelMap->GetScalarPointerForExtent(binaryLabelMap->GetExtent());
        if (!binaryLabelMapVoxelsPointer)
          {
          vtkErrorMacro("Convert: Failed to allocate memory for output labelmap image!");
          return false;
          }
        else
          {
          memset(binaryLabelMapVoxelsPointer, 0, ((extent[1]-extent[0]+1)*(extent[3]-extent[2]+1)*(extent[5]-extent[4]+1) * binaryLabelMap->GetScalarSize() * binaryLabelMap->GetNumberOfScalarComponents()));
          }

        // Convert stencil to image
        vtkNew<vtkImageStencil> stencil;
        stencil->SetInputData(binaryLabelMap);
        stencil->SetStencilData(imageStencilData);
        stencil->ReverseStencilOn();
        stencil->SetBackgroundValue(1); // General foreground value is 1 (background value because of reverse stencil)

        // Save result to output
        vtkNew<vtkImageCast> imageCast;
        imageCast->SetInputConnection(stencil->GetOutputPort());
        imageCast->SetOutputScalarTypeToUnsignedChar();
        imageCast->Update();
        
        binaryLabelMap->ShallowCopy(imageCast->GetOutput());
        this->AddBinaryLabelMapToFractionalLabelMap(binaryLabelMap, outputData);       

        this->UpdateProgress(((i+1)*(j+1)*(k+1))/(this->NumberOfOffsets*this->NumberOfOffsets*this->NumberOfOffsets));

      } // i
    } // j
  } // k


  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyDataToFractionalLabelMap::AddBinaryLabelMapToFractionalLabelMap(vtkOrientedImageData* binaryLabelMap, vtkOrientedImageData* fractionalLabelMap)
{

  if (!binaryLabelMap)
  {
    vtkErrorMacro("AddBinaryLabelMapToFractionalLabelMap: Invalid vtkOrientedImageData!");
    return;
  }

  if (!fractionalLabelMap)
  {
    vtkErrorMacro("AddBinaryLabelMapToFractionalLabelMap: Invalid vtkOrientedImageData!");
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

//----------------------------------------------------------------------------
void vtkPolyDataToFractionalLabelMap::FillImageStencilData(
  vtkImageStencilData *data, vtkPolyData* closedSurface,
  int extent[6])
{
  // Description of algorithm:
  // 1) cut the polydata at each z slice to create polylines
  // 2) find all "loose ends" and connect them to make polygons
  //    (if the input polydata is closed, there will be no loose ends)
  // 3) go through all line segments, and for each integer y value on
  //    a line segment, store the x value at that point in a bucket
  // 4) for each z integer index, find all the stored x values
  //    and use them to create one z slice of the vtkStencilData

  // the spacing and origin of the generated stencil
  double *spacing = data->GetSpacing();
  double *origin = data->GetOrigin();

  // if we have no data then return
  if (!this->GetInput()->GetNumberOfPoints())
    {
    return;
    }

  // Only divide once
  double invspacing[3];
  invspacing[0] = 1.0/spacing[0];
  invspacing[1] = 1.0/spacing[1];
  invspacing[2] = 1.0/spacing[2];

  // get the input data
  vtkPolyData *input = closedSurface;

  // the output produced by cutting the polydata with the Z plane
  vtkPolyData *slice = vtkPolyData::New();

  // This raster stores all line segments by recording all "x"
  // positions on the surface for each y integer position.
  vtkImageStencilRaster raster(&extent[2]);
  raster.SetTolerance(this->Tolerance);

  // The extent for one slice of the image
  int sliceExtent[6];
  sliceExtent[0] = extent[0]; sliceExtent[1] = extent[1];
  sliceExtent[2] = extent[2]; sliceExtent[3] = extent[3];
  sliceExtent[4] = extent[4]; sliceExtent[5] = extent[4];

  // Loop through the slices
  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {

    double z = idxZ*spacing[2] + origin[2];

    slice->PrepareForNewData();
    raster.PrepareForNewData();

    if ( this->LinesCache.count(z) == 0 )
      {

      // Step 1: Cut the data into slices
      if (input->GetNumberOfPolys() > 0 || input->GetNumberOfStrips() > 0)
        {
            this->PolyDataCutter(input, slice, z);
        }
      else
        {
        // if no polys, select polylines instead
        this->PolyDataSelector(input, slice, z, spacing[2]);
        }

      if (!slice->GetNumberOfLines())
        {
        continue;
        }

      vtkSmartPointer<vtkPolyData> sliceCopy = vtkSmartPointer<vtkPolyData>::New();
      sliceCopy->DeepCopy(slice);
      this->SliceCache.insert(std::pair<double, vtkPolyData*>(z, sliceCopy));

    }

    slice->DeepCopy(this->SliceCache[z]);

    // convert to structured coords via origin and spacing
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    points->DeepCopy(slice->GetPoints());
    vtkIdType numberOfPoints = points->GetNumberOfPoints();

    for (vtkIdType j = 0; j < numberOfPoints; j++)
      {
      double tempPoint[3];
      points->GetPoint(j, tempPoint);
      tempPoint[0] = (tempPoint[0] - origin[0])*invspacing[0];
      tempPoint[1] = (tempPoint[1] - origin[1])*invspacing[1];
      tempPoint[2] = (tempPoint[2] - origin[2])*invspacing[2];
      points->SetPoint(j, tempPoint);
      }

    if (this->LinesCache.count(z) == 0)
    {

      // Step 2: Find and connect all the loose ends
      std::vector<vtkIdType> pointNeighbors(numberOfPoints);
      std::vector<vtkIdType> pointNeighborCounts(numberOfPoints);
      std::fill(pointNeighborCounts.begin(), pointNeighborCounts.end(), 0);

      // get the connectivity count for each point
      vtkCellArray *lines = slice->GetLines();
      vtkIdType npts = 0;
      vtkIdType *pointIds = 0;
      vtkIdType count = lines->GetNumberOfConnectivityEntries();
      for (vtkIdType loc = 0; loc < count; loc += npts + 1)
        {
        lines->GetCell(loc, npts, pointIds);
        if (npts > 0)
          {
          pointNeighborCounts[pointIds[0]] += 1;
          for (vtkIdType j = 1; j < npts-1; j++)
            {
            pointNeighborCounts[pointIds[j]] += 2;
            }
          pointNeighborCounts[pointIds[npts-1]] += 1;
          if (pointIds[0] != pointIds[npts-1])
            {
            // store the neighbors for end points, because these are
            // potentially loose ends that will have to be dealt with later
            pointNeighbors[pointIds[0]] = pointIds[1];
            pointNeighbors[pointIds[npts-1]] = pointIds[npts-2];
            }
          }
        }

      // use connectivity count to identify loose ends and branch points
      std::vector<vtkIdType> looseEndIds;
      std::vector<vtkIdType> branchIds;

      for (vtkIdType j = 0; j < numberOfPoints; j++)
        {
        if (pointNeighborCounts[j] == 1)
          {
          looseEndIds.push_back(j);
          }
        else if (pointNeighborCounts[j] > 2)
          {
          branchIds.push_back(j);
          }
        }

      // remove any spurs
      for (size_t b = 0; b < branchIds.size(); b++)
        {
        for (size_t i = 0; i < looseEndIds.size(); i++)
          {
          if (pointNeighbors[looseEndIds[i]] == branchIds[b])
            {
            // mark this pointId as removed
            pointNeighborCounts[looseEndIds[i]] = 0;
            looseEndIds.erase(looseEndIds.begin() + i);
            i--;
            if (--pointNeighborCounts[branchIds[b]] <= 2)
              {
              break;
              }
            }
          }
        }

      // join any loose ends
      while (looseEndIds.size() >= 2)
        {
        size_t n = looseEndIds.size();

        // search for the two closest loose ends
        double maxval = -VTK_FLOAT_MAX;
        vtkIdType firstIndex = 0;
        vtkIdType secondIndex = 1;
        bool isCoincident = false;
        bool isOnHull = false;

        for (size_t i = 0; i < n && !isCoincident; i++)
          {
          // first loose end
          vtkIdType firstLooseEndId = looseEndIds[i];
          vtkIdType neighborId = pointNeighbors[firstLooseEndId];

          double firstLooseEnd[3];
          slice->GetPoint(firstLooseEndId, firstLooseEnd);
          double neighbor[3];
          slice->GetPoint(neighborId, neighbor);

          for (size_t j = i+1; j < n; j++)
            {
            vtkIdType secondLooseEndId = looseEndIds[j];
            if (secondLooseEndId != neighborId)
              {
              double currentLooseEnd[3];
              slice->GetPoint(secondLooseEndId, currentLooseEnd);

              // When connecting loose ends, use dot product to favor
              // continuing in same direction as the line already
              // connected to the loose end, but also favour short
              // distances by dividing dotprod by square of distance.
              double v1[2], v2[2];
              v1[0] = firstLooseEnd[0] - neighbor[0];
              v1[1] = firstLooseEnd[1] - neighbor[1];
              v2[0] = currentLooseEnd[0] - firstLooseEnd[0];
              v2[1] = currentLooseEnd[1] - firstLooseEnd[1];
              double dotprod = v1[0]*v2[0] + v1[1]*v2[1];
              double distance2 = v2[0]*v2[0] + v2[1]*v2[1];

              // check if points are coincident
              if (distance2 == 0)
                {
                firstIndex = i;
                secondIndex = j;
                isCoincident = true;
                break;
                }

              // prefer adding segments that lie on hull
              double midpoint[2], normal[2];
              midpoint[0] = 0.5*(currentLooseEnd[0] + firstLooseEnd[0]);
              midpoint[1] = 0.5*(currentLooseEnd[1] + firstLooseEnd[1]);
              normal[0] = currentLooseEnd[1] - firstLooseEnd[1];
              normal[1] = -(currentLooseEnd[0] - firstLooseEnd[0]);
              double sidecheck = 0.0;
              bool checkOnHull = true;
              for (size_t k = 0; k < n; k++)
                {
                if (k != i && k != j)
                  {
                  double checkEnd[3];
                  slice->GetPoint(looseEndIds[k], checkEnd);
                  double dotprod2 = ((checkEnd[0] - midpoint[0])*normal[0] +
                                     (checkEnd[1] - midpoint[1])*normal[1]);
                  if (dotprod2*sidecheck < 0)
                    {
                    checkOnHull = false;
                    }
                  sidecheck = dotprod2;
                  }
                }

              // check if new candidate is better than previous one
              if ((checkOnHull && !isOnHull) ||
                  (checkOnHull == isOnHull && dotprod > maxval*distance2))
                {
                firstIndex = i;
                secondIndex = j;
                isOnHull |= checkOnHull;
                maxval = dotprod/distance2;
                }
              }
            }
          }

        // get info about the two loose ends and their neighbors
        vtkIdType firstLooseEndId = looseEndIds[firstIndex];
        vtkIdType neighborId = pointNeighbors[firstLooseEndId];
        double firstLooseEnd[3];
        slice->GetPoint(firstLooseEndId, firstLooseEnd);
        double neighbor[3];
        slice->GetPoint(neighborId, neighbor);

        vtkIdType secondLooseEndId = looseEndIds[secondIndex];
        vtkIdType secondNeighborId = pointNeighbors[secondLooseEndId];
        double secondLooseEnd[3];
        slice->GetPoint(secondLooseEndId, secondLooseEnd);
        double secondNeighbor[3];
        slice->GetPoint(secondNeighborId, secondNeighbor);

        // remove these loose ends from the list
        looseEndIds.erase(looseEndIds.begin() + secondIndex);
        looseEndIds.erase(looseEndIds.begin() + firstIndex);

        if (!isCoincident)
          {
          // create a new line segment by connecting these two points
          lines->InsertNextCell(2);
          lines->InsertCellPoint(firstLooseEndId);
          lines->InsertCellPoint(secondLooseEndId);
          }
        }

        vtkSmartPointer<vtkCellArray> linesCopy = vtkSmartPointer<vtkCellArray>::New();
        linesCopy->DeepCopy(lines);

        this->LinesCache.insert(std::pair<double, vtkCellArray*>(z, linesCopy));
        this->PointIdsCache.insert(std::pair<double, vtkIdType*>(z, pointIds));
        this->NptsCache.insert(std::pair<double, vtkIdType>(z, npts));
        this->PointNeighborCountsCache.insert(std::pair<double, std::vector<vtkIdType> >(z, pointNeighborCounts));

      }

    vtkCellArray* lines = this->LinesCache[z];
    vtkIdType count = lines->GetNumberOfConnectivityEntries();
    vtkIdType* pointIds = this->PointIdsCache[z];
    vtkIdType npts = this->NptsCache[z];
    std::vector<vtkIdType> pointNeighborCounts = this->PointNeighborCountsCache[z];
    
    // Step 3: Go through all the line segments for this slice,
    // and for each integer y position on the line segment,
    // drop the corresponding x position into the y raster line.
    for (vtkIdType loc = 0; loc < count; loc += npts + 1)
      {
      lines->GetCell(loc, npts, pointIds);
      if (npts > 0)
        {
        vtkIdType pointId0 = pointIds[0];
        double point0[3];
        points->GetPoint(pointId0, point0);
        for (vtkIdType j = 1; j < npts; j++)
          {
          vtkIdType pointId1 = pointIds[j];
          double point1[3];
          points->GetPoint(pointId1, point1);

          // make sure points aren't flagged for removal
          if (pointNeighborCounts[pointId0] > 0 &&
              pointNeighborCounts[pointId1] > 0)
            {
            raster.InsertLine(point0, point1);
            }

          pointId0 = pointId1;
          point0[0] = point1[0];
          point0[1] = point1[1];
          point0[2] = point1[2];
          }
        }
      }

    // Step 4: Use the x values stored in the xy raster to create
    // one z slice of the vtkStencilData
    sliceExtent[4] = idxZ;
    sliceExtent[5] = idxZ;
    raster.FillStencilData(data, sliceExtent);

    }

  slice->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyDataToFractionalLabelMap::DeleteCache()
{

  this->SliceCache.clear();
  this->LinesCache.clear();
  this->NptsCache.clear();
  this->PointIdsCache.clear();
  this->PointNeighborCountsCache.clear();

}