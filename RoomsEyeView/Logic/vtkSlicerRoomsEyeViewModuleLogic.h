/*==============================================================================

Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
Queen's University, Kingston, ON, Canada. All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file was originally developed by Csaba Pinter, PerkLab, Queen's University
and was supported through the Applied Cancer Research Unit program of Cancer Care
Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerRoomsEyeViewModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes

#ifndef __vtkSlicerRoomsEyeViewModuleLogic_h
#define __vtkSlicerRoomsEyeViewModuleLogic_h

#include "vtkSlicerRoomsEyeViewModuleLogicExport.h"

// Slicer includes
#include <vtkSlicerModuleLogic.h>

class vtkCollisionDetectionFilter;
class vtkSlicerIECTransformLogic;
class vtkMRMLRoomsEyeViewNode;
class vtkPolyData;
class vtkMatrix4x4; //TODO: Remove once the members are removed

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkSlicerRoomsEyeViewModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static const char* COLLIMATOR_MODEL_NAME;
  static const char* GANTRY_MODEL_NAME;
  static const char* IMAGINGPANELLEFT_MODEL_NAME;
  static const char* IMAGINGPANELRIGHT_MODEL_NAME;
  static const char* LINACBODY_MODEL_NAME;
  static const char* PATIENTSUPPORT_MODEL_NAME;
  static const char* TABLETOP_MODEL_NAME;
  static const char* APPLICATORHOLDER_MODEL_NAME;
  static const char* ELECTRONAPPLICATOR_MODEL_NAME;
  static const char* ORIENTATION_MARKER_MODEL_NODE_NAME;

public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Return IEC logic that has been created for room's eye view
  vtkSlicerIECTransformLogic* GetIECLogic();

  /// Load pre-defined components of the treatment machine into the scene
  void LoadLinacModels();
  /// Create or get transforms taking part in the IEC logic and additional devices, and build the transform hierarchy
  void BuildRoomsEyeViewTransformHierarchy();
  /// Set up the IEC transforms on the treatment machine models
  void SetupTreatmentMachineModels();

  /// Update GantryToFixedReference transform based on gantry angle from UI slider
  void UpdateGantryToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Update CollimatorToGantry transform based on collimator angle parameter
  void UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Update left imaging panel to gantry transform based on imaging panel movement parameter
  void UpdateLeftImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Update right imaging panel to gantry transform based on imaging panel movement parameter
  void UpdateRightImagingPanelToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Update both left and right imaging panel transforms based on imaging panel movement parameter
  void UpdateImagingPanelMovementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Update PatientSupportRotrationToFixedReference transform based on patient support rotation parameter
  void UpdatePatientSupportRotationToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  ///  Update PatientSupportToPatientSupportRotation transform based on patient support vertical translation parameter
  void UpdatePatientSupportToPatientSupportRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Update TableTopEccentricRotationToPatientSupportRotation based on table top eccentric rotation parameter
  /// NOTE: This rotation is currently not supported (only rotate table top on the patient support)
  void UpdateTableTopEccentricRotationToPatientSupportRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Update TableTopToTableTopEccentricRotation based on all three table top translations
  void UpdateTableTopToTableTopEccentricRotationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
 
  ///TODO:
  void UpdateAdditionalCollimatorDevicesToCollimatorTransforms(vtkMRMLRoomsEyeViewNode* parameterNode);

  ///TODO:
  void UpdateAdditionalDevicesVisibility(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Check for collisions between pieces of linac model using vtkCollisionDetectionFilter
  /// \return string indicating whether collision occurred
  std::string CheckForCollisions(vtkMRMLRoomsEyeViewNode* parameterNode);

  ///TODO:
  void LoadAdditionalDevices();

  ///TODO:
  void UpdateTreatmentOrientationMarker();

protected:
  /// Get patient body closed surface poly data from segmentation node and segment selection in the parameter node
  bool GetPatientBodyPolyData(vtkMRMLRoomsEyeViewNode* parameterNode, vtkPolyData* patientBodyPolyData);

protected:
  vtkSlicerIECTransformLogic* IECLogic;

  //TODO: Remove these two members
  vtkMatrix4x4* CollimatorToWorldTransformMatrix;
  vtkMatrix4x4* TableTopToWorldTransformMatrix;

  vtkCollisionDetectionFilter* GantryPatientCollisionDetection;
  vtkCollisionDetectionFilter* GantryTableTopCollisionDetection;
  vtkCollisionDetectionFilter* GantryPatientSupportCollisionDetection;

  vtkCollisionDetectionFilter* CollimatorPatientCollisionDetection;
  vtkCollisionDetectionFilter* CollimatorTableTopCollisionDetection;

  vtkCollisionDetectionFilter* AdditionalModelsTableTopCollisionDetection;
  vtkCollisionDetectionFilter* AdditionalModelsPatientSupportCollisionDetection;

protected:
  vtkSlicerRoomsEyeViewModuleLogic();
  virtual ~vtkSlicerRoomsEyeViewModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

private:
  vtkSlicerRoomsEyeViewModuleLogic(const vtkSlicerRoomsEyeViewModuleLogic&); // Not implemented
  void operator=(const vtkSlicerRoomsEyeViewModuleLogic&);            // Not implemented
};

#endif

