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

#include "vtkPlanarContourToClosedSurfaceConversionRule.h"

// VTK includes
#include <vtkCleanPolyData.h>
#include <vtkExtractCells.h>
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>
#include <vtkImageDilateErode3D.h>
#include <vtkImageStencil.h>
#include <vtkLine.h>
#include <vtkMarchingSquares.h>
#include <vtkPlane.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkPolygon.h>
#include <vtkPriorityQueue.h>
#include <vtkStripper.h>
#include <vtkTextureMapToPlane.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnstructuredGrid.h>

// vtkAddon includes
#include <vtkAddonMathUtilities.h>

// STD includes
#include <algorithm>

// SegmentationCore includes
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
#include <vtkSegment.h>
#endif

// Directions used for dynamic programming table backtracking
enum BacktrackDirection
{
  DYNAMIC_BACKTRACK_UP,
  DYNAMIC_BACKTRACK_LEFT,
};

enum CappingDirection
{
  CAPPING_BELOW,
  CAPPING_ABOVE,
};
static const CappingDirection CappingDirections[] = { CAPPING_BELOW, CAPPING_ABOVE };

//----------------------------------------------------------------------------
vtkSegmentationConverterRuleNewMacro(vtkPlanarContourToClosedSurfaceConversionRule);

//----------------------------------------------------------------------------
vtkPlanarContourToClosedSurfaceConversionRule::vtkPlanarContourToClosedSurfaceConversionRule()
{
  this->DefaultSpacing[0] = 1.0;
  this->DefaultSpacing[1] = 1.0;

  this->AlternativeDimensions[0] = 28;
  this->AlternativeDimensions[1] = 28;
  this->AlternativeDimensions[2] = 1;

  this->ImagePadding[0] = 4;
  this->ImagePadding[1] = 4;
  this->ImagePadding[2] = 0;

  this->ConversionParameters[this->GetDefaultSliceThicknessParameterName()] = std::make_pair("0.0",
    "Default thickness for contours if slice spacing cannot be calculated.");
  this->ConversionParameters[this->GetEndCappingParameterName()] = std::make_pair("1",
    "Create end cap to close surface inside contours on the top and bottom of the structure.\n"
    "0 = leave contours open on surface exterior.\n"
    "1 (default) = close surface by generating smooth end caps.\n"
    "2 = close surface by generating straight end caps."
  );
}

//----------------------------------------------------------------------------
vtkPlanarContourToClosedSurfaceConversionRule::~vtkPlanarContourToClosedSurfaceConversionRule() = default;

