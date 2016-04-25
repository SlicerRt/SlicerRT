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

#ifndef __vtkMRMLRTProtonBeamNode_h
#define __vtkMRMLRTProtonBeamNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLScene.h>

// SlicerRT includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

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
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkMRMLRTProtonBeamNode : public vtkMRMLRTBeamNode
{
public:
    enum RTProtonAlgorithm
      {
        RayTracer = 0,
        CGS = 1, // Cartesian Geometry dose Summation
        DGS = 2, // Divergent Geometry dose Summation
        HGS = 3  // Hong Geometry dose Summation
      };

  static vtkMRMLRTProtonBeamNode *New();
  vtkTypeMacro(vtkMRMLRTProtonBeamNode,vtkMRMLRTBeamNode);
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
  virtual const char* GetNodeTagName() {return "RTProtonBeam";};

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  void UpdateReferences();

public:
  
  ///  Get/Set beam line type
  vtkGetMacro(BeamLineType, bool);
  vtkSetMacro(BeamLineType, bool);

  ///  Get/Set manual energy limits
  vtkGetMacro(ManualEnergyLimits, bool);
  vtkSetMacro(ManualEnergyLimits, bool);

  ///  Get/Set energy minimum
  vtkGetMacro(MinimumEnergy, double);
  vtkSetMacro(MinimumEnergy, double);

  ///  Get/Set energy maximum 
  vtkGetMacro(MaximumEnergy, double);
  vtkSetMacro(MaximumEnergy, double);

  ///  Get/Set proximal margin
  vtkGetMacro(ProximalMargin, double);
  vtkSetMacro(ProximalMargin, double);
  
  ///  Get/Set distal margin
  vtkGetMacro(DistalMargin, double);
  vtkSetMacro(DistalMargin, double);
 
  /// Get/Set energy resolution
  vtkGetMacro(EnergyResolution, float);
  vtkSetMacro(EnergyResolution, float);

  /// Get/Set energy resolution
  vtkGetMacro(EnergySpread, float);
  vtkSetMacro(EnergySpread, float);

  /// Get/Set energy resolution
  vtkGetMacro(StepLength, float);
  vtkSetMacro(StepLength, float);

  /// Get/Set pencil beam resolution
  vtkGetMacro(PBResolution, double);
  vtkSetMacro(PBResolution, double);

  /// Get/Set aperture offset
  vtkGetMacro(ApertureOffset, double);
  vtkSetMacro(ApertureOffset, double);

  /// Get/Set source size
  vtkGetMacro(SourceSize, double);
  vtkSetMacro(SourceSize, double);

  /// Get/Set lateral spread homogeneous approximation
  vtkGetMacro(LateralSpreadHomoApprox, bool);
  vtkSetMacro(LateralSpreadHomoApprox, bool);

  /// Get/Set range compensator Monte Carlo based
  vtkGetMacro(RangeCompensatorHighland, bool);
  vtkSetMacro(RangeCompensatorHighland, bool);

  /// Get/Set have prescription
  vtkGetMacro(HavePrescription, bool);
  vtkSetMacro(HavePrescription, bool);

  /// Get/Set algorithm
  vtkGetMacro(Algorithm, RTProtonAlgorithm);
  vtkSetMacro(Algorithm, RTProtonAlgorithm);

  const double* GetApertureSpacing();
  double GetApertureSpacing(int dim);
  void SetApertureSpacing(const float* spacing);
  void SetApertureSpacing(const double* spacing);

  const double* GetApertureOrigin();
  double GetApertureOrigin(int dim);
  void SetApertureOrigin(const float* origin);
  void SetApertureOrigin(const double* origin);

  const int* GetApertureDim();
  int GetApertureDim(int dim);
  void SetApertureDim(const int* dim);

  void UpdateApertureParameters();

protected:
  vtkMRMLRTProtonBeamNode();
  ~vtkMRMLRTProtonBeamNode();
  vtkMRMLRTProtonBeamNode(const vtkMRMLRTProtonBeamNode&);
  void operator=(const vtkMRMLRTProtonBeamNode&);

protected:
  double ProximalMargin;
  double DistalMargin;

  bool BeamLineType; // true = active, false = passive

  bool ManualEnergyLimits;
  double MaximumEnergy;
  double MinimumEnergy;

  float EnergyResolution;
  float EnergySpread;
  float StepLength;

  RTProtonAlgorithm Algorithm;
  double PBResolution;

  double ApertureOffset;
  double ApertureSpacing[2];
  double ApertureOrigin[2];
  int ApertureDim[2];

  double SourceSize;

  bool LateralSpreadHomoApprox;
  bool RangeCompensatorHighland;

  bool HavePrescription;
};

#endif // __vtkMRMLRTProtonBeamNode_h
