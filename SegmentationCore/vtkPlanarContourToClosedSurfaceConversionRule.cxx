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

  vtkSmartPointer<vtkPoints> outputPoints = inputContoursCopy->GetPoints();
  vtkSmartPointer<vtkCellArray> outputLines = inputContoursCopy->GetLines();
  vtkSmartPointer<vtkCellArray> outputPolygons = vtkSmartPointer<vtkCellArray>::New(); // add triangles to this

  int numberOfLines = inputContoursCopy->GetNumberOfLines(); // total number of lines

  // remove keyholes from the lines
  this->FixKeyholes(inputContoursCopy, numberOfLines, 0.1, 2);

  numberOfLines = inputContoursCopy->GetNumberOfLines();

  // set all lines to be counter-clockwise
  this->SetLinesCounterClockwise(inputContoursCopy);
  
  std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators(numberOfLines);
  std::vector<vtkSmartPointer<vtkIdList> > linePointIdLists(numberOfLines);
  for(int lineIndex = 0; lineIndex < numberOfLines; ++lineIndex)
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
  for (int i=0; i<numberOfLines; ++i)
  {
    lineTriganulatedToAbove[i] = false;
    lineTriganulatedToBelow[i] = false;
  }

  // Get two consecutive planes.
  int firstLineOnPlane1Index = 0; // pointer to first line on plane 1.
  int numberOfLinesInPlane1 = this->GetNumberOfLinesOnPlane(inputContoursCopy, numberOfLines, 0);

  while (firstLineOnPlane1Index + numberOfLinesInPlane1 < numberOfLines)
  {
    int firstLineOnPlane2Index = firstLineOnPlane1Index + numberOfLinesInPlane1; // pointer to first line on plane 2
    int numberOfLinesInPlane2 = this->GetNumberOfLinesOnPlane(inputContoursCopy, numberOfLines, firstLineOnPlane2Index); // number of lines on plane 2

    // initialize overlaps lists. - list of list
    // Each internal list represents a line from the plane and will store the pointers to the overlap lines

    // overlaps for lines from plane 1
    std::vector< std::vector< int > > plane1Overlaps;
    for (int line1Index = 0; line1Index < numberOfLinesInPlane1; ++line1Index)
    {
      std::vector< int > temp;
      plane1Overlaps.push_back(temp);
    }

    // overlaps for lines from plane 2
    std::vector< std::vector< int > > plane2Overlaps;
    for (int line2Index = 0; line2Index < numberOfLinesInPlane2; ++line2Index)
    {
      std::vector< int > temp;
      plane2Overlaps.push_back(temp);
    }

    // Fill the overlaps lists.
    for (int line1Index = 0; line1Index < numberOfLinesInPlane1; ++line1Index)
    {
      vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
      line1->DeepCopy(inputContoursCopy->GetCell(firstLineOnPlane1Index+line1Index));

      for (int line2Index=0; line2Index< numberOfLinesInPlane2; ++line2Index)
      {
        vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
        line2->DeepCopy(inputContoursCopy->GetCell(firstLineOnPlane2Index+line2Index));

        if (this->DoLinesOverlap(line1, line2))
        {
          // line from plane 1 overlaps with line from plane 2
          plane1Overlaps[line1Index].push_back(firstLineOnPlane2Index+line2Index);
          plane2Overlaps[line2Index].push_back(firstLineOnPlane1Index+line1Index);
        }
      }
    }

    // Go over the planeOverlaps lists.
    for (int line1Index = firstLineOnPlane1Index; line1Index < firstLineOnPlane1Index+numberOfLinesInPlane1; ++line1Index)
    {
      vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
      line1->DeepCopy(inputContoursCopy->GetCell(line1Index));
      vtkSmartPointer<vtkIdList> pointsInLine1 = line1->GetPointIds();
      int numberOfPointsInLine1 = line1->GetNumberOfPoints();

      bool intersects = false;

      std::vector<vtkSmartPointer<vtkPointLocator> > overlap1PointLocators(plane1Overlaps[line1Index-firstLineOnPlane1Index].size());
      std::vector<vtkSmartPointer<vtkIdList> > overlap1PointIds(plane1Overlaps[line1Index-firstLineOnPlane1Index].size());

      for (int i=0; i<plane1Overlaps[line1Index-firstLineOnPlane1Index].size(); ++i)
      {
        int j = plane1Overlaps[line1Index-firstLineOnPlane1Index][i];
        overlap1PointLocators[i] = (pointLocators[j]);
        overlap1PointIds[i] = (linePointIdLists[j]);
      }

      for (int overlapIndex = 0; overlapIndex < plane1Overlaps[line1Index-firstLineOnPlane1Index].size(); ++overlapIndex) // lines on plane 2 that overlap with line i
      {
        int line2Index = plane1Overlaps[line1Index-firstLineOnPlane1Index][overlapIndex];

        vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
        line2->DeepCopy(inputContoursCopy->GetCell(line2Index));
        vtkSmartPointer<vtkIdList> pointsInLine2 = line2->GetPointIds();
        int numberOfPointsInLine2 = line2->GetNumberOfPoints();

        std::vector<vtkSmartPointer<vtkPointLocator> > overlap2PointLocators(plane2Overlaps[line2Index-firstLineOnPlane2Index].size());
        std::vector<vtkSmartPointer<vtkIdList> > overlap2PointIds(plane2Overlaps[line2Index-firstLineOnPlane2Index].size());

        for (int i=0; i<plane2Overlaps[line2Index-firstLineOnPlane2Index].size(); ++i)
        {
          int j = plane2Overlaps[line2Index-firstLineOnPlane2Index][i];
          overlap2PointLocators[i] = (pointLocators[j]);
          overlap2PointIds[i] = (linePointIdLists[j]);
        }

        // Get the portion of line 1 that is close to line 2,
        vtkSmartPointer<vtkLine> dividedLine1 = vtkSmartPointer<vtkLine>::New();
        this->Branch(inputContoursCopy, pointsInLine1, numberOfPointsInLine1, line2Index, plane1Overlaps[line1Index-firstLineOnPlane1Index], overlap1PointLocators, overlap1PointIds, dividedLine1);
        vtkSmartPointer<vtkIdList> dividedPointsInLine1 = dividedLine1->GetPointIds();
        int numberOfdividedPointsInLine1 = dividedLine1->GetNumberOfPoints();

        // Get the portion of line 2 that is close to line 1.
        vtkSmartPointer<vtkLine> dividedLine2 = vtkSmartPointer<vtkLine>::New();
        this->Branch(inputContoursCopy, pointsInLine2, numberOfPointsInLine2, line1Index, plane2Overlaps[line2Index-firstLineOnPlane2Index], overlap2PointLocators, overlap2PointIds, dividedLine2);
        vtkSmartPointer<vtkIdList> dividedPointsInLine2 = dividedLine2->GetPointIds();
        int numberOfdividedPointsInLine2 = dividedLine2->GetNumberOfPoints();

        if (numberOfdividedPointsInLine1 > 1 && numberOfdividedPointsInLine2 > 1)
        {
          lineTriganulatedToAbove[line1Index] = true;
          lineTriganulatedToBelow[line2Index] = true;
          this->TriangulateContours(inputContoursCopy,
            dividedPointsInLine1, numberOfdividedPointsInLine1,
            dividedPointsInLine2, numberOfdividedPointsInLine2,
            outputPolygons);
        }
        
      }
    }

    // Advance the points
    firstLineOnPlane1Index = firstLineOnPlane2Index;
    numberOfLinesInPlane1 = numberOfLinesInPlane2;
  }

  // Triangulate all contours which are exposed.
  this->SealMesh( inputContoursCopy, outputLines, outputPolygons, lineTriganulatedToAbove, lineTriganulatedToBelow);

  // Initialize the output data.
  closedSurfacePolyData->SetPoints(outputPoints);
  //closedSurfacePolyData->SetLines(outputLines); // Do not include lines in poly data for nicer visualization
  closedSurfacePolyData->SetPolys(outputPolygons);

  return true;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::TriangulateContours(vtkPolyData* inputROIPoints,
                                                  vtkIdList* pointsInLine1, int numberOfPointsInLine1,
                                                  vtkIdList* pointsInLine2, int numberOfPointsInLine2,
                                                  vtkCellArray* outputPolygons)
{

  if(! inputROIPoints)
  {
    vtkErrorMacro("TriangulateContours: Invalid vtkPolyData!");
    return;
  }

  if (!pointsInLine1)
  {
    vtkErrorMacro("TriangulateContours: Invalid vtkIdList!");
    return;
  }

  if (!pointsInLine2)
  {
   vtkErrorMacro("TriangulateContours: Invalid vtkIdList!");
  }

  // Pre-calculate and store the closest points.

  // Closest point from line 1 to line 2
  std::vector< int > closest1;
  for (int line1PointIndex = 0; line1PointIndex < numberOfPointsInLine1; ++line1PointIndex)
  {
    double line1Point[3] = {0,0,0};
    inputROIPoints->GetPoint(pointsInLine1->GetId(line1PointIndex), line1Point);

    closest1.push_back(this->GetClosestPoint(inputROIPoints, line1Point, pointsInLine2, numberOfPointsInLine2));
  }

  // closest from line 2 to line 1
  std::vector< int > closest2;
  for (int line2PointIndex = 0; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
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

  // Determine if the loops are closed.
  // A loop is closed if the first point is repeated as the last point.
  bool line1Closed = (pointsInLine1->GetId(0) == pointsInLine1->GetId(numberOfPointsInLine1-1));
  bool line2Closed = (pointsInLine2->GetId(0) == pointsInLine2->GetId(numberOfPointsInLine2-1));

  // Determine the ending points.
  int line1EndPoint = this->GetEndLoop(startLine1, numberOfPointsInLine1, line1Closed);
  int line2EndPoint = this->GetEndLoop(startLine2, numberOfPointsInLine2, line2Closed);

  // for backtracking
  int left = -1;
  int up = 1;

  // Initialize the Dynamic Programming table.
  // Rows represent line 1. Columns represent line 2.

  // Initialize the score table.
  std::vector< std::vector< double > > scoreTable( numberOfPointsInLine1, std::vector< double >( numberOfPointsInLine2 ) );
  double distanceBetweenPoints = vtkMath::Distance2BetweenPoints(firstPointLine1, firstPointLine2);
  scoreTable[0][0] = distanceBetweenPoints;

  std::vector< std::vector< int > > backtrackTable( numberOfPointsInLine1, std::vector< int >( numberOfPointsInLine2 ) );
  backtrackTable[0][0] = 0;

  // Initialize the first row in the table.
  int currentPointIndexLine2 = this->GetNextLocation(startLine2, numberOfPointsInLine2, line2Closed);
  for (int line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
  {
    double currentPointLine2[3] = {0,0,0}; // current point on line 2
    inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIndexLine2),currentPointLine2);

    // Use the distance between first point on line 1 and current point on line 2.
    double distance = vtkMath::Distance2BetweenPoints(firstPointLine1, currentPointLine2);

    scoreTable[0][line2PointIndex] = scoreTable[0][line2PointIndex-1]+distance;
    backtrackTable[0][line2PointIndex] = left;

    currentPointIndexLine2 = this->GetNextLocation(currentPointIndexLine2, numberOfPointsInLine2, line2Closed);

  }

  // Initialize the first column in the table.
  int currentPointIndexLine1 = this->GetNextLocation(startLine1, numberOfPointsInLine2, line1Closed);
  for( int line1PointIndex=1; line1PointIndex < numberOfPointsInLine1; ++line1PointIndex)
  {
    double currentPointLine1[3] = {0,0,0}; // current point on line 1
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIndexLine1), currentPointLine1);

    // Use the distance between first point on line 2 and current point on line 1.
    double distance = vtkMath::Distance2BetweenPoints(currentPointLine1, firstPointLine2);

    scoreTable[line1PointIndex][0] = scoreTable[line1PointIndex-1][0]+distance;
    backtrackTable[line1PointIndex][0] = up;
    for(int line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
    {
      scoreTable[line1PointIndex][line2PointIndex] = 0;
      backtrackTable[line1PointIndex][line2PointIndex] = up;
    }

    currentPointIndexLine1 = this->GetNextLocation(currentPointIndexLine1, numberOfPointsInLine1, line1Closed);
  }

  // Fill the rest of the table.
  int previousLine1 = startLine1;
  int previousLine2 = startLine2;

  currentPointIndexLine1 = this->GetNextLocation(startLine1, numberOfPointsInLine1, line1Closed);
  currentPointIndexLine2 = this->GetNextLocation(startLine2, numberOfPointsInLine2, line2Closed);

  int line1PointIndex=1;
  int line2PointIndex=1;
  for (line1PointIndex = 1; line1PointIndex < numberOfPointsInLine1; ++line1PointIndex)
  {
    double pointOnLine1[3] = {0,0,0};
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIndexLine1), pointOnLine1);

    for (line2PointIndex = 1; line2PointIndex < numberOfPointsInLine2; ++line2PointIndex)
    {
      double pointOnLine2[3] = {0,0,0};
      inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIndexLine2), pointOnLine2);

      double distance = vtkMath::Distance2BetweenPoints(pointOnLine1, pointOnLine2);

      // Use the pre-calcualted closest point.
      if (currentPointIndexLine1 == closest2[previousLine2])
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex][line2PointIndex-1]+distance;
        backtrackTable[line1PointIndex][line2PointIndex] = left;

      }
      else if (currentPointIndexLine2 == closest1[previousLine1])
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex-1][line2PointIndex]+distance;
        backtrackTable[line1PointIndex][line2PointIndex] = up;

      }
      else if (scoreTable[line1PointIndex][line2PointIndex-1] <= scoreTable[line1PointIndex-1][line2PointIndex])
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex][line2PointIndex-1]+distance;
        backtrackTable[line1PointIndex][line2PointIndex] = left;

      }
      else
      {
        scoreTable[line1PointIndex][line2PointIndex] = scoreTable[line1PointIndex-1][line2PointIndex]+distance;
        backtrackTable[line1PointIndex][line2PointIndex] = up;

      }

      // Advance the pointers
      previousLine2 = currentPointIndexLine2;
      currentPointIndexLine2 = this->GetNextLocation(currentPointIndexLine2, numberOfPointsInLine2, line2Closed);
    }
    previousLine1 = currentPointIndexLine1;
    currentPointIndexLine1 = this->GetNextLocation(currentPointIndexLine1, numberOfPointsInLine1, line1Closed);
  }

  // Backtrack.
  currentPointIndexLine1 = line1EndPoint;
  currentPointIndexLine2 = line2EndPoint;
  --line1PointIndex;
  --line2PointIndex;
  while (line1PointIndex > 0  || line2PointIndex > 0)
  {
    double line1Point[3] = {0,0,0}; // current point on line 1
    inputROIPoints->GetPoint(pointsInLine1->GetId(currentPointIndexLine1), line1Point);

    double line2Point[3] = {0,0,0}; // current point on line 2
    inputROIPoints->GetPoint(pointsInLine2->GetId(currentPointIndexLine2), line2Point);

    double distanceBetweenPoints = vtkMath::Distance2BetweenPoints(line1Point, line2Point);

    if (backtrackTable[line1PointIndex][line2PointIndex] == left)
    {
      int previousPointIndexLine2 = this->GetPreviousLocation(currentPointIndexLine2, numberOfPointsInLine2, line2Closed);

      int currentTriangle[3] = {0,0,0};
      currentTriangle[0] = pointsInLine1->GetId(currentPointIndexLine1);
      currentTriangle[1] = pointsInLine2->GetId(currentPointIndexLine2);
      currentTriangle[2] = pointsInLine2->GetId(previousPointIndexLine2);

      outputPolygons->InsertNextCell(3);
      outputPolygons->InsertCellPoint(currentTriangle[0]);
      outputPolygons->InsertCellPoint(currentTriangle[1]);
      outputPolygons->InsertCellPoint(currentTriangle[2]);

      line2PointIndex -= 1;
      currentPointIndexLine2 = previousPointIndexLine2;
    }
    else // up
    {
      int previousPointIndexLine1 = this->GetPreviousLocation(currentPointIndexLine1, numberOfPointsInLine1, line1Closed);

      int currentTriangle[3] = {0,0,0};
      currentTriangle[0] = pointsInLine1->GetId(currentPointIndexLine1);
      currentTriangle[1] = pointsInLine2->GetId(currentPointIndexLine2);
      currentTriangle[2] = pointsInLine1->GetId(previousPointIndexLine1);

      outputPolygons->InsertNextCell(3);
      outputPolygons->InsertCellPoint(currentTriangle[0]);
      outputPolygons->InsertCellPoint(currentTriangle[1]);
      outputPolygons->InsertCellPoint(currentTriangle[2]);

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
int vtkPlanarContourToClosedSurfaceConversionRule::GetClosestPoint(vtkPolyData* inputROIPoints, double* originalPoint, vtkIdList* linePointIds, int numberOfPoints)
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
  
  double pointOnLine[3] = {0,0,0}; // point from the given line
  inputROIPoints->GetPoint(linePointIds->GetId(0), pointOnLine);

  double minimumDistance = vtkMath::Distance2BetweenPoints(originalPoint, pointOnLine); // minimum distance from the point to the line
  double closestPointIndex = 0;

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
void vtkPlanarContourToClosedSurfaceConversionRule::FixKeyholes(vtkPolyData* inputROIPoints, int numberOfLines, int epsilon, int minimumSeperation)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("inputROIPoints: Invalid vtkPolyData!");
    return;
  }

  vtkSmartPointer<vtkLine> originalLine;

  std::vector<vtkSmartPointer<vtkLine> > newLines;

  int pointsOfSeperation;

  for (int lineIndex = 0; lineIndex < numberOfLines; ++lineIndex)
  {
    originalLine = vtkSmartPointer<vtkLine>::New();
    originalLine->DeepCopy(inputROIPoints->GetCell(lineIndex));

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
    std::vector<int> flags(numberOfPointsInLine);

    // Initialize the list of flags to -1.
    for (int i=0; i<numberOfPointsInLine; ++i)
    {
      flags[i] = -1;
    }

    for (int point1Index = 0; point1Index < numberOfPointsInLine; ++point1Index)
    {
      double point1[3] = {0,0,0};
      originalLinePoints->GetPoint(point1Index, point1);

      vtkSmartPointer<vtkIdList> pointsWithinRadius = vtkSmartPointer<vtkIdList>::New();
      pointsWithinRadius->Initialize();
      pointLocator->FindPointsWithinRadius(epsilon, point1, pointsWithinRadius);

      for (int pointWithinRadiusIndex = 0; pointWithinRadiusIndex < pointsWithinRadius->GetNumberOfIds(); ++pointWithinRadiusIndex)
      {
        int point2Index = pointsWithinRadius->GetId(pointWithinRadiusIndex);

        // Make sure the points are not too close together on the line index-wise
        pointsOfSeperation = std::min(point2Index-point1Index, numberOfPointsInLine-1-point2Index+point1Index);
        if (pointsOfSeperation > minimumSeperation)
        {
          keyHoleExists = true;
          flags[point1Index] = point2Index;
          flags[point2Index] = point1Index;
        }

      }
    }

    if (!keyHoleExists)
    {
      newLines.push_back(originalLine);
    }
    else
    {

      int currentLayer = 0;
      bool pointInChannel = false;

      std::vector<vtkSmartPointer<vtkIdList> > rawLinePointIds;
      std::vector<vtkSmartPointer<vtkIdList> > finishedLinePointIds;

      // Loop through all of the points in the line
      for (int currentPointIndex = 0; currentPointIndex < numberOfPointsInLine; ++currentPointIndex)
      {
        // Add a new line if neccessary
        if (currentLayer == rawLinePointIds.size())
        {
          vtkSmartPointer<vtkLine> newLine = vtkSmartPointer<vtkLine>::New();
          newLine->GetPoints()->SetData(originalLinePoints->GetData());

          vtkSmartPointer<vtkIdList> newLineIds = newLine->GetPointIds();
          newLineIds->Initialize();

          newLines.push_back(newLine);
          rawLinePointIds.push_back(newLineIds);
        }

        // If the current point is not part of a keyhole, add it to the current line
        if (flags[currentPointIndex] == -1)
        {
          rawLinePointIds[currentLayer]->InsertNextId(originalLine->GetPointId(currentPointIndex));
          pointInChannel = false;
        }
        else
        {
          // If the current point is the start of a keyhole add the point to the line,
          // increment the layer, and start the channel.
          if (flags[currentPointIndex] > currentPointIndex && !pointInChannel)
          {
            rawLinePointIds[currentLayer]->InsertNextId(originalLine->GetPointId(currentPointIndex));
            ++currentLayer;
            pointInChannel = true;

          }
          // If the current point is the end of a volume in the keyhole, add the point
          // to the line, remove the current line from the working list, deincrement
          // the layer, add the current line to the finished lines and start the,
          // channel.
          else if (flags[currentPointIndex] < currentPointIndex && !pointInChannel)
          {
            rawLinePointIds[currentLayer]->InsertNextId(originalLine->GetPointId(currentPointIndex));
            finishedLinePointIds.push_back(rawLinePointIds[currentLayer]);
            rawLinePointIds.pop_back();
            --currentLayer;
            pointInChannel = true;
          }
        }
      }

      // Add the remaining line to the finished list.
      for (int currentLineIndex=0; currentLineIndex < rawLinePointIds.size(); ++currentLineIndex)
      {
        finishedLinePointIds.push_back(rawLinePointIds[currentLineIndex]);
      }

      // Seal the lines.
      for (int currentLineIndex = 0; currentLineIndex < finishedLinePointIds.size(); ++currentLineIndex)
      {
        if (finishedLinePointIds[currentLineIndex]->GetId(0) != finishedLinePointIds[currentLineIndex]->GetId(finishedLinePointIds[currentLineIndex]->GetNumberOfIds()-1))
        {
          finishedLinePointIds[currentLineIndex]->InsertNextId(finishedLinePointIds[currentLineIndex]->GetId(0));
        }

      }
    }
  }

  vtkSmartPointer<vtkCellArray> outputLines = vtkSmartPointer<vtkCellArray>::New();
  outputLines->Initialize();
  inputROIPoints->DeleteCells();
  for (int currentLineIndex = 0; currentLineIndex < newLines.size(); ++currentLineIndex)
  {
    outputLines->InsertNextCell(newLines[currentLineIndex]);
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

  for(int lineIndex=0 ; lineIndex < numberOfLines; ++lineIndex)
  {
    vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
    currentLine->DeepCopy(inputROIPoints->GetCell(lineIndex));

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
  for (int currentLineIndex = 0; currentLineIndex < newLines.size(); ++currentLineIndex)
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

  for (int pointIndex=0; pointIndex < numberOfPoints-1; ++pointIndex)
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
    vtkErrorMacro("GetNumberOfLinesOnPlane: Invalid vtkPolyData!");
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

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::Branch(vtkPolyData* inputROIPoints, vtkIdList* points, int numberOfPoints, int currentLineIndex, std::vector< int > overlappingLines, std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators, std::vector<vtkSmartPointer<vtkIdList> > lineIdLists, vtkLine* outputLine)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("Branch: Invalid vtkPolyData!");
    return;
  }

  if (!points)
  {
    vtkErrorMacro("Branch: Invalid vtkIdList!");
    return;
  }
  
  vtkSmartPointer<vtkIdList> outputLinePointIds = outputLine->GetPointIds();
  outputLinePointIds->Initialize();

  if (overlappingLines.size() == 1)
  {
    outputLinePointIds->DeepCopy(points);
    return;
  }

  // Discard some points on the trunk so that the branch connects to only a part of the trunk.
  bool prev = false;

  for (int currentPointIndex = 0; currentPointIndex < numberOfPoints; ++currentPointIndex)
  {
    double currentPoint[3] = {0,0,0};
    inputROIPoints->GetPoint(points->GetId(currentPointIndex), currentPoint);

    // See if the point's closest branch is the input branch.
    if (this->GetClosestBranch(inputROIPoints, currentPoint, overlappingLines, pointLocators, lineIdLists) == currentLineIndex)
    {
      outputLinePointIds->InsertNextId(points->GetId(currentPointIndex));
      prev = true;
    }
    else
    {
      if (prev)
      {
        // Add one extra point to close up the surface.
        outputLinePointIds->InsertNextId(points->GetId(currentPointIndex));
      }
      prev = false;
    }
  }
  int dividedNumberOfPoints = outputLine->GetNumberOfPoints();
  if (dividedNumberOfPoints > 1)
  {
    // Determine if the trunk was originally a closed contour.
    bool closed = (points->GetId(0) == points->GetId(numberOfPoints-1));
    if (closed && (outputLinePointIds->GetId(0) != outputLinePointIds->GetId(dividedNumberOfPoints-1)))
    {
      // Make the new one a closed contour as well.
      outputLinePointIds->InsertNextId(outputLinePointIds->GetId(0));
    }
  }
}

//----------------------------------------------------------------------------
int vtkPlanarContourToClosedSurfaceConversionRule::GetClosestBranch(vtkPolyData* inputROIPoints, double* originalPoint, std::vector< int > overlappingLines,  std::vector<vtkSmartPointer<vtkPointLocator> > pointLocators, std::vector<vtkSmartPointer<vtkIdList> > lineIdLists)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("GetClosestBranch: Invalid vtkPolyData!");
  }

  // No need to check if there is only one overlapping line.
  if (overlappingLines.size() == 1)
  {
    return overlappingLines[0];
  }

  double minimumDistance2 = VTK_DOUBLE_MAX;
  int closestLineIndex = overlappingLines[0];

  for (int currentOverlapIndex = 0; currentOverlapIndex < overlappingLines.size(); ++currentOverlapIndex)
  {
    
    int closestPointId = pointLocators[currentOverlapIndex]->FindClosestPoint(originalPoint);

    double currentPoint[3] = {0,0,0};
    inputROIPoints->GetPoint(lineIdLists[currentOverlapIndex]->GetId(closestPointId), currentPoint);

    double currentLineDistance2 = vtkMath::Distance2BetweenPoints(currentPoint, originalPoint);

    if (currentLineDistance2 < minimumDistance2)
    {
      minimumDistance2 = currentLineDistance2;
      closestLineIndex = overlappingLines[currentOverlapIndex];

    }

  }

  return closestLineIndex;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::SealMesh(vtkPolyData* inputROIPoints, vtkCellArray* inputLines, vtkCellArray* outputPolygons, std::vector< bool > lineTriganulatedToAbove, std::vector< bool > lineTriganulatedToBelow)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("SealMesh: Invalid vtkPolyData!");
    return;
  }

  if (!inputLines)
  {
    vtkErrorMacro("SealMesh: Invalid vtkCellArray!");
    return;
  }

  if (!outputPolygons)
  {
    vtkErrorMacro("SealMesh: Invalid vtkCellArray!");
    return;
  }

  int numberOfLines = inputLines->GetNumberOfCells();

  double lineSpacing = this->GetSpacingBetweenLines(inputROIPoints);

  for(int currentLineIndex = 0; currentLineIndex < numberOfLines; ++currentLineIndex)
  {
    vtkSmartPointer<vtkLine> currentLine = vtkSmartPointer<vtkLine>::New();
    currentLine->DeepCopy(inputROIPoints->GetCell(currentLineIndex));

    if (!lineTriganulatedToAbove[currentLineIndex])
    {
      vtkSmartPointer<vtkLine> externalLine = vtkSmartPointer<vtkLine>::New();
      vtkSmartPointer<vtkIdList> externalIds = vtkSmartPointer<vtkIdList>::New();
      externalIds->Initialize();

      this->CreateExternalLine(inputROIPoints, currentLine, externalLine, externalIds, lineSpacing);
      this->TriangulateLine(inputROIPoints, externalLine, externalIds, outputPolygons);
      this->TriangulateContours(inputROIPoints, 
                                currentLine->GetPointIds(), currentLine->GetNumberOfPoints(),
                                externalIds, externalLine->GetNumberOfPoints(),
                                outputPolygons);
    }

    if (!lineTriganulatedToBelow[currentLineIndex])
    {
      vtkSmartPointer<vtkLine> externalLine = vtkSmartPointer<vtkLine>::New();
      vtkSmartPointer<vtkIdList> externalIds = vtkSmartPointer<vtkIdList>::New();
      externalIds->Initialize();

      this->CreateExternalLine(inputROIPoints, currentLine, externalLine, externalIds, -lineSpacing);
      this->TriangulateLine(inputROIPoints, externalLine, externalIds, outputPolygons);
      this->TriangulateContours(inputROIPoints, 
                                currentLine->GetPointIds(), currentLine->GetNumberOfPoints(),
                                externalIds, externalLine->GetNumberOfPoints(),
                                outputPolygons);
    }
  }
}

