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

// MRML includes

// ITK includes
#include <itkImage.h>

// STD includes
#include <cstdlib>

#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

class vtkMRMLExternalBeamPlanningNode;
class vtkMRMLRTBeamNode;
class vtkMRMLRTPlanNode;
class vtkPolyData;
class vtkDoubleArray;
class vtkSlicerCLIModuleLogic;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkSlicerExternalBeamPlanningModuleLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerExternalBeamPlanningModuleLogic *New();
  vtkTypeMacro(vtkSlicerExternalBeamPlanningModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  /// TODO
  void SetAndObserveExternalBeamPlanningNode(vtkMRMLExternalBeamPlanningNode* node);

  /// Get the EBP Node
  vtkGetObjectMacro(ExternalBeamPlanningNode, vtkMRMLExternalBeamPlanningNode);

  /// Get the Plan associated with the EBP Node
  vtkMRMLRTPlanNode * GetRTPlanNode();

  /// Create a new beam, and add it to the plan associated with the EBP Node
  vtkMRMLRTBeamNode * AddBeam();

  /// Remove a beam with a specified beam name
  void RemoveBeam(char*);

  /// TODO
  void UpdateBeamTransform(char*);

  /// TODO
  void UpdateBeamGeometryModel(char*);

  /// TODO
  void UpdateDRR(char*);

  /// TODO
  vtkSmartPointer<vtkPolyData> CreateBeamPolyData(double, double, double, double, double, vtkDoubleArray*);

  /// TODO
  vtkSmartPointer<vtkPolyData> CreateBeamPolyData(double, double, double, double, double);

  /// TODO
  void ComputeDose (vtkMRMLRTBeamNode*);

  /// TODO
  void ComputeDoseByPlastimatch (vtkMRMLRTBeamNode*);

  /// TODO
  void ComputeWED ();

  /// TODO
  void SetMatlabDoseCalculationModuleLogic(vtkSlicerCLIModuleLogic* logic);
  vtkSlicerCLIModuleLogic* GetMatlabDoseCalculationModuleLogic();

  /// TODO
  void ComputeDoseByMatlab (vtkMRMLRTBeamNode*);

  /// TODO
  void InitializeAccumulatedDose();

  /// TODO
  void RegisterAccumulatedDose();

  /// Remove all MRML nodes created by dose calculation, such as 
  /// apertures, range compensators, and doses
  void RemoveDoseNodes();

protected:
  vtkSlicerExternalBeamPlanningModuleLogic();
  virtual ~vtkSlicerExternalBeamPlanningModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

  /// Parameter set MRML node
  vtkMRMLExternalBeamPlanningNode* ExternalBeamPlanningNode;
  int DRRImageSize[2];

private:
  vtkSlicerExternalBeamPlanningModuleLogic(const vtkSlicerExternalBeamPlanningModuleLogic&); // Not implemented
  void operator=(const vtkSlicerExternalBeamPlanningModuleLogic&);               // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
