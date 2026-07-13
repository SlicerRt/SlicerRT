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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkPlanarContourToRibbonModelConversionRule.h"

// VTK includes
#include <vtkObjectFactory.h>

// STD includes
#include <algorithm>
#include <iostream>
#include <vtkVector.h>
#include <vtkPolyData.h>
#include <vtkCell.h>
#include <vtkIdList.h>
#include <vtkPlane.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkRibbonFilter.h>
#include <vtkMath.h>

#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
// SegmentationCore includes
#include <vtkSegment.h>
#endif

//----------------------------------------------------------------------------
// Utility functions
namespace
{
  #if VTK_MAJOR_VERSION < 9 || (VTK_MAJOR_VERSION == 9 && VTK_MINOR_VERSION < 4)

  vtkVector3d operator -(const vtkVector3d& a, const vtkVector3d& b)
  {
    vtkVector3d result;
    result.SetX(a.GetX() - b.GetX());
    result.SetY(a.GetY() - b.GetY());
    result.SetZ(a.GetZ() - b.GetZ());
    return result;
  }
  #endif

  bool AreEqualWithTolerance(double a, double b)
  {
    return fabs(a - b) < EPSILON;
  }

  double MajorityValue(const std::vector<double>& spacingValues, std::string &outputMessage)
  {
    std::map<double, int> spacingValueFrequencies;
    double averageTotal(0.0);
    double averageCount(0.0);
    for (std::vector<double>::const_iterator it = spacingValues.begin(); it != spacingValues.end(); ++it)
    {
      double roundedSpacingValue = vtkMath::Round((*it) / EPSILON) * EPSILON; // Round value to consider spacings within tolerance the same
      averageTotal += *it;
      averageCount++;
      spacingValueFrequencies[roundedSpacingValue]++;
    }
    double majorityValue(0.0);
    int majorityCount(-1);
    std::stringstream outputStringStream;
    for (std::map<double, int>::iterator it = spacingValueFrequencies.begin(); it != spacingValueFrequencies.end(); ++it)
    {
      outputStringStream << "  Planar spacing: " << it->first << ". Frequency: " << it->second << "." << std::endl;
      if (it->second > majorityCount)
      {
        majorityValue = it->first;
        majorityCount = it->second;
      }
    }
    outputMessage = outputStringStream.str();
    return majorityValue;
  }
}

//----------------------------------------------------------------------------
vtkSegmentationConverterRuleNewMacro(vtkPlanarContourToRibbonModelConversionRule);

//----------------------------------------------------------------------------
vtkPlanarContourToRibbonModelConversionRule::vtkPlanarContourToRibbonModelConversionRule()
{
  //this->ConversionParameters[GetXYParameterName()] = std::make_pair("value", "description");
}

//----------------------------------------------------------------------------
vtkPlanarContourToRibbonModelConversionRule::~vtkPlanarContourToRibbonModelConversionRule() = default;

