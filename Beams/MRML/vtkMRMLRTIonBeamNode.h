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

public:

  /// Get VSADx distance
  vtkGetMacro( VSADx, double);
  /// Set VSADx distance. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetVSADx(double VSADxComponent);

  /// Get VSADy distance
  vtkGetMacro( VSADy, double);
  /// Set VSADy distance. Triggers \sa BeamGeometryModified event and re-generation of beam model
  void SetVSADy(double VSADyComponent);

protected:
  /// Create beam model from beam parameters, supporting MLC leaves
  /// \param beamModelPolyData Output polydata. If none given then the beam node's own polydata is used
  virtual void CreateBeamPolyData(vtkPolyData* beamModelPolyData=nullptr);

protected:
  vtkMRMLRTIonBeamNode();
  ~vtkMRMLRTIonBeamNode();
  vtkMRMLRTIonBeamNode(const vtkMRMLRTIonBeamNode&);
  void operator=(const vtkMRMLRTIonBeamNode&);

// Beam properties
protected:

  /// Virtual Source-axis distance x component
  double& VSADx; // using SAD to store value
  /// Virtual Source-axis distance y component
  double VSADy;
};

#endif // __vtkMRMLRTIonBeamNode_h
