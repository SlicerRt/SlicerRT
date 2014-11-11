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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerDoseAccumulationModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseAccumulationModuleLogic_h
#define __vtkSlicerDoseAccumulationModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerDoseAccumulationModuleLogicExport.h"

class vtkMRMLDoseAccumulationNode;

/// \ingroup SlicerRt_QtModules_DoseAccumulation
class VTK_SLICER_DOSEACCUMULATION_LOGIC_EXPORT vtkSlicerDoseAccumulationModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDoseAccumulationModuleLogic *New();
  vtkTypeMacro(vtkSlicerDoseAccumulationModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Collect and return volume nodes from the scene
  /// If in DoseAccumulationNode::ShowDoseVolumesOnly is set to true, then only return dose volumes
  vtkCollection* GetVolumeNodesFromScene();

  /// Determine if reference volume is a dose volume
  bool ReferenceDoseVolumeContainsDose();

  /// Accumulates dose volumes with the given IDs and corresponding weights
  /// \return Error message on failure, NULL otherwise
  const char* AccumulateDoseVolumes();

public:
  /// Set and observe dose accumulation parameter node 
  void SetAndObserveDoseAccumulationNode(vtkMRMLDoseAccumulationNode* node);

  /// Get dose accumulation parameter node 
  vtkGetObjectMacro(DoseAccumulationNode, vtkMRMLDoseAccumulationNode);

protected:
  vtkSlicerDoseAccumulationModuleLogic();
  virtual ~vtkSlicerDoseAccumulationModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerDoseAccumulationModuleLogic(const vtkSlicerDoseAccumulationModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDoseAccumulationModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLDoseAccumulationNode* DoseAccumulationNode;
};

#endif

