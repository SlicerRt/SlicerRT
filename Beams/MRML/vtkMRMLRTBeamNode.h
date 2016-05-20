/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __vtkMRMLRTBeamNode_h
#define __vtkMRMLRTBeamNode_h

// SlicerRT includes
#include "vtkSlicerBeamsModuleMRMLExport.h"

// MRML includes
#include <vtkMRMLModelNode.h>

// SegmentationCore includes
#include "vtkOrientedImageData.h"

class vtkPolyData;
class vtkMRMLScene;
class vtkMRMLDoubleArrayNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLRTPlanNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLSegmentationNode;

/// GCS 2015-09-04.  Why don't VTK macros support const functions?
#define vtkGetConstMacro(name,type)             \
  virtual type Get##name () const {             \
    return this->name;                          \
  }

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTBeamNode : public vtkMRMLModelNode
{
public:
  enum RTBeamType
  {
    Static = 0,
    Dynamic = 1
  };
  enum RTRadiationType
  {
    Photon = 0,
    Electron = 1,
    Proton = 2
  };
  enum RTCollimatorType
  {
    SquareHalfMM = 0,
    SquareOneMM = 1,
    SquareTwoMM = 2
  };
  enum IsocenterSpecificationType
  {
    CenterOfTarget,
    ArbitraryPoint
  };

  static const char* NEW_BEAM_NODE_NAME_PREFIX;

  enum
  {
    /// Fired if isocenter position changes
    IsocenterModifiedEvent = 62200,
    BeamGeometryModified,
    BeamTransformModified
  };

public:
  static vtkMRMLRTBeamNode *New();
  vtkTypeMacro(vtkMRMLRTBeamNode,vtkMRMLModelNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Handles events registered in the observer manager
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() { return "RTBeam"; };

  /// Create and observe default display node
  virtual void CreateDefaultDisplayNodes();

  /// Create a default beam model
  void CreateDefaultBeamModel();

  /// Create beam model from beam parameters, supporting MLC leaves
  /// \return Poly data, null on fail
  void CreateBeamPolyData(vtkPolyData* beamModelPolyData);

public:
  /// Get parent plan node
  vtkMRMLRTPlanNode* GetParentPlanNode();

  /// Get isocenter fiducial node
  vtkMRMLMarkupsFiducialNode* GetIsocenterFiducialNode();
  /// Set and observe isocenter fiducial node
  void SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode* node);

  /// Get target segmentation node from parent plan node
  vtkMRMLSegmentationNode* GetTargetSegmentationNode();
  /// Get target segment as a labelmap
  vtkSmartPointer<vtkOrientedImageData> GetTargetLabelmap();

  /// Get MLC position double array node
  vtkMRMLDoubleArrayNode* GetMLCPositionDoubleArrayNode();
  /// Set and observe MLC position double array node.
  /// Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode* node);

  /// Get DRR volume node
  vtkMRMLScalarVolumeNode* GetDRRVolumeNode();
  /// Set and observe DRR volume node
  void SetAndObserveDRRVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get contour BEV node
  vtkMRMLScalarVolumeNode* GetContourBEVVolumeNode();
  /// Set and observe contour BEV node
  void SetAndObserveContourBEVVolumeNode(vtkMRMLScalarVolumeNode* node);

// Isocenter parameters
public:
  /// Set isocenter specification
  /// If it's CenterOfTarget, then \sa SetIsocenterToTargetCenter is called to change isocenter to center of target
  void SetIsocenterSpecification(vtkMRMLRTBeamNode::IsocenterSpecificationType isoSpec);
  /// Calculate center of current target and set isocenter to that point
  void SetIsocenterToTargetCenter();

  /// Get center of gravity of target segment, return true if successful
  /// or false if no target segment has been specified
  bool ComputeTargetVolumeCenter(double* center);

// Beam parameters
public:
  /// Get beam number
  vtkGetMacro(BeamNumber, int);
  /// Get beam number
  vtkGetConstMacro(BeamNumber, int);
  /// Set beam number
  vtkSetMacro(BeamNumber, int);

  /// Get beam description
  vtkGetStringMacro(BeamDescription);
  /// Set beam description
  vtkSetStringMacro(BeamDescription);

  /// Get target segment ID
  vtkGetStringMacro(TargetSegmentID);
  /// Set target segment ID
  vtkSetStringMacro(TargetSegmentID);

  /// Get isocenter position
  vtkGetVector3Macro(Isocenter, double);
  /// Set isocenter position
  vtkSetVector3Macro(Isocenter, double);

  /// Get radiation type
  vtkGetMacro(RadiationType, vtkMRMLRTBeamNode::RTRadiationType);
  /// Set radiation type
  vtkGetConstMacro(RadiationType, vtkMRMLRTBeamNode::RTRadiationType);
  /// Set radiation type
  vtkSetMacro(RadiationType, vtkMRMLRTBeamNode::RTRadiationType);

  /// Get beam type
  vtkGetMacro(BeamType, vtkMRMLRTBeamNode::RTBeamType);
  /// Get beam type
  vtkGetConstMacro(BeamType, vtkMRMLRTBeamNode::RTBeamType);
  /// Set beam type
  vtkSetMacro(BeamType, vtkMRMLRTBeamNode::RTBeamType);

  vtkGetMacro(IsocenterSpecification, vtkMRMLRTBeamNode::IsocenterSpecificationType);
  vtkGetConstMacro(IsocenterSpecification, vtkMRMLRTBeamNode::IsocenterSpecificationType);

  vtkGetConstMacro(NominalEnergy, double);
  vtkSetMacro(NominalEnergy, double);

  vtkGetMacro(NominalmA, double);
  vtkGetConstMacro(NominalmA, double);
  vtkSetMacro(NominalmA, double);

  vtkGetMacro(BeamOnTime, double);
  vtkGetConstMacro(BeamOnTime, double);
  vtkSetMacro(BeamOnTime, double);

  vtkGetMacro(X1Jaw, double);
  vtkGetConstMacro(X1Jaw, double);
  /// Set X1 jaw position. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetX1Jaw(double x1Jaw);

  vtkGetMacro(X2Jaw, double);
  vtkGetConstMacro(X2Jaw, double);
  /// Set X2 jaw position. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetX2Jaw(double x2Jaw);

  vtkGetMacro(Y1Jaw, double);
  vtkGetConstMacro(Y1Jaw, double);
  /// Set Y1 jaw position. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetY1Jaw(double y1Jaw);

  vtkGetMacro(Y2Jaw, double);
  vtkGetConstMacro(Y2Jaw, double);
  /// Set Y2 jaw position. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetY2Jaw(double y2Jaw);

  vtkGetMacro(SAD, double);
  vtkGetConstMacro(SAD, double);
  /// Set source-axis distance. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetSAD(double sad);

  vtkGetMacro(GantryAngle, double);
  vtkGetConstMacro(GantryAngle, double);
  /// Set gantry angle. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetGantryAngle(double angle);

  vtkGetMacro(CollimatorAngle, double);
  vtkGetConstMacro(CollimatorAngle, double);
  /// Set collimator angle. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetCollimatorAngle(double angle);

  vtkGetMacro(CouchAngle, double);
  vtkGetConstMacro(CouchAngle, double);
  /// Set couch angle. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetCouchAngle(double angle);

  vtkGetMacro(BeamWeight, double);
  vtkGetConstMacro(BeamWeight, double);
  vtkSetMacro(BeamWeight, double);

protected:
  vtkMRMLRTBeamNode();
  ~vtkMRMLRTBeamNode();
  vtkMRMLRTBeamNode(const vtkMRMLRTBeamNode&);
  void operator=(const vtkMRMLRTBeamNode&);

// Beam properties
protected:
  /// Beam number
  int   BeamNumber;
  /// Beam description
  char* BeamDescription;

  /// Target segment ID in target segmentation node
  char* TargetSegmentID;

  /// TODO:
  RTBeamType BeamType;
  /// TODO:
  RTRadiationType RadiationType;
  /// TODO:
  RTCollimatorType CollimatorType;

  /// TODO:
  double NominalEnergy;
  /// TODO:
  double NominalmA;
  /// TODO: Remove
  double BeamOnTime;

  /// Isocenter specification determining whether it can be an arbitrary point or
  /// always calculated to be at the center of the target structure
  IsocenterSpecificationType IsocenterSpecification;
  /// Isocenter position
  ///TODO: This is redundant. Would be much better not to store the markups reference but get it from the plan,
  ///      and store the name of the beam's isocenter fiducial within the plan POIs markups node.
  double Isocenter[3];

  /// X1 jaw position
  double X1Jaw;
  /// X2 jaw position
  double X2Jaw;
  /// Y1 jaw position
  double Y1Jaw;
  /// Y2 jaw position
  double Y2Jaw;
  /// Source-axis distance
  double SAD;

  /// Gantry angle
  double GantryAngle;
  /// Collimator angle
  double CollimatorAngle;
  /// Couch angle
  double CouchAngle;

  /// Beam weight, taken into account when accumulating per-beam doses
  double BeamWeight;
};

#endif // __vtkMRMLRTBeamNode_h