//----------------------------------------------------------------------------
unsigned int vtkPlanarContourToRibbonModelConversionRule::GetConversionCost(vtkDataObject* vtkNotUsed(sourceRepresentation)/*=nullptr*/, vtkDataObject* vtkNotUsed(targetRepresentation)/*=nullptr*/)
{
  // Rough input-independent guess (ms)
  // Increase to quasi infinity, so that this path is never chosen by default, but the user has to manually request it.
  // This is due to the superiority of the triangulated and end-capped closed surface path.
  return vtkSegmentationConverterRule::GetConversionInfiniteCost();
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToRibbonModelConversionRule::ConstructRepresentationObjectByRepresentation(std::string representationName)
{
  if ( !representationName.compare(this->GetSourceRepresentationName())
    || !representationName.compare(this->GetTargetRepresentationName()) )
  {
    return (vtkDataObject*)vtkPolyData::New();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToRibbonModelConversionRule::ConstructRepresentationObjectByClass(std::string className)
{
  if (!className.compare("vtkPolyData"))
  {
    return (vtkDataObject*)vtkPolyData::New();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
bool vtkPlanarContourToRibbonModelConversionRule::Convert(vtkSegment* segment)
{
  this->CreateTargetRepresentation(segment);
#else
bool vtkPlanarContourToRibbonModelConversionRule::Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation)
{
#endif
  // Check validity of source and target representation objects
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  vtkPolyData* planarContourPolyData = vtkPolyData::SafeDownCast(segment->GetRepresentation(this->GetSourceRepresentationName()));
#else
  vtkPolyData* planarContourPolyData = vtkPolyData::SafeDownCast(sourceRepresentation);
#endif
  if (!planarContourPolyData)
  {
    vtkErrorMacro("Convert: Source representation is not a poly data!");
    return false;
  }
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  vtkPolyData* ribbonModelPolyData = vtkPolyData::SafeDownCast(segment->GetRepresentation(this->GetTargetRepresentationName()));
#else
  vtkPolyData* ribbonModelPolyData = vtkPolyData::SafeDownCast(targetRepresentation);
#endif
  if (!ribbonModelPolyData)
  {
    vtkErrorMacro("Convert: Target representation is not a poly data!");
    return false;
  }
  if (planarContourPolyData->GetNumberOfPoints() < 3 || planarContourPolyData->GetNumberOfCells() < 1)
  {
    vtkErrorMacro("Convert: Cannot create ribbon model from planar contour with number of points: " << planarContourPolyData->GetNumberOfPoints() << " and number of cells: " << planarContourPolyData->GetNumberOfCells());
    return false;
  }

  // Compute plane spacing of contours
  vtkSmartPointer<vtkPlane> contoursPlane = vtkSmartPointer<vtkPlane>::New();
  double sliceThickness = this->ComputeContourPlaneSpacing(planarContourPolyData, contoursPlane);

  // Remove coincident points (if there are multiple contour points at the same position then the ribbon filter fails)
  vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputData(planarContourPolyData);

  // Convert to ribbon using vtkRibbonFilter
  vtkSmartPointer<vtkRibbonFilter> ribbonFilter = vtkSmartPointer<vtkRibbonFilter>::New();
  ribbonFilter->SetInputConnection(cleaner->GetOutputPort());
  ribbonFilter->SetDefaultNormal(contoursPlane->GetNormal());
  ribbonFilter->UseDefaultNormalOn();
  ribbonFilter->SetWidth(sliceThickness / 2.0);
  ribbonFilter->SetAngle(90.0);
  ribbonFilter->Update();

  vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalFilter->SetInputConnection(ribbonFilter->GetOutputPort());
  normalFilter->ConsistencyOn();
  normalFilter->Update();

  ribbonModelPolyData->DeepCopy(normalFilter->GetOutput());

  return true;
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToRibbonModelConversionRule::ComputePlaneForContour(vtkPoints* contourPoints, vtkCell* contourCell, vtkPlane* contourPlane)
{
  if (!contourPoints || !contourCell || !contourPlane)
  {
    vtkErrorMacro("ComputePlaneForContour: Invalid arguments!");
    return false;
  }

  // Get point indices
  vtkIdList* currentContourPointIds = contourCell->GetPointIds();
  int numberOfPoints = currentContourPointIds->GetNumberOfIds();
  if (numberOfPoints < 3)
  {
    vtkWarningMacro("ComputePlaneForContour: Contour does not contain enough points to extract a planar equation.");
    return false;
  }

  // Compute equation of the current plane from the first point, and two other points about equal number of points away from the first one.
  // This heuristic method attempts to get an as accurate plane normal vector as possible by maximizing expected distance between plane points.
  int firstPointIndex = currentContourPointIds->GetId(0);
  int secondPointIndex = currentContourPointIds->GetId(numberOfPoints / 3);
  int thirdPointIndex = currentContourPointIds->GetId(2 * numberOfPoints / 3);
  // Sanity check of indices
  if (secondPointIndex == firstPointIndex || secondPointIndex == thirdPointIndex)
  {
    vtkErrorMacro("ComputePlaneForContour: Invalid contour point indices for computing plane equation. This is probably a bug, please report!");
    return false;
  }

  // Get plane points
  double firstPointArray[3] = {0.0,0.0,0.0};
  contourPoints->GetPoint(firstPointIndex, firstPointArray);
  double secondPointArray[3] = {0.0,0.0,0.0};
  contourPoints->GetPoint(secondPointIndex, secondPointArray);
  double thirdPointArray[3] = {0.0,0.0,0.0};
  contourPoints->GetPoint(thirdPointIndex, thirdPointArray);
  vtkVector3d firstPlanePoint(firstPointArray[0], firstPointArray[1], firstPointArray[2]);
  vtkVector3d secondPlanePoint(secondPointArray[0], secondPointArray[1], secondPointArray[2]);
  vtkVector3d thirdPlanePoint(thirdPointArray[0], thirdPointArray[1], thirdPointArray[2]);
  // Compute plane vectors: two vectors on the plane, and the normal
  vtkVector3d currentPlaneIVector(secondPlanePoint - firstPlanePoint);
  vtkVector3d currentPlaneJVector(thirdPlanePoint - firstPlanePoint);
  vtkVector3d currentPlaneKVector(currentPlaneIVector.Cross(currentPlaneJVector));

  // Attempt to compute plane equation with brute force
  // if the heuristic choice of plane points produces collinear vectors
  if (currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0)
  {
    for (int pointIndex=0; pointIndex<numberOfPoints-2; ++pointIndex)
    {
      // The point number counted here is relative to this contour alone, but the underlying point list
      // holds every contour's points one after another. That relative number has to be converted to its
      // real position in the shared list before use; skipping that conversion silently reads whichever
      // contour's points happen to occupy those early positions (in practice, the very first contour in
      // the structure), so the plane ends up being computed from an unrelated contour's coordinates
      // instead of this one's.
      contourPoints->GetPoint(currentContourPointIds->GetId(pointIndex), firstPointArray);
      contourPoints->GetPoint(currentContourPointIds->GetId(pointIndex+1), secondPointArray);
      contourPoints->GetPoint(currentContourPointIds->GetId(pointIndex+2), thirdPointArray);
      firstPlanePoint = vtkVector3d(firstPointArray[0], firstPointArray[1], firstPointArray[2]);
      secondPlanePoint = vtkVector3d(secondPointArray[0], secondPointArray[1], secondPointArray[2]);
      thirdPlanePoint = vtkVector3d(thirdPointArray[0], thirdPointArray[1], thirdPointArray[2]);
      currentPlaneIVector = secondPlanePoint - firstPlanePoint;
      currentPlaneJVector = thirdPlanePoint - firstPlanePoint;
      currentPlaneKVector = currentPlaneIVector.Cross(currentPlaneJVector);

      if (!(currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0))
      {
        break;
      }
    }

    // Check for valid plane vector and report failure if all attempts failed to produce a valid one
    if (currentPlaneKVector.GetX() == 0 && currentPlaneKVector.GetY() == 0 && currentPlaneKVector.GetZ() == 0)
    {
      vtkErrorMacro("ComputePlaneForContour: All points in contour plane produce co-linear vectors. Unable to determine equation of the plane.");
      return false;
    }
  }

  // Normalize normal vector
  currentPlaneKVector.Normalize();

  // Setup output computed plane
  contourPlane->SetNormal(currentPlaneKVector.GetX(), currentPlaneKVector.GetY(), currentPlaneKVector.GetZ());
  contourPlane->SetOrigin(firstPlanePoint.GetX(), firstPlanePoint.GetY(), firstPlanePoint.GetZ());

  return true;
}

//----------------------------------------------------------------------------
double vtkPlanarContourToRibbonModelConversionRule::ComputeContourPlaneSpacing(vtkPolyData* planarContourPolyData, vtkPlane* contoursPlane)
{
  // Check input planar contour for suitable number of planes
  int numberOfPlanes = planarContourPolyData->GetNumberOfCells();
  if (!numberOfPlanes)
  {
    vtkErrorMacro("ComputeContourPlaneSpacing: Empty planar contour. Returning default spacing 1.0.");
    return 1.0;
  }
  if (numberOfPlanes < 2)
  {
    vtkErrorMacro("ComputeContourPlaneSpacing: Less than two planes detected, thus it is not possible to calculate plane spacing. Returning default spacing 1.0");
    return 1.0;
  }

  // Get all contour points
  vtkPoints* points = planarContourPolyData->GetPoints();

  // Computed output variables
  std::vector<double> planeSpacingValues;
  bool consistentPlaneSpacing = true;
  double distanceBetweenContourPlanes = -1.0; // The found distance between planes if consistent

  // All contour planes, with their distances from the first plane
  std::map< vtkSmartPointer<vtkPlane>, double > contourPlanes;
  double firstNormal[3] = {0.0,0.0,0.0};
  double firstOrigin[3] = {0.0,0.0,0.0};

  // Iterate over each contour in the set, computing planar spacing values for every contour
  for (int contourIndex = 0; contourIndex < planarContourPolyData->GetNumberOfCells(); ++contourIndex)
  {
    // Get contour cell
    vtkCell* currentContour = planarContourPolyData->GetCell(contourIndex);

    // Compute contour plane
    vtkSmartPointer<vtkPlane> currentContourPlane = vtkSmartPointer<vtkPlane>::New();
    bool validPlane = this->ComputePlaneForContour(points, currentContour, currentContourPlane);
    if (!validPlane)
    {
      vtkWarningMacro("ComputeContourPlaneSpacing: Unable to calculate plane for contour " << contourIndex << ". Skipping contour.");
      continue;
    }

    // Store first plane parameters to compute distances
    if (contourPlanes.empty())
    {
      currentContourPlane->GetNormal(firstNormal);
      currentContourPlane->GetOrigin(firstOrigin);
      contourPlanes[currentContourPlane] = 0.0;

      // Set first valid plane as output contours plane
      if (contoursPlane)
      {
        contoursPlane->SetOrigin(currentContourPlane->GetOrigin());
        contoursPlane->SetNormal(currentContourPlane->GetNormal());
      }
    }
    // Compute distance from first plane (also check if they are parallel)
    else
    {
      double normal[3] = {0.0,0.0,0.0};
      currentContourPlane->GetNormal(normal);
      // We accept normal vectors with the exact opposite direction, so we accept if their dot product is 1 or -1 (normal vectors have magnitude of 1)
      double dotProduct = (normal[0]*firstNormal[0]) + (normal[1]*firstNormal[1]) + (normal[2]*firstNormal[2]);
      if (!AreEqualWithTolerance(fabs(dotProduct), 1.0))
      {
        vtkErrorMacro("ComputeContourPlaneSpacing: Contour planes in structures set are not parallel!");
      }

      // Store distance of current plane from first plane
      double distanceFromFirstPlane = vtkPlane::DistanceToPlane(firstOrigin, normal, currentContourPlane->GetOrigin());
      contourPlanes[currentContourPlane] = distanceFromFirstPlane;
    }
  } // For all contour planes

  // Order computed contour planes
  std::map< double, vtkSmartPointer<vtkPlane> > orderedContourPlanes;
  std::map< vtkSmartPointer<vtkPlane>, double >::iterator planesIt;
  for (planesIt=contourPlanes.begin(); planesIt != contourPlanes.end(); ++planesIt)
  {
    // Swap map to have the ordering by distance from first plane
    orderedContourPlanes[planesIt->second] = planesIt->first;
  }

  // Relative tolerance for downgrading inconsistency messages to debug level: deviations smaller than
  // 0.1% of the total contour span cannot be avoided due to the limited precision of DICOM decimal strings,
  // so they do not warrant a warning
  double totalContourSpan = orderedContourPlanes.empty() ? 0.0 : (orderedContourPlanes.rbegin()->first - orderedContourPlanes.begin()->first);
  double relativeSpacingTolerance = std::max(EPSILON, 0.001 * totalContourSpan);

  // Compute distances between adjacent planes
  std::map< double, vtkSmartPointer<vtkPlane> >::iterator orderedPlanesIt;
  double previousDistance = 0.0;
  for (orderedPlanesIt=orderedContourPlanes.begin(); orderedPlanesIt != orderedContourPlanes.end(); ++orderedPlanesIt)
  {
    if (orderedPlanesIt != orderedContourPlanes.begin()) // We skip the first one, just save its distance as previous
    {
      double currentDistance = fabs(orderedPlanesIt->first - previousDistance);
      if (!AreEqualWithTolerance(currentDistance, 0.0))
      {
        // Only add spacing value if it's not 0 - multiple contours may be drawn on the same plane and it's not considered for slice thickness computation
        planeSpacingValues.push_back(currentDistance);

        // Store current spacing as found spacing if has not been set
        if (distanceBetweenContourPlanes == -1.0)
        {
          distanceBetweenContourPlanes = currentDistance;
        }

        // Check for inconsistency
        if ( !AreEqualWithTolerance(currentDistance, distanceBetweenContourPlanes)
          && consistentPlaneSpacing ) // Only prompt the warning once
        {
          if (fabs(currentDistance - distanceBetweenContourPlanes) < relativeSpacingTolerance)
          {
            vtkDebugMacro("ComputeContourPlaneSpacing: Contour does not have consistent plane spacing (" << currentDistance << " != " << distanceBetweenContourPlanes << "). Using majority spacing value.");
          }
          else
          {
            vtkWarningMacro("ComputeContourPlaneSpacing: Contour does not have consistent plane spacing (" << currentDistance << " != " << distanceBetweenContourPlanes << "). Using majority spacing value.");
          }
          consistentPlaneSpacing = false;
        }
      } // If non-zero
    }
    previousDistance = orderedPlanesIt->first;
  }

  // Calculate the majority value for the plane spacing from plane spacing values if inconsistent spacing was found
  if (!consistentPlaneSpacing)
  {
    std::string message("");
    distanceBetweenContourPlanes = MajorityValue(planeSpacingValues, message);

    // Only warn if any spacing value deviates from the majority by more than the relative tolerance;
    // smaller deviations are expected due to DICOM decimal string precision limits and are not actionable
    double maxDeviation = 0.0;
    for (std::vector<double>::const_iterator valueIt = planeSpacingValues.begin(); valueIt != planeSpacingValues.end(); ++valueIt)
    {
      maxDeviation = std::max(maxDeviation, fabs(*valueIt - distanceBetweenContourPlanes));
    }
    if (maxDeviation < relativeSpacingTolerance)
    {
      vtkDebugMacro("ComputeContourPlaneSpacing: Inconsistent plane spacing. Details:\n" << message << "Used contour spacing: " << distanceBetweenContourPlanes);
    }
    else
    {
      vtkWarningMacro("ComputeContourPlaneSpacing: Inconsistent plane spacing. Details:\n" << message << "Used contour spacing: " << distanceBetweenContourPlanes);
    }
  }

  return distanceBetweenContourPlanes;
}
