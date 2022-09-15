/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MLC Position Logic includes
#include "vtkSlicerMLCPositionLogic.h"

// Slicer Models includes
#include <vtkSlicerModelsLogic.h>

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLStorageNode.h>
#include <vtkMRMLMarkupsClosedCurveNode.h> // MLC curve
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLTableNode.h>

#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelDisplayNode.h>

// Subject Hierarchy includes
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// SlicerRT MRML includes
#include <vtkMRMLRTPlanNode.h>
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLRTIonBeamNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSegmentation.h>
#include <vtkOrientedImageData.h>
#include <vtkImageData.h>
#include <vtkDoubleArray.h>
#include <vtkTable.h>
#include <vtkPlane.h>
#include <vtkCubeSource.h>

#include <vtkTransformPolyDataFilter.h>
#include <vtkAlgorithmOutput.h>

#include <vtkCellLocator.h>
#include <vtkModifiedBSPTree.h>
#include <vtkOBBTree.h>

#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkIdList.h>
#include <vtkPointsProjectedHull.h>

#include <vtkPolyData.h>
#include <vtkPolyLine.h>
#include <vtkPointData.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkDataArray.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkMath.h> // cross, dot vector operations

// SlicerRtCommon includes
#include <vtkSlicerRtCommon.h>
#include <vtkCollisionDetectionFilter.h>

// STD includes
#include <algorithm>

namespace
{

const char* MLCX_BOUNDARYANDPOSITION = "MLCX_BoundaryAndPosition";
const char* MLCY_BOUNDARYANDPOSITION = "MLCY_BoundaryAndPosition";

} // namespace

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMLCPositionLogic);

