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
class vtkMRMLModelNode;
class vtkPolyData;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkSlicerRoomsEyeViewModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static const char* COLLIMATOR_MODEL_NAME;
  static const char* GANTRY_MODEL_NAME;
  static const char* PATIENTSUPPORT_MODEL_NAME;
  static const char* TABLETOP_MODEL_NAME;

  static const char* LINACBODY_MODEL_NAME;
  static const char* IMAGINGPANELLEFT_MODEL_NAME;
  static const char* IMAGINGPANELRIGHT_MODEL_NAME;
  static const char* FLATPANEL_MODEL_NAME;

  static const char* APPLICATORHOLDER_MODEL_NAME;
  static const char* ELECTRONAPPLICATOR_MODEL_NAME;
  static const char* ORIENTATION_MARKER_MODEL_NODE_NAME;

public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Load pre-defined components of the treatment machine into the scene
  /// \param parameterNode Parameter node contains the type of treatment machine
  ///        (must match folder name where the models can be found)
  void LoadTreatmentMachineModels(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Set up the IEC transforms and model properties on the treatment machine models
  void SetupTreatmentMachineModels();
  /// Create or get transforms taking part in the IEC logic and additional devices, and build the transform hierarchy
  void BuildRoomsEyeViewTransformHierarchy();

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
 
  /// Update orientation marker based on the current transforms
  vtkMRMLModelNode* UpdateTreatmentOrientationMarker();

  /// Check for collisions between pieces of linac model using vtkCollisionDetectionFilter
  /// \return string indicating whether collision occurred
  std::string CheckForCollisions(vtkMRMLRoomsEyeViewNode* parameterNode);

// Additional device related methods
public:
  /// Load basic additional devices (deployed with SlicerRT)
  void LoadBasicCollimatorMountedDevices();
  /// Set up the IEC transforms and model properties on the basic additional device models
  void SetupBasicCollimatorMountedDeviceModels();

  ///TODO:
  void UpdateAdditionalCollimatorDevicesToCollimatorTransforms(vtkMRMLRoomsEyeViewNode* parameterNode);

  ///TODO:
  void UpdateAdditionalDevicesVisibility(vtkMRMLRoomsEyeViewNode* parameterNode);

// Set/get methods
public:
  vtkGetObjectMacro(IECLogic, vtkSlicerIECTransformLogic);

  vtkGetObjectMacro(GantryPatientCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(GantryTableTopCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(GantryPatientSupportCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(CollimatorPatientCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(CollimatorTableTopCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(AdditionalModelsTableTopCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(AdditionalModelsPatientSupportCollisionDetection, vtkCollisionDetectionFilter);

protected:
  /// Get patient body closed surface poly data from segmentation node and segment selection in the parameter node
  bool GetPatientBodyPolyData(vtkMRMLRoomsEyeViewNode* parameterNode, vtkPolyData* patientBodyPolyData);

protected:
  vtkSlicerIECTransformLogic* IECLogic;

  vtkCollisionDetectionFilter* GantryPatientCollisionDetection;
  vtkCollisionDetectionFilter* GantryTableTopCollisionDetection;
  vtkCollisionDetectionFilter* GantryPatientSupportCollisionDetection;

  vtkCollisionDetectionFilter* CollimatorPatientCollisionDetection;
  vtkCollisionDetectionFilter* CollimatorTableTopCollisionDetection;

  vtkCollisionDetectionFilter* AdditionalModelsTableTopCollisionDetection;
  vtkCollisionDetectionFilter* AdditionalModelsPatientSupportCollisionDetection;

protected:
  vtkSlicerRoomsEyeViewModuleLogic();
  ~vtkSlicerRoomsEyeViewModuleLogic() override;

  /// Register MRML Node classes to scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  void SetMRMLSceneInternal(vtkMRMLScene * newScene) override;

private:
  vtkSlicerRoomsEyeViewModuleLogic(const vtkSlicerRoomsEyeViewModuleLogic&) = delete;
  void operator=(const vtkSlicerRoomsEyeViewModuleLogic&) = delete;
};

#endif