//----------------------------------------------------------------------------
unsigned int vtkPlanarContourToClosedSurfaceConversionRule::GetConversionCost(
  vtkDataObject* vtkNotUsed(sourceRepresentation)/*=nullptr*/,
  vtkDataObject* vtkNotUsed(targetRepresentation)/*=nullptr*/)
{
  // Rough input-independent guess (ms)
  return 700;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToClosedSurfaceConversionRule::ConstructRepresentationObjectByRepresentation(std::string representationName)
{
  if (!representationName.compare(this->GetSourceRepresentationName())
    || !representationName.compare(this->GetTargetRepresentationName()))
  {
    return (vtkDataObject*)vtkPolyData::New();
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToClosedSurfaceConversionRule::ConstructRepresentationObjectByClass(std::string className)
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
bool vtkPlanarContourToClosedSurfaceConversionRule::Convert(vtkSegment* segment)
{
  this->CreateTargetRepresentation(segment);
#else
bool vtkPlanarContourToClosedSurfaceConversionRule::Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation)
{
#endif
  // Check validity of source and target representation objects
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  vtkPolyData* planarContoursPolyData = vtkPolyData::SafeDownCast(segment->GetRepresentation(this->GetSourceRepresentationName()));
#else
  vtkPolyData* planarContoursPolyData = vtkPolyData::SafeDownCast(sourceRepresentation);
#endif
  if (!planarContoursPolyData)
  {
    vtkErrorMacro("Convert: Source representation is not a poly data!");
    return false;
  }
#if Slicer_VERSION_MAJOR >= 5 || (Slicer_VERSION_MAJOR >= 4 && Slicer_VERSION_MINOR >= 11)
  vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(segment->GetRepresentation(this->GetTargetRepresentationName()));
#else
  vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(targetRepresentation);
#endif
  if (!closedSurfacePolyData)
  {
    vtkErrorMacro("Convert: Target representation is not a poly data!");
    return false;
  }

  // Copy the contours so that we can make modifications without affecting the original
  vtkSmartPointer<vtkPolyData> inputContoursCopy = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkMatrix4x4> contourToRASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  this->CalculateContourTransform(planarContoursPolyData, contourToRASMatrix);

  vtkSmartPointer<vtkTransform> transformRASToContours = vtkSmartPointer<vtkTransform>::New();
  transformRASToContours->SetMatrix(contourToRASMatrix);
  vtkNew<vtkTransformPolyDataFilter> transformRASToContoursFilter;
  transformRASToContoursFilter->SetInputData(planarContoursPolyData);
  transformRASToContoursFilter->SetTransform(transformRASToContours);
  transformRASToContoursFilter->Update();
  inputContoursCopy->DeepCopy(transformRASToContoursFilter->GetOutput());

  // Make sure the contours are in the right order.
  this->SortContours(inputContoursCopy);

  // remove keyholes from the lines
  this->FixKeyholes(inputContoursCopy, 0.001, 3);

  // set all lines to be counter-clockwise
  this->SetLinesCounterClockwise(inputContoursCopy);

  vtkSmartPointer<vtkPoints> outputPoints = inputContoursCopy->GetPoints();
  vtkSmartPointer<vtkCellArray> outputLines = inputContoursCopy->GetLines();
  vtkSmartPointer<vtkCellArray> outputPolygons = vtkSmartPointer<vtkCellArray>::New(); // Triangles should be added to this

  // Total number of lines in the contours
  int numberOfLines = inputContoursCopy->GetNumberOfLines();

  double spacing = this->GetSpacingBetweenLines(inputContoursCopy);

  std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators(numberOfLines);
  std::vector<vtkSmartPointer<vtkIdList> > linePointIdLists(numberOfLines);
  for (int lineIndex = 0; lineIndex < numberOfLines; ++lineIndex)
  {
    vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
    currentLine->DeepCopy(inputContoursCopy->GetCell(lineIndex));
    linePointIdLists[lineIndex] = currentLine->GetPointIds();
    vtkSmartPointer<vtkPolyData> linePolyData = vtkSmartPointer<vtkPolyData>::New();
    linePolyData->SetPoints(currentLine->GetPoints());
    pointLocators[lineIndex] = vtkSmartPointer<vtkPointLocator>::New();
    pointLocators[lineIndex]->SetDataSet(linePolyData);
    pointLocators[lineIndex]->BuildLocator();
  }

  // Vector of booleans to determine which lines are triangulated from above and from below.
  std::vector< bool > lineTriganulatedToAbove(numberOfLines);
  std::vector< bool > lineTriganulatedToBelow(numberOfLines);
  for (int i = 0; i < numberOfLines; ++i)
  {
    lineTriganulatedToAbove[i] = false;
    lineTriganulatedToBelow[i] = false;
  }

  // Get two consecutive planes.
  vtkIdType firstLineOnPlane1Index = 0; // pointer to first line on plane 1.
  int numberOfLinesInPlane1 = this->GetNumberOfLinesOnPlane(inputContoursCopy, 0, spacing);

  // Loop through all of the contours in the polydata
  while (firstLineOnPlane1Index + numberOfLinesInPlane1 < numberOfLines)
  {
    vtkIdType firstLineOnPlane2Index = firstLineOnPlane1Index + numberOfLinesInPlane1; // pointer to first line on plane 2
    int numberOfLinesInPlane2 = this->GetNumberOfLinesOnPlane(inputContoursCopy, firstLineOnPlane2Index, spacing); // number of lines on plane 2

    // initialize overlaps lists. - list of list
    // Each internal list represents a line from the plane and will store the pointers to the overlap lines

    // List of Overlaps for lines that overlap with other lines from plane 1 and 2
    std::vector< std::vector< vtkIdType > > plane1Overlaps(numberOfLinesInPlane1);
    std::vector< std::vector< vtkIdType > > plane2Overlaps(numberOfLinesInPlane2);

    // Loop through the lines in the first plane
    for (int line1Index = 0; line1Index < numberOfLinesInPlane1; ++line1Index)
    {
      vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
      line1->DeepCopy(inputContoursCopy->GetCell(firstLineOnPlane1Index + line1Index));

      // Loop through the lines in the second plane
      for (int line2Index = 0; line2Index < numberOfLinesInPlane2; ++line2Index)
      {
        vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
        line2->DeepCopy(inputContoursCopy->GetCell(firstLineOnPlane2Index + line2Index));

        // If the two lines overlap, then add them to the lists
        if (this->DoLinesOverlap(line1, line2))
        {
          // line from plane 1 overlaps with line from plane 2
          plane1Overlaps[line1Index].push_back(firstLineOnPlane2Index + line2Index);
          plane2Overlaps[line2Index].push_back(firstLineOnPlane1Index + line1Index);
        }
      }
    }

    // Loop through all of the lines in the first plane
    for (int line1Index = firstLineOnPlane1Index; line1Index < firstLineOnPlane1Index + numberOfLinesInPlane1; ++line1Index)
    {
      vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
      line1->DeepCopy(inputContoursCopy->GetCell(line1Index));

      std::vector<vtkSmartPointer<vtkPointLocator> > overlap1PointLocators(plane1Overlaps[line1Index - firstLineOnPlane1Index].size());
      std::vector<vtkSmartPointer<vtkIdList> > overlap1PointIds(plane1Overlaps[line1Index - firstLineOnPlane1Index].size());

      // Loop through all of the lines in the second plane that overlap with the current line in the first plane
      for (size_t overlapIndex = 0; overlapIndex < plane1Overlaps[line1Index - firstLineOnPlane1Index].size(); ++overlapIndex) // lines on plane 2 that overlap with line 1
      {
        vtkIdType j = plane1Overlaps[line1Index - firstLineOnPlane1Index][overlapIndex];
        overlap1PointLocators[overlapIndex] = pointLocators[j];
        overlap1PointIds[overlapIndex] = linePointIdLists[j];
      }

      // Loop through all of the lines in the second plane that overlap with the current line in the first plane
      for (size_t overlapIndex = 0; overlapIndex < plane1Overlaps[line1Index - firstLineOnPlane1Index].size(); ++overlapIndex) // lines on plane 2 that overlap with line 1
      {
        vtkIdType line2Index = plane1Overlaps[line1Index - firstLineOnPlane1Index][overlapIndex];

        vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
        line2->DeepCopy(inputContoursCopy->GetCell(line2Index));

        std::vector<vtkSmartPointer<vtkPointLocator> > overlap2PointLocators(plane2Overlaps[line2Index - firstLineOnPlane2Index].size());
        std::vector<vtkSmartPointer<vtkIdList> > overlap2PointIds(plane2Overlaps[line2Index - firstLineOnPlane2Index].size());

        for (size_t i = 0; i < plane2Overlaps[line2Index - firstLineOnPlane2Index].size(); ++i)
        {
          int j = plane2Overlaps[line2Index - firstLineOnPlane2Index][i];
          overlap2PointLocators[i] = pointLocators[j];
          overlap2PointIds[i] = linePointIdLists[j];
        }

        // Get the portion of line 1 that is close to line 2,
        vtkSmartPointer<vtkLine> dividedLine1 = vtkSmartPointer<vtkLine>::New();
        this->Branch(inputContoursCopy, line1, line2Index, plane1Overlaps[line1Index - firstLineOnPlane1Index], overlap1PointLocators, overlap1PointIds, dividedLine1);
        vtkSmartPointer<vtkIdList> dividedPointsInLine1 = dividedLine1->GetPointIds();
        int numberOfdividedPointsInLine1 = dividedLine1->GetNumberOfPoints();

        // Get the portion of line 2 that is close to line 1.
        vtkSmartPointer<vtkLine> dividedLine2 = vtkSmartPointer<vtkLine>::New();
        this->Branch(inputContoursCopy, line2, line1Index, plane2Overlaps[line2Index - firstLineOnPlane2Index], overlap2PointLocators, overlap2PointIds, dividedLine2);
        vtkSmartPointer<vtkIdList> dividedPointsInLine2 = dividedLine2->GetPointIds();
        int numberOfdividedPointsInLine2 = dividedLine2->GetNumberOfPoints();

        if (numberOfdividedPointsInLine1 > 1 && numberOfdividedPointsInLine2 > 1)
        {
          lineTriganulatedToAbove[line1Index] = true;
          lineTriganulatedToBelow[line2Index] = true;
          this->TriangulateBetweenContours(inputContoursCopy, dividedPointsInLine1, dividedPointsInLine2, outputPolygons);
        }

      }
    }

    // Advance the points
    firstLineOnPlane1Index = firstLineOnPlane2Index;
    numberOfLinesInPlane1 = numberOfLinesInPlane2;
  }

  // Triangulate all contours which are exposed.
  if (vtkVariant(this->GetConversionParameter(this->GetEndCappingParameterName())).ToInt() != EndCappingModes::None)
  {
    this->EndCapping(inputContoursCopy, outputPolygons, lineTriganulatedToAbove, lineTriganulatedToBelow);
  }

  // Initialize the output data.
  closedSurfacePolyData->SetPoints(outputPoints);
  //closedSurfacePolyData->SetLines(outputLines); // Do not include lines in poly data for nicer visualization
  closedSurfacePolyData->SetPolys(outputPolygons);

  vtkSmartPointer<vtkTransform> transformContoursToRAS = vtkSmartPointer<vtkTransform>::New();
  transformContoursToRAS->SetMatrix(contourToRASMatrix);
  transformContoursToRAS->Inverse();

  vtkNew<vtkTransformPolyDataFilter> transformPolyDataToRASFilter;
  transformPolyDataToRASFilter->SetInputData(closedSurfacePolyData);
  transformPolyDataToRASFilter->SetTransform(transformContoursToRAS);
  transformPolyDataToRASFilter->Update();
  closedSurfacePolyData->DeepCopy(transformPolyDataToRASFilter->GetOutput());

  return true;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::TriangulateBetweenContours(vtkPolyData* inputROIPoints, vtkIdList* pointsInLine1, vtkIdList* pointsInLine2, vtkCellArray* outputPolygons)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("TriangulateBetweenContours: Invalid vtkPolyData!");
    return;
  }

  if (!pointsInLine1)
  {
    vtkErrorMacro("TriangulateBetweenContours: Invalid vtkIdList!");
    return;
  }

  if (!pointsInLine2)
  {
    vtkErrorMacro("TriangulateBetweenContours: Invalid vtkIdList!");
    return;
  }

  if (pointsInLine1->GetNumberOfIds() == 0 || pointsInLine2->GetNumberOfIds() == 0)
  {
    return;
  }

  int numberOfPointsInLine1 = pointsInLine1->GetNumberOfIds();
  int numberOfPointsInLine2 = pointsInLine2->GetNumberOfIds();

  // Pre-calculate and store the closest points.

  // Closest point from line 1 to line 2
  std::vector< int > closestPointFromLine1ToLine2Ids(numberOfPointsInLine1);
  for (int line1PointIndex = 0; line1PointIndex < numberOfPointsInLine1; ++line1PointIndex)
  {
    double line1Point[3] = { 0,0,0 };
    inputROIPoints->GetPoint(pointsInLine1->GetId(line1PointIndex), line1Point);
    closestPointFromLine1ToLine2Ids[line1PointIndex] = this->GetClosestPoint(inputROIPoints, line1Point, pointsInLine2);
  }

  // Closest from line 2 to line 1
  std::vector< int > closestPointFromLine2ToLine1Ids(numberOfPointsInLine2);
  for (int line2PointIndex = 0; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
  {
    double line2Point[3] = { 0,0,0 };
    inputROIPoints->GetPoint(pointsInLine2->GetId(line2PointIndex), line2Point);
    closestPointFromLine2ToLine1Ids[line2PointIndex] = this->GetClosestPoint(inputROIPoints, line2Point, pointsInLine1);
  }

  // Orient loops.
  // Use the 0th point on line 1 and the closest point on line 2.
  vtkIdType startLine1PointId = 0;
  vtkIdType startLine2PointId = closestPointFromLine1ToLine2Ids[0];

  double firstPointLine1[3] = { 0,0,0 }; // first point on line 1;
  inputROIPoints->GetPoint(pointsInLine1->GetId(startLine1PointId), firstPointLine1);

  double firstPointLine2[3] = { 0,0,0 }; // first point on line 2;
  inputROIPoints->GetPoint(pointsInLine2->GetId(startLine2PointId), firstPointLine2);

  // Determine if the loops are closed.
  // A loop is closed if the first point is repeated as the last point.
  bool line1Closed = (pointsInLine1->GetId(0) == pointsInLine1->GetId(numberOfPointsInLine1 - 1));
  bool line2Closed = (pointsInLine2->GetId(0) == pointsInLine2->GetId(numberOfPointsInLine2 - 1));

  // Determine the ending points.
  int line1EndPoint = this->GetEndLoop(startLine1PointId, numberOfPointsInLine1, line1Closed);
  int line2EndPoint = this->GetEndLoop(startLine2PointId, numberOfPointsInLine2, line2Closed);

  // Initialize the Dynamic Programming table.
  // Rows represent line 1. Columns represent line 2.

  // Initialize the score table.
  std::vector< std::vector< double > > scoreTable(numberOfPointsInLine1, std::vector< double >(numberOfPointsInLine2));
  scoreTable[0][0] = vtkMath::Distance2BetweenPoints(firstPointLine1, firstPointLine2);

  std::vector< std::vector< BacktrackDirection > > backtrackTable(numberOfPointsInLine1, std::vector< BacktrackDirection >(numberOfPointsInLine2));
  backtrackTable[0][0] = DYNAMIC_BACKTRACK_UP;

  // Initialize the first row in the table.
  vtkIdType currentPointIdLine2 = this->GetNextLocation(startLine2PointId, numberOfPointsInLine2, line2Closed);
  for (int line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
  {
    double currentPointLine2[3] = { 0,0,0 }; // current point on line 2
    inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIdLine2), currentPointLine2);

    // Use the distance between first point on line 1 and current point on line 2.
    double distance = vtkMath::Distance2BetweenPoints(firstPointLine1, currentPointLine2);

    scoreTable[0][line2PointIndex] = scoreTable[0][line2PointIndex - 1] + distance;
    backtrackTable[0][line2PointIndex] = DYNAMIC_BACKTRACK_LEFT;

    currentPointIdLine2 = this->GetNextLocation(currentPointIdLine2, numberOfPointsInLine2, line2Closed);
  }

  // Initialize the first column in the table.
  vtkIdType currentPointIdLine1 = this->GetNextLocation(startLine1PointId, numberOfPointsInLine2, line1Closed);
  for (int line1PointIndex = 1; line1PointIndex < numberOfPointsInLine1; ++line1PointIndex)
  {
    double currentPointLine1[3] = { 0,0,0 }; // current point on line 1
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIdLine1), currentPointLine1);

    // Use the distance between first point on line 2 and current point on line 1.
    double distance = vtkMath::Distance2BetweenPoints(currentPointLine1, firstPointLine2);

    scoreTable[line1PointIndex][0] = scoreTable[line1PointIndex - 1][0] + distance;
    backtrackTable[line1PointIndex][0] = DYNAMIC_BACKTRACK_UP;
    for (int line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
    {
      scoreTable[line1PointIndex][line2PointIndex] = 0;
      backtrackTable[line1PointIndex][line2PointIndex] = DYNAMIC_BACKTRACK_UP;
    }

    currentPointIdLine1 = this->GetNextLocation(currentPointIdLine1, numberOfPointsInLine1, line1Closed);
  }

  // Fill the rest of the table.
  vtkIdType previousLine1 = startLine1PointId;
  vtkIdType previousLine2 = startLine2PointId;

  currentPointIdLine1 = this->GetNextLocation(startLine1PointId, numberOfPointsInLine1, line1Closed);
  currentPointIdLine2 = this->GetNextLocation(startLine2PointId, numberOfPointsInLine2, line2Closed);

  vtkIdType line1PointIndex = 1;
  vtkIdType line2PointIndex = 1;
  for (line1PointIndex = 1; line1PointIndex < numberOfPointsInLine1; ++line1PointIndex)
  {
    double pointOnLine1[3] = { 0,0,0 };
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIdLine1), pointOnLine1);

    for (line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
    {
      double pointOnLine2[3] = { 0,0,0 };
      inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIdLine2), pointOnLine2);

      double distance = vtkMath::Distance2BetweenPoints(pointOnLine1, pointOnLine2);

      // Use the pre-calculated closest point.
      if (currentPointIdLine1 == closestPointFromLine2ToLine1Ids[previousLine2])
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex][line2PointIndex - 1] + distance;
        backtrackTable[line1PointIndex][line2PointIndex] = DYNAMIC_BACKTRACK_LEFT;
      }
      else if (currentPointIdLine2 == closestPointFromLine1ToLine2Ids[previousLine1])
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex - 1][line2PointIndex] + distance;
        backtrackTable[line1PointIndex][line2PointIndex] = DYNAMIC_BACKTRACK_UP;
      }
      else if (scoreTable[line1PointIndex][line2PointIndex - 1] <= scoreTable[line1PointIndex - 1][line2PointIndex])
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex][line2PointIndex - 1] + distance;
        backtrackTable[line1PointIndex][line2PointIndex] = DYNAMIC_BACKTRACK_LEFT;
      }
      else
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex - 1][line2PointIndex] + distance;
        backtrackTable[line1PointIndex][line2PointIndex] = DYNAMIC_BACKTRACK_UP;
      }

      // Advance the pointers
      previousLine2 = currentPointIdLine2;
      currentPointIdLine2 = this->GetNextLocation(currentPointIdLine2, numberOfPointsInLine2, line2Closed);
    }
    previousLine1 = currentPointIdLine1;
    currentPointIdLine1 = this->GetNextLocation(currentPointIdLine1, numberOfPointsInLine1, line1Closed);
  }

  // Backtrack.
  currentPointIdLine1 = line1EndPoint;
  currentPointIdLine2 = line2EndPoint;
  --line1PointIndex;
  --line2PointIndex;
  while (line1PointIndex > 0 || line2PointIndex > 0)
  {
    double line1Point[3] = { 0,0,0 }; // current point on line 1
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIdLine1), line1Point);

    double line2Point[3] = { 0,0,0 }; // current point on line 2
    inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIdLine2), line2Point);

    vtkIdType currentTriangle[3] = { 0,0,0 };
    currentTriangle[0] = pointsInLine1->GetId(currentPointIdLine1);
    currentTriangle[1] = pointsInLine2->GetId(currentPointIdLine2);
    if (backtrackTable[line1PointIndex][line2PointIndex] == DYNAMIC_BACKTRACK_LEFT)
    {
      vtkIdType previousPointIndexLine2 = this->GetPreviousLocation(currentPointIdLine2, numberOfPointsInLine2, line2Closed);
      currentTriangle[2] = pointsInLine2->GetId(previousPointIndexLine2);
      line2PointIndex -= 1;
      currentPointIdLine2 = previousPointIndexLine2;
    }
    else // DYNAMIC_BACKTRACK_UP
    {
      vtkIdType previousPointIndexLine1 = this->GetPreviousLocation(currentPointIdLine1, numberOfPointsInLine1, line1Closed);
      currentTriangle[2] = pointsInLine1->GetId(previousPointIndexLine1);
      line1PointIndex -= 1;
      currentPointIdLine1 = previousPointIndexLine1;
    }

    outputPolygons->InsertNextCell(3);
    outputPolygons->InsertCellPoint(currentTriangle[0]);
    outputPolygons->InsertCellPoint(currentTriangle[1]);
    outputPolygons->InsertCellPoint(currentTriangle[2]);
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkPlanarContourToClosedSurfaceConversionRule::GetEndLoop(vtkIdType startLoopIndex, int numberOfPoints, bool loopClosed)
{
  if (startLoopIndex != 0)
  {
    if (loopClosed)
    {
      return startLoopIndex;
    }

    return startLoopIndex - 1;
  }

  // If startLoop was 0, then it doesn't matter whether or not the loop was closed.
  return numberOfPoints - 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkPlanarContourToClosedSurfaceConversionRule::GetClosestPoint(vtkPolyData* inputROIPoints, double* originalPoint, vtkIdList* linePointIds)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return 0;
  }

  if (!linePointIds)
  {
    vtkErrorMacro("GetClosestPoint: Invalid vtkIdList!");
    return 0;
  }

  double pointOnLine[3] = { 0,0,0 }; // point from the given line
  inputROIPoints->GetPoint(linePointIds->GetId(0), pointOnLine);

  double minimumDistance = vtkMath::Distance2BetweenPoints(originalPoint, pointOnLine); // minimum distance from the point to the line
  double closestPointIndex = 0;

  int numberOfPoints = linePointIds->GetNumberOfIds();

  // Loop through all of the points in the current line
  for (int currentPointIndex = 1; currentPointIndex < numberOfPoints; ++currentPointIndex)
  {
    inputROIPoints->GetPoint(linePointIds->GetId(currentPointIndex), pointOnLine);

    double distanceBetweenPoints = vtkMath::Distance2BetweenPoints(originalPoint, pointOnLine);
    if (distanceBetweenPoints < minimumDistance)
    {
      minimumDistance = distanceBetweenPoints;
      closestPointIndex = currentPointIndex;
    }
  }

  return closestPointIndex;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::SortContours(vtkPolyData* inputROIPoints)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }
  int numberOfLines = inputROIPoints->GetNumberOfLines();

  vtkSmartPointer<vtkLine> originalLine;
  std::vector<std::pair<double, vtkIdType> > lineZIdPairs;

  // Loop through all of the lines
  for (vtkIdType currentLineID = 0; currentLineID < numberOfLines; ++currentLineID)
  {
    vtkSmartPointer<vtkCell> currentLine = inputROIPoints->GetCell(currentLineID);

    // Calculate the average Z value of the line based on the bounds
    double averageZ = (currentLine->GetBounds()[4] + currentLine->GetBounds()[5]) / 2.0;

    // Add the pair as: (average Z :: line id)
    lineZIdPairs.push_back(std::make_pair(averageZ, currentLineID));
  }
  std::sort(lineZIdPairs.begin(), lineZIdPairs.end());

  vtkSmartPointer<vtkCellArray> outputLines = vtkSmartPointer<vtkCellArray>::New();
  outputLines->Initialize();
  inputROIPoints->DeleteCells();

  // Loop through all of the lines
  for (vtkIdType currentLineID = 0; currentLineID < numberOfLines; ++currentLineID)
  {
    outputLines->InsertNextCell(inputROIPoints->GetCell(lineZIdPairs[currentLineID].second));
  }
  inputROIPoints->SetLines(outputLines);
  inputROIPoints->BuildCells();
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::FixKeyholes(vtkPolyData* inputROIPoints, double epsilon, int minimumSeperation)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  vtkSmartPointer<vtkLine> originalLine;
  std::vector<vtkSmartPointer<vtkLine> > newLines;
  int pointsOfSeperation;

  int numberOfLines = inputROIPoints->GetNumberOfLines();

  // Loop through all of the lines
  for (vtkIdType currentLineId = 0; currentLineId < numberOfLines; ++currentLineId)
  {
    originalLine = vtkSmartPointer<vtkLine>::New();
    originalLine->DeepCopy(inputROIPoints->GetCell(currentLineId));

    vtkSmartPointer<vtkPoints> originalLinePoints = originalLine->GetPoints();
    int numberOfPointsInLine = originalLine->GetNumberOfPoints();

    vtkSmartPointer<vtkPolyData> linePolyData = vtkSmartPointer<vtkPolyData>::New();
    linePolyData->SetPoints(originalLinePoints);

    vtkSmartPointer<vtkPointLocator> pointLocator = vtkSmartPointer<vtkPointLocator>::New();
    pointLocator->SetDataSet(linePolyData);
    pointLocator->BuildLocator();

    bool keyHoleExists = false;

    // If the value of flags[i] is -1, the point is not part of a keyhole
    // If the value of flags[i] is >= 0, it represents a point that is
    // close enough that it could be considered part of a keyhole.
    std::vector< int > flags(numberOfPointsInLine);

    // Loop through and initialize the list of flags to -1.
    for (int i = 0; i < numberOfPointsInLine; ++i)
    {
      flags[i] = -1;
    }

    for (int point1Id = 0; point1Id < numberOfPointsInLine; ++point1Id)
    {
      double point1[3] = { 0,0,0 };
      originalLinePoints->GetPoint(point1Id, point1);

      vtkSmartPointer<vtkIdList> pointsWithinRadius = vtkSmartPointer<vtkIdList>::New();
      pointsWithinRadius->Initialize();
      pointLocator->FindPointsWithinRadius(epsilon, point1, pointsWithinRadius);

      for (vtkIdType currentPointIndex = 0; currentPointIndex < pointsWithinRadius->GetNumberOfIds(); ++currentPointIndex)
      {
        int point2Id = pointsWithinRadius->GetId(currentPointIndex);

        // Make sure the points are not too close together on the line index-wise
        pointsOfSeperation = std::min(point2Id - point1Id, numberOfPointsInLine - 1 - point2Id + point1Id);
        if (pointsOfSeperation > minimumSeperation)
        {
          keyHoleExists = true;
          flags[point1Id] = point2Id;
          flags[point2Id] = point1Id;
        }

      }
    }

    if (!keyHoleExists)
    {
      newLines.push_back(originalLine);
    }
    else
    {
      size_t currentLayer = 0;
      bool pointInChannel = false;

      std::vector<vtkSmartPointer<vtkIdList> > rawLinePointIds;
      std::vector<vtkSmartPointer<vtkIdList> > finishedLinePointIds;

      // Loop through all of the points in the line
      for (int currentPointIndex = 0; currentPointIndex < numberOfPointsInLine; ++currentPointIndex)
      {

        // Add a new line if necessary
        if (currentLayer == rawLinePointIds.size())
        {
          vtkSmartPointer<vtkLine> newLine = vtkSmartPointer<vtkLine>::New();
          newLine->GetPoints()->SetData(originalLinePoints->GetData());

          vtkSmartPointer<vtkIdList> newLineIds = newLine->GetPointIds();
          newLineIds->Initialize();

          newLines.push_back(newLine);
          rawLinePointIds.push_back(newLineIds);
        }

        int currentPointId = originalLine->GetPointId(currentPointIndex);

        // If the current point is not part of a keyhole, add it to the current line
        if (flags[currentPointIndex] == -1)
        {
          rawLinePointIds[currentLayer]->InsertNextId(currentPointId);
          pointInChannel = false;
        }
        else
        {
          // If the current point is the start of a keyhole add the point to the line,
          // increment the layer, and start the channel.
          if (flags[currentPointIndex] > currentPointIndex && !pointInChannel)
          {
            rawLinePointIds[currentLayer]->InsertNextId(currentPointId);
            ++currentLayer;
            pointInChannel = true;
          }
          // If the current point is the end of a volume in the keyhole, add the point
          // to the line, remove the current line from the working list, deincrement
          // the layer, add the current line to the finished lines and start the,
          // channel.
          else if (flags[currentPointIndex] < currentPointIndex && !pointInChannel)
          {
            rawLinePointIds[currentLayer]->InsertNextId(currentPointId);
            finishedLinePointIds.push_back(rawLinePointIds[currentLayer]);
            rawLinePointIds.pop_back();
            if (currentLayer > 0)
            {
              --currentLayer;
            }
            pointInChannel = true;
          }
        }
      }

      // Add the remaining line to the finished list.
      for (size_t remainingLineId = 0; remainingLineId < rawLinePointIds.size(); ++remainingLineId)
      {
        finishedLinePointIds.push_back(rawLinePointIds[remainingLineId]);
      }

      // Loop through the completed lines and make sure that they are closed
      for (size_t finishedLineId = 0; finishedLineId < finishedLinePointIds.size(); ++finishedLineId)
      {
        if (finishedLinePointIds[finishedLineId]->GetNumberOfIds() > 0
          && finishedLinePointIds[finishedLineId]->GetId(0) != finishedLinePointIds[finishedLineId]->GetId(finishedLinePointIds[finishedLineId]->GetNumberOfIds() - 1))
        {
          finishedLinePointIds[finishedLineId]->InsertNextId(finishedLinePointIds[finishedLineId]->GetId(0));
        }
      }

    }
  }

  vtkSmartPointer<vtkCellArray> outputLines = vtkSmartPointer<vtkCellArray>::New();
  outputLines->Initialize();
  inputROIPoints->DeleteCells();
  for (size_t currentLineIndex = 0; currentLineIndex < newLines.size(); ++currentLineIndex)
  {
    // Only add the lines if they have more than 2 points
    if (newLines[currentLineIndex]->GetNumberOfPoints() > 1)
    {
      outputLines->InsertNextCell(newLines[currentLineIndex]);
    }
  }
  inputROIPoints->SetLines(outputLines);
  inputROIPoints->BuildCells();
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::SetLinesCounterClockwise(vtkPolyData* inputROIPoints)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  int numberOfLines = inputROIPoints->GetNumberOfLines();

  std::vector<vtkSmartPointer<vtkLine> > newLines;

  for (int currentLineId = 0; currentLineId < numberOfLines; ++currentLineId)
  {
    vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
    currentLine->DeepCopy(inputROIPoints->GetCell(currentLineId));

    vtkSmartPointer<vtkPoints> currentPoints = currentLine->GetPoints();

    if (IsLineClockwise(inputROIPoints, currentLine))
    {
      vtkSmartPointer<vtkLine> newLine = vtkSmartPointer<vtkLine>::New();
      vtkSmartPointer<vtkIdList> newLineIds = newLine->GetPointIds();
      newLineIds->Initialize();

      this->ReverseLine(currentLine, newLine);
      newLines.push_back(newLine);

    }
    else
    {
      newLines.push_back(currentLine);
    }
  }

  // Replace the lines in the input data with the modified lines.
  vtkSmartPointer<vtkCellArray> outputLines = vtkSmartPointer<vtkCellArray>::New();
  outputLines->Initialize();
  inputROIPoints->DeleteCells();
  for (size_t currentLineIndex = 0; currentLineIndex < newLines.size(); ++currentLineIndex)
  {
    outputLines->InsertNextCell(newLines[currentLineIndex]);
  }
  inputROIPoints->SetLines(outputLines);
  inputROIPoints->BuildCells();

}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::IsLineClockwise(vtkPolyData* inputROIPoints, vtkLine* line)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("IsLineClockwise: Invalid vtkPolyData!");
    return false;
  }

  if (!line)
  {
    vtkErrorMacro("IsLineClockwise: Invalid vtkLine!");
    return false;
  }

  int numberOfPoints = line->GetNumberOfPoints();

  // Calculate twice the area of the line.
  double areaSum = 0;

  for (int currentPointId = 0; currentPointId < numberOfPoints - 1; ++currentPointId)
  {
    double point1[3];
    inputROIPoints->GetPoint(line->GetPointId(currentPointId), point1);

    double point2[3];
    inputROIPoints->GetPoint(line->GetPointId(currentPointId + 1), point2);

    areaSum += (point2[0] - point1[0]) * (point2[1] + point1[1]);
  }

  // If the area is positive, the contour is clockwise,
  // If it is negative, the contour is counter-clockwise.
  return areaSum > 0;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::ReverseLine(vtkLine* originalLine, vtkLine* newLine)
{
  if (!originalLine)
  {
    vtkErrorMacro("ReverseLine: Invalid vtkLine!");
    return;
  }

  if (!newLine)
  {
    vtkErrorMacro("ReverseLine: Invalid vtkLine!");
    return;
  }

  int numberOfPoints = originalLine->GetNumberOfPoints();

  vtkSmartPointer<vtkIdList> newPoints = newLine->GetPointIds();

  // Loop through the points in the original line in reverse
  for (int currentPointIndex = numberOfPoints - 1; currentPointIndex >= 0; currentPointIndex--)
  {
    vtkIdType currentPointId = originalLine->GetPointId(currentPointIndex);
    newPoints->InsertNextId(currentPointId);
  }
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetNumberOfLinesOnPlane(vtkPolyData* inputROIPoints, vtkIdType originalLineIndex, double spacing)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("GetNumberOfLinesOnPlane: Invalid vtkPolyData!");
    return 0;
  }

  int numberOfLines = inputROIPoints->GetNumberOfLines();

  double contourPlaneThreshold = 0.1*spacing;
  double lineZ = (inputROIPoints->GetCell(originalLineIndex)->GetBounds()[4] + inputROIPoints->GetCell(originalLineIndex)->GetBounds()[5]) / 2.0; // z-value
  vtkIdType currentLineId = originalLineIndex + 1;

  while (currentLineId < numberOfLines)
  {
    double currentLineZDifference = std::abs(0.5*(inputROIPoints->GetCell(currentLineId)->GetBounds()[4] + inputROIPoints->GetCell(currentLineId)->GetBounds()[5]) - lineZ);
    if (currentLineZDifference < contourPlaneThreshold)
    {
      currentLineId++;
    }
    else
    {
      break;
    }
  }
  return currentLineId - originalLineIndex;
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::DoLinesOverlap(vtkLine* line1, vtkLine* line2)
{
  if (!line1)
  {
    vtkErrorMacro("DoLinesOverlap: Invalid vtkLine!");
    return false;
  }

  if (!line2)
  {
    vtkErrorMacro("DoLinesOverlap: Invalid vtkLine!");
    return false;
  }

  double bounds1[6];
  line1->GetBounds(bounds1);

  double bounds2[6];
  line2->GetBounds(bounds2);

  return bounds1[0] < bounds2[1] &&
    bounds1[1] > bounds2[0] &&
    bounds1[2] < bounds2[3] &&
    bounds1[3] > bounds2[2];
}

