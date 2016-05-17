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
    IsocenterModifiedEvent = 62200
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
  vtkMRMLRTPlanNode* GetPlanNode();

  /// Get isocenter fiducial node
  vtkMRMLMarkupsFiducialNode* GetIsocenterFiducialNode();
  /// Set and observe isocenter fiducial node
  void SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode* node);

  /// Get proton target segmentation node
  vtkMRMLSegmentationNode* GetTargetSegmentationNode();
  /// Set and observe proton target segmentation node
  void SetAndObserveTargetSegmentationNode(vtkMRMLSegmentationNode* node);

  /// Get target segment as a labelmap
  vtkSmartPointer<vtkOrientedImageData> GetTargetLabelmap();

  /// Get MLC position double array node
  vtkMRMLDoubleArrayNode* GetMLCPositionDoubleArrayNode();
  /// Set and observe MLC position double array node
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
  void SetIsocenterSpecification(vtkMRMLRTBeamNode::IsocenterSpecificationType);
  void SetIsocenterToTargetCenter();
  void GetIsocenterPosition(double*); //TODO: array
  void SetIsocenterPosition(double*);

  /// Get center of gravity of target segment, return true if successful
  /// or false if no target segment has been specified
  bool ComputeTargetVolumeCenter(double* center);

// Beam parameters
public:
  const double* GetReferenceDosePointPosition();
  double GetReferenceDosePointPosition(int dim);
  void SetReferenceDosePointPosition(const float* position);
  void SetReferenceDosePointPosition(const double* position);

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
  vtkSetMacro(X1Jaw, double);

  vtkGetMacro(X2Jaw, double);
  vtkGetConstMacro(X2Jaw, double);
  vtkSetMacro(X2Jaw, double);

  vtkGetMacro(Y1Jaw, double);
  vtkGetConstMacro(Y1Jaw, double);
  vtkSetMacro(Y1Jaw, double);

  vtkGetMacro(Y2Jaw, double);
  vtkGetConstMacro(Y2Jaw, double);
  vtkSetMacro(Y2Jaw, double);

  vtkGetMacro(GantryAngle, double);
  vtkGetConstMacro(GantryAngle, double);
  vtkSetMacro(GantryAngle, double);

  vtkGetMacro(CollimatorAngle, double);
  vtkGetConstMacro(CollimatorAngle, double);
  vtkSetMacro(CollimatorAngle, double);

  vtkGetMacro(CouchAngle, double);
  vtkGetConstMacro(CouchAngle, double);
  vtkSetMacro(CouchAngle, double);

  vtkGetMacro(Smearing, double);
  vtkSetMacro(Smearing, double);

  vtkGetMacro(SAD, double);
  vtkGetConstMacro(SAD, double);
  vtkSetMacro(SAD, double);

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

  RTBeamType  BeamType;
  RTRadiationType RadiationType;
  RTCollimatorType CollimatorType;

  double NominalEnergy;
  double NominalmA;
  double BeamOnTime;

  IsocenterSpecificationType IsocenterSpecification;
  double Isocenter[3];
  double ReferenceDosePoint[3];

  double X1Jaw;
  double X2Jaw;
  double Y1Jaw;
  double Y2Jaw;

  double GantryAngle;
  double CollimatorAngle;
  double CouchAngle;
  double Smearing;

  double SAD;
  double BeamWeight;
};

#endif // __vtkMRMLRTBeamNode_h
