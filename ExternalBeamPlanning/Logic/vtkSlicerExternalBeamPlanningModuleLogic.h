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
#include "vtkOrientedImageData.h"

// MRML includes

// ITK includes
#include <itkImage.h>

// STD includes
#include <cstdlib>

#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

class vtkMRMLRTPlanNode;
class vtkMRMLRTBeamNode;
class vtkSlicerCLIModuleLogic;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkSlicerExternalBeamPlanningModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerExternalBeamPlanningModuleLogic *New();
  vtkTypeMacro(vtkSlicerExternalBeamPlanningModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  /// Set and observe RT plan node
  void SetAndObserveRTPlanNode(vtkMRMLRTPlanNode* node);
  /// Get RT plan Node
  vtkGetObjectMacro(RTPlanNode, vtkMRMLRTPlanNode);

  /// Create a new beam, and add it to the plan associated with the EBP Node
  vtkMRMLRTBeamNode* AddBeam(vtkMRMLRTBeamNode* copyFrom);

  /// Remove a specific beam
  void RemoveBeam(vtkMRMLRTBeamNode *beam);

  // Update the beam model for a new isocenter, gantry angle, etc.
  void UpdateBeamTransform(vtkMRMLRTBeamNode *beamNode);

  /// TODO
  void UpdateBeamGeometryModel(char*);

  /// TODO
  bool ComputeTargetVolumeCenter(vtkMRMLRTBeamNode *beam, double* center);

  /// TODO
  void SetBeamIsocenterToTargetCenter(vtkMRMLRTBeamNode *beam);

  /// TODO
  void UpdateDRR(char*);

  /// Get labelmap from target segment of beam node
  vtkSmartPointer<vtkOrientedImageData> GetTargetLabelmap(vtkMRMLRTBeamNode* beamNode);

  /// Compute dose for one beam
  std::string ComputeDose(vtkMRMLRTBeamNode*);

  /// TODO
  std::string ComputeDoseByPlastimatch(vtkMRMLRTBeamNode*);

  /// TODO
  void ComputeWED();

  /// TODO
  void SetMatlabDoseCalculationModuleLogic(vtkSlicerCLIModuleLogic* logic);
  vtkSlicerCLIModuleLogic* GetMatlabDoseCalculationModuleLogic();

  /// TODO
  std::string ComputeDoseByMatlab(vtkMRMLRTBeamNode*);

  /// TODO
  std::string InitializeAccumulatedDose();

  /// TODO
  std::string FinalizeAccumulatedDose();

  /// Remove MRML nodes created by dose calculation for the current RT plan,
  /// such as apertures, range compensators, and doses
  void RemoveDoseNodes();

protected:
  vtkSlicerExternalBeamPlanningModuleLogic();
  virtual ~vtkSlicerExternalBeamPlanningModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  virtual void UpdateFromMRMLScene();

  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

  /// RT plan MRML node
  vtkMRMLRTPlanNode* RTPlanNode;

  /// TODO:
  int DRRImageSize[2];

private:
  vtkSlicerExternalBeamPlanningModuleLogic(const vtkSlicerExternalBeamPlanningModuleLogic&); // Not implemented
  void operator=(const vtkSlicerExternalBeamPlanningModuleLogic&);               // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
