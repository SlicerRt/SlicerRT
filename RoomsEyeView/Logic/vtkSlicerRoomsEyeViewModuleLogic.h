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

// SlicerRT includes
#include "vtkCollisionDetectionFilter.h"

class vtkMRMLRoomsEyeViewNode;
class vtkMRMLRTBeamNode;

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

public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Load pre-defined components of the treatment machine into the scene
  void LoadLinacModels();
  /// Create or get transforms taking part in the IEC logic and additional devices, and build the transform hierarchy
  void BuildRoomsEyeViewTransformHierarchy();
  /// Set up the IEC transforms on the treatment machine models
  void SetupTreatmentMachineModels();

  /// Update CollimatorToFixedReference Isocenter transform by translating collimator model to isocenter
  void UpdateCollimatorToFixedReferenceIsocenterTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Update FixedReferenceIsocenterToCollimatorRotated transform based on collimator angle from UI slider
  void UpdateFixedReferenceIsocenterToCollimatorRotatedTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Update CollimatorToGantry transform by translating collimator model back to the gantry
  void UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  void UpdateAdditionalCollimatorDevicesToCollimatorTransforms(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Update GantryToFixedReference transform based on gantry angle from UI slider
  void UpdateGantryToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Translate center of left imaging panel to isocenter of fixed reference coordinate system
  void UpdateLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform();
  /// Rotate left imaging panel based on imagingPanelMovement from UI slider
  void UpdateLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Translate rotated left imaging panel from fixed reference isocenter back to gantry
  void UpdateLeftImagingPanelRotatedToGantryTransform();
  /// Translate the left imaging panel forward based on imagingPanelMovement from UI slider
  void UpdateLeftImagingPanelTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Translate center of right imaging panel to isocenter of fixed reference coordinate system
  void UpdateRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform();
  /// Rotate rights imaging panel based on imagingPanelMovement from UI slider
  void UpdateRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Translate rotated right imaging panel from fixed reference isocenter back to gantry
  void UpdateRightImagingPanelRotatedToGantryTransform();
  /// Translate the right imaging panel forward based on imagingPanelMovement from UI slider
  void UpdateRightImagingPanelTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Initiate imagingPanel transform functions based on imagingPanelMovement from UI slider
  void UpdateImagingPanelMovementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Rotate patient support (couch) with respect to fixed reference coordinate system based on rotation angle from UI slider
  void UpdatePatientSupportToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Translate scaled patient support back to position with respect to the vertically translated table top
  void UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Scale the patient support based on the vertical displacement of the table top from the starting position of 0
  void UpdatePatientSupportScaledByTableTopVerticalMovementTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Translate the patient support positively so that base of the support is located at 0, preventing the base from getting scaled
  void UpdatePatientSupportPositiveVerticalTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode);

  /// Call functions that translates the table top along the x,y, and z axes based on changes to table top displacement UI sliders
  void UpdateTableTopEccentricRotationToPatientSupportTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Translate the table top vertically along the z axes  based on change to Vertical Table Top Displacement UI slider
  void UpdateVerticalDisplacementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode);
 
  void UpdateAdditionalDevicesVisibility(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Check for collisions between pieces of linac model using vtkCollisionDetectionFilter
  /// \return string indicating whether collision occurred
  std::string CheckForCollisions(vtkMRMLRoomsEyeViewNode* parameterNode);

  void LoadAdditionalDevices();

  void UpdateTreatmentOrientationMarker();

  //TODO:
  //bool CalculateNewSourcePosition(vtkMRMLRTBeamNode* beamNode, double oldSourcePosition[3], double newSourcePosition[3]);

protected:
  /// Get patient body closed surface poly data from segmentation node and segment selection in the parameter node
  bool GetPatientBodyPolyData(vtkMRMLRoomsEyeViewNode* parameterNode, vtkPolyData* patientBodyPolyData);

protected:
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