// TODO: It may be possible to speed up this function by only calling the branch function once. -- need to look into this
//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::Branch(vtkPolyData* inputROIPoints, vtkLine* branchingLine, vtkIdType currentLineId, std::vector< vtkIdType > overlappingLineIds, std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators, std::vector<vtkSmartPointer<vtkIdList> > lineIdLists, vtkLine* outputLine)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("Branch: Invalid vtkPolyData!");
    return;
  }

  if (!branchingLine)
  {
    vtkErrorMacro("Branch: Invalid vtkLine!");
    return;
  }

  vtkSmartPointer<vtkIdList> outputLinePointIds = outputLine->GetPointIds();
  outputLinePointIds->Initialize();

  if (overlappingLineIds.size() == 1)
  {
    outputLinePointIds->DeepCopy(branchingLine->GetPointIds());
    return;
  }

  // Discard some points on the trunk so that the branch connects to only a part of the trunk.
  bool prev = false; // TODO: Clean up

  // Loop through all of the points in the current line
  for (int currentPointIndex = 0; currentPointIndex < branchingLine->GetNumberOfPoints(); ++currentPointIndex)
  {
    vtkIdType currentPointId = branchingLine->GetPointId(currentPointIndex);

    double currentPoint[3] = { 0,0,0 };
    inputROIPoints->GetPoint(currentPointId, currentPoint);

    // See if the point's closest branch is the input branch.
    if (this->GetClosestBranch(inputROIPoints, currentPoint, overlappingLineIds, pointLocators, lineIdLists) == currentLineId)
    {
      outputLinePointIds->InsertNextId(currentPointId);
      prev = true;
    }
    else
    {
      if (prev)
      {
        // Add one extra point to close up the surface.
        outputLinePointIds->InsertNextId(currentPointId);
      }
      prev = false;
    }
  }
  int dividedNumberOfPoints = outputLine->GetNumberOfPoints();
  if (dividedNumberOfPoints > 1)
  {
    // Determine if the trunk was originally a closed contour.
    bool lineIsClosed = (branchingLine->GetPointId(0) == branchingLine->GetPointId(branchingLine->GetNumberOfPoints() - 1));

    if (lineIsClosed && (outputLinePointIds->GetId(0) != outputLinePointIds->GetId(dividedNumberOfPoints - 1)))
    {
      // Make the new one a closed contour as well.
      outputLinePointIds->InsertNextId(outputLinePointIds->GetId(0));
    }
  }
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetClosestBranch(vtkPolyData* inputROIPoints, double* originalPoint, std::vector< vtkIdType > overlappingLineIds, std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators, std::vector<vtkSmartPointer<vtkIdList> > lineIdLists)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("GetClosestBranch: Invalid vtkPolyData!");
  }

  // No need to check if there is only one overlapping line.
  if (overlappingLineIds.size() == 1)
  {
    return overlappingLineIds[0];
  }

  double minimumDistanceSquared = VTK_DOUBLE_MAX;
  vtkIdType closestLineId = overlappingLineIds[0];

  // Loop through all of the lines that overlap with the line the original point is on
  for (size_t currentOverlapIndex = 0; currentOverlapIndex < overlappingLineIds.size(); ++currentOverlapIndex)
  {
    vtkIdType closestPointId = pointLocators[currentOverlapIndex]->FindClosestPoint(originalPoint);
    double currentPoint[3] = { 0,0,0 };
    inputROIPoints->GetPoint(lineIdLists[currentOverlapIndex]->GetId(closestPointId), currentPoint);

    double currentDistanceToLineSquared = vtkMath::Distance2BetweenPoints(currentPoint, originalPoint);
    if (currentDistanceToLineSquared < minimumDistanceSquared)
    {
      minimumDistanceSquared = currentDistanceToLineSquared;
      closestLineId = overlappingLineIds[currentOverlapIndex];
    }
  }

  return closestLineId;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::EndCapping(vtkPolyData* inputROIPoints, vtkCellArray* outputPolygons, std::vector< bool > lineTriganulatedToAbove, std::vector< bool > lineTriganulatedToBelow)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("EndCapping: Invalid input ROI points!");
    return;
  }

  if (!outputPolygons)
  {
    vtkErrorMacro("EndCapping: Invalid output poly data!");
    return;
  }

  int numberOfLines = inputROIPoints->GetNumberOfLines();
  double lineSpacing = this->GetSpacingBetweenLines(inputROIPoints);

  // Loop through all of the lines in the polydata
  for (int currentLineIndex = 0; currentLineIndex < numberOfLines; ++currentLineIndex)
  {
    vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
    currentLine->DeepCopy(inputROIPoints->GetCell(currentLineIndex));

    // If the current was not connected by a polygon to a contour above or below, then it needs to be capped
    for (CappingDirection direction : CappingDirections)
    {
      bool lineTriangulated = false;
      if (direction == CAPPING_ABOVE)
      {
        lineTriangulated = lineTriganulatedToAbove[currentLineIndex];
      }
      else // CAPPING_BELOW
      {
        lineTriangulated = lineTriganulatedToBelow[currentLineIndex];
      }

      if (!lineTriangulated)
      {
        double lineOffset = lineSpacing;
        if (direction == CAPPING_BELOW)
        {
          lineOffset = -lineSpacing;
        }

        // Create a new contour that is a half-slice thickness above the line
        vtkSmartPointer<vtkCellArray> externalLines = vtkSmartPointer<vtkCellArray>::New();
        this->CreateEndCapContour(inputROIPoints, currentLine, externalLines, lineOffset);

        int numberOfCells = externalLines->GetNumberOfCells();
        std::vector<vtkIdType> overlapLineIds(numberOfCells);
        std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators(numberOfCells);
        std::vector<vtkSmartPointer<vtkIdList> >  idLists(numberOfCells);

        // Loop through all of the external lines that were created
        for (int currentLineId = 0; currentLineId < numberOfCells; ++currentLineId)
        {
          overlapLineIds[currentLineId] = currentLineId;

          vtkSmartPointer<vtkIdList> lineIdList = vtkSmartPointer<vtkIdList>::New();
          externalLines->GetNextCell(lineIdList);
          idLists[currentLineId] = lineIdList;

          vtkIdType newLineId = inputROIPoints->InsertNextCell(VTK_LINE, lineIdList);
          inputROIPoints->BuildCells();

          vtkSmartPointer<vtkLine> newLine = vtkSmartPointer<vtkLine>::New();
          newLine->DeepCopy(inputROIPoints->GetCell(newLineId));

          this->TriangulateContourInterior(newLine, outputPolygons, direction == CAPPING_ABOVE);

          vtkSmartPointer<vtkPolyData> linePolyData = vtkSmartPointer<vtkPolyData>::New();
          linePolyData->SetPoints(newLine->GetPoints());

          vtkSmartPointer<vtkPointLocator> pointLocator = vtkSmartPointer<vtkPointLocator>::New();
          pointLocator->SetDataSet(linePolyData);
          pointLocator->BuildLocator();
          pointLocators[currentLineId] = pointLocator;
        }

        // Loop through all of the external lines that were created
        for (int currentLineId = 0; currentLineId < numberOfCells; ++currentLineId)
        {
          vtkSmartPointer<vtkLine> dividedLine = vtkSmartPointer<vtkLine>::New();
          this->Branch(inputROIPoints, currentLine, currentLineId, overlapLineIds, pointLocators, idLists, dividedLine);
          if (direction == CAPPING_ABOVE)
          {
            this->TriangulateBetweenContours(inputROIPoints, dividedLine->GetPointIds(), idLists[currentLineId], outputPolygons);
          }
          else
          {
            this->TriangulateBetweenContours(inputROIPoints, idLists[currentLineId], dividedLine->GetPointIds(), outputPolygons);
          }
        }
      } // end if (!lineTriangulated)
    } // end for (CappingDirection direction : CappingDirections)
  } // end for (int currentLineIndex = 0; currentLineIndex < numberOfLines; ++currentLineIndex)
}

