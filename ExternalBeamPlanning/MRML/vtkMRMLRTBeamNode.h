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

#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLDisplayableNode;
class vtkMRMLRTPlanNode;
class vtkMRMLModelNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLContourNode;
class vtkMRMLDoubleArrayNode;

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
  /// Set/Get structure name
  vtkGetStringMacro(BeamName);
  vtkSetStringMacro(BeamName);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(BeamNumber, int);
  vtkSetMacro(BeamNumber, int);

  /// Set/Get structure name
  vtkGetStringMacro(BeamDescription);
  vtkSetStringMacro(BeamDescription);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(RxDose, double);
  vtkSetMacro(RxDose, double);

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
  vtkGetMacro(ProtonSmearing, double);
  vtkSetMacro(ProtonSmearing, double);
  
  ///
  vtkGetMacro(ProtonProximalMargin, double);
  vtkSetMacro(ProtonProximalMargin, double);
  
  ///
  vtkGetMacro(ProtonDistalMargin, double);
  vtkSetMacro(ProtonDistalMargin, double);
 
  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(ProtonSAD, double);
  vtkSetMacro(ProtonSAD, double);

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
  char* BeamName;
  int   BeamNumber;
  char* BeamDescription;
  RTRadiationType RadiationType;

  RTBeamType  BeamType;
  double RxDose;
  double Isocenter[3];
  double dosePoint[3];

  double NominalEnergy;
  double NominalmA;
  double BeamOnTime;
  double X1Jaw;
  double X2Jaw;
  double Y1Jaw;
  double Y2Jaw;
  RTCollimatorType CollimatorType;

  double GantryAngle;
  double CollimatorAngle;
  double CouchAngle;

  double ProtonSmearing;
  double ProtonProximalMargin;
  double ProtonDistalMargin;
  double ProtonSAD;

  /// Beam model representation
  vtkMRMLModelNode* BeamModelNode;
  
  /// Ribbon model representation model node ID
  char* BeamModelNodeId;
};

#endif // __vtkMRMLRTBeamNode_h
