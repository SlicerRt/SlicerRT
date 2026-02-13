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

==============================================================================*/

#include "vtkPlanarContourToClosedSurfaceByRasterizationConversionRule.h"

// VTK includes
#include <vtkCleanPolyData.h>
#include <vtkExtractCells.h>
#include <vtkImageData.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageStencil.h>
#include <vtkLine.h>
#include <vtkMarchingCubes.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnstructuredGrid.h>

// vtkAddon includes
#include <vtkAddonMathUtilities.h>

// SegmentationCore includes
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
#include <vtkSegment.h>
#endif

// STD includes
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlanarContourToClosedSurfaceByRasterizationConversionRule);

//----------------------------------------------------------------------------
vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::vtkPlanarContourToClosedSurfaceByRasterizationConversionRule()
{
  this->ConversionParameters->SetParameter(
    "Default slice thickness", "2.0",
    "Default slice thickness (mm) used when contour spacing cannot be determined.");
}

//----------------------------------------------------------------------------
vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::~vtkPlanarContourToClosedSurfaceByRasterizationConversionRule() = default;

//----------------------------------------------------------------------------
vtkSegmentationConverterRule* vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::CreateRuleInstance()
{
  return vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::New();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::ConstructRepresentationObjectByRepresentation(std::string representationName)
{
  if (!representationName.compare(this->GetSourceRepresentationName())
    || !representationName.compare(this->GetTargetRepresentationName()))
  {
    return vtkPolyData::New();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::ConstructRepresentationObjectByClass(std::string className)
{
  if (!className.compare("vtkPolyData"))
  {
    return vtkPolyData::New();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
unsigned int vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::GetConversionCost(
  vtkDataObject* /*sourceRepresentation*/,
  vtkDataObject* /*targetRepresentation*/)
{
  // Higher than the default triangulation rule (700) so that the direct
  // contour-to-surface approach is preferred by default.
  return 1000;
}

//----------------------------------------------------------------------------
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
bool vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::Convert(vtkSegment* segment)
{
  this->CreateTargetRepresentation(segment);
#else
bool vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation)
{
#endif
  // Check validity of source and target representation objects
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  vtkPolyData* planarContoursPolyData = vtkPolyData::SafeDownCast(
    segment->GetRepresentation(this->GetSourceRepresentationName()));
#else
  vtkPolyData* planarContoursPolyData = vtkPolyData::SafeDownCast(sourceRepresentation);
#endif
  if (!planarContoursPolyData)
  {
    vtkErrorMacro("Convert: Source representation is not a poly data");
    return false;
  }
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(
    segment->GetRepresentation(this->GetTargetRepresentationName()));
#else
  vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(targetRepresentation);
#endif
  if (!closedSurfacePolyData)
  {
    vtkErrorMacro("Convert: Target representation is not a poly data");
    return false;
  }

  if (planarContoursPolyData->GetNumberOfLines() == 0)
  {
    return true;
  }

  // -----------------------------------------------------------------------
  // Rasterization-based conversion: fill each contour on its slice into a
  // binary volume, then extract a closed surface using marching cubes.
  // -----------------------------------------------------------------------

  // Transform contours so that contour normals are aligned with the Z axis.
  vtkSmartPointer<vtkMatrix4x4> contourToRASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  this->CalculateContourTransform(planarContoursPolyData, contourToRASMatrix);

  vtkSmartPointer<vtkTransform> transformRASToContours = vtkSmartPointer<vtkTransform>::New();
  transformRASToContours->SetMatrix(contourToRASMatrix);
  vtkNew<vtkTransformPolyDataFilter> transformRASToContoursFilter;
  transformRASToContoursFilter->SetInputData(planarContoursPolyData);
  transformRASToContoursFilter->SetTransform(transformRASToContours);
  transformRASToContoursFilter->Update();

  vtkSmartPointer<vtkPolyData> alignedContours = vtkSmartPointer<vtkPolyData>::New();
  alignedContours->DeepCopy(transformRASToContoursFilter->GetOutput());

  // Get the contour spacing and bounding box.
  double contourSpacing = this->GetSpacingBetweenLines(alignedContours);
  double bounds[6] = { 0, 0, 0, 0, 0, 0 };
  alignedContours->GetBounds(bounds);

  // -----------------------------------------------------------------------
  // Step 1: Group contours by unique Z value (with tolerance).
  // -----------------------------------------------------------------------
  int numberOfLines = alignedContours->GetNumberOfLines();
  std::vector<double> contourZValues(numberOfLines);
  for (int lineIndex = 0; lineIndex < numberOfLines; ++lineIndex)
  {
    double cellBounds[6] = { 0, 0, 0, 0, 0, 0 };
    alignedContours->GetCell(lineIndex)->GetBounds(cellBounds);
    contourZValues[lineIndex] = (cellBounds[4] + cellBounds[5]) / 2.0;
  }

  struct ContourGroup
  {
    double z;
    std::vector<int> lineIndices;
  };
  std::vector<ContourGroup> groups;
  {
    std::vector<std::pair<double, int>> sorted;
    sorted.reserve(numberOfLines);
    for (int i = 0; i < numberOfLines; ++i)
    {
      sorted.push_back({ contourZValues[i], i });
    }
    std::sort(sorted.begin(), sorted.end());

    const double zTol = 0.05; // mm tolerance for co-planar grouping
    for (const auto& p : sorted)
    {
      if (groups.empty() || std::abs(p.first - groups.back().z) > zTol)
      {
        ContourGroup g;
        g.z = p.first;
        g.lineIndices.push_back(p.second);
        groups.push_back(g);
      }
      else
      {
        groups.back().lineIndices.push_back(p.second);
      }
    }
  }

  int numGroups = static_cast<int>(groups.size());
  if (numGroups == 0)
  {
    return true;
  }

  // -----------------------------------------------------------------------
  // Step 2: Compute isotropic image spacing for the rasterization volume.
  // -----------------------------------------------------------------------
  double xRange = bounds[1] - bounds[0];
  double yRange = bounds[3] - bounds[2];
  double maxRange = std::max(xRange, yRange);

  // Target ~256 voxels across the largest XY dimension, clamped to [0.5, 2.0] mm.
  double isoSpacing = maxRange / 256.0;
  isoSpacing = std::max(isoSpacing, 0.5);
  isoSpacing = std::min(isoSpacing, 2.0);

  double xyPad = 3.0 * isoSpacing;
  double zPad = contourSpacing;
  double origin[3] = {
    bounds[0] - xyPad,
    bounds[2] - xyPad,
    groups.front().z - zPad
  };

  int dimsX = static_cast<int>(std::ceil((bounds[1] + xyPad - origin[0]) / isoSpacing)) + 1;
  int dimsY = static_cast<int>(std::ceil((bounds[3] + xyPad - origin[1]) / isoSpacing)) + 1;
  int dimsZ = static_cast<int>(std::ceil((groups.back().z + zPad - origin[2]) / isoSpacing)) + 2;
  int sliceSize = dimsX * dimsY;

  // -----------------------------------------------------------------------
  // Step 3: Rasterize each contour group into a cached 2D binary slice.
  //         All contours on the same Z plane are combined using even-odd
  //         fill, so nested contours correctly create holes.
  // -----------------------------------------------------------------------
  std::vector<std::vector<unsigned char>> sliceFills(numGroups);
  for (int gi = 0; gi < numGroups; ++gi)
  {
    sliceFills[gi].resize(sliceSize, 0);

    vtkNew<vtkPoints> pts;
    vtkNew<vtkCellArray> lines;
    for (int lineIdx : groups[gi].lineIndices)
    {
      vtkCell* cell = alignedContours->GetCell(lineIdx);
      if (!cell || cell->GetNumberOfPoints() < 3)
      {
        continue;
      }
      vtkIdType nPts = cell->GetNumberOfPoints();
      lines->InsertNextCell(nPts);
      for (vtkIdType i = 0; i < nPts; ++i)
      {
        double pt[3];
        cell->GetPoints()->GetPoint(i, pt);
        vtkIdType pid = pts->InsertNextPoint(pt[0], pt[1], 0.0);
        lines->InsertCellPoint(pid);
      }
    }

    vtkNew<vtkPolyData> poly2d;
    poly2d->SetPoints(pts);
    poly2d->SetLines(lines);

    int ext2d[6] = { 0, dimsX - 1, 0, dimsY - 1, 0, 0 };
    double org2d[3] = { origin[0], origin[1], 0.0 };
    double sp2d[3] = { isoSpacing, isoSpacing, 1.0 };

    vtkNew<vtkImageData> sliceImg;
    sliceImg->SetSpacing(sp2d);
    sliceImg->SetExtent(ext2d);
    sliceImg->SetOrigin(org2d);
    sliceImg->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    std::memset(sliceImg->GetScalarPointer(), 0, sliceSize);

    vtkNew<vtkPolyDataToImageStencil> stencilFilter;
    stencilFilter->SetInputData(poly2d);
    stencilFilter->SetOutputSpacing(sp2d);
    stencilFilter->SetOutputOrigin(org2d);
    stencilFilter->SetOutputWholeExtent(ext2d);
    stencilFilter->Update();

    vtkNew<vtkImageStencil> stencil;
    stencil->SetInputData(sliceImg);
    stencil->SetStencilConnection(stencilFilter->GetOutputPort());
    stencil->ReverseStencilOn();
    stencil->SetBackgroundValue(1);
    stencil->Update();

    unsigned char* src = static_cast<unsigned char*>(stencil->GetOutput()->GetScalarPointer());
    std::memcpy(sliceFills[gi].data(), src, sliceSize);
  }

  // -----------------------------------------------------------------------
  // Step 4: Build a binary volume with nearest-neighbor fill.
  //         Per-group spacing prevents small/isolated contours from being
  //         stretched: disconnected groups extend only 1 voxel in Z,
  //         while connected groups extend half the distance to their
  //         overlapping neighbor.
  // -----------------------------------------------------------------------
  double imgSp[3] = { isoSpacing, isoSpacing, isoSpacing };
  int ext[6] = { 0, dimsX - 1, 0, dimsY - 1, 0, dimsZ - 1 };

  vtkNew<vtkImageData> floatVolume;
  floatVolume->SetSpacing(imgSp);
  floatVolume->SetExtent(ext);
  floatVolume->SetOrigin(origin);
  floatVolume->AllocateScalars(VTK_FLOAT, 1);
  size_t totalVoxels = static_cast<size_t>(dimsX) * dimsY * dimsZ;
  float* volData = static_cast<float*>(floatVolume->GetScalarPointer());
  std::memset(volData, 0, totalVoxels * sizeof(float));

  // Precompute whether consecutive groups overlap in XY.
  std::vector<bool> adjacentOverlap(numGroups > 1 ? numGroups - 1 : 0, false);
  for (int gi = 0; gi < numGroups - 1; ++gi)
  {
    const auto& f1 = sliceFills[gi];
    const auto& f2 = sliceFills[gi + 1];
    for (int i = 0; i < sliceSize; ++i)
    {
      if (f1[i] > 0 && f2[i] > 0)
      {
        adjacentOverlap[gi] = true;
        break;
      }
    }
  }

  // Per-group local half-spacing: connected groups extend to meet their
  // overlapping neighbor; disconnected groups get minimal extension (1 voxel)
  // so single-slice contours stay thin.
  std::vector<double> groupHalfSpacing(numGroups, isoSpacing);
  for (int gi = 0; gi < numGroups; ++gi)
  {
    double nearestOverlapDist = 1e9;
    if (gi + 1 < numGroups && adjacentOverlap[gi])
    {
      nearestOverlapDist = std::min(nearestOverlapDist,
        std::abs(groups[gi + 1].z - groups[gi].z));
    }
    if (gi - 1 >= 0 && adjacentOverlap[gi - 1])
    {
      nearestOverlapDist = std::min(nearestOverlapDist,
        std::abs(groups[gi].z - groups[gi - 1].z));
    }
    if (nearestOverlapDist < 1e9)
    {
      groupHalfSpacing[gi] = nearestOverlapDist * 0.5;
    }
    // else: stays at isoSpacing (one voxel) for isolated contours.
  }

  for (int z = 0; z < dimsZ; ++z)
  {
    double wz = origin[2] + z * isoSpacing;
    float* dst = volData + static_cast<size_t>(z) * sliceSize;

    // Find the nearest contour group.
    int nearestGi = -1;
    double nearestDist = 1e9;
    for (int gi = 0; gi < numGroups; ++gi)
    {
      double d = std::abs(wz - groups[gi].z);
      if (d < nearestDist)
      {
        nearestDist = d;
        nearestGi = gi;
      }
    }
    if (nearestGi < 0 || nearestDist > groupHalfSpacing[nearestGi])
    {
      continue;
    }

    const auto& fill = sliceFills[nearestGi];

    // Collect adjacent overlapping groups for per-pixel check.
    std::vector<int> adjIndices;
    if (nearestGi + 1 < numGroups && adjacentOverlap[nearestGi])
    {
      adjIndices.push_back(nearestGi + 1);
    }
    if (nearestGi - 1 >= 0 && adjacentOverlap[nearestGi - 1])
    {
      adjIndices.push_back(nearestGi - 1);
    }

    for (int i = 0; i < sliceSize; ++i)
    {
      if (fill[i] == 0)
      {
        continue;
      }
      // Per-pixel overlap check: pixels that overlap with an adjacent
      // group's fill are part of a multi-slice structure and get normal
      // extension.  Pixels that DON'T overlap are isolated on this slice
      // and only get 1-voxel extension to avoid stretching.
      bool pixelConnected = false;
      for (int adj : adjIndices)
      {
        if (sliceFills[adj][i] > 0)
        {
          pixelConnected = true;
          break;
        }
      }
      if (pixelConnected || nearestDist <= isoSpacing)
      {
        dst[i] = 1.0f;
      }
    }
  }

  // -----------------------------------------------------------------------
  // Step 5: Low-pass filter the binary volume before surface extraction.
  //         This smooths out the sharp voxel edges caused by discrete slice
  //         sampling, producing a clean surface directly from marching cubes.
  //         Use max(smoothed, original) so the filter only ADDS material
  //         (fills concavities) and never erodes small contours.
  // -----------------------------------------------------------------------
  vtkNew<vtkImageGaussianSmooth> gaussianSmooth;
  gaussianSmooth->SetInputData(floatVolume);
  gaussianSmooth->SetStandardDeviation(
    isoSpacing * 1.0, isoSpacing * 1.0, contourSpacing * 0.35);
  gaussianSmooth->SetRadiusFactor(3.0);
  gaussianSmooth->Update();

  vtkImageData* smoothedVol = gaussianSmooth->GetOutput();
  float* smoothedData = static_cast<float*>(smoothedVol->GetScalarPointer());
  for (size_t i = 0; i < totalVoxels; ++i)
  {
    if (volData[i] > smoothedData[i])
    {
      smoothedData[i] = volData[i];
    }
  }

  // -----------------------------------------------------------------------
  // Step 6: Extract closed surface using marching cubes on the smoothed volume.
  // -----------------------------------------------------------------------
  vtkNew<vtkMarchingCubes> marchingCubes;
  marchingCubes->SetInputData(smoothedVol);
  marchingCubes->SetValue(0, 0.5);
  marchingCubes->ComputeNormalsOn();
  marchingCubes->Update();

  // Minimal cleanup only — no mesh smoothing.
  vtkNew<vtkCleanPolyData> cleaner;
  cleaner->SetInputConnection(marchingCubes->GetOutputPort());
  cleaner->Update();

  // Transform back to RAS coordinates.
  vtkSmartPointer<vtkTransform> transformContoursToRAS = vtkSmartPointer<vtkTransform>::New();
  transformContoursToRAS->SetMatrix(contourToRASMatrix);
  transformContoursToRAS->Inverse();

  vtkNew<vtkTransformPolyDataFilter> transformPolyDataToRASFilter;
  transformPolyDataToRASFilter->SetInputConnection(cleaner->GetOutputPort());
  transformPolyDataToRASFilter->SetTransform(transformContoursToRAS);
  transformPolyDataToRASFilter->Update();

  // Clean up the output mesh.
  vtkNew<vtkCleanPolyData> cleanFilter;
  cleanFilter->SetInputConnection(transformPolyDataToRASFilter->GetOutputPort());
  cleanFilter->Update();

  closedSurfacePolyData->DeepCopy(cleanFilter->GetOutput());

  return true;
}

//----------------------------------------------------------------------------
double vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::GetSpacingBetweenLines(vtkPolyData* inputROIPoints)
{
  double defaultSliceThickness = this->ConversionParameters->GetValueAsDouble("Default slice thickness");

  // Use a reasonable minimum fallback if the default slice thickness parameter is zero or negative.
  const double minimumSliceThickness = 1.0; // 1.0 mm fallback
  if (defaultSliceThickness <= 0.0)
  {
    defaultSliceThickness = minimumSliceThickness;
  }

  if (!inputROIPoints)
  {
    vtkErrorMacro("GetSpacingBetweenLines: Invalid vtkPolyData");
    return defaultSliceThickness;
  }
  if (inputROIPoints->GetNumberOfCells() < 2)
  {
    vtkWarningMacro("GetSpacingBetweenLines: Input polydata has less than two cells."
      " Unable to calculate spacing, using default slice thickness: " << defaultSliceThickness << " mm.");
    return defaultSliceThickness;
  }

  // Vector containing the distance between lines on adjacent contour slices.
  std::vector<double> distances;
  double distanceSum = 0.0;

  // Loop through all of the lines
  vtkIdType lineId = 0;
  while (lineId < inputROIPoints->GetNumberOfLines() - 1)
  {
    // First line
    vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
    line1->DeepCopy(inputROIPoints->GetCell(lineId));

    double line1Bounds[6] = { 0, 0, 0, 0, 0, 0 };
    line1->GetBounds(line1Bounds);

    // Second line
    vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
    line2->DeepCopy(inputROIPoints->GetCell(lineId + 1));

    double line2Bounds[6] = { 0, 0, 0, 0, 0, 0 };
    line2->GetBounds(line2Bounds);

    // Calculate the distance as the difference between the z value in the middle of the bounding boxes.
    double distance = std::abs((line1Bounds[4] + line1Bounds[5]) / 2 - (line2Bounds[4] + line2Bounds[5]) / 2);

    // If the distance between the lines is not zero, add it to the list
    if (distance > 0.01)
    {
      distances.push_back(distance);
      distanceSum += distance;
    }
    ++lineId;
  }

  if (distances.size() == 0)
  {
    return defaultSliceThickness;
  }

  // Calculate the mean distance between the lines.
  double distanceMean = distanceSum / distances.size();

  distanceSum = 0;
  int convergentCount = 0;
  for (std::vector<double>::iterator distIt = distances.begin(); distIt != distances.end(); ++distIt)
  {
    double distance = (*distIt);
    if (std::abs(distance - distanceMean) >= distanceMean / 10)
    {
      continue;
    }
    distanceSum += distance;
    ++convergentCount;
  }

  if (convergentCount == 0)
  {
    vtkWarningMacro("GetSpacingBetweenLines: Contour spacing is not consistent.");
    return distanceMean;
  }

  distanceMean = distanceSum / convergentCount;
  return distanceMean;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::CalculateContourTransform(
  vtkPolyData* inputPolyData, vtkMatrix4x4* contourToRAS)
{
  double zVector[3] = { 0, 0, 1 };
  double meshNormal[3] = { 0, 0, 0 };

  // Calculate the normal vector for the surface mesh by iterating through all of the contours.
  // Ignores small contours (less than 6 points) since the plane fitting is sometimes unreliable.
  this->CalculateContourNormal(inputPolyData, meshNormal, 6);
  if (vtkMath::Norm(meshNormal) == 0.0)
  {
    // If the normal vector that was returned was zero (no contours with more than 6 points),
    // run the calculation again with no restriction on the number of points used.
    this->CalculateContourNormal(inputPolyData, meshNormal, 0);
  }

  double theta = std::acos(vtkMath::Dot(meshNormal, zVector)) * 180.0 / vtkMath::Pi();
  double axis[3] = { 0, 0, 0 };
  vtkMath::Cross(meshNormal, zVector, axis);

  vtkNew<vtkTransform> transform;
  transform->RotateWXYZ(theta, axis);
  contourToRAS->DeepCopy(transform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceByRasterizationConversionRule::CalculateContourNormal(
  vtkPolyData* inputPolyData, double outputNormal[3], int minimumContourSize)
{
  vtkSmartPointer<vtkIdList> currentContour = vtkSmartPointer<vtkIdList>::New();

  double averageNormalSum[3] = { 0, 0, 0 };

  int numberOfSmallContours = 0;
  int currentContourIndex = 0;
  inputPolyData->GetLines()->InitTraversal();
  while (inputPolyData->GetLines()->GetNextCell(currentContour))
  {
    double contourNormal[3] = { 0, 0, 0 };

    vtkNew<vtkExtractCells> extract;
    extract->SetInputData(inputPolyData);
    extract->AddCellRange(currentContourIndex, currentContourIndex);
    extract->Update();
    if (extract->GetOutput()->GetNumberOfPoints() > minimumContourSize)
    {
      vtkNew<vtkPlane> plane;
      vtkAddonMathUtilities::FitPlaneToPoints(extract->GetOutput()->GetPoints(), plane);
      plane->GetNormal(contourNormal);
      // Ensure that the normal faces the same direction as the average normal
      if (vtkMath::Dot(contourNormal, averageNormalSum) < 0.0)
      {
        vtkMath::MultiplyScalar(contourNormal, -1.0);
      }
      vtkMath::Add(contourNormal, averageNormalSum, averageNormalSum);
    }
    else
    {
      ++numberOfSmallContours;
    }
    ++currentContourIndex;
  }

  // All contours had less than the minimum number of points.
  if (numberOfSmallContours >= inputPolyData->GetNumberOfLines() || vtkMath::Norm(averageNormalSum) == 0.0)
  {
    return;
  }

  for (int i = 0; i < 3; ++i)
  {
    outputNormal[i] = averageNormalSum[i] / (inputPolyData->GetNumberOfLines() - numberOfSmallContours);
  }
  vtkMath::Normalize(outputNormal);
}
