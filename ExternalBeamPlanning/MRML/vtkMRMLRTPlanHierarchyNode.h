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

#ifndef __vtkMRMLRTPlanHierarchyNode_h
#define __vtkMRMLRTPlanHierarchyNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLScene.h>

#include "vtkSlicerExternalBeamPlanningModuleMRMLExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLColorTableNode;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_MRML_EXPORT vtkMRMLRTPlanHierarchyNode : public vtkMRMLDisplayableHierarchyNode
{
public:
  static vtkMRMLRTPlanHierarchyNode *New();
  vtkTypeMacro(vtkMRMLRTPlanHierarchyNode,vtkMRMLHierarchyNode);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "RTPlanHierarchy";};

  /// Handles events registered in the observer manager
  /// - Invalidates (deletes) all non-active representations when the active is modified
  /// - Follows parent transform changes
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData);

protected:
  vtkMRMLRTPlanHierarchyNode();
  ~vtkMRMLRTPlanHierarchyNode();
  vtkMRMLRTPlanHierarchyNode(const vtkMRMLRTPlanHierarchyNode&);
  void operator=(const vtkMRMLRTPlanHierarchyNode&);

protected:
  char* StructureName; //TODO: This should be stored in the subject hierarchy
};

#endif // __vtkMRMLRTPlanHierarchyNode_h