//----------------------------------------------------------------------------
double vtkPlanarContourToClosedSurfaceConversionRule::GetSpacingBetweenLines(vtkPolyData* inputROIPoints)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("GetSpacingBetweenLines: Invalid vtkPolyData!");
    return 0.0;
  }
  if (inputROIPoints->GetNumberOfCells() < 2)
  {
    vtkErrorMacro("GetSpacingBetweenLines: Input polydata has less than two cells! Unable to calculate spacing.");
    return 0.0;
  }

  vtkSmartPointer<vtkLine> line1 = vtkSmartPointer<vtkLine>::New();
  line1->DeepCopy(inputROIPoints->GetCell(0));
  double pointOnLine1[3] = {0,0,0};
  inputROIPoints->GetPoint(line1->GetPointId(0), pointOnLine1);
  
  vtkSmartPointer<vtkLine> line2 = vtkSmartPointer<vtkLine>::New();
  line2->DeepCopy(inputROIPoints->GetCell(1));
  double pointOnLine2[3] = {0,0,0};
  inputROIPoints->GetPoint(line2->GetPointId(0), pointOnLine2);

  return std::abs(pointOnLine1[2] - pointOnLine2[2]);
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::IsPointOnLine(vtkIdList* pointIds, vtkIdType originalPointId)
{
  if (!pointIds)
  {
    vtkErrorMacro("IsPointOnLine: Invalid vtkIdList!");
    return false;
  }

  int numberOfPoints = pointIds->GetNumberOfIds();
  for (int currentPointId = 0; currentPointId < numberOfPoints; ++currentPointId)
  {
    if (pointIds->GetId(currentPointId) == originalPointId)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::CreateExternalLine(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkLine* outputLine, vtkIdList* outputLinePointIds, double lineSpacing)
{
  
  if (!inputROIPoints)
  {
    vtkErrorMacro("CreateExternalLine: invalid vtkPolyData");
    return;
  }

  if (!inputLine)
  {
    vtkErrorMacro("CreateExternalLine: invalid vtkLine");
    return;
  }

  if (!outputLinePointIds)
  {
    vtkErrorMacro("CreateExternalLine: Invalid vtkIdList!");
  }

  vtkSmartPointer<vtkCellArray> lines = inputROIPoints->GetLines();
  lines->InsertNextCell(outputLine);
  
  int numberOfPoints = inputLine->GetNumberOfPoints();

  vtkSmartPointer<vtkPoints> inputPoints = inputROIPoints->GetPoints();
  
  vtkSmartPointer<vtkPoints> inputLinePoints = inputLine->GetPoints();
  vtkSmartPointer<vtkPoints> outputPoints = outputLine->GetPoints();
  outputPoints->Initialize();

  vtkSmartPointer<vtkIdList> outputPointIds = outputLine->GetPointIds();
  outputPointIds->Initialize();

  for (int currentLocation=0; currentLocation < numberOfPoints-1; ++currentLocation)
  {
    double currentPoint[3] = {0,0,0};
    inputLinePoints->GetPoint(currentLocation, currentPoint);

    double outputPoint[3] = {0,0,0};
    outputPoint[0] = currentPoint[0];
    outputPoint[1] = currentPoint[1];
    outputPoint[2] = currentPoint[2] + lineSpacing/2;

    outputPoints->InsertNextPoint(outputPoint);
    outputPointIds->InsertNextId(currentLocation);

    int inputPointIndex = inputPoints->InsertNextPoint(outputPoint);
    outputLinePointIds->InsertNextId(inputPointIndex);

  }
  outputPointIds->InsertNextId(outputPointIds->GetId(0));
  outputLinePointIds->InsertNextId(outputLinePointIds->GetId(0));

}

//----------------------------------------------------------------------------
void vtkPlanarContourToClosedSurfaceConversionRule::TriangulateLine(vtkPolyData* inputROIPoints, vtkLine* inputLine, vtkIdList* inputLinePointIds, vtkCellArray* outputPolys)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("TriangulateLine: Invalid vtkPolyData!");
    return;
  }

  if (!inputLine)
  {
   vtkErrorMacro("TriangulateLine: Invalid vtkLine!");
   return;
  }

  if (!outputPolys)
  {
   vtkErrorMacro("TriangulateLine: Invalid vtkCellArray!");
   return;
  }

  if (!inputLinePointIds)
  {
    vtkErrorMacro("TriangulateLine: Invalid vtkIdList!");
  }

  int numberOfPoints = inputLine->GetNumberOfPoints();
  vtkSmartPointer<vtkPolyData> linePolyData = vtkSmartPointer<vtkPolyData>::New(); 
  linePolyData->SetPoints(inputLine->GetPoints());

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

  for(int currentPolygon = 0; currentPolygon < numberOfPolygons; ++currentPolygon)
  {
    newPolygons->GetNextCell(currentPolygonIds);

    // Get the center of the polygon
    float x = 0;
    float y = 0;
    for (int coordinate = 0; coordinate < 3; ++coordinate)
    {
      double currentPoint[3] = {0,0,0};
      inputROIPoints->GetPoint(inputLinePointIds->GetId(currentPolygonIds->GetId(coordinate)), currentPoint);
      x += currentPoint[0];
      y += currentPoint[1];
    }
    x /= 3.0;
    y /= 3.0;
    double centerPoint[2] = {x, y};

    // Check to see if the center of the polyon is inside the line.
    if (this->IsPointInsideLine(inputROIPoints, inputLine, centerPoint))
    {
      outputPolys->InsertNextCell(3);
      outputPolys->InsertCellPoint(inputLinePointIds->GetId(currentPolygonIds->GetId(0)));
      outputPolys->InsertCellPoint(inputLinePointIds->GetId(currentPolygonIds->GetId(1)));
      outputPolys->InsertCellPoint(inputLinePointIds->GetId(currentPolygonIds->GetId(2)));
    }
  }
}

