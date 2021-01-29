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

#ifndef __vtkPlanarContourToClosedSurfaceConversionRule_h
#define __vtkPlanarContourToClosedSurfaceConversionRule_h

// Slicer include
#include <vtkSlicerVersionConfigure.h>

// SegmentationCore includes
#include "vtkSegmentationConverterRule.h"
#include "vtkSegmentationConverter.h"

#include "vtkSlicerDicomRtImportExportConversionRulesExport.h"

// VTK includes
#include "vtkPointLocator.h"

class vtkPolyData;
class vtkIdList;
class vtkCellArray;
class vtkLine;
class vtkPoints;
class vtkPriorityQueue;

/// \ingroup DicomRtImportImportExportConversionRules
/// \brief Convert planar contour representation (vtkPolyData type) to
///   closed surface representation (also vtkPolyData type). The conversion algorithm
///   triangulates the contour points while handling special cases (keyhole contours,
///   rapid changes, and branching).
///   Paper about the method:
///   http://perk.cs.queensu.ca/contents/reconstruction-surfaces-planar-contours-through-contour-interpolation
///
class VTK_SLICER_DICOMRTIMPORTEXPORT_CONVERSIONRULES_EXPORT vtkPlanarContourToClosedSurfaceConversionRule
  : public vtkSegmentationConverterRule
{
public:
  static vtkPlanarContourToClosedSurfaceConversionRule *New();
  vtkTypeMacro(vtkPlanarContourToClosedSurfaceConversionRule, vtkSegmentationConverterRule);
  vtkSegmentationConverterRule* CreateRuleInstance() override;

  static const std::string GetDefaultSliceThicknessParameterName() { return "Default slice thickness"; };
  static const std::string GetEndCappingParameterName() { return "End capping"; };
  enum EndCappingModes
  {
    None = 0,
    Smooth = 1,
    Straight = 2
  };

  /// Constructs representation object from representation name for the supported representation classes
  /// (typically source and target representation VTK classes, subclasses of vtkDataObject)
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  vtkDataObject* ConstructRepresentationObjectByRepresentation(std::string representationName) override;

  /// Constructs representation object from class name for the supported representation classes
  /// (typically source and target representation VTK classes, subclasses of vtkDataObject)
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  vtkDataObject* ConstructRepresentationObjectByClass(std::string className) override;

  /// Update the target representation based on the source representation
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  bool Convert(vtkSegment* segment) override;
#else
  bool Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation) override;
#endif

  /// Get the cost of the conversion.
  unsigned int GetConversionCost(vtkDataObject* sourceRepresentation = nullptr, vtkDataObject* targetRepresentation = nullptr) override;

  /// Human-readable name of the converter rule
  const char* GetName() override { return "Planar contour to closed surface"; };

  /// Human-readable name of the source representation
  const char* GetSourceRepresentationName() override { return vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName(); };

  /// Human-readable name of the target representation
  const char* GetTargetRepresentationName() override { return vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(); };

