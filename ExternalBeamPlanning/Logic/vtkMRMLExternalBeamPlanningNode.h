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

#ifndef __vtkMRMLExternalBeamPlanningNode_h
#define __vtkMRMLExternalBeamPlanningNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

class vtkMRMLScalarVolumeNode;
class vtkMRMLRTPlanNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLSegmentationNode;
class vtkMRMLDoubleArrayNode;

#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkMRMLExternalBeamPlanningNode : public vtkMRMLNode
{
public:
  // ExternalBeamPlanning constants
  static const char* NEW_BEAM_NODE_NAME_PREFIX;
  static const char* TOTAL_PROTON_DOSE_VOLUME_REFERENCE_ROLE;

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
  virtual const char* GetNodeTagName() { return "ExternalBeamPlanning"; };

public:
  /// Get RT plan node
  vtkMRMLRTPlanNode* GetRTPlanNode();
  /// Set and observe RT plan node
  void SetAndObserveRTPlanNode(vtkMRMLRTPlanNode* node);

  /// Get MLC position double array node
  vtkMRMLDoubleArrayNode* GetMLCPositionDoubleArrayNode();
  /// Set and observe MLC position double array node
  void SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode* node);

  /// Assemble reference volume node reference role for aperture volume node with given beam
  static std::string AssembleApertureVolumeReference(vtkMRMLNode* beamNode);
  /// Assemble reference volume node reference role for range compensator volume node with given beam
  static std::string AssembleRangeCompensatorVolumeReference(vtkMRMLNode* beamNode);
  /// Assemble reference volume node reference role for proton dose volume node with given beam
  static std::string AssembleProtonDoseVolumeReference(vtkMRMLNode* beamNode);

protected:
  vtkMRMLExternalBeamPlanningNode();
  ~vtkMRMLExternalBeamPlanningNode();
  vtkMRMLExternalBeamPlanningNode(const vtkMRMLExternalBeamPlanningNode&);
  void operator=(const vtkMRMLExternalBeamPlanningNode&);
};

#endif
