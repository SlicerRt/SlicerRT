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

#ifndef __vtkMRMLRTIonBeamNode_h
#define __vtkMRMLRTIonBeamNode_h

// Beams includes
#include "vtkSlicerBeamsModuleMRMLExport.h"

// MRML includes
#include "vtkMRMLRTBeamNode.h"

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_MODULE_MRML_EXPORT vtkMRMLRTIonBeamNode : public vtkMRMLRTBeamNode
{

public:
  static vtkMRMLRTIonBeamNode *New();
  vtkTypeMacro(vtkMRMLRTIonBeamNode,vtkMRMLRTBeamNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Create instance of a GAD node. 
  vtkMRMLNode* CreateNodeInstance() override;

  /// Set node attributes from name/value pairs 
  void ReadXMLAttributes(const char** atts) override;

  /// Write this node's information to a MRML file in XML format. 
  void WriteXML(ostream& of, int indent) override;

  /// Copy the node's attributes to this object 
  void Copy(vtkMRMLNode *node) override;

  /// Copy node content (excludes basic data, such a name and node reference)
  vtkMRMLCopyContentMacro(vtkMRMLRTIonBeamNode);

  /// Make sure display node and transform node are present and valid
  void SetScene(vtkMRMLScene* scene) override;

  /// Get unique node XML tag name (like Volume, Model) 
  const char* GetNodeTagName() override { return "RTIonBeam"; };

  /// Create and observe default display node
  void CreateDefaultDisplayNodes() override;

  /// Create transform node that places the beam poly data in the right position based on geometry.
  /// Only creates it if missing
  void CreateDefaultTransformNode() override;

  /// Create transform node that places the beam poly data in the right position based on geometry.
  /// Always creates a new transform node.
  void CreateNewBeamTransformNode() override;

  /// Create transform node that places the beam poly data in the right position based on geometry.
  /// Always creates a new transform node.
  vtkMRMLLinearTransformNode* CreateBeamTransformNode(vtkMRMLScene*) override;

public:
  /// Get VSAD distance x component
  vtkGetMacro( VSADx, double);
  /// Set VSAD distance x component
  vtkSetMacro( VSADx, double);
  /// Get VSAD distance y component
  vtkGetMacro( VSADy, double);
  /// Set VSAD distance y component
  vtkSetMacro( VSADy, double);

  /// Set VSAD distance. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetVSAD( double VSADx, double VSADy);
  void SetVSAD(double VSAD[2]);
  void SetVSAD(const std::array< double, 2 >& VSAD);

  vtkGetVector2Macro( ScanningSpotSize, float);

  /// Set scanning spot size for modulated beam
  /// Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetScanningSpotSize(float ScanSpotSize[2]);
  void SetScanningSpotSize(const std::array< float, 2 >& ScanSpotSize);

  /// Get isocenter to jaws X distance
  vtkGetMacro(IsocenterToJawsDistanceX, double);
  /// Set isocenter to jaws X distance. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetIsocenterToJawsDistanceX(double distance);

  /// Get isocenter to jaws Y distance
  vtkGetMacro(IsocenterToJawsDistanceY, double);
  /// Set isocenter to jaws Y distance. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetIsocenterToJawsDistanceY(double distance);

  /// Get isocenter to range shifter distance
  vtkGetMacro(IsocenterToRangeShifterDistance, double);
  /// Set isocenter to range shifter. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetIsocenterToRangeShifterDistance(double distance);

  /// Get isocenter to multi-leaf collimator distance
  vtkGetMacro(IsocenterToMultiLeafCollimatorDistance, double);
  /// Set isocenter to multi-leaf collimator distance. Triggers \sa BeamTransformModified event and re-generation of beam model
  void SetIsocenterToMultiLeafCollimatorDistance(double distance);

  /// Set and observe scan spot position map & meterset weights table node
  /// Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetAndObserveScanSpotTableNode(vtkMRMLTableNode* node);
  /// Get scan spot position map & meterset weights table node
  vtkMRMLTableNode* GetScanSpotTableNode();

protected:
  /// Create beam model from beam parameters, supporting MLC leaves, jaws
  /// and scan spot map for modulated scan mode
  /// \param beamModelPolyData Output polydata. If none given then the beam node's own polydata is used
  void CreateBeamPolyData(vtkPolyData* beamModelPolyData=nullptr) override;

protected:
  vtkMRMLRTIonBeamNode();
  ~vtkMRMLRTIonBeamNode();
  vtkMRMLRTIonBeamNode(const vtkMRMLRTIonBeamNode&);
  void operator=(const vtkMRMLRTIonBeamNode&);

// Beam properties
protected:

  /// Virtual Source-axis distance x component
  double& VSADx; // using SAD to store value from vtkMRMLRTBeamNode
  /// Virtual Source-axis distance y component
  double VSADy;
  /// distance from isocenter to beam limiting device X, ASYMX
  // using SourceToJawsDistanceX to store value from vtkMRMLRTBeamNode
  double& IsocenterToJawsDistanceX;
  /// distance from isocenter to beam limiting device Y, ASYMY
  // using SourceToJawsDistanceY to store value from vtkMRMLRTBeamNode
  double& IsocenterToJawsDistanceY;
  /// distance from isocenter to beam limiting device MLCX, MLCY
  // using SourceToMultiLeafCollimatorDistance to store value from vtkMRMLRTBeamNode
  double& IsocenterToMultiLeafCollimatorDistance;
  /// distance from isocenter to range shifter device
  double IsocenterToRangeShifterDistance;

  float ScanningSpotSize[2];
};

#endif // __vtkMRMLRTIonBeamNode_h
