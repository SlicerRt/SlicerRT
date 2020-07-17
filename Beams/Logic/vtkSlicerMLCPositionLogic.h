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

// .NAME vtkSlicerMLCPositionLogic - slicer logic class for MLC Position Calculation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMLCPositionLogic_h
#define __vtkSlicerMLCPositionLogic_h

#include "vtkSlicerBeamsModuleLogicExport.h"

// Slicer includes
#include "vtkMRMLAbstractLogic.h"

class vtkPolyData;
class vtkMRMLMarkupsCurveNode;
class vtkMRMLRTBeamNode;
class vtkMRMLTableNode;
class vtkTable;
class vtkAlgorithmOutput;

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerMLCPositionLogic : public vtkMRMLAbstractLogic
{
public:

  static vtkSlicerMLCPositionLogic *New();
  vtkTypeMacro( vtkSlicerMLCPositionLogic, vtkMRMLAbstractLogic);
  void PrintSelf( ostream& os, vtkIndent indent);

  /// Create table with Multi Leaf Collimator boundary data.
  /// Based on DICOMRT BeamLimitingDeviceEntry description of MLC.
  /// DICOM standard describes two kinds of multi-leaf collimators: 
  /// "MLCX" - leaves moves along X-axis, "MCLY" - leaves moves along Y-axis.
  /// X-axis and Y-axis are axises of IEC BEAM LIMITING DEVICE coordinate system.
  /// @param mlcType - type of Multi Leaf Collimator: true = MLCX, false = MLCY
  /// @param nofLeafPairs - number of leaf pairs in MLC
  /// @param leafPairSize - size of a leaf pair in mm
  /// @param isocenterOffset - offset of MLC boundary for isocenter point
  /// @return valid pointer if successfull, nullptr otherwise
  vtkMRMLTableNode* CreateMultiLeafCollimatorTableNodeBoundaryData( bool mlcType, unsigned int nofLeafPairs,
    double leafPairSize, double isocenterOffset = 0.0);

  /// Calculate Multi-leaf collimator leaves projection on 
  /// BEAM LIMITING DEVICE isocenter plain using geometry data
  /// @param beam - beam node
  /// @param mlcTable - MLC table of parallel beam
  /// @return true if mlcTable was recalculated and updated, false otherwise  
  bool CalculateLeavesProjection( vtkMRMLRTBeamNode* beam, vtkMRMLTableNode* mlcTable);

  /// Update table with Multi Leaf Collimator boundary data.
  /// @param mlcType - type of Multi Leaf Collimator: true = MLCX, false = MLCY
  /// @param nofLeafPairs - number of leaf pairs in MLC
  /// @param leafPairSize - size of a leaf pair in mm
  /// @param isocenterOffset - offset of MLC boundary for isocenter point
  /// @return true if update was successfull, false otherwise
  /// \warning: it will also change the table node's name
  bool UpdateMultiLeafCollimatorTableNodeBoundaryData( vtkMRMLTableNode* mlcTable, 
    bool mlcType, unsigned int nofLeafPairs, double leafPairSize, double isocenterOffset = 0.0);

  /// Calculate convex hull curve on isocenter plane for MLC position computation
  /// @param beamNode - beam node
  /// @param targetPoly - poly data of the target region
  /// @param parallelBeam - flag if beam is parallel
  /// @return valid pointer if successfull, nullptr otherwise
  vtkMRMLMarkupsCurveNode* CalculatePositionConvexHullCurve( vtkMRMLRTBeamNode* beamNode, 
    vtkPolyData* targetPoly, bool parallelBeam = true);

  /// Calculate MLC table position for convex hull curve (first pass).
  /// Both mlc table boundary data and convex hull curve are on
  /// IEC BEAM LIMITING DEVICE coordinate system plane.
  /// @param mlcTableNode - table node with MLC boundary data
  /// @param curveNode - closed curve node data
  /// @return true if position calculation is successfull, false otherwise
  bool CalculateMultiLeafCollimatorPosition( vtkMRMLTableNode* mlcTableNode, 
    vtkMRMLMarkupsCurveNode* curveNode);

  /// Calculate MLC table position using target polydata (second pass)
  /// @param beamNode - beam node
  /// @param mlcTableNode - table node with MLC boundary data and positions after first pass
  /// @param targetPoly - poly data of the target region
  /// @return true if position calculation is successfull, false otherwise
  bool CalculateMultiLeafCollimatorPosition( vtkMRMLRTBeamNode* beamNode, 
    vtkMRMLTableNode* mlcTableNode, vtkPolyData* targetPoly);

  /// Calculate MLC position opening area, for statistic purposes.
  /// @return positive area value is successfull, negative value otherwise 
  double CalculateMultiLeafCollimatorPositionArea(vtkMRMLRTBeamNode* beamNode);

  /// Calculate area of convex hull planar closed curve, for statistic purposes.
  /// @return positive area value is successfull, negative value otherwise 
  double CalculateCurvePolygonArea(vtkMRMLMarkupsCurveNode* curveNode);

  /// Set observed MLC table node as a child of the beam node
  void SetParentForMultiLeafCollimatorTableNode(vtkMRMLRTBeamNode* beamNode);
  /// Set convex hull closed curve node as a child of the beam node
  void SetParentForMultiLeafCollimatorCurve( vtkMRMLRTBeamNode* beamNode, 
    vtkMRMLMarkupsCurveNode* curveNode);

protected:
  vtkSlicerMLCPositionLogic();
  virtual ~vtkSlicerMLCPositionLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);

  /// Reimplemented to delete the storage/display nodes when a displayable
  /// node is being removed.
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

