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

#include "vtkCollisionDetectionFilter.h"

class vtkMRMLRoomsEyeViewNode;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkSlicerRoomsEyeViewModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Load pre-defined components of the treatment machine into the scene
  void LoadLinacModels();
  /// Set up the IEC transform hierarchy
  void InitializeIEC();
  
  /// Update CollimatorToGantry transform based on collimator angle from UI slider
  void UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Update GantryToFixedReference transform based on gantry angle from UI slider
  void GantryRotationValueChanged(double gantryAngle);
  
  /// Translate center of left imaging panel to isocenter of fixed reference coordinate system
  void LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter(double imagingPanelMovement);
  /// Rotate left imaging panel based on imagingPanelMovement from UI slider
  void LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated(double imagingPanelMovement);
  /// Translate rotated left imaging panel from fixed reference isocenter back to gantry
  void LeftImagingPanelRotatedToGantry(double imagingPanelMovement);
  /// Translate the left imaging panel forward based on imagingPanelMovement from UI slider
  void LeftImagingPanelTranslation(double imagingPanelMovement);

  /// Translate center of right imaging panel to isocenter of fixed reference coordinate system
  void RightImagingPanelToRightImagingPanelFixedReferenceIsocenter(double imagingPanelMovement);
  /// Rotate rights imaging panel based on imagingPanelMovement from UI slider
  void RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated(double imagingPanelMovement);
  /// Translate rotated right imaging panel from fixed reference isocenter back to gantry
  void RightImagingPanelRotatedToGantry(double imagingPanelMovement);
  /// Translate the right imaging panel forward based on imagingPanelMovement from UI slider
  void RightImagingPanelTranslation(double imagingPanelMovement);

  /// Initiate imagingPanel transform functions based on imagingPanelMovement from UI slider
  void ImagingPanelMovementValueChanged(double imagingPanelMovement);

  /// Rotate patient support (couch) with respect to fixed reference coordinate system based on rotation angle from UI slider
  void PatientSupportRotationValueChanged(double rotationAngle);

  /// Translate scaled patient support back to position with respect to the vertically translated table top
  void PatientSupportScaledTranslatedToTableTopVerticalTranslation(double tableTopVerticalDisplacement);
  /// Scale the patient support based on the vertical displacement of the table top from the starting position of 0
  void TableTopDisplacementPatientSupportChanged(double tableTopVerticalDisplacement);
  /// Translate the patient support positively so that base of the support is located at 0, preventing the base from getting scaled
  void PatientSupportPositiveVerticalTranslation(double tableTopVerticalDisplacement);
  
  /// Call functions that translates the table top along the x,y, and z axes based on changes to table top displacement UI sliders
  void TableTopDisplacementValueChanged(double latTableTopDisplacement , double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Translate the table top vertically along the z axes  based on change to Vertical Table Top Displacement UI slider
  void VerticalDisplacementValueChanged(double latTableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Translate the table top laterally along the x axes  based on change to Lateral Table Top Displacement UI slider
  void LateralDisplacementValueChanged(double latTableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement);
  /// Translate the table top laterally along the y axes  based on change to Longitudinal Table Top Displacement UI slider
  void LongitudinalDisplacementValueChanged(double latTableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement);

  /// Check for collisions between pieces of linac model using vtkCollisionDetectionFilter
  /// \return string indicating whether collision occurred
  std::string CheckForCollisions(vtkMRMLRoomsEyeViewNode* parameterNode);

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

protected:
  vtkSlicerRoomsEyeViewModuleLogic();
  virtual ~vtkSlicerRoomsEyeViewModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

private:
  vtkSlicerRoomsEyeViewModuleLogic(const vtkSlicerRoomsEyeViewModuleLogic&); // Not implemented
  void operator=(const vtkSlicerRoomsEyeViewModuleLogic&);            // Not implemented
};

#endif

