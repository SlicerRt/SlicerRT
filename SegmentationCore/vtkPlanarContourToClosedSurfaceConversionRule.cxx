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
#include <vtkVersion.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkMath.h>
#include <vtkIdList.h>
#include <vtkDelaunay2D.h>

//----------------------------------------------------------------------------
vtkSegmentationConverterRuleNewMacro(vtkPlanarContourToClosedSurfaceConversionRule);

//----------------------------------------------------------------------------
vtkPlanarContourToClosedSurfaceConversionRule::vtkPlanarContourToClosedSurfaceConversionRule()
{
  //this->ConversionParameters[GetXYParameterName()] = std::make_pair("value", "description");
}

//----------------------------------------------------------------------------
vtkPlanarContourToClosedSurfaceConversionRule::~vtkPlanarContourToClosedSurfaceConversionRule()
{
}

//----------------------------------------------------------------------------
unsigned int vtkPlanarContourToClosedSurfaceConversionRule::GetConversionCost(vtkDataObject* sourceRepresentation/*=NULL*/, vtkDataObject* targetRepresentation/*=NULL*/)
{
  // Rough input-independent guess (ms)
  return 700;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkPlanarContourToClosedSurfaceConversionRule::ConstructRepresentationObjectByRepresentation(std::string representationName)
{
  if ( !representationName.compare(this->GetSourceRepresentationName())
    || !representationName.compare(this->GetTargetRepresentationName()) )
  {
    return (vtkDataObject*)vtkPolyData::New();
  }
  else
  {
    return NULL;
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
    return NULL;
  }
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::Convert(vtkDataObject* sourceRepresentation, vtkDataObject* targetRepresentation)
{
  // Check validity of source and target representation objects
  vtkPolyData* planarContoursPolyData = vtkPolyData::SafeDownCast(sourceRepresentation);
  if (!planarContoursPolyData)
  {
    vtkErrorMacro("Convert: Source representation is not a poly data!");
    return false;
  }
  vtkPolyData* closedSurfacePolyData = vtkPolyData::SafeDownCast(targetRepresentation);
  if (!closedSurfacePolyData)
  {
    vtkErrorMacro("Convert: Target representation is not a poly data!");
    return false;
  }

  vtkSmartPointer<vtkPolyData> inputContoursCopy = vtkSmartPointer<vtkPolyData>::New();
  inputContoursCopy->DeepCopy(planarContoursPolyData);

  vtkSmartPointer<vtkPoints> points = inputContoursCopy->GetPoints();
  vtkSmartPointer<vtkCellArray> lines = inputContoursCopy->GetLines();
  vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New(); // add triangles to this

  int numberOfLines = inputContoursCopy->GetNumberOfLines(); // total number of lines

  // remove keyholes form the lines
  this->FixKeyholes(inputContoursCopy, numberOfLines, 0.1, 2);

  // set all lines to be clockwise
  this->SetLinesClockwise(inputContoursCopy);

  // Get two consecutive planes.
  int line1Index = 0; // pointer to first line on plane 1.
  int numberOfLinesInContour1 = this->GetNumberOfLinesOnPlane(inputContoursCopy, numberOfLines, 0);

  while (line1Index + numberOfLinesInContour1 < numberOfLines)
  {
    int line2Index = line1Index+numberOfLinesInContour1; // pointer to first line on plane 2
    int numberOfLinesInContour2 = this->GetNumberOfLinesOnPlane(inputContoursCopy, numberOfLines, line2Index); // number of lines on plane 2

    // initialize DoLinesOverlaps lists. - list of list
    // Each internal list represents a line from the plane and will store the pointers to the DoLinesOverlapping lines

    // DoLinesOverlaps for lines from plane 1
    std::vector< std::vector< int > > DoLinesOverlaps1;
    for (int contourIndex1 = 0; contourIndex1 < numberOfLinesInContour1; contourIndex1++)
    {
      std::vector< int > temp;
      DoLinesOverlaps1.push_back(temp);
    }

    // DoLinesOverlaps for lines from plane 2
    std::vector< std::vector< int > > DoLinesOverlaps2;
    for (int contourIndex2 = 0; contourIndex2 < numberOfLinesInContour2; contourIndex2++)
    {
      std::vector< int > temp;
      DoLinesOverlaps2.push_back(temp);
    }

    // Fill the DoLinesOverlaps lists.
    for (int contourIndex1 = 0; contourIndex1 < numberOfLinesInContour1; contourIndex1++)
    {
      vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
      line1->DeepCopy(inputContoursCopy->GetCell(line1Index+contourIndex1));

      for (int contourIndex2=0; contourIndex2< numberOfLinesInContour2; contourIndex2++)
      {
        vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
        line2->DeepCopy(inputContoursCopy->GetCell(line2Index+contourIndex2));

        if (this->DoLinesOverlap(line1, line2))
        {
          // line from plane 1 DoLinesOverlaps with line from plane 2
          DoLinesOverlaps1[contourIndex1].push_back(line2Index+contourIndex2);
          DoLinesOverlaps2[contourIndex2].push_back(line1Index+contourIndex1);
        }
      }
    }

    // Go over the DoLinesOverlaps lists.
    for (int currentLine1Index = line1Index; currentLine1Index < line1Index+numberOfLinesInContour1; currentLine1Index++)
    {
      vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
      line1->DeepCopy(inputContoursCopy->GetCell(currentLine1Index));

      vtkSmartPointer<vtkIdList> pointsInLine1 = line1->GetPointIds();
      int numberOfPointsInLine1 = line1->GetNumberOfPoints();

      bool intersects = false;

      for (int DoLinesOverlapLineIndex = 0; DoLinesOverlapLineIndex < DoLinesOverlaps1[currentLine1Index-line1Index].size(); DoLinesOverlapLineIndex++) // lines on plane 2 that DoLinesOverlap with line i
      {
        int currentLine2Index = DoLinesOverlaps1[currentLine1Index-line1Index][DoLinesOverlapLineIndex];

        vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
        line2->DeepCopy(inputContoursCopy->GetCell(currentLine2Index));
        vtkSmartPointer<vtkIdList> pointsInLine2 = line2->GetPointIds();
        int numberOfPointsInLine2 = line2->GetNumberOfPoints();

        // Deal with possible branches.

        // Get the portion of line 1 that is close to line 2,
        vtkSmartPointer<vtkLine> dividedLine1 = vtkSmartPointer<vtkLine>::New();
        this->Branch(inputContoursCopy, pointsInLine1, numberOfPointsInLine1, currentLine2Index, DoLinesOverlaps1[currentLine1Index-line1Index], dividedLine1);
        vtkSmartPointer<vtkIdList> dividedPointsInLine1 = dividedLine1->GetPointIds();
        int numberOfdividedPointsInLine1 = dividedLine1->GetNumberOfPoints();

        // Get the portion of line 2 that is close to line 1.
        vtkSmartPointer<vtkLine> dividedLine2 = vtkSmartPointer<vtkLine>::New();
        this->Branch(inputContoursCopy, pointsInLine2, numberOfPointsInLine2, currentLine1Index, DoLinesOverlaps2[currentLine2Index-line2Index], dividedLine2);
        vtkSmartPointer<vtkIdList> dividedPointsInLine2 = dividedLine2->GetPointIds();
        int numberOfdividedPointsInLine2 = dividedLine2->GetNumberOfPoints();

        if (numberOfdividedPointsInLine1 > 1 && numberOfdividedPointsInLine2 > 1)
        {
          this->TriangulateContours(inputContoursCopy,
            dividedPointsInLine1, numberOfdividedPointsInLine1,
            dividedPointsInLine2, numberOfdividedPointsInLine2,
            polys, points);
        }
      }
    }

    // Advance the points
    line1Index = line2Index;
    numberOfLinesInContour1 = numberOfLinesInContour2;
  }

  // Triangulate all contours which are exposed.
  this->SealMesh( inputContoursCopy, lines, polys );

  // Initialize the output data.
  closedSurfacePolyData->SetPoints(points);
  // closedSurfacePolyData->SetLines(lines); // Do not include lines in poly data for nicer visualization
  closedSurfacePolyData->SetPolys(polys);
  
  return true;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::TriangulateContours(vtkPolyData* inputROIPoints,
                                                  vtkIdList* pointsInLine1, int numberOfPointsInLine1,
                                                  vtkIdList* pointsInLine2, int numberOfPointsInLine2,
                                                  vtkCellArray* polys, vtkPoints* points)
{

  if(! inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  if (!pointsInLine1)
  {
    vtkErrorMacro("pointsInLine1: Invalid vtkIdList!");
    return;
  }

  if (!pointsInLine2)
  {
   vtkErrorMacro("pointsInLine2: Invalid vtkIdList!");
  }

  // Pre-calculate and store the closest points.

  // Closest point from line 1 to line 2
  std::vector< int > closest1;
  for (int line1PointIndex = 0; line1PointIndex < numberOfPointsInLine1; line1PointIndex++)
  {
    double line1Point[3] = {0,0,0};
    inputROIPoints->GetPoint(pointsInLine1->GetId(line1PointIndex), line1Point);

    closest1.push_back(this->GetClosestPoint(inputROIPoints, line1Point, pointsInLine2, numberOfPointsInLine2));
  }

  // closest from line 2 to line 1
  std::vector< int > closest2;
  for (int line2PointIndex = 0; line2PointIndex < numberOfPointsInLine2; line2PointIndex++)
  {
    double line2Point[3] = {0,0,0};
    inputROIPoints->GetPoint(pointsInLine2->GetId(line2PointIndex),line2Point);

    closest2.push_back(this->GetClosestPoint(inputROIPoints, line2Point, pointsInLine1, numberOfPointsInLine1));
  }

  // Orient loops.
  // Use the 0th point on line 1 and the closest point on line 2.
  int startLine1 = 0;
  int startLine2 = closest1[0];

  double firstPointLine1[3] = {0,0,0}; // first point on line 1;
  inputROIPoints->GetPoint(pointsInLine1->GetId(startLine1),firstPointLine1);

  double firstPointLine2[3] = {0,0,0}; // first point on line 2;
  inputROIPoints->GetPoint(pointsInLine2->GetId(startLine2), firstPointLine2);

  double distanceBetweenPoints = vtkMath::Distance2BetweenPoints(firstPointLine1, firstPointLine2);

  // Determine if the loops are closed.
  // A loop is closed if the first point is repeated as the last point.
  bool line1Closed = (pointsInLine1->GetId(0) == pointsInLine1->GetId(numberOfPointsInLine1-1));
  bool line2Closed = (pointsInLine2->GetId(0) == pointsInLine2->GetId(numberOfPointsInLine2-1));

  // Determine the ending points.
  int endLinePoint1 = this->GetEndLoop(startLine1, numberOfPointsInLine1, line1Closed);
  int endLinePoint2 = this->GetEndLoop(startLine2, numberOfPointsInLine2, line2Closed);

  // for backtracking
  int left = -1;
  int up = 1;

  // Initialize the Dynamic Programming table.
  // Rows represent line 1. Columns represent line 2.

  // Fill the first row.
  std::vector< double > firstRow;
  firstRow.push_back(distanceBetweenPoints);

  std::vector< int > backtrackRow;
  backtrackRow.push_back(0);

  int currentPointIndexLine2 = this->GetNextLocation(startLine2, numberOfPointsInLine2, line2Closed);

  for (int line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; line2PointIndex++)
  {
    double currentPointLine2[3] = {0,0,0}; // current point on line 2
    inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIndexLine2),currentPointLine2);

    // Use the distance between first point on line 1 and current point on line 2.
    double distance = vtkMath::Distance2BetweenPoints(firstPointLine1, currentPointLine2);

    firstRow.push_back(firstRow[line2PointIndex-1]+distance);
    backtrackRow.push_back(left);

    currentPointIndexLine2 = this->GetNextLocation(currentPointIndexLine2, numberOfPointsInLine2, line2Closed);

  }

  // Fill the first column.
  std::vector< std::vector< double > > score;
  score.push_back(firstRow);

  std::vector< std::vector< int > > backtrack;
  backtrack.push_back(backtrackRow);

  int currentPointIndexLine1 = this->GetNextLocation(startLine1, numberOfPointsInLine2, line1Closed);

  for( int line1PointIndex=1; line1PointIndex < numberOfPointsInLine1; line1PointIndex++)
  {
    double currentPointLine1[3] = {0,0,0}; // current point on line 1
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIndexLine1), currentPointLine1);

    // Use the distance between first point on line 2 and current point on line 1.
    double distance = vtkMath::Distance2BetweenPoints(currentPointLine1, firstPointLine2);

    std::vector< double > newScoreRow;
    newScoreRow.push_back(score[line1PointIndex-1][0]+distance);
    for(int line2PointIndex = 0; line2PointIndex < numberOfPointsInLine2-1; line2PointIndex++)
    {
      newScoreRow.push_back(0);
    }
    score.push_back(newScoreRow);

    std::vector< int > newBacktrackRow;
    newBacktrackRow.push_back(up);
    for(int line2PointIndex = 0; line2PointIndex < numberOfPointsInLine2-1; line2PointIndex++)
    {
      newBacktrackRow.push_back(0);
    }
    backtrack.push_back(newBacktrackRow);

    currentPointIndexLine1 = this->GetNextLocation(currentPointIndexLine1, numberOfPointsInLine1, line1Closed);
  }

  // Fill the rest of the table.
  int previousLine1 = startLine1;
  int previousLine2 = startLine2;

  currentPointIndexLine1 = this->GetNextLocation(startLine1, numberOfPointsInLine1, line1Closed);
  currentPointIndexLine2 = this->GetNextLocation(startLine2, numberOfPointsInLine2, line2Closed);

  int line1PointIndex=1;
  int line2PointIndex=1;
  for (line1PointIndex = 1; line1PointIndex< numberOfPointsInLine1; line1PointIndex++)
  {
    double pointOnLine1[3] = {0,0,0};
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIndexLine1), pointOnLine1);

    for (line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; line2PointIndex++)
    {
      double pointOnLine2[3] = {0,0,0};
      inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIndexLine2), pointOnLine2);

      double distance = vtkMath::Distance2BetweenPoints(pointOnLine1, pointOnLine2);

      // Use the pre-calcualted closest point.
      if (currentPointIndexLine1 == closest2[previousLine2])
      {
        score[line1PointIndex][line2PointIndex] = score[line1PointIndex][line2PointIndex-1]+distance;
        backtrack[line1PointIndex][line2PointIndex] = left;

      }
      else if (currentPointIndexLine2 == closest1[previousLine1])
      {
        score[line1PointIndex][line2PointIndex] = score[line1PointIndex-1][line2PointIndex]+distance;
        backtrack[line1PointIndex][line2PointIndex] = up;

      }
      else if (score[line1PointIndex][line2PointIndex-1] <= score[line1PointIndex-1][line2PointIndex])
      {
        score[line1PointIndex][line2PointIndex] = score[line1PointIndex][line2PointIndex-1]+distance;
        backtrack[line1PointIndex][line2PointIndex] = left;

      }
      else
      {
        score[line1PointIndex][line2PointIndex] = score[line1PointIndex-1][line2PointIndex]+distance;
        backtrack[line1PointIndex][line2PointIndex] = up;

      }

      // Advance the pointers
      previousLine2 = currentPointIndexLine2;
      currentPointIndexLine2 = this->GetNextLocation(currentPointIndexLine2, numberOfPointsInLine2, line2Closed);
    }
    previousLine1 = currentPointIndexLine1;
    currentPointIndexLine1 = this->GetNextLocation(currentPointIndexLine1, numberOfPointsInLine1, line1Closed);
  }

  // Backtrack.
  currentPointIndexLine1 = endLinePoint1;
  currentPointIndexLine2 = endLinePoint2;
  line1PointIndex --;
  line2PointIndex --;
  while (line1PointIndex > 0  || line2PointIndex > 0)
  {
    double line1Point[3] = {0,0,0}; // current point on line 1
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIndexLine1), line1Point);

    double line2Point[3] = {0,0,0}; // current point on line 2
    inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIndexLine2), line2Point);

    double distanceBetweenPoints = vtkMath::Distance2BetweenPoints(line1Point, line2Point);

    if (backtrack[line1PointIndex][line2PointIndex] == left)
    {
      int previousPointIndexLine2 = this->GetPreviousLocation(currentPointIndexLine2, numberOfPointsInLine2, line2Closed);
      double previousPointLine2[3] = {0,0,0};
      inputROIPoints->GetPoint(pointsInLine2->GetId(previousPointIndexLine2),previousPointLine2);

      int currentTriangle[3] = {0,0,0};
      currentTriangle[0] = pointsInLine1->GetId(currentPointIndexLine1);
      currentTriangle[1] = pointsInLine2->GetId(currentPointIndexLine2);
      currentTriangle[2] = pointsInLine2->GetId(previousPointIndexLine2);

      polys->InsertNextCell(3);
      polys->InsertCellPoint(currentTriangle[0]);
      polys->InsertCellPoint(currentTriangle[1]);
      polys->InsertCellPoint(currentTriangle[2]);

      line2PointIndex -= 1;
      currentPointIndexLine2 = previousPointIndexLine2;
    }
    else // up
    {
      int previousPointIndexLine1 = this->GetPreviousLocation(currentPointIndexLine1, numberOfPointsInLine1, line2Closed);
      double previousPointLine1[3] = {0,0,0};
      inputROIPoints->GetPoint(pointsInLine1->GetId(previousPointIndexLine1), previousPointLine1);

      int currentTriangle[3] = {0,0,0};
      currentTriangle[0] = pointsInLine1->GetId(currentPointIndexLine1);
      currentTriangle[1] = pointsInLine2->GetId(currentPointIndexLine2);
      currentTriangle[2] = pointsInLine1->GetId(previousPointIndexLine1);

      polys->InsertNextCell(3);
      polys->InsertCellPoint(currentTriangle[0]);
      polys->InsertCellPoint(currentTriangle[1]);
      polys->InsertCellPoint(currentTriangle[2]);

      line1PointIndex -= 1;
      currentPointIndexLine1 = previousPointIndexLine1;
    }
  }
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetEndLoop(int startLoopIndex, int numberOfPoints, bool loopClosed)
{
  if (startLoopIndex != 0)
  {
    if (loopClosed)
    {
      return startLoopIndex;
    }

    return startLoopIndex-1;
  }

  // If startLoop was 0, then it doesn't matter whether or not the loop was closed.
  return numberOfPoints-1;
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetClosestPoint(vtkPolyData* inputROIPoints, double* originalPoint, vtkIdList* points, int numberOfPoints)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return 0;
  }

  if (!points)
  {
    vtkErrorMacro("points: Invalid vtkIdList!");
    return 0;
  }

  double pointOnLine[3] = {0,0,0}; // point from the given line
  pointOnLine[0] = inputROIPoints->GetPoint(points->GetId(0))[0];
  pointOnLine[1] = inputROIPoints->GetPoint(points->GetId(0))[1];
  pointOnLine[2] = inputROIPoints->GetPoint(points->GetId(0))[2];

  double minimumDistance = vtkMath::Distance2BetweenPoints(originalPoint, pointOnLine); // minimum distance from the point to the line
  double closestPointIndex = 0;

  for (int currentPointIndex = 1; currentPointIndex <numberOfPoints; currentPointIndex++)
  {
    pointOnLine[0] = inputROIPoints->GetPoint(points->GetId(currentPointIndex))[0];
    pointOnLine[1] = inputROIPoints->GetPoint(points->GetId(currentPointIndex))[1];
    pointOnLine[2] = inputROIPoints->GetPoint(points->GetId(currentPointIndex))[2];

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
void vtkPlanarContourToClosedSurfaceConversionRule::FixKeyholes(vtkPolyData* inputROIPoints, int numberOfLines, int epsilon, int minimumSeperation)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  vtkSmartPointer<vtkLine> originalLine;
  vtkSmartPointer<vtkPoints> points;
  int numberOfPoints;

  std::vector<vtkSmartPointer<vtkLine> > newLines;

  double point1[3] = {0,0,0};
  double point2[3] = {0,0,0};
  double distance;
  int pointsOfSeperation;

  std::vector<vtkSmartPointer<vtkIdList> > finishedNewLines;

  for (int lineIndex = 0; lineIndex < numberOfLines; lineIndex++)
  {
    originalLine = vtkSmartPointer<vtkLine>::New();
    originalLine->DeepCopy(inputROIPoints->GetCell(lineIndex));

    points = originalLine->GetPoints();
    numberOfPoints = originalLine->GetNumberOfPoints();

    // If the value of flags[i] is -1, the point is not part of a keyhole
    // If the value of flags[i] is >= 0, it represents a point that is
    // close enough that it could be considered part of a keyhole.
    std::vector<int> flags(numberOfPoints);
    for (int point1Index = 0; point1Index < numberOfPoints; point1Index++)
    {
      flags[point1Index] = -1;

      point1[0] = points->GetPoint(point1Index)[0];
      point1[1] = points->GetPoint(point1Index)[1];
      point1[2] = points->GetPoint(point1Index)[2];

      for (int point2Index = point1Index + 1; point2Index < numberOfPoints - 1; point2Index++)
      {
        // Make sure the points are not too close together on the line index-wise
        pointsOfSeperation = std::min(point2Index-point1Index,numberOfPoints-1-point2Index+point1Index);
        if (pointsOfSeperation > minimumSeperation)
        {
          // if the points are close together, mark both of them as part of a keyhole
          point2[0] = points->GetPoint(point2Index)[0];
          point2[1] = points->GetPoint(point2Index)[1];
          point2[2] = points->GetPoint(point2Index)[2];

          distance = vtkMath::Distance2BetweenPoints(point1,point2);
          if (distance <= epsilon)
          {
            flags[point1Index] = point2Index;
            flags[point2Index] = point1Index;
          }
        }
      }
    }

    int currentLayer = 0;

    bool pointInChannel = false;

    std::vector<vtkSmartPointer<vtkIdList> > rawNewLines;
    vtkSmartPointer<vtkLine> newLine;
    vtkSmartPointer<vtkIdList> newLinePoints;

    // Loop through all of the points in the line
    for (int currentPointIndex = 0; currentPointIndex < numberOfPoints; currentPointIndex++)
    {
      // Add a new line if neccessary
      if (currentLayer == rawNewLines.size())
      {
        newLine = vtkSmartPointer<vtkLine>::New();
        newLinePoints = newLine->GetPointIds();
        newLinePoints->Initialize();
        newLine->GetPoints()->SetData(points->GetData());

        newLines.push_back(newLine.GetPointer());
        rawNewLines.push_back(newLinePoints);
      }

      // If the current point is not part of a keyhole, add it to the current line
      if (flags[currentPointIndex] == -1)
      {
        rawNewLines[currentLayer]->InsertNextId(originalLine->GetPointId(currentPointIndex));
        pointInChannel = false;
      }
      else
      {
        // If the current point is the start of a keyhole add the point to the line,
        // increment the layer, and start the channel.
        if (flags[currentPointIndex] > currentPointIndex && !pointInChannel)
        {
          rawNewLines[currentLayer]->InsertNextId(originalLine->GetPointId(currentPointIndex));
          currentLayer += 1;
          pointInChannel = true;

        }
        // If the current point is the end of a volume in the keyhole, add the point
        // to the line, remove the current line from the working list, deincrement
        // the layer, add the current line to the finished lines and start the,
        // channel.
        else if (flags[currentPointIndex] < currentPointIndex && !pointInChannel)
        {
          rawNewLines[currentLayer]->InsertNextId(originalLine->GetPointId(currentPointIndex));
          finishedNewLines.push_back(rawNewLines[currentLayer]);
          rawNewLines.pop_back();
          currentLayer -= 1;
          pointInChannel = true;
        }
      }
    }

    // Add the remaining line to the finished list.
    for (int currentLineIndex=0; currentLineIndex < rawNewLines.size(); currentLineIndex++)
    {
      finishedNewLines.push_back(rawNewLines[currentLineIndex]);
    }

    // Seal the lines.
    for (int currentLineIndex = 0; currentLineIndex < finishedNewLines.size(); currentLineIndex++)
    {
      if (finishedNewLines[currentLineIndex]->GetNumberOfIds() != 0)
      {
        finishedNewLines[currentLineIndex]->InsertNextId(finishedNewLines[currentLineIndex]->GetId(0));
      }
    }
  }

  vtkSmartPointer<vtkCellArray> outputLines = vtkSmartPointer<vtkCellArray>::New();
  outputLines->Initialize();

  inputROIPoints->DeleteCells();

  for (int currentLineIndex = 0; currentLineIndex < newLines.size(); currentLineIndex++)
  {
    outputLines->InsertNextCell(newLines[currentLineIndex]);
  }
  inputROIPoints->SetLines(outputLines);
  inputROIPoints->BuildCells();
}


