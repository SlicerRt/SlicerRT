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

// .NAME vtkSlicerExternalBeamPlanningModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerExternalBeamPlanningModuleLogic_h
#define __vtkSlicerExternalBeamPlanningModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// Segmentations includes
#include "vtkOrientedImageData.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

class vtkMRMLRTPlanNode;
class vtkMRMLRTBeamNode;
class vtkSlicerCLIModuleLogic;
class vtkSlicerBeamsModuleLogic;
class vtkSlicerDoseAccumulationModuleLogic;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkSlicerExternalBeamPlanningModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerExternalBeamPlanningModuleLogic *New();
  vtkTypeMacro(vtkSlicerExternalBeamPlanningModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set Beams module logic
  void SetBeamsLogic(vtkSlicerBeamsModuleLogic* beamsLogic);
  /// Get Beams module logic
  vtkGetObjectMacro(BeamsLogic, vtkSlicerBeamsModuleLogic);

public:
  /// Create a new beam based on another beam, and add it to the plan
  /// \param copiedBeamNode Beam node to clone
  /// \param planNode Plan to add beam clone to. If omitted then beam clone is added in same plan as copiedBeamNode
  /// \return The new beam node that has been copied and added to the plan
  vtkMRMLRTBeamNode* CloneBeamInPlan(vtkMRMLRTBeamNode* copiedBeamNode, vtkMRMLRTPlanNode* planNode=nullptr);

//TODO: Obsolete functions
public:
  /// TODO Fix
  /// TODO Move to separate logic
  void UpdateDRR(vtkMRMLRTPlanNode* planNode, char* beamName);

  /// TODO
  void ComputeWED();

  /// TODO
  void SetMatlabDoseCalculationModuleLogic(vtkSlicerCLIModuleLogic* logic);
  vtkSlicerCLIModuleLogic* GetMatlabDoseCalculationModuleLogic();

  /// TODO Use plugin mechanism instead of dedicated function
  std::string ComputeDoseByMatlab(vtkMRMLRTPlanNode* planNode, vtkMRMLRTBeamNode* beamNode);

protected:
  vtkSlicerExternalBeamPlanningModuleLogic();
  virtual ~vtkSlicerExternalBeamPlanningModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes() override;

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  virtual void UpdateFromMRMLScene() override;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneEndImport() override;
  virtual void OnMRMLSceneEndClose() override;

  /// Handles events registered in the observer manager
  virtual void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData) override;

protected:
  /// TODO:
  int DRRImageSize[2];

private:
  vtkSlicerExternalBeamPlanningModuleLogic(const vtkSlicerExternalBeamPlanningModuleLogic&); // Not implemented
  void operator=(const vtkSlicerExternalBeamPlanningModuleLogic&);               // Not implemented

  //TODO: Remove internal class and this member when it becomes empty (Matlab dose engines)
  class vtkInternal;
  vtkInternal* Internal;

  /// Beams module logic instance
  vtkSlicerBeamsModuleLogic* BeamsLogic;
};

#endif