protected:
  vtkPlanarContourToClosedSurfaceConversionRule();
  ~vtkPlanarContourToClosedSurfaceConversionRule() override;

  /// Construct a surface triangulation between two lines using a dynamic programming algorithm.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param pointsInLine1 List of points that are contained in the line to be triangulated
  /// \param pointsInLine2 List of points that are contained in the line to be triangulated
  /// \param Cell array that polygons are added to by the triangulation algorithm
  void TriangulateBetweenContours(vtkPolyData* inputROIPoints, vtkIdList* pointsInLine1, vtkIdList* pointsInLine2, vtkCellArray* outputPolygons);

  /// Find the index of the last point in a contour.
  /// \param startLoopIndex The index of the first point in the contour
  /// \param numberOfPoints The number of points in the contour
  /// \param loopClosed Boolean indicating if the loop is closed or not
  /// \return The index of the last point in the contour
  vtkIdType GetEndLoop(vtkIdType startLoopIndex, int numberOfPoints, bool loopClosed);

  /// Find the point on the given line that is closest to the given point.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param originalPoint The point that is being compared to the line
  /// \param linePointIds The line that is being compared to the point
  /// \return The index of the point in the line that is closet to the specified point
  vtkIdType GetClosestPoint(vtkPolyData* inputROIPoints, double* originalPoint, vtkIdList* linePointIds);

  /// Sort the contours based on Z value.
  /// \param inputROIPoints Polydata containing all of the points and contours
  void SortContours(vtkPolyData* inputROIPoints);

  /// Remove the keyholes from the contours.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param The minimum distance between two points in mm before points are considered to be part of a keyhole
  /// \param The minimum number of seperation of indices between points before they can be part of a keyhole
  void FixKeyholes(vtkPolyData* inputROIPoints, double epsilon, int minimumSeperation);

  /// Set all of the lines to be oriented in the clockwise direction.
  /// \param inputROIPoints Polydata containing all of the points and contours
  void SetLinesCounterClockwise(vtkPolyData* inputROIPoints);

  /// Determine if a line runs in a clockwise orientation.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param line The line that is being checked
  bool IsLineClockwise(vtkPolyData* inputROIPoints, vtkLine* line);

  /// Reverse the orientation of a line from clockwise to counter-clockwise and vice versa.
  /// \param originalLine The line that is being reversed
  /// \param newLine The output reversed line
  void ReverseLine(vtkLine* originalLine, vtkLine* newLine);

  /// Determine the number of contours that share the same Z-coordinates.
  /// WARNING: This function requires that the normal vector of all contours is aligned with the Z-axis.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param originalLineIndex The index of the line that is part of the plane being checked
  /// \param spacing The spacing between lines
  int GetNumberOfLinesOnPlane(vtkPolyData* inputROIPoints, vtkIdType originalLineIndex, double spacing);

  /// Determine if two contours overlap in the XY axis.
  /// \param The first line
  /// \param The second line
  bool DoLinesOverlap(vtkLine* line1, vtkLine* line2);

  /// Create a branching pattern for overlapping contours.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param branchingLine The orignal line that is being divided
  /// \param currentLineId The ID of the current line in the input polydata that is being compared
  /// \param overlappingLineIds List of line IDs for lines that overlap with the current line
  /// \param pointLocators List of point locators for lines in the overlap list
  /// \param lineIdLists List of vtkIdLists for all of the lines in the overlap list
  /// \param outputLine The output branched line
  void Branch(vtkPolyData* inputROIPoints, vtkLine* branchingLine, vtkIdType currentLineId, std::vector< vtkIdType > overlappingLineIds, std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators, std::vector<vtkSmartPointer<vtkIdList> > lineIdLists, vtkLine* outputLine);

  /// Find the branch closest from the point on the trunk
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param originalPoint The point that is being compared
  /// \param overlappingLineIds List of line IDs for lines that overlap with the current line
  /// \param pointLocators List of point locators for lines in the overlap list
  /// \param lineIdLists List of vtkIdLists for all of the lines in the overlap list
  int GetClosestBranch(vtkPolyData* inputROIPoints, double* originalPoint, std::vector< vtkIdType > overlappingLineIds, std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators, std::vector<vtkSmartPointer<vtkIdList> > lineIdLists);

  /// Seal the exterior contours of the mesh.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param inputLines Lines
  /// \param outputPolygons
  /// \param lineTriganulatedToAbove
  /// \param lineTriganulatedToBelow
  void EndCapping(vtkPolyData* inputROIPoints, vtkCellArray* outputPolygons, std::vector< bool > lineTriganulatedToAbove, std::vector< bool > lineTriganulatedToBelow);

  /// Calculate the spacing between the lines in the polydata
  /// WARNING: This function requires that the normal vector of all contours is aligned with the Z-axis.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \return The size of the spacing between the contours
  double GetSpacingBetweenLines(vtkPolyData* inputROIPoints);

  /// Create an additional contour on the exterior of the surface to compensate for slice thickness.
  /// This step is generally called end-capping.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param inputLine The original line that needs to be extended
  /// \param outputLines Cell array containing all of the lines that are created by the algorithm
  /// \param The size of the spacing between the contours. Contours created by this function will be offset by 1/2 of this amount
  void CreateEndCapContour(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkCellArray* outputLines, double lineSpacing);

  /// Create smoothly interpolated end cap contours on the exterior of the surface.
  /// Called by CreateEndCapContour based on the "End capping" conversion parameter.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param inputLine The original line that needs to be extended
  /// \param outputLines Cell array containing all of the lines that are created by the algorithm
  /// \param The size of the spacing between the contours. Contours created by this function will be offset by 1/2 of this amount
  void CreateSmoothEndCapContour(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkCellArray* outputLines, double lineSpacing);

  /// Create straight extruded end cap contours on the exterior of the surface.
  /// Called by CreateEndCapContour based on the "End capping" conversion parameter.
  /// \param inputROIPoints Polydata containing all of the points and contours
  /// \param inputLine The original line that needs to be extended
  /// \param outputLines Cell array containing all of the lines that are created by the algorithm
  /// \param The size of the spacing between the contours. Contours created by this function will be offset by 1/2 of this amount
  void CreateStraightEndCapContour(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkCellArray* outputLines, double lineSpacing);

  /// Triangulate the interior of a contour on the xy plane.
  /// \param Contour that is being triangulated
  /// \param Cell array that the polygons are added to
  /// \param True if the normals are positive in the z direction, false if the normals are negative
  void TriangulateContourInterior(vtkLine* inputLine, vtkCellArray* outputPolys, bool normalsUp);

  /// Find the index of the next point in the contour.
  /// \param The location of the currentId
  /// \param The number of points in the contours
  /// \param Whether the loop is closed
  /// \return The id of the point that occurs next in the contour
  vtkIdType GetNextLocation(vtkIdType currentLocation, int numberOfPoints, bool loopClosed);

  /// Find the index of the next point in the contour.
  /// \param The location of the currentId
  /// \param The number of points in the contours
  /// \param Whether the loop is closed
  /// \return The id of the point that occurs previously in the contour
  vtkIdType GetPreviousLocation(vtkIdType currentLocation, int numberOfPoints, bool loopClosed);

  /// Remove points from the input lines, until there are no points with error less than VTK_DBL_EPSILON and
  /// the following is acheived (number of points in new line) / (number of points in old line) < decimation factor
  /// \param inputLines Lines to be decimated
  /// \param decimationFactor Represents the goal decimation
  void DecimateLines(vtkPolyData* inputPolyData, double decimationFactor);

  /// Remove a point from the line, and make sure the points in the line are still in order
  /// \param idList The idList that the point is to be removed from
  /// \param pointId The id of the point that is being removed
  void RemovePointDecimation(vtkIdList* idList, vtkIdType pointId);

  /// Calculate the distance of the specified point from the line defined by the two points on either side
  /// \param points Contains the point data
  /// \param lineIds Contains the current line
  /// \param pointId The id of the point
  /// \return Distance of the specified point from the line defined by the two points on either side
  double ComputeError(vtkPoints* points, vtkIdList* lineIds, vtkIdType pointId);

  /// Remove some points from the start and end of the line
  /// TODO: This step is based on trial and error, to fix an issue from the contour generated by
  ///       vtkMarchingSquares and vtkStripper. It will probably need to be revised when the true
  ///       cause of the problem is found
  /// \param inputLine The line that points are being removed from
  /// \param outputLine The line with points removed
  void FixLine(vtkLine* inputLine, vtkLine* outputLine);

  /// Attempt fix some errors in the contours generated by vtkMarchingSquares and vtkStripper.
  /// TODO: This step is based on trial and error, to fix an issue from the contour generated by
  ///       vtkMarchingSquares and vtkStripper. It will probably need to be revised when the true
  ///       cause of the problem is found
  /// \param inputLine The polydata containing the original lines
  /// \param outputLine The output polydata with the "fixed" lines
  void FixLines(vtkPolyData* inputLines, vtkPolyData* outputLines);

  /// Find the transform to align the contour normals with the Z-axis
  ///\param inputPolyData Polydata containing all of the points and contours
  ///\param contourToRAS Output transform
  void CalculateContourTransform(vtkPolyData* inputPolyData, vtkMatrix4x4* contourToRAS);

  /// Find the normal of all of the contours in the polydata
  ///\param inputPolyData Polydata containing all of the points and contours
  ///\param outputNormal Output normal
  ///\param minimumContourSize The minimum number of points in contours to be considered
  void CalculateContourNormal(vtkPolyData* inputPolyData, double outputNormal[3], int minimumContourSize);

protected:

  // Spacing that is used for the image in the end-capping process
  double DefaultSpacing[2];

  // Alternative dimensions used for the image in the end-capping process
  int AlternativeDimensions[3];

  // Image padding size that is used in the end-capping process
  int ImagePadding[3];

private:
  vtkPlanarContourToClosedSurfaceConversionRule(const vtkPlanarContourToClosedSurfaceConversionRule&) = delete;
  void operator=(const vtkPlanarContourToClosedSurfaceConversionRule&) = delete;
};

#endif
