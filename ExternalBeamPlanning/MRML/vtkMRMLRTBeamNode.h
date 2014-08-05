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

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLScene.h>

// SlicerRT includes
//#include "vtkRTBeamData.h"
#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLDisplayableNode;
class vtkMRMLRTPlanNode;
class vtkMRMLModelNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLContourNode;
class vtkMRMLDoubleArrayNode;
class vtkRTBeamData;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkMRMLRTBeamNode : public vtkMRMLDisplayableNode
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

public:
  static vtkMRMLRTBeamNode *New();
  vtkTypeMacro(vtkMRMLRTBeamNode,vtkMRMLDisplayableNode);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(NominalEnergy, double);
  vtkSetMacro(NominalEnergy, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(NominalmA, double);
  vtkSetMacro(NominalmA, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(BeamOnTime, double);
  vtkSetMacro(BeamOnTime, double);

  vtkGetMacro(X1Jaw, double);
  vtkSetMacro(X1Jaw, double);
  vtkGetMacro(X2Jaw, double);
  vtkSetMacro(X2Jaw, double);
  vtkGetMacro(Y1Jaw, double);
  vtkSetMacro(Y1Jaw, double);
  vtkGetMacro(Y2Jaw, double);
  vtkSetMacro(Y2Jaw, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(GantryAngle, double);
  vtkSetMacro(GantryAngle, double);

  /// 
  vtkGetMacro(CollimatorAngle, double);
  vtkSetMacro(CollimatorAngle, double);

  ///
  vtkGetMacro(CouchAngle, double);
  vtkSetMacro(CouchAngle, double);

  ///
  vtkGetMacro(Smearing, double);
  vtkSetMacro(Smearing, double);
  
  ///
  vtkGetMacro(ProximalMargin, double);
  vtkSetMacro(ProximalMargin, double);
  
  ///
  vtkGetMacro(DistalMargin, double);
  vtkSetMacro(DistalMargin, double);
 
  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(SAD, double);
  vtkSetMacro(SAD, double);

  /// Get/Set beam weight
  vtkGetMacro(BeamWeight, double);
  vtkSetMacro(BeamWeight, double);

  /// Get/Set energy resolution
  vtkGetMacro(EnergyResolution, float);
  vtkSetMacro(EnergyResolution, float);

  ///
  vtkGetMacro(BeamFlavor, char);
  vtkSetMacro(BeamFlavor, char);

  /// Get/Set aperture offset
  vtkGetMacro(ApertureOffset, double);
  vtkSetMacro(ApertureOffset, double);

  /// Get/Set aperture offset
  vtkGetMacro(ApertureSpacingAtIso, double);
  vtkSetMacro(ApertureSpacingAtIso, double);

  /// Get/Set source size
  vtkGetMacro(SourceSize, double);
  vtkSetMacro(SourceSize, double);

  /// Get/Set beam data
  const vtkRTBeamData* GetBeamData() const { return BeamData; }
  vtkRTBeamData* GetBeamData() { return BeamData; }

  /// Return true if the beam name matches the argument
  bool BeamNameIs (const char *beamName);

  const double* GetIsocenterPosition ();
  double GetIsocenterPosition (int dim);
  void SetIsocenterPosition (const float* position);
  void SetIsocenterPosition (const double* position);

  const double* GetApertureSpacing ();
  double GetApertureSpacing (int dim);
  void SetApertureSpacing (const float* spacing);
  void SetApertureSpacing (const double* spacing);

  const double* GetApertureOrigin ();
  double GetApertureOrigin (int dim);
  void SetApertureOrigin (const float* origin);
  void SetApertureOrigin (const double* origin);

  const int* GetApertureDim ();
  int GetApertureDim (int dim);
  void SetApertureDim (const int* dim);

  void UpdateApertureParameters();

  /// Get beam model node ID
  vtkGetStringMacro(BeamModelNodeId);

  /// Get ribbon model node
  vtkMRMLModelNode* GetBeamModelNode();

  /// Set and observe ribbon model node
  void SetAndObserveBeamModelNodeId(const char *nodeID);

  ///
  vtkMRMLRTPlanNode* GetRTPlanNode();

  /// Get isocenter fiducial node
  vtkMRMLMarkupsFiducialNode* GetIsocenterFiducialNode();
  /// Set and observe isocenter fiducial node
  void SetAndObserveIsocenterFiducialNode(vtkMRMLMarkupsFiducialNode* node);

  /// Get proton target contour node
  vtkMRMLContourNode* GetTargetContourNode();
  /// Set and observe proton target contour node
  void SetAndObserveTargetContourNode(vtkMRMLContourNode* node);

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

protected:
  /// Set beam model node ID
  vtkSetReferenceStringMacro(BeamModelNodeId);

protected:
  vtkMRMLRTBeamNode();
  ~vtkMRMLRTBeamNode();
  vtkMRMLRTBeamNode(const vtkMRMLRTBeamNode&);
  void operator=(const vtkMRMLRTBeamNode&);

protected:
  /// Name of the structure that corresponds to this contour
//  char* BeamName;
//  int   BeamNumber;
//  char* BeamDescription;
  RTRadiationType RadiationType;

  RTBeamType  BeamType;
  RTCollimatorType CollimatorType;
  double NominalEnergy;
  double NominalmA;
  double BeamOnTime;

  double Isocenter[3];
  double dosePoint[3];

  double X1Jaw;
  double X2Jaw;
  double Y1Jaw;
  double Y2Jaw;

  double GantryAngle;
  double CollimatorAngle;
  double CouchAngle;

  double Smearing;
  double ProximalMargin;
  double DistalMargin;
  double SAD;
  double BeamWeight;

  float EnergyResolution;

  char BeamFlavor;

  double ApertureOffset;
  double ApertureSpacing[2];
  double ApertureSpacingAtIso;
  double ApertureOrigin[2];
  int ApertureDim[2];

  double SourceSize;

  /// Beam-specific data
  vtkRTBeamData* BeamData;

  /// Beam model representation
  vtkMRMLModelNode* BeamModelNode;
  
  /// Ribbon model representation model node ID
  char* BeamModelNodeId;
};

#endif // __vtkMRMLRTBeamNode_h
