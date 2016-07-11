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
#include "vtkSlicerModuleLogic.h"

#include <vtkCollisionDetectionFilter.h>

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkSlicerRoomsEyeViewModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Load separate pieces of the linac model into the vtkMRMLScene (takes model file name as parameter)
  void LoadLinacModels(vtkMRMLScene* scene, const char* modelFileName );
  /// Set up the IEC transform hierarchy and the collision detection filters
  void ModelToParentTransforms(vtkMRMLScene* scene);
  
  /// Update CollimatorToGantry transform based on collimator angle from UI slider
  void CollimatorRotationValueChanged(vtkMRMLScene* scene, double collimatorAngle);
  /// Update GantryToFixedReference transform based on gantry angle from UI slider
  void GantryRotationValueChanged(vtkMRMLScene* scene, double gantryAngle);
  
  /// Translate center of left imaging panel to isocenter of fixed reference coordinate system
  void LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter(vtkMRMLScene* scene, double imagingPanelMovement);
  /// Rotate left imaging panel based on imagingPanelMovement from UI slider
  void LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated(vtkMRMLScene* scene, double imagingPanelMovement);
  /// Translate rotated left imaging panel from fixed reference isocenter back to gantry
  void LeftImagingPanelRotatedToGantry(vtkMRMLScene* scene, double imagingPanelMovement);
  /// Translate the left imaging panel forward based on imagingPanelMovement from UI slider
  void LeftImagingPanelTranslation(vtkMRMLScene* scene, double imagingPanelMovement);

  /// Translate center of right imaging panel to isocenter of fixed reference coordinate system
  void RightImagingPanelToRightImagingPanelFixedReferenceIsocenter(vtkMRMLScene* scene, double imagingPanelMovement);
  /// Rotate rights imaging panel based on imagingPanelMovement from UI slider
  void RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated(vtkMRMLScene* scene, double imagingPanelMovement);
  /// Translate rotated right imaging panel from fixed reference isocenter back to gantry
  void RightImagingPanelRotatedToGantry(vtkMRMLScene* scene, double imagingPanelMovement);
  /// Translate the right imaging panel forward based on imagingPanelMovement from UI slider
  void RightImagingPanelTranslation(vtkMRMLScene* scene, double imagingPanelMovement);

  /// Initiate imagingPanel transform functions based on imagingPanelMovement from UI slider
  void ImagingPanelMovementValueChanged(vtkMRMLScene* scene, double imagingPanelMovement);

  /// Rotate patient support (couch) with respect to fixed reference coordinate system based on rotation angle from UI slider
  void PatientSupportRotationValueChanged(vtkMRMLScene* scene, double rotationAngle);

  /// Translate scaled patient support back to position with respect to the vertically translated table top
  void PatientSupportScaledTranslatedToTableTopVerticalTranslation(vtkMRMLScene* scene, double tableTopVerticalDisplacement);
  /// Scale the patient support based on the vertical displacement of the table top from the starting position of 0
  void TableTopDisplacementPatientSupportChanged(vtkMRMLScene* scene, double tableTopVerticalDisplacement);
  /// Translate the patient support positively so that base of the support is located at 0, preventing the base from getting scaled
  void PatientSupportPositiveVerticalTranslation(vtkMRMLScene* scene, double tableTopVerticalDisplacement);
  
  /// Call functions that translates the table top along the x,y, and z axes based on changes to table top displacement UI sliders
  void TableTopDisplacementValueChanged(vtkMRMLScene* scene, double latTableTopDisplacement , double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Translate the table top vertically along the z axes  based on change to Vertical Table Top Displacement UI slider
  void VerticalDisplacementValueChanged(vtkMRMLScene*, double latTableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Translate the table top laterally along the x axes  based on change to Lateral Table Top Displacement UI slider
  void LateralDisplacementValueChanged(vtkMRMLScene*, double latTableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Translate the table top laterally along the y axes  based on change to Longitudinal Table Top Displacement UI slider
  void LongitudinalDisplacementValueChanged(vtkMRMLScene*, double latTableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Check for collisions between pieces of linac model using vtkCollisionDetectionFilter
  /// \return string indicating whether collision occurred
  std::string CheckForCollisions();

protected:
  vtkMatrix4x4* CollimatorToWorldTransformMatrix;
  vtkMatrix4x4* TableTopToWorldTransformMatrix;

  vtkCollisionDetectionFilter* GantryPatientCollisionDetection;
  vtkCollisionDetectionFilter* GantryTableTopCollisionDetection;
  vtkCollisionDetectionFilter* GantryPatientSupportCollisionDetection;

  vtkCollisionDetectionFilter* CollimatorPatientCollisionDetection;
  vtkCollisionDetectionFilter* CollimatorTableTopCollisionDetection;

protected:
  vtkSlicerRoomsEyeViewModuleLogic();
  virtual ~vtkSlicerRoomsEyeViewModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

private:
  vtkSlicerRoomsEyeViewModuleLogic(const vtkSlicerRoomsEyeViewModuleLogic&); // Not implemented
  void operator=(const vtkSlicerRoomsEyeViewModuleLogic&);            // Not implemented
};

#endif

