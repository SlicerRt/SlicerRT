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

#ifndef __vtkMRMLExternalBeamPlanningNode_h
#define __vtkMRMLExternalBeamPlanningNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

class vtkMRMLScalarVolumeNode;
class vtkMRMLRTPlanNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLContourNode;
class vtkMRMLDoubleArrayNode;

#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkMRMLExternalBeamPlanningNode : public vtkMRMLNode
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
  static vtkMRMLExternalBeamPlanningNode *New();
  vtkTypeMacro(vtkMRMLExternalBeamPlanningNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "ExternalBeamPlanning";};

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
  vtkGetMacro(NominalEnergy, double);
  vtkSetMacro(NominalEnergy, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(NominalmA, double);
  vtkSetMacro(NominalmA, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(BeamOnTime, double);
  vtkSetMacro(BeamOnTime, double);

  /// Get/Set Save labelmaps checkbox state
  vtkGetMacro(RxDose, double);
  vtkSetMacro(RxDose, double);

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

public:
  /// Get reference volume node
  vtkMRMLScalarVolumeNode* GetReferenceVolumeNode();
  /// Set and observe reference volume node
  void SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get RT plan node
  vtkMRMLRTPlanNode* GetRtPlanNode();
  /// Set and observe RT plan node
  void SetAndObserveRtPlanNode(vtkMRMLRTPlanNode* node);

  /// Get contour set node (can be contour node or contour set (subject hierarchy) node)
  vtkMRMLNode* GetPlanContourSetNode();
  /// Set and observe contour set node (can be contour node or contour set (subject hierarchy) node)
  void SetAndObservePlanContourSetNode(vtkMRMLNode* node);

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

protected:
  vtkMRMLExternalBeamPlanningNode();
  ~vtkMRMLExternalBeamPlanningNode();
  vtkMRMLExternalBeamPlanningNode(const vtkMRMLExternalBeamPlanningNode&);
  void operator=(const vtkMRMLExternalBeamPlanningNode&);

protected:
  /// Name of the structure that corresponds to this contour
  char* BeamName;
  int   BeamNumber;
  char* BeamDescription;
  RTRadiationType RadiationType;

  RTBeamType  BeamType;
  RTCollimatorType CollimatorType;
  double NominalEnergy;
  double NominalmA;
  double RxDose;
  double BeamOnTime;
  double X1Jaw;
  double X2Jaw;
  double Y1Jaw;
  double Y2Jaw;

  double Isocenter[3];
  double GantryAngle;
  double CollimatorAngle;
  double CouchAngle;

  double Smearing;
  double ProximalMargin;
  double DistalMargin;
  double SAD;
};

#endif
