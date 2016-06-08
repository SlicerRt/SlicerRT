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
#include <vtkMRMLScene.h>

// SlicerRT includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkSlicerBeamsModuleMRMLExport.h"

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

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "RTProtonBeam";};

  /// Calculate and set aperture parameters (origin, spacing, dimensions) based on
  /// the jaw positions, SAD, aperture offset, and pencil beam resolution
  void UpdateApertureParameters();

public:
  vtkGetMacro(BeamLineTypeActive, bool);
  vtkSetMacro(BeamLineTypeActive, bool);

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
 
  vtkGetMacro(PencilBeamResolution, double);
  vtkSetMacro(PencilBeamResolution, double);

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

  vtkGetMacro(RangeCompensatorSmearingRadius, double);
  vtkSetMacro(RangeCompensatorSmearingRadius, double);

  vtkGetMacro(Algorithm, RTProtonAlgorithm);
  vtkSetMacro(Algorithm, RTProtonAlgorithm);

  vtkGetVector2Macro(ApertureSpacing, double);
  vtkSetVector2Macro(ApertureSpacing, double);

  vtkGetVector2Macro(ApertureOrigin, double);
  vtkSetVector2Macro(ApertureOrigin, double);

  vtkGetVector2Macro(ApertureDim, int);
  vtkSetVector2Macro(ApertureDim, int);

protected:
  vtkMRMLRTProtonBeamNode();
  ~vtkMRMLRTProtonBeamNode();
  vtkMRMLRTProtonBeamNode(const vtkMRMLRTProtonBeamNode&);
  void operator=(const vtkMRMLRTProtonBeamNode&);

protected:
  /// Beam line type: true = active, false = passive
  bool BeamLineTypeActive;

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
  double PencilBeamResolution;

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

  /// TODO:
  double RangeCompensatorSmearingRadius;

  /// Proton dose calculation algorithm type
  RTProtonAlgorithm Algorithm;

  double ApertureSpacing[2];
  double ApertureOrigin[2];
  int ApertureDim[2];
};

#endif // __vtkMRMLRTProtonBeamNode_h