//----------------------------------------------------------------------------
double vtkPlanarContourToClosedSurfaceConversionRule::GetSpacingBetweenLines(vtkPolyData* inputROIPoints)
{
  double defaultSliceThickness = vtkVariant(this->ConversionParameters[this->GetDefaultSliceThicknessParameterName()].first).ToDouble();

  if (!inputROIPoints)
  {
    vtkErrorMacro("GetSpacingBetweenLines: Invalid vtkPolyData!");
    return defaultSliceThickness;
  }
  if (inputROIPoints->GetNumberOfCells() < 2)
  {
    vtkErrorMacro("GetSpacingBetweenLines: Input polydata has less than two cells! Unable to calculate spacing.");
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

    // Calculate the distance as the difference between the z value in the middle of the bounding boxes of the two lines.
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
  int numberOfLines = 0;
  for (std::vector<double>::iterator distIt = distances.begin(); distIt != distances.end(); ++distIt)
  {
    // If the distance is greater than 10% of the mean, discard it.
    double distance = (*distIt);
    if (std::abs(distance - distanceMean) >= distanceMean / 10)
    {
      continue;
    }

    distanceSum += distance;
    ++numberOfLines;
  }

  // If the number of lines is zero, return the uncorrected mean.
  if (numberOfLines == 0)
  {
    vtkWarningMacro("GetSpacingBetweenLines: Contour spacing is not consistent.");
    return distanceMean;
  }

  // Recalculate the mean distance between the lines.
  distanceMean = distanceSum / numberOfLines;

  return distanceMean;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::CreateEndCapContour(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkCellArray* outputLines, double lineSpacing)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkPolyData");
    return;
  }

  if (!inputLine)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkLine");
    return;
  }

  if (!outputLines)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkCellArray");
  }

  if (vtkVariant(this->GetConversionParameter(this->GetEndCappingParameterName())).ToInt() == EndCappingModes::Smooth)
  {
    this->CreateSmoothEndCapContour(inputROIPoints, inputLine, outputLines, lineSpacing);
  }
  else if (vtkVariant(this->GetConversionParameter(this->GetEndCappingParameterName())).ToInt() == EndCappingModes::Straight)
  {
    this->CreateStraightEndCapContour(inputROIPoints, inputLine, outputLines, lineSpacing);
  }
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::CreateStraightEndCapContour(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkCellArray* outputLines, double lineSpacing)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkPolyData");
    return;
  }

  if (!inputLine)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkLine");
    return;
  }

  if (!outputLines)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkCellArray");
  }

  vtkSmartPointer<vtkPoints> inputPoints = inputROIPoints->GetPoints();
  vtkNew<vtkIdList> endCapLine;
  for (int i = 0; i < inputLine->GetNumberOfPoints(); ++i)
  {
    double endCapPoint[3] = { 0.0, 0.0, 0.0 };
    inputPoints->GetPoint(inputLine->GetPointId(i), endCapPoint);
    endCapPoint[2] += lineSpacing / 2.0;
    endCapLine->InsertNextId(inputPoints->InsertNextPoint(endCapPoint));
  }
  outputLines->InsertNextCell(endCapLine);
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::CreateSmoothEndCapContour(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkCellArray* outputLines, double lineSpacing)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkPolyData");
    return;
  }

  if (!inputLine)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkLine");
    return;
  }

  if (!outputLines)
  {
    vtkErrorMacro("CreateEndCapContour: invalid vtkCellArray");
  }

  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->Initialize();
  lines->InsertNextCell(inputLine);

  vtkSmartPointer<vtkPolyData> linePolyData = vtkSmartPointer<vtkPolyData>::New();
  linePolyData->Initialize();
  linePolyData->SetPoints(inputROIPoints->GetPoints());
  linePolyData->SetLines(lines);

  // Remove unused points
  vtkNew<vtkCleanPolyData> cleanFilter;
  cleanFilter->SetInputData(linePolyData);
  cleanFilter->Update();
  linePolyData->ShallowCopy(cleanFilter->GetOutput());

  double bounds[6] = { 0, 0, 0, 0, 0, 0 };
  linePolyData->GetBounds(bounds);

  // Calculate the spacing using alternative dimensions
  double alternativeSpacing[2] = { 0, 0 };
  alternativeSpacing[0] = (bounds[1] - bounds[0]) / this->AlternativeDimensions[0];
  alternativeSpacing[1] = (bounds[3] - bounds[2]) / this->AlternativeDimensions[1];

  // The image spacing is set to either the default or alternative, whichever is smaller
  double spacing[3] = { 1.0, 1.0, 1.0 };

  if (alternativeSpacing[0] > 0 && alternativeSpacing[1] > 0)
  {
    spacing[0] = std::min(this->DefaultSpacing[0], alternativeSpacing[0]);
    spacing[1] = std::min(this->DefaultSpacing[1], alternativeSpacing[1]);
  }

  // Add a border of pixels to the outside of the image
  // Recalculate the image bounds based on the added padding and spacing
  bounds[0] -= (this->ImagePadding[0] / 2) * spacing[0];
  bounds[1] += (this->ImagePadding[0] / 2) * spacing[0];
  bounds[2] -= (this->ImagePadding[0] / 2) * spacing[1];
  bounds[3] += (this->ImagePadding[0] / 2) * spacing[1];

  int dimensions[3] = { static_cast<int>(std::ceil((bounds[1] - bounds[0]) / spacing[0])),
                        static_cast<int>(std::ceil((bounds[3] - bounds[2]) / spacing[1])),
                        1 };
  double origin[3] = { bounds[0], bounds[2], bounds[4] };
  int extent[6] = { 0, dimensions[0] - 1, 0, dimensions[1] - 1, 0, dimensions[2] - 1 };

  vtkSmartPointer<vtkImageData> blankImage = vtkSmartPointer<vtkImageData>::New();
  blankImage->SetSpacing(spacing);
  blankImage->SetExtent(extent);
  blankImage->SetOrigin(origin);
  blankImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  void* blankImagePtr = blankImage->GetScalarPointerForExtent(blankImage->GetExtent());
  if (!blankImagePtr)
  {
    vtkErrorMacro("CreateExternalLine: Failed to allocate memory for output labelmap image!");
    return;
  }
  else
  {
    blankImage->GetExtent(extent);
    memset(blankImagePtr, 0, ((extent[1] - extent[0] + 1)*(extent[3] - extent[2] + 1)*(extent[5] - extent[4] + 1) * blankImage->GetScalarSize() * blankImage->GetNumberOfScalarComponents()));
  }

  // Convert the polydata to an image stencil
  vtkSmartPointer<vtkPolyDataToImageStencil> polyDataToImageStencil = vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  polyDataToImageStencil->SetInputData(linePolyData);
  polyDataToImageStencil->SetOutputSpacing(spacing);
  polyDataToImageStencil->SetOutputOrigin(origin);
  polyDataToImageStencil->SetOutputWholeExtent(extent);

  // Convert the stencil to a binary image
  vtkSmartPointer<vtkImageStencil> stencil = vtkSmartPointer<vtkImageStencil>::New();
  stencil->SetInputData(blankImage);
  stencil->SetStencilConnection(polyDataToImageStencil->GetOutputPort());
  stencil->ReverseStencilOn();
  stencil->SetBackgroundValue(1);
  stencil->Update();

  vtkSmartPointer<vtkImageData> newContourImage = vtkSmartPointer<vtkImageData>::New();
  newContourImage = stencil->GetOutput();

  // Calculate the number of non-zero pixels in the image
  vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();
  imageAccumulate->SetInputData(newContourImage);
  imageAccumulate->IgnoreZeroOn();
  imageAccumulate->Update();

  int totalNumberOfVoxels = imageAccumulate->GetVoxelCount();
  int numberOfVoxels = totalNumberOfVoxels;
  int voxelDifference = VTK_INT_MAX;

  // Loop while the number of voxels in the image is still greater than half the original number,
  // and while the number of voxels is still changing
  while (numberOfVoxels > totalNumberOfVoxels / 2 && voxelDifference > 0)
  {

    // Erode the image using a kernel of size 5, 5, 1
    vtkSmartPointer<vtkImageDilateErode3D> imageDilateErode3D = vtkSmartPointer<vtkImageDilateErode3D>::New();
    imageDilateErode3D->SetErodeValue(1);
    imageDilateErode3D->SetKernelSize(5, 5, 1);
    imageDilateErode3D->SetInputData(newContourImage);
    imageDilateErode3D->Update();
    newContourImage = imageDilateErode3D->GetOutput();

    // Calculate the number of voxels in the new image
    imageAccumulate->SetInputData(newContourImage);
    imageAccumulate->Update();
    voxelDifference = numberOfVoxels - imageAccumulate->GetVoxelCount();
    numberOfVoxels -= voxelDifference;
  }

  // Create contours from the image
  vtkSmartPointer<vtkMarchingSquares> marchingSquares = vtkSmartPointer<vtkMarchingSquares>::New();
  marchingSquares->SetInputData(newContourImage);
  marchingSquares->SetNumberOfContours(1);
  marchingSquares->SetValue(0, 1.0);

  // Connect the contours in a continuous line
  vtkSmartPointer<vtkStripper> stripper = vtkSmartPointer<vtkStripper>::New();
  stripper->SetInputConnection(marchingSquares->GetOutputPort());
  stripper->SetMaximumLength(VTK_INT_MAX);
  stripper->Update();

  // Fix the lines to prevent later issues
  // TODO: This will need to be changed when the true cause is found
  vtkSmartPointer<vtkPolyData> newLines = vtkSmartPointer<vtkPolyData>::New();
  this->FixLines(stripper->GetOutput(), newLines);

  // Calculate the decimation factor with the following formula: ( # of lines in input * number of points in original line ) / number of points in input
  double decimationFactor = (1.0 * newLines->GetNumberOfLines() * inputLine->GetNumberOfPoints() + 1) / newLines->GetNumberOfPoints();

  // Reduce the number of points in the line until the ratio between the input and output lines meets the specified decimation factor
  this->DecimateLines(newLines, decimationFactor);

  vtkSmartPointer<vtkPoints> inputPoints = inputROIPoints->GetPoints();

  // If a line is successfully created, the use it for end capping.
  // Otherwise, create an external line by extending the original contour by a 1/2 slice thickness in the Z direction.
  if (newLines && newLines->GetNumberOfLines() > 0 && newLines->GetNumberOfPoints() > 0)
  {
    vtkSmartPointer<vtkPoints> points = newLines->GetPoints();

    // Loop through all of the lines generated
    for (int currentLineId = 0; currentLineId < newLines->GetNumberOfLines(); ++currentLineId)
    {
      vtkSmartPointer<vtkIdList> outputLinePointIds = vtkSmartPointer<vtkIdList>::New();
      outputLinePointIds->Initialize();

      vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
      currentLine->DeepCopy(newLines->GetCell(currentLineId));

      vtkSmartPointer<vtkLine> newLine = vtkSmartPointer<vtkLine>::New();
      if (this->IsLineClockwise(newLines, currentLine))
      {
        this->ReverseLine(currentLine, newLine);
      }
      else
      {
        newLine = currentLine;
      }

      // Loop through the points in the current line
      for (int currentPointIndex = 0; currentPointIndex < newLine->GetNumberOfPoints(); ++currentPointIndex)
      {
        vtkIdType currentPointId = newLine->GetPointId(currentPointIndex);

        double currentPoint[3] = { 0,0,0 };
        points->GetPoint(currentPointId, currentPoint);
        currentPoint[2] += lineSpacing / 2;

        vtkIdType inputPointId = inputPoints->InsertNextPoint(currentPoint);
        outputLinePointIds->InsertNextId(inputPointId);
      }

      // Make sure the line is closed and add it to the output
      if (outputLinePointIds->GetId(0) != outputLinePointIds->GetId(outputLinePointIds->GetNumberOfIds() - 1))
      {
        outputLinePointIds->InsertNextId(outputLinePointIds->GetId(0));
      }
      outputLines->InsertNextCell(outputLinePointIds);
    }
  }
  // If there is something wrong with the lines generated, create an external line by extending the
  // original contour by a 1/2 slice thickness in the Z direction.
  else
  {
    vtkSmartPointer<vtkPoints> inputLinePoints = inputLine->GetPoints();

    vtkSmartPointer<vtkLine> outputLine = vtkSmartPointer<vtkLine>::New();
    vtkSmartPointer<vtkPoints> outputLinePoints = outputLine->GetPoints();
    outputLinePoints->Initialize();
    vtkSmartPointer<vtkIdList> outputLinePointIds = outputLine->GetPointIds();
    outputLinePointIds->Initialize();

    int numberOfPoints = inputLine->GetNumberOfPoints();

    // Loop through all of the points in the current line
    for (int currentPointId = 0; currentPointId < numberOfPoints - 1; ++currentPointId)
    {
      double currentPoint[3] = { 0,0,0 };
      inputLinePoints->GetPoint(currentPointId, currentPoint);
      currentPoint[2] += lineSpacing / 2;

      vtkIdType inputPointId = inputPoints->InsertNextPoint(currentPoint);
      outputLinePointIds->InsertNextId(inputPointId);
    }

    outputLinePointIds->InsertNextId(outputLinePointIds->GetId(0));
    outputLines->InsertNextCell(outputLine);

  }
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::TriangulateContourInterior(vtkLine* inputLine, vtkCellArray* outputPolys, bool normalsUp)
{
  if (!inputLine)
  {
    vtkErrorMacro("TriangulateContourInterior: Invalid vtkLine!");
    return;
  }

  if (!outputPolys)
  {
    vtkErrorMacro("TriangulateContourInterior: Invalid vtkCellArray!");
    return;
  }

  vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
  line->DeepCopy(inputLine);

  // Make sure that the input line is not closed
  // This is required by the vtkPolygon triangulation algorithm
  if (line->GetPointId(0) == line->GetPointId(line->GetNumberOfPoints() - 1))
  {
    line->GetPointIds()->SetNumberOfIds(line->GetPointIds()->GetNumberOfIds() - 1);
  }

  // Convert the line to a vtkPolygon so that it can be used in the triangulation process
  vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();
  polygon->DeepCopy(line);

  // Triangulate the inside of the line
  vtkSmartPointer<vtkIdList> polygonIds = vtkSmartPointer<vtkIdList>::New();
  polygon->Triangulate(polygonIds);

  // Loop through the polygons created by the polygon triangulation
  for (int currentPolygonIndex = 0; currentPolygonIndex < polygonIds->GetNumberOfIds(); currentPolygonIndex += 3)
  {
    outputPolys->InsertNextCell(3);

    // Add the triangles to the mesh, making sure that the normals for the triangles are facing the correct direction
    if (normalsUp)
    {
      outputPolys->InsertCellPoint(inputLine->GetPointId(polygonIds->GetId(currentPolygonIndex)));
      outputPolys->InsertCellPoint(inputLine->GetPointId(polygonIds->GetId(currentPolygonIndex + 1)));
      outputPolys->InsertCellPoint(inputLine->GetPointId(polygonIds->GetId(currentPolygonIndex + 2)));
    }
    else
    {
      outputPolys->InsertCellPoint(inputLine->GetPointId(polygonIds->GetId(currentPolygonIndex + 2)));
      outputPolys->InsertCellPoint(inputLine->GetPointId(polygonIds->GetId(currentPolygonIndex + 1)));
      outputPolys->InsertCellPoint(inputLine->GetPointId(polygonIds->GetId(currentPolygonIndex)));
    }
  }

}