private:
  vtkSlicerMLCPositionLogic(const vtkSlicerMLCPositionLogic&); // Not implemented
  void operator=(const vtkSlicerMLCPositionLogic&); // Not implemented

  /// Calculate closed convex hull curve boundary
  /// @param curveBound (xmin, xmax, ymin, ymax)
  /// @return true if successfull, false otherwise
  bool CalculateCurveBoundary( vtkMRMLMarkupsCurveNode* node, double curveBound[4]);

  /// Find first and last MLC leaf pair index for position calculation
  /// @param curveBound (xmin, xmax, ymin, ymax)
  void FindLeafPairRangeIndexes( double curveBound[4], vtkTable* mlcTable, 
    int& leafPairIndexFirst, int& leafPairIndexLast);

  /// Find first and last leaf index for position calculation
  /// @param curveBound (xmin, xmax, ymin, ymax)
  void FindLeafPairRangeIndexes( vtkMRMLRTBeamNode* beamNode, vtkMRMLTableNode* mlcTableNode, 
    int& leafPairIndexFirst, int& leafPairIndexLast);

  /// Find leaf pair position using convex hull curve data (first pass, fast and coarse)
  /// @param convexHullCurveNode - convex hull curve
  /// @param mlcTableNode is used only to get leaf pair boundary data
  /// @param leafPairIndex - index of a leaf pair (starts from 0)
  /// @param side1 - leaf pair position on side 1
  /// @param side2 - leaf pair position on side 1
  /// @param strategy - position strategy (1 = out-of-field; 2 = in-field; 3 = cross-boundary; only out-of-field implemented)
  /// @param maxPositionDistance - maximum position of the leaf pair
  /// @param positionStep - position step of leaf pair
  /// @return true if successfull, false otherwise
  bool FindLeafPairPositions( vtkMRMLMarkupsCurveNode* convexHullCurveNode,
    vtkMRMLTableNode* mlcTableNode, size_t leafPairIndex, 
    double& side1, double& side2, int strategy = 1, 
    double maxPositionDistance = 100., double positionStep = 0.01);

  /// Find leaf pair position using collision filter between leaf 
  /// rectangle projection and target polydata (second pass, slow and more precise)
  /// @param beamNode - beam node with observed mlc table
  /// @param leafPoly - leaf polydata
  /// @param targetPoly - target polydata
  /// @param sidePos - final leaf position on current side 
  /// @param initialPosition - initial leaf position on current side 
  /// @param sideType - leaf side (1 = side 1; 2 = side 2)
  /// @param mlcType - type of MLC (true = MLCX, false = MLCY)
  /// @param maxPositionDistance - maximum position of the leaf pair
  /// @param positionStep - position step of leaf pair
  /// @return true if successfull, false otherwise
  bool FindLeafAndTargetCollision( vtkMRMLRTBeamNode* beamNode, 
    vtkPolyData* leafPoly, vtkPolyData* targetPoly, double& sidePos, 
    double initialPosition, int sideType = 1, bool mlcType = true, 
    double maxPositionDistance = 100., double positionStep = 0.01);
};

#endif
