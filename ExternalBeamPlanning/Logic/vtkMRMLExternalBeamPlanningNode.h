/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __vtkMRMLExternalBeamPlanningNode_h
#define __vtkMRMLExternalBeamPlanningNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <vector>

#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkMRMLExternalBeamPlanningNode : public vtkMRMLNode
{
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

public:
  /// Get TODO
  vtkGetStringMacro(ReferenceVolumeNodeID);

  /// Set TODO
  void SetAndObserveReferenceVolumeNodeID(const char* id);

  /// Get TODO
  vtkGetStringMacro(RTPlanNodeID);

  /// Set TODO
  void SetAndObserveRTPlanNodeID(const char* id);

  /// Get TODO
  vtkGetStringMacro(IsocenterNodeID);

  /// Set TODO
  void SetAndObserveIsocenterNodeID(const char* id);

  /// Update the stored reference to another node in the scene 
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// Get TODO
  vtkGetStringMacro(ProtonTargetVolumeNodeID);

  /// Set TODO
  void SetAndObserveProtonTargetVolumeNodeID(const char* id);

  vtkGetMacro(GantryAngle, double);
  vtkSetMacro(GantryAngle, double);
  vtkGetMacro(Smearing, double);
  vtkSetMacro(Smearing, double);
  vtkGetMacro(ProximalMargin, double);
  vtkSetMacro(ProximalMargin, double);
  vtkGetMacro(DistalMargin, double);
  vtkSetMacro(DistalMargin, double);

protected:
  /// Set TODO
  vtkSetStringMacro(ReferenceVolumeNodeID);

  /// Set TODO
  vtkSetStringMacro(RTPlanNodeID);
 
  /// Set TODO
  vtkSetStringMacro(IsocenterNodeID);
 
  /// Set TODO
  vtkSetStringMacro(ProtonTargetVolumeNodeID);

 
protected:
  vtkMRMLExternalBeamPlanningNode();
  ~vtkMRMLExternalBeamPlanningNode();
  vtkMRMLExternalBeamPlanningNode(const vtkMRMLExternalBeamPlanningNode&);
  void operator=(const vtkMRMLExternalBeamPlanningNode&);

  /// TODO
  char* ReferenceVolumeNodeID;

  /// TODO
  char* RTPlanNodeID;

  /// TODO
  char* IsocenterNodeID;

  /// TODO
  char* ProtonTargetVolumeNodeID;

  double GantryAngle;
  double Smearing;
  double ProximalMargin;
  double DistalMargin;
};

#endif