//----------------------------------------------------------------------------
vtkIdType vtkPlanarContourToClosedSurfaceConversionRule::GetNextLocation(vtkIdType currentLocation, int numberOfPoints, bool loopClosed)
{
  if (currentLocation + 1 == numberOfPoints) // If the current location is the last point.
  {
    if (loopClosed)
    {
      // Skip the repeated point.
      return 1;
    }
    return 0;
  }
  return currentLocation + 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkPlanarContourToClosedSurfaceConversionRule::GetPreviousLocation(vtkIdType currentLocation, int numberOfPoints, bool loopClosed)
{
  if (currentLocation == 0) // If the current location is the first point.
  {
    if (loopClosed)
    {
      // Loop is closed, return second to last point
      return numberOfPoints - 2;
    }
    // Return last point
    return numberOfPoints - 1;
  }
  return currentLocation - 1;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::FixLines(vtkPolyData* oldLines, vtkPolyData* newLines)
{
  if (!oldLines)
  {
    vtkErrorMacro("FixLines: Invalid vtkPolyData!");
    return;
  }

  if (!newLines)
  {
    vtkErrorMacro("FixLines: Invalid vtkPolyData!");
    return;
  }

  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->Initialize();

  for (vtkIdType lineId = 0; lineId < oldLines->GetNumberOfLines(); ++lineId)
  {
    vtkSmartPointer<vtkLine> oldLine = vtkSmartPointer<vtkLine>::New();
    oldLine->DeepCopy(oldLines->GetCell(lineId));

    // We identified an issue with vtkMarchingSquares that caused the some of the lines generated
    // to loop back on themselves by the third point. This check causes these contours to be ignored.
    //  When there is only one contour, it is not acceptable to throw it away.
    // TODO: This method is not working well and needs to be improved.
    if (oldLine->GetNumberOfPoints() <= 2)
    {
      continue;
    }
    else if (oldLine->GetPointId(0) == oldLine->GetPointId(2) && oldLine->GetNumberOfPoints() != 3)
    {

      if (oldLines->GetNumberOfLines() > 1)
      {
        continue;
      }

      vtkSmartPointer<vtkLine> fixedLine = vtkSmartPointer<vtkLine>::New();
      this->FixLine(oldLine, fixedLine);

      if (fixedLine->GetNumberOfPoints() > 1)
      {
        lines->InsertNextCell(fixedLine);
      }
    }
    else
    {
      if (oldLine->GetNumberOfPoints() > 1)
      {
        lines->InsertNextCell(oldLine);
      }
    }

  }

  newLines->SetPoints(oldLines->GetPoints());
  newLines->SetLines(lines);
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::FixLine(vtkLine* oldLine, vtkLine* newLine)
{

  if (!oldLine)
  {
    vtkErrorMacro("FixLine: Invalid vtkLine!");
    return;
  }

  if (!newLine)
  {
    vtkErrorMacro("FixLine: Invalid vtkLine!");
    return;
  }

  vtkSmartPointer<vtkIdList> oldIdList = oldLine->GetPointIds();
  vtkSmartPointer<vtkPoints> oldPoints = oldLine->GetPoints();

  vtkSmartPointer<vtkPoints> newPoints = newLine->GetPoints();
  newPoints->Initialize();

  vtkSmartPointer<vtkIdList> newIdList = newLine->GetPointIds();
  newIdList->Initialize();

  // Loop through the points in the original line, except the first and the last and add it to the new line
  for (int currentIndex = 1; currentIndex < oldIdList->GetNumberOfIds() - 1; ++currentIndex)
  {
    vtkIdType oldId = oldIdList->GetId(currentIndex);

    double point[3] = { 0,0,0 };
    oldPoints->GetPoint(oldId, point);

    // Add the point to the new line
    newPoints->InsertPoint(oldId, point);
    newIdList->InsertNextId(oldId);
  }
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::DecimateLines(vtkPolyData* inputPolyData, double decimationFactor)
{

  if (!inputPolyData)
  {
    vtkErrorMacro("DecimateLines: Invalid vtkPolyData!");
    return;
  }

  vtkSmartPointer<vtkCellArray> inputLines = inputPolyData->GetLines();
  vtkSmartPointer<vtkPoints> inputPoints = inputPolyData->GetPoints();

  vtkSmartPointer<vtkCellArray> outputLines = vtkSmartPointer<vtkCellArray>::New();

  vtkSmartPointer<vtkPriorityQueue> priorityQueue = vtkSmartPointer<vtkPriorityQueue>::New();

  // Loop through all of the lines
  for (vtkIdType inputLineId = 0; inputLineId < inputPolyData->GetNumberOfLines(); ++inputLineId)
  {

    vtkSmartPointer<vtkLine> outputLine = vtkSmartPointer<vtkLine>::New();
    outputLine->DeepCopy(inputPolyData->GetCell(inputLineId));

    vtkSmartPointer<vtkLine> inputLine = vtkSmartPointer<vtkLine>::New();
    inputLine->ShallowCopy(inputPolyData->GetCell(inputLineId));

    vtkSmartPointer<vtkIdList> outputLineIds = outputLine->GetPointIds();

    // If there are less than 2 points, then just copy the line
    if (outputLineIds->GetNumberOfIds() > 2)
    {
      //// Loop through the points in the current line
      for (int pointIndex = 0; pointIndex < outputLineIds->GetNumberOfIds(); ++pointIndex)
      {
        // Add the current point to the output line
        vtkIdType pointId = outputLineIds->GetId(pointIndex);

        // Calculate the error and add it to the priority queue
        priorityQueue->Insert(this->ComputeError(inputPoints, outputLineIds, pointIndex), pointId);
      }

      // While the priority queue is not empty, doesn't contain any errors that are less than the machine epsilon,
      // and while the ratio of the # output points / # input points is greater than the decimation factor
      while (priorityQueue->GetNumberOfItems() > 3 &&
        ((priorityQueue->GetPriority(priorityQueue->Peek()) < VTK_DBL_EPSILON) ||
        (1.0 * outputLineIds->GetNumberOfIds() / inputLine->GetNumberOfPoints() > decimationFactor)))
      {
        // Remove the point from the outputLineIds
        this->RemovePointDecimation(outputLineIds, priorityQueue->Pop());
      }

    }

    // Make sure the contour is closed
    if (outputLineIds->GetNumberOfIds() > 1)
    {
      if (outputLineIds->GetId(0) != outputLineIds->GetId(outputLineIds->GetNumberOfIds() - 1))
      {
        outputLineIds->InsertNextId(outputLineIds->GetId(0));
      }
    }

    // If there are no points, then the line doesn't need to be added
    if (outputLineIds->GetNumberOfIds() > 1)
    {
      outputLines->InsertNextCell(outputLineIds);
    }

  }

  // Set the output data
  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  outputPolyData->SetPoints(inputPoints);
  outputPolyData->SetLines(outputLines);
  inputPolyData->DeepCopy(outputPolyData);

}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::RemovePointDecimation(vtkIdList* originalIdList, vtkIdType pointId)
{

  if (!originalIdList)
  {
    vtkErrorMacro("RemovePointDecimation: Invalid vtkIdList!");
    return;
  }

  vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
  idList->Initialize();
  for (int i = 0; i < originalIdList->GetNumberOfIds(); ++i)
  {
    vtkIdType id = originalIdList->GetId(i);

    if (pointId != id)
    {
      idList->InsertNextId(id);
    }
  }
  originalIdList->DeepCopy(idList);

}

//----------------------------------------------------------------------------
double vtkPlanarContourToClosedSurfaceConversionRule::ComputeError(vtkPoints* points, vtkIdList* lineIds, vtkIdType pointIndex)
{
  if (!points)
  {
    vtkErrorMacro("ComputeError: Invalid vtkPoints!");
    return 0.0;
  }

  if (!lineIds)
  {
    vtkErrorMacro("ComputeError: Invalid vtkIdList!");
    return 0.0;
  }

  bool isClosed = (lineIds->GetId(0) == lineIds->GetId(lineIds->GetNumberOfIds() - 1));

  vtkIdType currentId = lineIds->GetId(pointIndex);
  vtkIdType nextId = lineIds->GetId(this->GetNextLocation(pointIndex, lineIds->GetNumberOfIds(), isClosed));
  vtkIdType previousId = lineIds->GetId(this->GetPreviousLocation(pointIndex, lineIds->GetNumberOfIds(), isClosed));

  double currentPoint[3] = { 0,0,0 };
  points->GetPoint(currentId, currentPoint);

  double nextPoint[3] = { 0,0,0 };
  points->GetPoint(nextId, nextPoint);

  double previousPoint[3] = { 0,0,0 };
  points->GetPoint(previousId, previousPoint);

  // The the points are coincident, there is no line. Return 0.0
  if (vtkMath::Distance2BetweenPoints(previousPoint, nextPoint) == 0.0)
  {
    return 0.0;
  }
  else
  {
    // Return the distance from the current point to the line defined by the previous and next point
    return vtkLine::DistanceToLine(currentPoint, nextPoint, previousPoint);
  }
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::CalculateContourTransform(vtkPolyData* inputPolyData, vtkMatrix4x4* contourToRAS)
{
  double zVector[3] = { 0, 0, 1 };
  double meshNormal[3] = { 0, 0, 0 };

  // Calculate the normal vector for the surface mesh by iterating through all of the contours
  // Ignores small contours (less than 6 points) since the plane fitting is sometimes unreliable
  this->CalculateContourNormal(inputPolyData, meshNormal, 6);
  if (vtkMath::Norm(meshNormal) == 0.0)
  {
    // If the normal vector that was returned was zero (no contours with less than 6 points)
    // Run the calculation again with no restriction on the number of points used
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
void vtkPlanarContourToClosedSurfaceConversionRule::CalculateContourNormal(vtkPolyData* inputPolyData, double outputNormal[3], int minimumContourSize)
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
  // Return before divide by 0 error
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