//----------------------------------------------------------------------------
vtkSlicerMLCPositionLogic::vtkSlicerMLCPositionLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerMLCPositionLogic::~vtkSlicerMLCPositionLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::PrintSelf( ostream& os, vtkIndent indent)
{
  os << indent << "vtkSlicerModuleLogic:     " << this->GetClassName() << "\n";
  this->Superclass::PrintSelf( os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  this->SetAndObserveMRMLSceneEventsInternal( newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::UpdateFromMRMLScene()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsCurveNode* vtkSlicerMLCPositionLogic::CalculatePositionConvexHullCurve( 
  vtkMRMLRTBeamNode* beamNode, vtkPolyData* targetPoly, bool parallelBeam)
{
  if (!beamNode)
  {
    vtkErrorMacro("CalculatePositionConvexHullCurve: Beam node is invalid");
    return nullptr;
  }

  vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
  vtkNew<vtkMatrix4x4> beamInverseMatrix;
  vtkNew<vtkTransform> beamInverseTransform;
  if (beamTransformNode)
  {
    beamTransformNode->GetMatrixTransformToWorld(beamInverseMatrix);
    beamInverseMatrix->Invert();
    beamInverseTransform->SetMatrix(beamInverseMatrix);
  }
  else
  {
    vtkErrorMacro("CalculatePositionConvexHullCurve: Beam transform node is invalid");
    return nullptr;
  }

  // Create markups node (subject hierarchy node is created automatically)
  vtkNew<vtkMRMLMarkupsClosedCurveNode> curveNode;
  curveNode->SetCurveTypeToLinear();
  curveNode->SetName("MultiLeafCollimatorMinimumConvexHullCurve");

  this->GetMRMLScene()->AddNode(curveNode);

  // transform target poly data into beam frame
  vtkNew<vtkTransformPolyDataFilter> beamInverseTransformFilter;
  beamInverseTransformFilter->SetTransform(beamInverseTransform);
  beamInverseTransformFilter->SetInputData(targetPoly);
  beamInverseTransformFilter->Update();

  vtkPolyData* targetPolyBeamFrame = beamInverseTransformFilter->GetOutput();

  // external points for MLC opening calculation, projected on the isocenter plane
  vtkNew<vtkPointsProjectedHull> points; 

  if (parallelBeam)
  {
    for( vtkIdType i = 0; i < targetPolyBeamFrame->GetNumberOfPoints(); i++)
    {
      double projectedPoint[4] = {}; // projected point in isocenter plane coordinates

      targetPolyBeamFrame->GetPoint( i, projectedPoint);

      // projection on XY plane of BEAM LIMITING DEVICE frame
      points->InsertPoint( i, projectedPoint[0], projectedPoint[1], 0.0);
    }
  }
  else
  {
    vtkNew<vtkPlane> projectionPlane;
    projectionPlane->SetOrigin( 0., 0., 0.);
    projectionPlane->SetNormal( 0., 0., 1.);
    for( vtkIdType i = 0; i < targetPolyBeamFrame->GetNumberOfPoints(); i++)
    {
      double beamFramePoint[4] = {}; // target region point in beam frame coordinates
      double t = 0.0001;
      double projectedPoint[3]; // target region point projected on plane

      targetPolyBeamFrame->GetPoint( i, beamFramePoint);
      
      double sourcePoint[3] = { 0., 0., -1. * beamNode->GetSAD() };
      double targetPoint[3] = { beamFramePoint[0], beamFramePoint[1], projectedPoint[2] };

      if (projectionPlane->IntersectWithLine( sourcePoint, targetPoint, t, projectedPoint))
      {
        // projection on XY plane of BEAM LIMITING DEVICE coordinate system
        points->InsertPoint( i, projectedPoint[0], projectedPoint[1], 0.0);
      }
    }
  }

  // get x and y coordinates of convex hull on the isocenter IEC BEAM LIMITING DEVICE coordinate system plane
  int zSize = points->GetSizeCCWHullZ(); // number of points
  if (zSize >= 3)
  {
    double* xyCoordinates = new double[zSize * 2]; // x and y coordinates buffer
    points->GetCCWHullZ( xyCoordinates, zSize); // get points of convex hull on the plane

    for( int i = 0; i < zSize; i++)
    {
      double xval = xyCoordinates[2 * i]; // x coordinate
      double yval = xyCoordinates[2 * i + 1]; // y coordinate

      vtkVector3d point( xval, yval, 0.0);
      curveNode->AddControlPoint(point); // add point to the closed curve
    }

    delete [] xyCoordinates;
    return curveNode.GetPointer();
  }
  else
  {
    this->GetMRMLScene()->RemoveNode(curveNode);
    curveNode->Delete();
    return nullptr;
  }
}

//---------------------------------------------------------------------------
vtkMRMLTableNode* vtkSlicerMLCPositionLogic::CreateMultiLeafCollimatorTableNodeBoundaryData(
  bool mlcType, unsigned int nofLeafPairs, double leafPairSize, double isocenterOffset)
{
  vtkNew<vtkMRMLTableNode> tableNode;
  this->GetMRMLScene()->AddNode(tableNode);
  const char* name = mlcType ? MLCX_BOUNDARYANDPOSITION : MLCY_BOUNDARYANDPOSITION;
  tableNode->SetName(name);

  std::vector<double> leafPairsBoundary(nofLeafPairs + 1);
  double middle = leafPairSize * nofLeafPairs / 2.;
  for( auto iter = leafPairsBoundary.begin(); iter != leafPairsBoundary.end(); ++iter)
  {
    size_t pos = iter - leafPairsBoundary.begin();
    *iter = -middle + isocenterOffset + pos * leafPairSize;
  }

  vtkTable* table = tableNode->GetTable();
  if (!table)
  {
    vtkErrorMacro("CreateMultiLeafCollimatorTableNodeBoundaryData: Unable to create vtkTable to fill MLC data");
    return nullptr;
  }

  // Column 0; Leaf pair boundary values
  vtkNew<vtkDoubleArray> boundaryArray;
  boundaryArray->SetName("Boundary");
  table->AddColumn(boundaryArray);

  // Column 1; Leaf positions on the side "1"
  vtkNew<vtkDoubleArray> pos1Array;
  pos1Array->SetName("1");
  table->AddColumn(pos1Array);

  // Column 2; Leaf positions on the side "2"
  vtkNew<vtkDoubleArray> pos2Array;
  pos2Array->SetName("2");
  table->AddColumn(pos2Array);

  table->SetNumberOfRows(leafPairsBoundary.size());
  for ( size_t row = 0; row < leafPairsBoundary.size(); ++row)
  {
    table->SetValue( row, 0, leafPairsBoundary[row]);
  }

  for ( unsigned int row = 0; row < nofLeafPairs; ++row)
  {
    table->SetValue( row, 1, -20.0); // default meaningful value for side "1"
    table->SetValue( row, 2, -20.0); // default meaningful value for side "2"
  }
  table->SetValue( nofLeafPairs, 1, 0.); // side "1" set last unused value to zero
  table->SetValue( nofLeafPairs, 2, 0.); // side "2" set last unused value to zero

  tableNode->SetUseColumnNameAsColumnHeader(true);
  tableNode->SetColumnDescription( "Boundary", "Leaf pair boundary");
  tableNode->SetColumnDescription( "1", "Leaf position on the side \"1\"");
  tableNode->SetColumnDescription( "2", "Leaf position on the side \"2\"");
  return tableNode;
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::CalculateLeavesProjection( vtkMRMLRTBeamNode* beamNode, vtkMRMLTableNode* mlcTableNode)
{
  if (!beamNode || !mlcTableNode)
  {
    return false;
  }

  vtkTable* mlcTable = mlcTableNode->GetTable();
  if (!mlcTable)
  {
    vtkErrorMacro("CalculateLeavesProjection: Unable to get vtkTable to recalculate MLC data");
    return false;
  }
  if (mlcTable->GetNumberOfColumns() != 3)
  {
    vtkErrorMacro("CalculateLeavesProjection: Wrong number of table columns");
    return false;
  }
  if (mlcTable->GetNumberOfRows() - 1 <= 0)
  {
    vtkErrorMacro("CalculateLeavesProjection: Wrong number of table rows");
    return false;
  }

  const char* mlcName = mlcTableNode->GetName();
  bool typeMLCY = !strncmp( "MLCY", mlcName, strlen("MLCY"));
  bool typeMLCX = !strncmp( "MLCX", mlcName, strlen("MLCX"));
  if (typeMLCY && !typeMLCX)
  {
    typeMLCX = false;
  }

  for ( int row = 0; row < mlcTable->GetNumberOfRows(); ++row)
  {
    double leafBoundary = mlcTable->GetValue( row, 0).ToDouble();

    vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(beamNode);
    if (beamNode && !ionBeamNode)
    {
      double coeff = beamNode->GetSAD() / beamNode->GetSourceToMultiLeafCollimatorDistance();
      leafBoundary *= coeff;
    }
    else if (ionBeamNode)
    {
      double isocenterToMLC = ionBeamNode->GetIsocenterToMultiLeafCollimatorDistance();
      double vsad = (typeMLCX) ? ionBeamNode->GetVSADx() : ionBeamNode->GetVSADy();
      double coeff = vsad / (vsad - isocenterToMLC);
      leafBoundary *= coeff;
    }
    mlcTable->SetValue( row, 0, leafBoundary);
  }
  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::UpdateMultiLeafCollimatorTableNodeBoundaryData( vtkMRMLTableNode* mlcTable, 
  bool mlcType, unsigned int nofLeafPairs, double leafPairSize, double isocenterOffset)
{
  if (!mlcTable)
  {
    return false;
  }
  if (mlcTable->GetNumberOfColumns() != 3)
  {
    vtkErrorMacro("UpdateMultiLeafCollimatorTableNodeBoundaryData: Wrong number of table columns");
    return false;
  }

  const char* name = mlcType ? MLCX_BOUNDARYANDPOSITION : MLCY_BOUNDARYANDPOSITION;
  mlcTable->SetName(name);

  // number of MLC boundary data = number of MLC leaf pairs + 1, see DICOM RT standard
  std::vector<double> leafPairsBoundary(nofLeafPairs + 1);
  double middle = leafPairSize * nofLeafPairs / 2.;
  for( auto iter = leafPairsBoundary.begin(); iter != leafPairsBoundary.end(); ++iter)
  {
    size_t pos = iter - leafPairsBoundary.begin();
    *iter = -middle + isocenterOffset + pos * leafPairSize;
  }

  vtkTable* table = mlcTable->GetTable();
  if (!table)
  {
    vtkErrorMacro("UpdateMultiLeafCollimatorTableNodeBoundaryData: Unable to get vtkTable to fill MLC data");
    return false;
  }

  // Column 0; Leaf pair boundary values
  // Column 1; Leaf positions on the side "1"
  // Column 2; Leaf positions on the side "2"
  vtkAbstractArray* boundaryArray = table->GetColumn(0);
  boundaryArray->SetName("Boundary");
  vtkAbstractArray* pos1Array = table->GetColumn(1);
  pos1Array->SetName("1");
  vtkAbstractArray* pos2Array = table->GetColumn(2);
  pos2Array->SetName("2");

  table->SetNumberOfRows(leafPairsBoundary.size()); // number of rows for MLC boundary data
  for ( size_t row = 0; row < leafPairsBoundary.size(); ++row)
  {
    table->SetValue( row, 0, leafPairsBoundary[row]);
  }
  for ( unsigned int row = 0; row < nofLeafPairs; ++row)
  {
    table->SetValue( row, 1, -20.0); // default meaningful value for side "1"
    table->SetValue( row, 2, -20.0); // default meaningful value for side "2"
  }
  table->SetValue( nofLeafPairs, 1, 0.); // side "1" set last unused value to zero
  table->SetValue( nofLeafPairs, 2, 0.); // side "2" set last unused value to zero

  mlcTable->SetUseColumnNameAsColumnHeader(true);
  mlcTable->SetColumnDescription( "Boundary", "Leaf pair boundary");
  mlcTable->SetColumnDescription( "1", "Leaf position on the side \"1\"");
  mlcTable->SetColumnDescription( "2", "Leaf position on the side \"2\"");
  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::CalculateMultiLeafCollimatorPosition( vtkMRMLTableNode* mlcTableNode, 
  vtkMRMLMarkupsCurveNode* curveNode)
{
  if (!mlcTableNode)
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPosition: invalid MLC table node");
    return false;
  }

  size_t nofLeafPairs = 0;
  if (mlcTableNode)
  {
    nofLeafPairs = mlcTableNode->GetNumberOfRows() - 1;
    if (nofLeafPairs <= 0)
    {
      nofLeafPairs = 0;
    }
  }

  double curveBounds[4] = {};
  if (!nofLeafPairs || !CalculateCurveBoundary( curveNode, curveBounds))
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPosition: Number of leaf pairs is zero or unable to calculate curve boundary");
    return false;
  }

  vtkTable* mlcTable = mlcTableNode->GetTable();

  int leafPairStart = -1, leafPairEnd = -1;
  FindLeafPairRangeIndexes( curveBounds, mlcTable, leafPairStart, leafPairEnd);
  if (leafPairStart == -1 || leafPairEnd == -1)
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPosition: Unable to find leaves range");
    return false;
  }

  if (leafPairStart > 0)
  {
    leafPairStart -= 1;
  }
  if (leafPairEnd < int(nofLeafPairs - 1))
  {
    leafPairEnd += 1;
  }

  for ( int leafPairIndex = leafPairStart; leafPairIndex <= leafPairEnd; ++leafPairIndex)
  {
    double side1 = 0.0, side2 = 0.0;
    size_t leafIndex = leafPairIndex;

    if (mlcTable && FindLeafPairPositions( curveNode, mlcTableNode, leafIndex, side1, side2))
    {
      // positions found
      mlcTable->SetValue( leafIndex, 1, side1);
      mlcTable->SetValue( leafIndex, 2, side2);
    }
  }

  return true;
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::CalculateCurveBoundary( vtkMRMLMarkupsCurveNode* curveNode, double* curveBound)
{
  if (!curveNode)
  {
    vtkErrorMacro("CalculateCurveBoundary: Invalid closed curve node");
    return false;
  }

  const auto controlPoints = curveNode->GetControlPoints();
  if (controlPoints->size() < 3)
  {
    vtkErrorMacro("CalculateCurveBoundary: Not enough points (less than 3) in closed curve node");
    return false;
  }

  double xmin = controlPoints->front()->Position[0];
  double xmax = controlPoints->front()->Position[0];
  double ymin = controlPoints->front()->Position[1];
  double ymax = controlPoints->front()->Position[1];
  for ( auto it = controlPoints->begin(); it != controlPoints->end(); ++it)
  {
    vtkMRMLMarkupsNode::ControlPoint* point = *it;
    xmin = std::min( xmin, point->Position[0]);
    xmax = std::max( xmax, point->Position[0]);
    ymin = std::min( ymin, point->Position[1]);
    ymax = std::max( ymax, point->Position[1]);
  }
  curveBound[0] = xmin;
  curveBound[1] = xmax;
  curveBound[2] = ymin;
  curveBound[3] = ymax;
  return true;
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::FindLeafPairRangeIndexes( double* curveBound, 
  vtkTable* mlcTable, int& leafIndexFirst, int& leafIndexLast)
{
  leafIndexFirst = -1;
  leafIndexLast = -1;
  int nofLeafPairs = mlcTable->GetNumberOfRows() - 1;

  for ( int leafPair = 0; leafPair < nofLeafPairs; ++leafPair)
  {
    double boundBegin = mlcTable->GetValue( leafPair, 0).ToDouble();
    double boundEnd = mlcTable->GetValue( leafPair + 1, 0).ToDouble();

    if (curveBound[0] >= boundBegin && curveBound[0] <= boundEnd)
    {
      leafIndexFirst = leafPair;
    }
    if (curveBound[1] >= boundBegin && curveBound[1] <= boundEnd)
    {
      leafIndexLast = leafPair;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::FindLeafPairRangeIndexes( 
  vtkMRMLRTBeamNode* beamNode, vtkMRMLTableNode* mlcTableNode, 
  int& leafIndexFirst, int& leafIndexLast)
{
  leafIndexFirst = -1;
  leafIndexLast = -1;

  if (!beamNode)
  {
    vtkErrorMacro("FindLeafPairRangeIndexes: invalid beam node");
    return;
  }

  if (!mlcTableNode)
  {
    vtkErrorMacro("FindLeafPairRangeIndexes: MLC table node is invalid");
    return;
  }

  const char* mlcName = mlcTableNode->GetName();
  bool typeMLCY = !strncmp( "MLCY", mlcName, strlen("MLCY"));
  bool typeMLCX = !strncmp( "MLCX", mlcName, strlen("MLCX"));
  if (typeMLCY && !typeMLCX)
  {
    typeMLCX = false;
  }

  vtkTable* mlcTable = mlcTableNode->GetTable();

  int nofLeafPairs = mlcTable->GetNumberOfRows() - 1;

  double jawBegin = beamNode->GetY1Jaw();
  double jawEnd = beamNode->GetY2Jaw();
  if (typeMLCX) // MLCX
  {
    jawBegin = beamNode->GetY1Jaw();
    jawEnd = beamNode->GetY2Jaw();
  }
  else // MLCY
  {
    jawBegin = beamNode->GetX1Jaw();
    jawEnd = beamNode->GetX2Jaw();
  }

  for ( int leafPair = 0; leafPair < nofLeafPairs; ++leafPair)
  {
    double boundBegin = mlcTable->GetValue( leafPair, 0).ToDouble();
    double boundEnd = mlcTable->GetValue( leafPair + 1, 0).ToDouble();

    if (jawBegin >= boundBegin && jawBegin <= boundEnd)
    {
      leafIndexFirst = leafPair;
    }
    if (jawEnd >= boundBegin && jawEnd <= boundEnd)
    {
      leafIndexLast = leafPair;
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::FindLeafPairPositions( 
  vtkMRMLMarkupsCurveNode* curveNode, vtkMRMLTableNode* mlcTableNode, 
  size_t leafPairIndex, 
  double& side1, double& side2, 
  int vtkNotUsed(strategy), 
  double maxPositionDistance, 
  double positionStep)
{
  if (!mlcTableNode)
  {
    vtkErrorMacro("FindLeafPairPositions: MLC table node is invalid");
    return false;
  }

  const char* mlcName = mlcTableNode->GetName();
  bool typeMLCY = !strncmp( "MLCY", mlcName, strlen("MLCY"));
  bool typeMLCX = !strncmp( "MLCX", mlcName, strlen("MLCX"));

  vtkTable* mlcTable = mlcTableNode->GetTable();

  double leafStart = mlcTable->GetValue( leafPairIndex, 0).ToDouble();
  double leafEnd = mlcTable->GetValue( leafPairIndex + 1, 0).ToDouble();

  vtkPolyData* curvePoly = curveNode->GetCurveWorld();
  if (!curvePoly)
  {
    vtkErrorMacro("FindLeafPairPositions: Curve polydata is invalid");
    return false;
  }

  // Build locator
  vtkNew<vtkCellLocator> cellLocator;
//  vtkNew<vtkModifiedBSPTree> cellLocator;
  cellLocator->SetDataSet(curvePoly);
  cellLocator->BuildLocator();

  // intersection with side1
  bool side1Flag = false;
  double pStart[3] = { };
  double pEnd[3] = { };

  if (typeMLCX)
  {
   pStart[1] = leafStart;
   pEnd[1] = leafEnd;
  }
  else if (typeMLCY)
  {
   pStart[0] = leafStart;
   pEnd[0] = leafEnd;
  }

  int subId = -1;
  double t = 0., xyz[3] = {}, pcoords[3] = {};
  for ( double c = -1. * maxPositionDistance; c <= maxPositionDistance; c += positionStep)
  {
    if (typeMLCX)
    {
      pStart[0] = c;
      pEnd[0] = c;
    }
    else if (typeMLCY)
    {
      pStart[1] = c;
      pEnd[1] = c;
    }

    if (cellLocator->IntersectWithLine( pStart, pEnd, 0.0001, t, xyz, pcoords, subId))
    {
      side1Flag = true;
      // xyz values
      if (typeMLCX)
      {
        side1 = xyz[0];
      }
      else if (typeMLCY)
      {
        side1 = xyz[1];
      }
      break;
    }
  }

  // intersection with side2
  bool side2Flag = false;
  for ( double c = maxPositionDistance; c >= -1. * maxPositionDistance; c -= positionStep)
  {
    if (typeMLCX)
    {
      pStart[0] = c;
      pEnd[0] = c;
    }
    else if (typeMLCY)
    {
      pStart[1] = c;
      pEnd[1] = c;
    }

    if (cellLocator->IntersectWithLine( pStart, pEnd, 0.0001, t, xyz, pcoords, subId))
    {
      side2Flag = true;
      // xyz values
      if (typeMLCX)
      {
        side2 = xyz[0];
      }
      else if (typeMLCY)
      {
        side2 = xyz[1];
      }
      break;
    }
  }
  return (side1Flag && side2Flag);
}

//---------------------------------------------------------------------------
double vtkSlicerMLCPositionLogic::CalculateMultiLeafCollimatorPositionArea( 
  vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPositionArea: invalid beam node");
    return -1.;
  }

  vtkMRMLTableNode* mlcTableNode = beamNode->GetMultiLeafCollimatorTableNode();

  int nofLeafPairs = 0;
  if (mlcTableNode)
  {
    nofLeafPairs = mlcTableNode->GetNumberOfRows() - 1;
  }

  double area = -1;
  // MLC position data
  if ((nofLeafPairs > 0) && (mlcTableNode->GetNumberOfColumns() == 3))
  {
    area = 0.;
    for ( size_t leafPair = 0; leafPair < size_t(nofLeafPairs); ++leafPair)
    {
      vtkTable* mlcTable = mlcTableNode->GetTable();

      double boundBegin = mlcTable->GetValue(leafPair, 0).ToDouble();
      double boundEnd = mlcTable->GetValue(leafPair + 1, 0).ToDouble();

      double side1 = mlcTable->GetValue( leafPair, 1).ToDouble();
      double side2 = mlcTable->GetValue( leafPair, 2).ToDouble();
//      area += (fabs(side1) + fabs(side2)) * fabs(boundEnd - boundBegin);
      area += fabs(side2 - side1) * fabs(boundEnd - boundBegin);
    }
  }
  return area;
}

//---------------------------------------------------------------------------
double vtkSlicerMLCPositionLogic::CalculateCurvePolygonArea(vtkMRMLMarkupsCurveNode* curveNode)
{
  if (!curveNode)
  {
    vtkErrorMacro("CalculateCurvePolygonArea: invalid curve node");
    return -1.;
  }

  const auto controlPoints = curveNode->GetControlPoints();

  // Setup points
  vtkNew<vtkPoints> points;

  for ( auto it = controlPoints->begin(); it != controlPoints->end(); ++it)
  {
    vtkMRMLMarkupsNode::ControlPoint* point = *it;
    double x = point->Position[0];
    double y = point->Position[1];
    points->InsertNextPoint( x, y, 0.0);
  }

  double normal[3] = { 1, 0, 0};
  return vtkPolygon::ComputeArea( points, controlPoints->size(), nullptr, normal);
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::SetParentForMultiLeafCollimatorTableNode(vtkMRMLRTBeamNode* beamNode)
{
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("SetParentForMultiLeafCollimatorTableNode: Subject hierarchy node is invalid");
    return;
  }
  if (!beamNode)
  {
    vtkErrorMacro("SetParentForMultiLeafCollimatorTableNode: Beam node is invalid");
    return;
  }
  vtkMRMLTableNode* mlcTableNode = beamNode->GetMultiLeafCollimatorTableNode();
  if (!mlcTableNode)
  {
    vtkErrorMacro("SetParentForMultiLeafCollimatorTableNode: Observed MLC data table node is invalid");
    return;
  }

  vtkIdType beamShId = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  vtkIdType mlcTableShId = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;

  // put observed mlc data under beam and ion beam node parent
  beamShId = shNode->GetItemByDataNode(beamNode);
  mlcTableShId = shNode->GetItemByDataNode(mlcTableNode);
  if (beamShId != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID && 
    mlcTableShId != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    shNode->SetItemParent( mlcTableShId, beamShId);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMLCPositionLogic::SetParentForMultiLeafCollimatorCurve( vtkMRMLRTBeamNode* beamNode, 
  vtkMRMLMarkupsCurveNode* curveNode)
{
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("SetParentForMultiLeafCollimatorTableNode: Subject hierarchy node is invalid");
    return;
  }
  if (!beamNode)
  {
    vtkErrorMacro("SetParentForMultiLeafCollimatorTableNode: Beam node is invalid");
    return;
  }
  if (!curveNode)
  {
    vtkErrorMacro("SetParentForMultiLeafCollimatorTableNode: Convex hull closed curve node is invalid");
    return;
  }

  vtkIdType beamShId = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  vtkIdType mlcCurveShId = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;

  // put observed mlc data under beam and ion beam node parent
  beamShId = shNode->GetItemByDataNode(beamNode);
  mlcCurveShId = shNode->GetItemByDataNode(curveNode);
  if (beamShId != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID && 
    mlcCurveShId != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    shNode->SetItemParent( mlcCurveShId, beamShId);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::CalculateMultiLeafCollimatorPosition( vtkMRMLRTBeamNode* beamNode, 
  vtkMRMLTableNode* mlcTableNode, vtkPolyData* targetPoly)
{
  if (!beamNode)
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPosition: invalid beam node");
    return false;
  }

  int nofLeafPairs = 0;
  if (mlcTableNode)
  {
    nofLeafPairs = mlcTableNode->GetNumberOfRows() - 1;
  }
  if (nofLeafPairs <= 0)
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPosition: invalid number of MLC leaf pairs, current value is "
      << nofLeafPairs << ", is must be more than zero!");
    return false;
  }

  vtkMRMLTransformNode* beamTransformNode = beamNode->GetParentTransformNode();
  vtkNew<vtkMatrix4x4> beamInverseMatrix;
  vtkNew<vtkTransform> beamInverseTransform;
  if (beamTransformNode)
  {
    beamTransformNode->GetMatrixTransformToWorld(beamInverseMatrix);
    beamInverseMatrix->Invert();
    beamInverseTransform->SetMatrix(beamInverseMatrix);
  }

  double isocenterToMLCDistance = beamNode->GetSAD() - beamNode->GetSourceToMultiLeafCollimatorDistance();

  // transform target poly data into beam frame
  vtkNew<vtkTransformPolyDataFilter> beamInverseTransformFilter;
  beamInverseTransformFilter->SetTransform(beamInverseTransform);
  beamInverseTransformFilter->SetInputData(targetPoly);
  beamInverseTransformFilter->Update();
  vtkPolyData* targetPolyData = vtkPolyData::SafeDownCast(beamInverseTransformFilter->GetOutput());
  if (!targetPolyData)
  {
    vtkErrorMacro("CalculateMultiLeafCollimatorPosition: Transformed target polydata is invalid");
    return false;
  }

  const char* mlcName = mlcTableNode->GetName();
  bool typeMLCX = !strncmp( "MLCX", mlcName, strlen("MLCX")); // MLCX by default
  bool typeMLCY = !strncmp( "MLCY", mlcName, strlen("MLCY"));
  // if MLCX then typeMLCX = true, if MLCY then typeMLCX = false
  if (typeMLCY && !typeMLCX)
  {
    typeMLCX = false;
  }

  // each leaf is represented as a perpendicular parallelepiped
  vtkNew<vtkCubeSource> leaf1; // leaf on side 1
  vtkNew<vtkCubeSource> leaf2; // leaf on side 2

  for ( vtkIdType leafPair = 0; leafPair < nofLeafPairs; leafPair++)
  { // leaf pair for cycle begins

    vtkTable* table = mlcTableNode->GetTable();
    double InitialPos1 = table->GetValue( leafPair, 1).ToDouble();
    double InitialPos2 = table->GetValue( leafPair, 2).ToDouble();

    if (vtkSlicerRtCommon::AreEqualWithTolerance( InitialPos1, InitialPos2))
    {
      continue;
    }

    double boundBegin = table->GetValue( leafPair, 0).ToDouble();
    double boundEnd = table->GetValue( leafPair + 1, 0).ToDouble();

    double mlcLeafLength = 200.; // meaningful leaf side of MLC leaf in mm

    // generate leaf on side "1" and "2" for MLCX or MLCY
    if (typeMLCX) // MLCX
    {
      leaf1->SetBounds(-1. * mlcLeafLength, 0., boundBegin, boundEnd, 
        -1. * isocenterToMLCDistance, isocenterToMLCDistance);
      leaf2->SetBounds( 0., mlcLeafLength, boundBegin, boundEnd, 
        -1. * isocenterToMLCDistance, isocenterToMLCDistance);
    }
    else // MLCY
    {
      leaf1->SetBounds( boundBegin, boundEnd, -1. * mlcLeafLength, 0.0, 
        -1. * isocenterToMLCDistance, isocenterToMLCDistance);
      leaf2->SetBounds( boundBegin, boundEnd, 0.0, mlcLeafLength, 
        -1. * isocenterToMLCDistance, isocenterToMLCDistance);
    }

    leaf1->Update();
    leaf2->Update();

    vtkPolyData* leaf2PolyData = vtkPolyData::SafeDownCast(leaf2->GetOutput(0));
    vtkPolyData* leaf1PolyData = vtkPolyData::SafeDownCast(leaf1->GetOutput(0));
    if (!leaf1PolyData || !leaf2PolyData)
    {
      vtkErrorMacro("CalculateMultiLeafCollimatorPosition: leaf on side 1 or 2 is invalid, leaf pair index is " << leafPair);
      return false;
    }

    double side1 = 0.0;
    // find a collition between moving leaf and stationaty target on side "1"
    if (FindLeafAndTargetCollision( beamNode, leaf1PolyData, targetPolyData,
      side1, InitialPos1 - 1., 1, typeMLCX))
    {
      table->SetValue( leafPair, 1, side1);
    }
    else
    {
      vtkWarningMacro("CalculateMultiLeafCollimatorPosition: Side 1 collision hasn't been found");
    }

    double side2 = 0.0;
    // find a collition between moving leaf and stationaty target on side "2"
    if (FindLeafAndTargetCollision( beamNode, leaf2PolyData, targetPolyData,
      side2, InitialPos2 + 1., 2, typeMLCX))
    {
      table->SetValue( leafPair, 2, side2);
    }
    else
    {
      vtkWarningMacro("CalculateMultiLeafCollimatorPosition: Side 2 collision hasn't been found");
    }
  } // leafPair for cycle ends
  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerMLCPositionLogic::FindLeafAndTargetCollision( vtkMRMLRTBeamNode* vtkNotUsed(beamNode), 
  vtkPolyData* leafPolyData, vtkPolyData* targetPolyData,
  double& sidePos, double initialPosition, int sideType, bool mlcType, 
  double maxPositionDistance, double positionStep)
{
  int contactMode = 1; // move leaf until first contact
  vtkNew<vtkMatrix4x4> targetMatrix; // fixed target matrix
  targetMatrix->Identity();
  vtkNew<vtkTransform> leafTransform; // moving leaf transform
  leafTransform->Identity();
  vtkNew<vtkCollisionDetectionFilter> collide;
  collide->SetInputData( 0, leafPolyData); // moving leaf polydata
  collide->SetTransform( 0, leafTransform);
  collide->SetInputData( 1, targetPolyData); // stationary target polydata
  collide->SetMatrix( 1, targetMatrix);
  collide->SetBoxTolerance(0.0);
  collide->SetCellTolerance(0.0);
  collide->SetNumberOfCellsPerNode(2);
  if (contactMode == 0)
  {
    collide->SetCollisionModeToAllContacts();
  }
  else if (contactMode == 1)
  {
    collide->SetCollisionModeToFirstContact();
  }
  else
  {
    collide->SetCollisionModeToHalfContacts();
  }
  collide->GenerateScalarsOn();

  // Move the leaf in IEC BEAM LIMITING DEVICE coordinate system plane to initial position
  if (sideType == 1) // leaf situated on side "1"
  {
    if (mlcType) // MLCX
    {
      leafTransform->Translate( initialPosition, 0.0, 0.0);
    }
    else // MLCY
    {
      leafTransform->Translate( 0., initialPosition, 0.0);
    }
  }
  else // leaf situated on side "2"
  {
    if (mlcType) // MLCX
    {
      leafTransform->Translate( initialPosition, 0.0, 0.0);
    }
    else // MLCY
    {
      leafTransform->Translate( 0., initialPosition, 0.0);
    }
  }

  // find a collition between leaf and target by moving leaf polydata with small steps
  int numSteps = fabs(initialPosition) + fabs(maxPositionDistance) / fabs(positionStep);
  bool res = false;
  double p[3] = {};
  for (int i = 0; i < numSteps; ++i)
  {
    if (sideType == 1) // leaf situated on side "1"
    {
      if (mlcType) // MLCX
      {
        leafTransform->Translate( positionStep, 0.0, 0.0);
      }
      else // MLCY
      {
        leafTransform->Translate( 0.0, positionStep, 0.0);
      }
    }
    else // leaf situated on side "2"
    {
      if (mlcType) // MLCX
      {
        leafTransform->Translate( -1. * positionStep, 0.0, 0.0);
      }
      else // MLCY
      {
        leafTransform->Translate( 0.0, -1. * positionStep, 0.0);
      }
    }
    
    leafTransform->Update();
    collide->Update();

    // Check if collision (first contact) is found, get a coordinate of the contact
    // and break the cycle
    // If there is no collision, then process to the next leaf step 
    if (collide->GetNumberOfContacts() > 0)
    {
      vtkPolyData* contacts = collide->GetContactsOutput();
      vtkIdType nofCells = contacts->GetNumberOfCells();

      for ( vtkIdType i = 0; i < nofCells; ++i)
      {
        vtkCell* cell = contacts->GetCell(i);
        if (cell)
        {
          vtkPoints* points = cell->GetPoints();
          if (points)
          {
            for ( vtkIdType j = 0; j < points->GetNumberOfPoints(); ++j)
            {
              points->GetPoint( j, p);
              if (mlcType) // MLCX
              {
                sidePos = p[0];
              }
              else // MLCY
              {
                sidePos = p[1];
              }
            }
          }
        }
      }
      res = true;
      break;
    }
  }

  return res;
}