//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::SetLinesClockwise(vtkPolyData* inputROIPoints)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  int numberOfLines = inputROIPoints->GetNumberOfLines();

  std::vector<vtkSmartPointer<vtkLine> > newLines;

  for(int lineIndex=0 ; lineIndex < numberOfLines; lineIndex++)
  {
    vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
    currentLine->DeepCopy(inputROIPoints->GetCell(lineIndex));

    vtkSmartPointer<vtkPoints> currentPoints = currentLine->GetPoints();

    if (!IsLineClockwise(inputROIPoints, currentLine))
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
  for (int currentLineIndex = 0; currentLineIndex < newLines.size(); currentLineIndex++)
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
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return false;
  }

  if (!line)
  {
    vtkErrorMacro("line: Invalid vtkLine!");
    return false;
  }

  int numberOfPoints = line->GetNumberOfPoints();

  // Calculate twice the area of the line.
  double areaSum = 0;

  for (int pointIndex=0; pointIndex < numberOfPoints-1; pointIndex++)
  {
    double point1[3];
    inputROIPoints->GetPoint(line->GetPointId(pointIndex), point1);

    double point2[3];
    inputROIPoints->GetPoint(line->GetPointId(pointIndex+1), point2);

    areaSum += (point2[0]-point1[0])*(point2[1]+point1[1]);
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
    vtkErrorMacro("originalLine: Invalid vtkLine!");
    return;
  }

  if (!newLine)
  {
    vtkErrorMacro("newLine: Invalid vtkLine!");
    return;
  }

  int numberOfPoints = originalLine->GetNumberOfPoints();

  vtkSmartPointer<vtkIdList> newPoints = newLine->GetPointIds();

  for (int pointInLineIndex = numberOfPoints-1; pointInLineIndex >= 0; pointInLineIndex--)
  {
    newPoints->InsertNextId(originalLine->GetPointId(pointInLineIndex));
  }
}