//----------------------------------------------------------------------------
bool vtkPlanarContourToClosedSurfaceConversionRule::IsPointInsideLine(vtkPolyData* inputROIPoints, vtkLine* inputLine, double* inputPoint)
{
  if (!inputROIPoints)
  {
    vtkErrorMacro("IsPointInsideLine: Invalid vtkPolyData!");
    return false;
  }

  if (!inputLine)
  {
    vtkErrorMacro("IsPointInsideLine: Invalid vtkLine!");
    return false;
  }

  // Create a ray that starts outside the polygon and goes to the point being checked.
  double bounds[6];
  inputLine->GetBounds(bounds);

  double ray[4] = {bounds[0]-10, bounds[2]-10, inputPoint[0], inputPoint[1]};

  // Check all of the edges to see if they intersect.
  int numberOfIntersections = 0;
  for(int currentPointId = 0; currentPointId < inputLine->GetNumberOfPoints()-1; ++currentPointId)
  {
    double edge[4];
    edge[0] = inputLine->GetPoints()->GetPoint(inputLine->GetPointId(currentPointId))[0];
    edge[1] = inputLine->GetPoints()->GetPoint(inputLine->GetPointId(currentPointId))[1];

    edge[2] = inputLine->GetPoints()->GetPoint(inputLine->GetPointId(currentPointId+1))[0];
    edge[3] = inputLine->GetPoints()->GetPoint(inputLine->GetPointId(currentPointId+1))[1];

    // Check to see if the point is on either end of the edge.
    if ((inputPoint[0] == edge[0] && inputPoint[1] == edge[1]) ||
        (inputPoint[0] == edge[2] && inputPoint[1] == edge[3]))
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
