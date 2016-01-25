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
#include <vtkMRML.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLScene.h>

// Slicer includes
#include "vtkOrientedImageData.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLDisplayableNode;
class vtkMRMLRTPlanNode;
class vtkMRMLModelNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLSegmentationNode;
class vtkMRMLDoubleArrayNode;

/// GCS 2015-09-04.  Why don't VTK macros support const functions?
#define vtkGetConstMacro(name,type)             \
  virtual type Get##name () const {             \
    return this->name;                          \
  }

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTBeamNode : public vtkMRMLDisplayableNode
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
  enum IsocenterSpecification
  {
    CenterOfTarget,
    ArbitraryPoint
  };

public:
  static vtkMRMLRTBeamNode *New();
  vtkTypeMacro(vtkMRMLRTBeamNode,vtkMRMLDisplayableNode);
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

  /// Updates the referenced nodes from the updated scene
  virtual void UpdateScene(vtkMRMLScene *scene);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "RTBeam";};

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  void UpdateReferences();

public:
  /// Set/Get structure name
  vtkGetStringMacro(BeamName);
  vtkSetStringMacro(BeamName);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(BeamNumber, int);
  vtkGetConstMacro(BeamNumber, int);
  vtkSetMacro(BeamNumber, int);

  /// Set/Get structure name
  vtkGetStringMacro(BeamDescription);
  vtkSetStringMacro(BeamDescription);

  /// Get/Set target segment ID
  vtkGetStringMacro(TargetSegmentID);
  vtkSetStringMacro(TargetSegmentID);

  /// Get/Set RadiationType
  vtkGetMacro(RadiationType, vtkMRMLRTBeamNode::RTRadiationType);
  vtkGetConstMacro(RadiationType, vtkMRMLRTBeamNode::RTRadiationType);
  vtkSetMacro(RadiationType, vtkMRMLRTBeamNode::RTRadiationType);

  /// Get/Set RadiationType
  vtkGetMacro(BeamType, vtkMRMLRTBeamNode::RTBeamType);
  vtkGetConstMacro(BeamType, vtkMRMLRTBeamNode::RTBeamType);
  vtkSetMacro(BeamType, vtkMRMLRTBeamNode::RTBeamType);

  /// Get/Set IsocenterSpec
  vtkGetMacro(IsocenterSpec, vtkMRMLRTBeamNode::IsocenterSpecification);
  vtkGetConstMacro(IsocenterSpec, vtkMRMLRTBeamNode::IsocenterSpecification);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(NominalEnergy, double);
  vtkGetConstMacro(NominalEnergy, double);
  vtkSetMacro(NominalEnergy, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(NominalmA, double);
  vtkGetConstMacro(NominalmA, double);
  vtkSetMacro(NominalmA, double);

  /// Get/Set Save labelmaps checkbox state
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

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(GantryAngle, double);
  vtkGetConstMacro(GantryAngle, double);
  vtkSetMacro(GantryAngle, double);

  ///
  vtkGetMacro(CollimatorAngle, double);
  vtkGetConstMacro(CollimatorAngle, double);
  vtkSetMacro(CollimatorAngle, double);

  ///
  vtkGetMacro(CouchAngle, double);
  vtkGetConstMacro(CouchAngle, double);
  vtkSetMacro(CouchAngle, double);

  /// Set/Get smearing
  vtkGetMacro(Smearing, double);
  vtkSetMacro(Smearing, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(SAD, double);
  vtkGetConstMacro(SAD, double);
  vtkSetMacro(SAD, double);

  /// Get/Set beam weight
  vtkGetMacro(BeamWeight, double);
  vtkGetConstMacro(BeamWeight, double);
  vtkSetMacro(BeamWeight, double);

  /// Return true if the beam name matches the argument
  bool BeamNameIs (const std::string& beamName);
  bool BeamNameIs (const char *beamName);

  void SetIsocenterSpec (vtkMRMLRTBeamNode::IsocenterSpecification);
  void SetIsocenterToTargetCenter ();
  void GetIsocenterPosition (double*);
  void SetIsocenterPosition (double*);

  const double* GetReferenceDosePointPosition ();
  double GetReferenceDosePointPosition (int dim);
  void SetReferenceDosePointPosition (const float* position);
  void SetReferenceDosePointPosition (const double* position);

  /// Get beam model node ID
  vtkGetStringMacro(BeamModelNodeId);

  /// Get beam model node
  vtkMRMLModelNode* GetBeamModelNode();

  /// Set and observe beam model node
  void SetAndObserveBeamModelNodeId(const char *nodeID);

  ///
  vtkMRMLRTPlanNode* GetRTPlanNode();

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

  /// Get center of gravity of target segment, return true if successful
  /// or false if no target segment has been specified
  bool ComputeTargetVolumeCenter (double* center);

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

  // Update the beam model for a new isocenter, gantry angle, etc.
  void UpdateBeamTransform();

protected:
  /// Copy isocenter coordinates into fiducial
  void CopyIsocenterCoordinatesToMarkups (double*);

  /// Copy isocenter coordinates from fiducial
  void CopyIsocenterCoordinatesFromMarkups (double*);

protected:
  /// Set beam model node ID
  vtkSetReferenceStringMacro(BeamModelNodeId);

protected:
  vtkMRMLRTBeamNode();
  ~vtkMRMLRTBeamNode();
  vtkMRMLRTBeamNode(const vtkMRMLRTBeamNode&);
  void operator=(const vtkMRMLRTBeamNode&);

protected:
  char* BeamName;
  int   BeamNumber;
  char* BeamDescription;

  /// Target segment ID in target segmentation node
  char* TargetSegmentID;

  // Beam properties
  RTBeamType  BeamType;
  RTRadiationType RadiationType;
  RTCollimatorType CollimatorType;

  double NominalEnergy;
  double NominalmA;
  double BeamOnTime;

  IsocenterSpecification IsocenterSpec;
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

  //TODO: Change these references to MRML references. 
  // No need to store neither the node pointer nor the ID.

  /// Beam model representation
  vtkMRMLModelNode* BeamModelNode;
  
  /// Beam model node ID
  char* BeamModelNodeId;
};

#endif // __vtkMRMLRTBeamNode_h