//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetNumberOfLinesOnPlane(vtkPolyData* inputROIPoints, int numberOfLines, int originalLineIndex)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return 0;
  }

  double lineZ = inputROIPoints->GetCell(originalLineIndex)->GetBounds()[4]; // z-value

  int currentLineIndex = originalLineIndex+1;
  while (currentLineIndex < numberOfLines && inputROIPoints->GetCell(currentLineIndex)->GetBounds()[4] == lineZ)
  {
    currentLineIndex ++;
  }
  return currentLineIndex-originalLineIndex;
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::DoLinesOverlap(vtkLine* line1, vtkLine* line2)
{
  if (!line1)
  {
    vtkErrorMacro("line1: Invalid vtkLine!");
    return false;
  }

  if (!line2)
  {
    vtkErrorMacro("line2: Invalid vtkLine!");
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

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::Branch(vtkPolyData* inputROIPoints, vtkIdList* points, int numberOfPoints, int currentLineIndex, std::vector< int > DoLinesOverlaps, vtkLine* dividedLine)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  if (!points)
  {
    vtkErrorMacro("points: Invalid vtkIdList!");
    return;
  }

  vtkSmartPointer<vtkIdList> dividedLinePoints = dividedLine->GetPointIds();
  dividedLinePoints->Initialize();

  // Discard some points on the trunk so that the branch connects to only a part of the trunk.
  bool prev = false;

  for (int currentPointIndex = 0; currentPointIndex < numberOfPoints; currentPointIndex++)
  {
    double currentPoint[3] = {0,0,0};
    inputROIPoints->GetPoint(points->GetId(currentPointIndex), currentPoint);

    // See if the point's closest branch is the input branch.
    if (this->GetClosestBranch(inputROIPoints, currentPoint, DoLinesOverlaps) == currentLineIndex)
    {
      dividedLinePoints->InsertNextId(points->GetId(currentPointIndex));
      prev = true;
    }
    else
    {
      if (prev)
      {
        // Add one extra point to close up the surface.
        dividedLinePoints->InsertNextId(points->GetId(currentPointIndex));
      }
      prev = false;
    }
  }
  int dividedNumberOfPoints = dividedLine->GetNumberOfPoints();
  if (dividedNumberOfPoints > 1)
  {
    // Determine if the trunk was originally a closed contour.
    bool closed = (points->GetId(0) == points->GetId(numberOfPoints-1));
    if (closed && (dividedLinePoints->GetId(0) != dividedLinePoints->GetId(dividedNumberOfPoints-1)))
    {
      // Make the new one a closed contour as well.
      dividedLinePoints->InsertNextId(dividedLinePoints->GetId(0));
    }
  }
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetClosestBranch(vtkPolyData* inputROIPoints, double originalPoint[], std::vector< int > DoLinesOverlaps)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
  }

  vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
  double bestLineDistance = 0;
  int closestLineIndex = 0;

  for (int currentDoLinesOverlapIndex = 0; currentDoLinesOverlapIndex < DoLinesOverlaps.size(); currentDoLinesOverlapIndex++)
  {
    line->DeepCopy(inputROIPoints->GetCell(DoLinesOverlaps[currentDoLinesOverlapIndex]));
    vtkSmartPointer<vtkIdList> points = line->GetPointIds();
    int numberOfPoints = line->GetNumberOfPoints();

    double branchPoint[3] = {0,0,0}; // a point from the branch
    inputROIPoints->GetPoint(points->GetId(0), branchPoint);

    double minimumDistance = vtkMath::Distance2BetweenPoints(originalPoint,branchPoint); // minimum distance from the point to the branch

    for (int currentPointIndex = 1; currentPointIndex < numberOfPoints; currentPointIndex++)
    {
      inputROIPoints->GetPoint(points->GetId(currentPointIndex), branchPoint);

      double currentDistance = vtkMath::Distance2BetweenPoints(originalPoint,branchPoint);

      if (currentDistance < minimumDistance)
      {
        minimumDistance = currentDistance;
      }
    }
    if (bestLineDistance == 0 || minimumDistance < bestLineDistance)
    {
      bestLineDistance = minimumDistance;
      closestLineIndex = DoLinesOverlaps[currentDoLinesOverlapIndex];
    }
  }

  return closestLineIndex;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::SealMesh(vtkPolyData* inputROIPoints, vtkCellArray* lines, vtkCellArray* polys)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  if (!lines)
  {
    vtkErrorMacro("lines: Invalid vtkCellArray!");
    return;
  }

  if (!polys)
  {
    vtkErrorMacro("points: Invalid vtkCellArray!");
    return;
  }

  int numberOfLines = lines->GetNumberOfCells();
  int numberOfPolygons = polys->GetNumberOfCells();

  std::vector< bool > polygonsToBelow(numberOfLines);
  std::vector< bool > polygonsToAbove(numberOfLines);

  // Loop through the lines.
  vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
  for (int currentLineIndex = 0; currentLineIndex < numberOfLines-1; currentLineIndex++)
  {
    currentLine->DeepCopy(inputROIPoints->GetCell(currentLineIndex));
    vtkSmartPointer<vtkIdList> currentLinePoints = currentLine->GetPointIds();
    double currentZ = currentLine->GetBounds()[4];

    // If polgons connect to the current contour from above and below,
    // it doesn't need to be sealed.
    if (polygonsToAbove[currentLineIndex] && polygonsToBelow[currentLineIndex])
    {
      break;
    }

    // Loop through the polygons
    vtkSmartPointer<vtkIdList> polygonIds = vtkSmartPointer<vtkIdList>::New();
    polys->SetTraversalLocation(0);
    for (int currentPolygonIndex = 0; currentPolygonIndex < numberOfPolygons; currentPolygonIndex++)
    {
      // Get the ID and z coordinates of the polygon corners.
      polys->GetNextCell(polygonIds);
      vtkIdType point1Id = polygonIds->GetId(0);
      vtkIdType point2Id = polygonIds->GetId(1);
      vtkIdType point3Id = polygonIds->GetId(2);

      double point1Z = inputROIPoints->GetPoint(point1Id)[2];
      double point2Z = inputROIPoints->GetPoint(point2Id)[2];
      double point3Z = inputROIPoints->GetPoint(point3Id)[2];
      double zNotOnLine = 0;

      if (point1Z == currentZ ||  point2Z == currentZ || point3Z == currentZ)
      {
        // Check to see if the corners of the polygon lie on the line.
        bool point1OnLine = this->IsPointOnLine(currentLinePoints, point1Id);
        bool point2OnLine = this->IsPointOnLine(currentLinePoints, point2Id);
        bool point3OnLine = this->IsPointOnLine(currentLinePoints, point3Id);

        if (point1OnLine || point2OnLine || point3OnLine)
        {
          if (!point1OnLine)
          {
            zNotOnLine = point1Z;
          }
          else if (!point2OnLine)
          {
            zNotOnLine = point2Z;
          }
          else if (!point3OnLine)
          {
            zNotOnLine = point3Z;
          }

          if (zNotOnLine > currentZ)
          {
            polygonsToAbove[currentLineIndex] = true;
          }
          else if (zNotOnLine < currentZ)
          {
            polygonsToBelow[currentLineIndex] = true;
          }
        }
      }
    }
  }

  for(int currentLineIndex = 0; currentLineIndex < numberOfLines; currentLineIndex++)
  {
    vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
    line->DeepCopy(inputROIPoints->GetCell(currentLineIndex));

    if (!polygonsToAbove[currentLineIndex] || !polygonsToBelow[currentLineIndex])
    {
      this->TriangulateLine(inputROIPoints, line, polys);
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::IsPointOnLine(vtkIdList* pointIds, vtkIdType originalPointId)
{
  if (!pointIds)
  {
    vtkErrorMacro("pointIds: Invalid vtkIdList!");
    return false;
  }

  int numberOfPoints = pointIds->GetNumberOfIds();
  for (int currentPointId = 0; currentPointId < numberOfPoints; currentPointId++)
  {
    if (pointIds->GetId(currentPointId) == originalPointId)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::TriangulateLine(vtkPolyData* inputROIPoints, vtkLine* line, vtkCellArray* polys)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  if (!line)
  {
   vtkErrorMacro("line: Invalid vtkLine!");
   return;
  }

  if (!polys)
  {
   vtkErrorMacro("polys: Invalid vtkCellArray!");
   return;
  }

  int numberOfPoints = line->GetNumberOfPoints();

  vtkSmartPointer<vtkPolyData> linePolyData = vtkSmartPointer<vtkPolyData>::New();
  linePolyData->SetPoints(line->GetPoints());

  vtkSmartPointer<vtkPolyData> boundary = vtkSmartPointer<vtkPolyData>::New();
  boundary->SetPoints(linePolyData->GetPoints());

  // Use vtkDelaunay2D to triangulate the line and produce new polygons
  vtkSmartPointer<vtkDelaunay2D> delaunay = vtkSmartPointer<vtkDelaunay2D>::New();

#if (VTK_MAJOR_VERSION <= 5)
    delaunay->SetInput(linePolyData);
    delaunay->SetSource(boundary);
#else
    delaunay->SetInputData(linePolyData);
    delaunay->SetSourceData(boundary);
#endif

  delaunay->Update();
  vtkSmartPointer<vtkPolyData> output = delaunay->GetOutput();
  vtkSmartPointer<vtkCellArray> newPolygons = output->GetPolys();

  // Check each new polygon to see if it is inside the line.
  int numberOfPolygons = output->GetNumberOfPolys();
  vtkSmartPointer<vtkIdList> currentPolygonIds = vtkSmartPointer<vtkIdList>::New();

  for(int currentPolygon = 0; currentPolygon < numberOfPolygons; currentPolygon++)
  {
    newPolygons->GetNextCell(currentPolygonIds);

    // Get the center of the polygon
    float x = 0;
    float y = 0;
    for (int coordinate = 0; coordinate < 3; coordinate++)
    {
      double currentPoint[3] = {0,0,0};
      inputROIPoints->GetPoint(line->GetPointId(currentPolygonIds->GetId(coordinate)), currentPoint);
      x += currentPoint[0];
      y += currentPoint[1];
    }
    x /= 3.0;
    y /= 3.0;
    double centerPoint[2] = {x, y};

    // Check to see if the center of the polyon is inside the line.
    if (this->IsPointInsideLine(inputROIPoints, line, centerPoint))
    {
      polys->InsertNextCell(3);
      polys->InsertCellPoint(line->GetPointId(currentPolygonIds->GetId(0)));
      polys->InsertCellPoint(line->GetPointId(currentPolygonIds->GetId(1)));
      polys->InsertCellPoint(line->GetPointId(currentPolygonIds->GetId(2)));
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::IsPointInsideLine(vtkPolyData* inputROIPoints, vtkLine* line, double point[])
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return false;
  }

  if (!line)
  {
    vtkErrorMacro("line: Invalid vtkLine!");
    return false;
  }

  // Create a ray that starts outside the polygon and goes to the point being checked.
  double bounds[6];
  line->GetBounds(bounds);

  double ray[4] = {bounds[0]-10, bounds[2]-10, point[0], point[1]};

  // Check all of the edges to see if they intersect.
  int numberOfIntersections = 0;
  for(int currentPointId = 0; currentPointId < line->GetNumberOfPoints()-1; currentPointId++)
  {
    double edge[4];
    edge[0] = inputROIPoints->GetPoint(line->GetPointId(currentPointId))[0];
    edge[1] = inputROIPoints->GetPoint(line->GetPointId(currentPointId))[1];

    edge[2] = inputROIPoints->GetPoint(line->GetPointId(currentPointId+1))[0];
    edge[3] = inputROIPoints->GetPoint(line->GetPointId(currentPointId+1))[1];

    // Check to see if the point is on either end of the edge.
    if ((point[0] == edge[0] && point[1] == edge[1]) ||
        (point[0] == edge[2] && point[1] == edge[3]))
    {
      return true;
    }

    if (this->DoLineSegmentsIntersect(ray, edge))
    {
      numberOfIntersections++;
    }
  }

  // If the number of intersections is odd, the point is inside.
  return numberOfIntersections%2 == 1;

}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::DoLineSegmentsIntersect(double line1[], double line2[])
{
  // Create a bounding box for the segment intersection range.
  double xMin = std::max(std::min(line1[0], line1[2]),std::min(line2[0], line2[2]));
  double xMax = std::min(std::max(line1[0], line1[2]),std::max(line2[0], line2[2]));
  double yMin = std::max(std::min(line1[1], line1[3]),std::min(line2[1], line2[3]));
  double yMax = std::min(std::max(line1[1], line1[3]),std::max(line2[1], line2[3]));

  // If the bounding box for the two segments doesn't overlap, the lines cannot intersect.
  if (xMin > xMax || yMin > yMax)
  {
    return false;
  }

  // Calculate the change in X and Y for line1.
  double deltaX1 = line1[0] - line1[2];
  double deltaY1 = line1[1] - line1[3];

  // Calculate the change in X and Y for line2.
  double deltaX2 = line2[0] - line2[2];
  double deltaY2 = line2[1] - line2[3];

  // Intersection location
  double x = 0;
  double y = 0;

  // If there are parallel vertical lines
  if (deltaX1 == 0 && deltaX2 == 0)
  {
    return false;
  }
  // If line 1 is vertical
  else if (deltaX1 == 0)
  {
    x = line1[0];
    double slope2 = (deltaY2/deltaX2);
    double intercept2 = line2[1]-slope2*line2[0];
    y = slope2*x+intercept2;
  }
  // If line 2 is vertical
  else if (deltaX2 == 0)
  {
    x = line2[0];
    double slope1 = (deltaY1/deltaX1);
    double intercept1 = line1[1]-slope1*line1[0];
    y = slope1*x+intercept1;
  }
  else
  {
    double slope1 = (deltaY1/deltaX1);
    double intercept1 = line1[1]-slope1*line1[0];

    double slope2 = (deltaY2/deltaX2);
    double intercept2 = line2[1]-slope2*line2[0];

    // Calculate the x intersection
    x = (intercept2-intercept1)/(slope1-slope2);

    // Calculate the y intersection
    if (slope1 == 0)
    {
      y = intercept1;
    }
    else if (slope2 == 0)
    {
      y = intercept2;
    }
    else
    {
      y = slope1*x+intercept1;
    }
  }

  // Check if the point is on the segment.
  if (x >= xMin &&
      x <= xMax &&
      y >= yMin &&
      y <= yMax)
  {
    return true;
  }

  // Line segments do not intersect.
  return false;
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetNextLocation(int currentLocation, int numberOfPoints, bool loopClosed)
{
  if (currentLocation+1 == numberOfPoints) // If the current location is the last point.
  {
    if (loopClosed)
    {
      // Skip the repeated point.
      return 1;
    }
    return 0;
  }
  return currentLocation+1;
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetPreviousLocation(int currentLocation, int numberOfPoints, bool loopClosed)
{
  if (currentLocation-1 == -1) // If the current location is the last point.
  {
    if (loopClosed)
    {
      // Skip the repeated point.
      return numberOfPoints-2;
    }
    return numberOfPoints-1;
  }
  return currentLocation-1;
}
