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
#include "vtkSlicerBeamsModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLDisplayableNode;
class vtkMRMLRTPlanNode;
class vtkMRMLModelNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLSegmentationNode;
class vtkMRMLDoubleArrayNode;

//TODO: GCS 2015-09-04.  Why don't VTK macros support const functions?
#define vtkGetConstMacro(name,type)             \
  virtual type Get##name () const {             \
    return this->name;                          \
  }

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTProtonBeamNode : public vtkMRMLRTBeamNode
{
public:
    enum RTProtonAlgorithm
      {
        RayTracer = 0,
        CGS = 1, // Cartesian Geometry dose summation
        DGS = 2, // Divergent Geometry dose summation
        HGS = 3  // Hong Geometry dose summation
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
  vtkGetMacro(BeamLineType, bool);
  vtkSetMacro(BeamLineType, bool);

  vtkGetMacro(ManualEnergyLimits, bool);
  vtkSetMacro(ManualEnergyLimits, bool);

  vtkGetMacro(MinimumEnergy, double);
  vtkSetMacro(MinimumEnergy, double);

  vtkGetMacro(MaximumEnergy, double);
  vtkSetMacro(MaximumEnergy, double);

  vtkGetMacro(EnergyResolution, float);
  vtkSetMacro(EnergyResolution, float);

  vtkGetMacro(EnergySpread, float);
  vtkSetMacro(EnergySpread, float);

  vtkGetMacro(StepLength, float);
  vtkSetMacro(StepLength, float);

  vtkGetMacro(ProximalMargin, double);
  vtkSetMacro(ProximalMargin, double);
  
  vtkGetMacro(DistalMargin, double);
  vtkSetMacro(DistalMargin, double);
 
  vtkGetMacro(PBResolution, double);
  vtkSetMacro(PBResolution, double);

  vtkGetMacro(ApertureOffset, double);
  vtkSetMacro(ApertureOffset, double);

  vtkGetMacro(SourceSize, double);
  vtkSetMacro(SourceSize, double);

  vtkGetMacro(LateralSpreadHomoApprox, bool);
  vtkSetMacro(LateralSpreadHomoApprox, bool);

  vtkGetMacro(RangeCompensatorHighland, bool);
  vtkSetMacro(RangeCompensatorHighland, bool);

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
  /// Beam line type: true = active, false = passive
  bool BeamLineType;

  /// Manual energy limits
  bool ManualEnergyLimits;
  /// Energy minimum
  double MinimumEnergy;
  /// Energy maximum
  double MaximumEnergy;

  /// Energy resolution
  float EnergyResolution;
  /// Energy resolution
  float EnergySpread;
  /// Step length
  float StepLength;

  /// Proximal margin
  double ProximalMargin;
  /// Distal margin
  double DistalMargin;

  /// Pencil beam resolution
  double PBResolution;

  /// Aperture offset (must be smaller than SAD)
  double ApertureOffset;

  /// Source size
  double SourceSize;

  /// Lateral spread homogeneous approximation
  bool LateralSpreadHomoApprox;
  /// Range compensator Monte Carlo based
  bool RangeCompensatorHighland;

  /// Have prescription flag
  bool HavePrescription;

  /// Proton dose calculation algorithm type
  RTProtonAlgorithm Algorithm;

  double ApertureSpacing[2];
  double ApertureOrigin[2];
  int ApertureDim[2];
};

#endif // __vtkMRMLRTProtonBeamNode_h
