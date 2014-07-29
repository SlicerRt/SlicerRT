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

#ifndef __vtkMRMLRTPlanNode_h
#define __vtkMRMLRTPlanNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLScene.h>

#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLColorTableNode;
class vtkMRMLRTBeamNode;
class vtkCollection;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkMRMLRTPlanNode : public vtkMRMLDisplayableNode
{
public:
  enum DoseEngineType
  {
    Plastimatch = 0,
    PMH = 1,
    Matlab = 2
  };

public:
  static vtkMRMLRTPlanNode *New();
  vtkTypeMacro(vtkMRMLRTPlanNode,vtkMRMLDisplayableNode);
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
  virtual const char* GetNodeTagName() {return "RTPlan";};

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

  /// Get RT Plan Dose volume node
  vtkMRMLScalarVolumeNode* GetRTPlanDoseVolumeNode();
  /// Set and observe proton target contour node
  void SetAndObserveRTPlanDoseVolumeNode(vtkMRMLScalarVolumeNode* node);

  /// Get/Set Save labelmaps checkbox state
  void SetRTPlanDoseEngine(DoseEngineType doseEngineType) {this->RTPlanDoseEngine = doseEngineType;}
  DoseEngineType GetRTPlanDoseEngine() {return this->RTPlanDoseEngine;}

  ///
  void AddRTBeamNode(vtkMRMLRTBeamNode *);

  ///
  void RemoveRTBeamNode(vtkMRMLRTBeamNode *);

  ///
  void GetRTBeamNodes(vtkCollection *);

  /// Set RTPlanName
  vtkSetStringMacro(RTPlanName);

  /// Get RTPlanName
  vtkGetStringMacro(RTPlanName);

protected:
  vtkMRMLRTPlanNode();
  ~vtkMRMLRTPlanNode();
  vtkMRMLRTPlanNode(const vtkMRMLRTPlanNode&);
  void operator=(const vtkMRMLRTPlanNode&);

protected:
  char* RTPlanName;
  DoseEngineType RTPlanDoseEngine;
};

#endif // __vtkMRMLRTPlanNode_h
