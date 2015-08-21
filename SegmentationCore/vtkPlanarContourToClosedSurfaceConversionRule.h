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

// SegmentationCore includes
#include "vtkSegmentationConverterRule.h"
#include "vtkSegmentationConverter.h"

#include "vtkSegmentationCoreConfigure.h"

// VTK includes
#include "vtkPointLocator.h"

class vtkPolyData;
class vtkIdList;
class vtkCellArray;
class vtkLine;
class vtkPoints;

/// \ingroup SegmentationCore
/// \brief Convert planar contour representation (vtkPolyData type) to
///   closed surface representation (also vtkPolyData type). The conversion algorithm
///   triangulates the contour points while handling special cases (keyhole contours,
///   rapid changes, and branching).
///   Paper about the method:
///   http://perk.cs.queensu.ca/contents/reconstruction-surfaces-planar-contours-through-contour-interpolation
///
class vtkSegmentationCore_EXPORT vtkPlanarContourToClosedSurfaceConversionRule
  : public vtkSegmentationConverterRule
{
public:
  static vtkPlanarContourToClosedSurfaceConversionRule *New();
  vtkTypeMacro(vtkPlanarContourToClosedSurfaceConversionRule, vtkSegmentationConverterRule );
  virtual vtkSegmentationConverterRule* CreateRuleInstance();

  // /// Convert a set of contours into a surface mesh.
  // void ConvertContoursToMesh(vtkPolyData*, vtkPolyData*);

  /// Constructs representation object from representation name for the supported representation classes
  /// (typically source and target representation VTK classes, subclasses of vtkDataObject)
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  virtual vtkDataObject* ConstructRepresentationObjectByRepresentation(std::string representationName);

  /// Constructs representation object from class name for the supported representation classes
  /// (typically source and target representation VTK classes, subclasses of vtkDataObject)
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  virtual vtkDataObject* ConstructRepresentationObjectByClass(std::string className);

  /// Update the target representation based on the source representation
  virtual bool Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation);

  /// Get the cost of the conversion.
  virtual unsigned int GetConversionCost(vtkDataObject* sourceRepresentation=NULL, vtkDataObject* targetRepresentation=NULL);

  /// Human-readable name of the converter rule
  virtual const char* GetName(){ return "Planar contour to closed surface"; };
  
  /// Human-readable name of the source representation
  virtual const char* GetSourceRepresentationName() { return vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName(); };
  
  /// Human-readable name of the target representation
  virtual const char* GetTargetRepresentationName() { return vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(); };

protected:
  vtkPlanarContourToClosedSurfaceConversionRule();
  virtual ~vtkPlanarContourToClosedSurfaceConversionRule();

  /// Construct a surface triangulation using a dynamic programming algorithm.
  void TriangulateContours(vtkPolyData*, vtkIdList*, int, vtkIdList*, int, vtkCellArray*);

  /// Find the index of the last point in a contour.
  int GetEndLoop(int, int, bool);

  /// Find the point on the given line that is closest to the given point.
  int GetClosestPoint(vtkPolyData*, double*, vtkIdList*, int);

  /// Remove the keyholes from the contours.
  void FixKeyholes(vtkPolyData*, int, int, int);

  /// Set all of the lines to be oriented in the clockwise direction.
  void SetLinesCounterClockwise(vtkPolyData*);

  /// Determine if a line runs in a clockwise orientation.
  bool IsLineClockwise(vtkPolyData*, vtkLine*);

  /// Reverse the orientation of a line from clockwise to counter-clockwise and vice versa.
  void ReverseLine(vtkLine*, vtkLine*);

  /// Determine the number of contours that share the same Z-coordinates.
  int GetNumberOfLinesOnPlane(vtkPolyData*, int, int);

  /// Determine if two contours overlap in the XY axis.
  bool DoLinesOverlap(vtkLine*, vtkLine*);

  /// Create a branching pattern for overlapping contours.
  void Branch(vtkPolyData*, vtkIdList*, int, int, std::vector< int >, std::vector<vtkSmartPointer<vtkPointLocator> >, std::vector<vtkSmartPointer<vtkIdList> >, vtkLine*);
  
  /// Find the branch closest from the point on the trunk
  int GetClosestBranch(vtkPolyData*, double*, std::vector< int >, std::vector<vtkSmartPointer<vtkPointLocator> >, std::vector<vtkSmartPointer<vtkIdList> >);

  /// Seal the exterior contours of the mesh.
  void SealMesh(vtkPolyData*, vtkCellArray*, vtkCellArray*, std::vector< bool >, std::vector< bool >);

  double GetSpacingBetweenLines(vtkPolyData*);

  /// Check to see if the given point is on the line
  bool IsPointOnLine(vtkIdList*, vtkIdType);

  /// Create an additional contour on the exterior of the surface to compensate for slice thickness.
  void CreateExternalLine(vtkPolyData*, vtkLine*, vtkLine*, vtkIdList*, double);

  /// Triangulate the interior of a contour on the xy plane.
  void TriangulateLine(vtkPolyData*, vtkLine*, vtkIdList*, vtkCellArray*);

  /// Determine if a point is on the interior of a line.
  bool IsPointInsideLine(vtkPolyData*, vtkLine*, double[]);

  /// Determine if two line segments intersect.
  bool DoLineSegmentsIntersect(double[], double[]);

  /// Find the index of the next point in the contour.
  int GetNextLocation(int, int, bool);

  /// Find the index of the next point in the contour.
  int GetPreviousLocation(int, int, bool);

private:
  vtkPlanarContourToClosedSurfaceConversionRule(const vtkPlanarContourToClosedSurfaceConversionRule&); // Not implemented
  void operator=(const vtkPlanarContourToClosedSurfaceConversionRule&);               // Not implemented
};

#endif
