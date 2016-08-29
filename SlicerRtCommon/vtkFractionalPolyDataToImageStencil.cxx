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


#include "vtkFractionalPolyDataToImageStencil.h"

// VTK includes
#include <vtkImageStencilData.h>
#include <vtkPolyData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

// std includes
#include <map>

vtkStandardNewMacro(vtkFractionalPolyDataToImageStencil);

std::map<double, vtkSmartPointer<vtkCellArray> > LinesCache;
std::map<double, vtkSmartPointer<vtkPolyData> > SliceCache;
std::map<double, vtkIdType*> PointIdsCache;
std::map<double, vtkIdType> NptsCache;
std::map<double, std::vector<vtkIdType> > PointNeighborCountsCache;

//----------------------------------------------------------------------------
vtkFractionalPolyDataToImageStencil::vtkFractionalPolyDataToImageStencil()
{
  LinesCache = std::map<double, vtkSmartPointer<vtkCellArray> >();
  SliceCache = std::map<double, vtkSmartPointer<vtkPolyData> >();
  PointIdsCache = std::map<double, vtkIdType*>();
  NptsCache = std::map<double, vtkIdType>();
  PointNeighborCountsCache = std::map<double,  std::vector<vtkIdType> >();
}

//----------------------------------------------------------------------------
vtkFractionalPolyDataToImageStencil::~vtkFractionalPolyDataToImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkFractionalPolyDataToImageStencil::ThreadedExecute(
  vtkImageStencilData *data,
  int extent[6],
  int threadId)
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
  vtkPolyData *input = this->GetInput();

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
    if (threadId == 0)
      {
      this->UpdateProgress((idxZ - extent[4])*1.0/(extent[5] - extent[4] + 1));
      }

    double z = idxZ*spacing[2] + origin[2];

    slice->PrepareForNewData();
    raster.PrepareForNewData();

    if ( LinesCache.count(z) == 0 )
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
      SliceCache.insert(std::pair<double, vtkPolyData*>(z, sliceCopy));

    }

    slice->DeepCopy(SliceCache[z]);

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

    if (LinesCache.count(z) == 0)
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

        LinesCache.insert(std::pair<double, vtkCellArray*>(z, linesCopy));
        PointIdsCache.insert(std::pair<double, vtkIdType*>(z, pointIds));
        NptsCache.insert(std::pair<double, vtkIdType>(z, npts));
        PointNeighborCountsCache.insert(std::pair<double, std::vector<vtkIdType> >(z, pointNeighborCounts));

      }

    vtkCellArray* lines = LinesCache[z];
    vtkIdType count = lines->GetNumberOfConnectivityEntries();
    vtkIdType* pointIds = PointIdsCache[z];
    vtkIdType npts = NptsCache[z];
    std::vector<vtkIdType> pointNeighborCounts = PointNeighborCountsCache[z];
    
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
int vtkFractionalPolyDataToImageStencil::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::Superclass::RequestData(request, inputVector, outputVector);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int extent[6];
  data->GetExtent(extent);
  // ThreadedExecute is only called from a single thread for
  // now, but it could as easily be called from ThreadedRequestData
  this->ThreadedExecute(data, extent, 0);

  return 1;
}