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

  The collision detection module was partly supported by Conselleria de
  Educación, Investigación, Cultura y Deporte (Generalitat Valenciana), Spain
  under grant number CDEIGENT/2019/011.

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
#include <vtkSlicerIECTransformLogic.h>
#include <vtkSlicerBeamsModuleLogic.h>

class vtkCollisionDetectionFilter;
class vtkMatrix4x4;
class vtkPolyData;
class vtkVector3d;

class vtkMRMLRoomsEyeViewNode;
class vtkMRMLModelNode;
class vtkSlicerIECTransformLogic;
class vtkSlicerBeamsModuleLogic;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class VTK_SLICER_ROOMSEYEVIEW_LOGIC_EXPORT vtkSlicerRoomsEyeViewModuleLogic : public vtkSlicerModuleLogic
{
public:
  /// Treatment machine part types
  /// \sa GetTreatmentMachinePartTypeAsString()
  enum TreatmentMachinePartType
    {
    Collimator = 0,
    Gantry,
    PatientSupport,
    TableTop,
    Body,
    ImagingPanelLeft,
    ImagingPanelRight,
    FlatPanel,
    ApplicatorHolder,
    ElectronApplicator,
    LastPartType
    }; 

  static const char* ORIENTATION_MARKER_MODEL_NODE_NAME;
  static const char* TREATMENT_MACHINE_DESCRIPTOR_FILE_PATH_ATTRIBUTE_NAME;
  static unsigned int MAX_TRIANGLE_NUMBER_PRODUCT_FOR_COLLISIONS;

public:
  static vtkSlicerRoomsEyeViewModuleLogic *New();
  vtkTypeMacro(vtkSlicerRoomsEyeViewModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Load and setup components of the treatment machine into the scene based on its description.
  /// \param parameterNode Parameter node contains the treatment machine descriptor file path.
  /// \return List of parts that were successfully set up.
  std::vector<TreatmentMachinePartType> LoadTreatmentMachine(vtkMRMLRoomsEyeViewNode* parameterNode);
  /// Set up the IEC transforms and model properties on the treatment machine models.
  /// \param forceEnableCollisionDetection Enable collision detection between parts even if calculation is potentially
  ///        lengthy absed on the number of triangles of the parts.
  /// \return List of parts that were successfully set up.
  std::vector<TreatmentMachinePartType> SetupTreatmentMachineModels(
    vtkMRMLRoomsEyeViewNode* parameterNode, bool forceEnableCollisionDetection=false);
  /// Create or get transforms taking part in the IEC logic and additional devices, and build the transform hierarchy
  void BuildRoomsEyeViewTransformHierarchy();

  /// Update GantryToFixedReference transform based on gantry angle parameter
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
  vtkMRMLModelNode* UpdateTreatmentOrientationMarker(vtkMRMLRoomsEyeViewNode* parameterNode);

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

// Get treatment machine properties from descriptor file
public:
  /// Get part name for part type in the currently loaded treatment machine description
  std::string GetNameForPartType(std::string partType);
  /// Get relative file path for part type in the currently loaded treatment machine description
  std::string GetFilePathForPartType(std::string partType);
  /// Get transform matrix between loaded part file and RAS for part type in the currently loaded treatment machine description
  /// \param fileToPartTransformMatrix Output file to RAS
  /// \return Success flag
  bool GetFileToRASTransformMatrixForPartType(std::string partType, vtkMatrix4x4* fileToPartTransformMatrix);
  /// Get color for part type in the currently loaded treatment machine description
  vtkVector3d GetColorForPartType(std::string partType);
  /// Get state for part type in the currently loaded treatment machine description.
  /// Valid states are "Disabled" (not loaded), "Active" (loaded and collisions computed), "Passive" (loaded but no collisions).
  std::string GetStateForPartType(std::string partType);

// Set/get methods
public:
  /// Get part type as string
  const char* GetTreatmentMachinePartTypeAsString(TreatmentMachinePartType type);

  vtkGetObjectMacro(IECLogic, vtkSlicerIECTransformLogic);

  vtkGetObjectMacro(GantryPatientCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(GantryTableTopCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(GantryPatientSupportCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(CollimatorPatientCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(CollimatorTableTopCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(AdditionalModelsTableTopCollisionDetection, vtkCollisionDetectionFilter);
  vtkGetObjectMacro(AdditionalModelsPatientSupportCollisionDetection, vtkCollisionDetectionFilter);

public:
  /// Get transform from one coordinate frame to another
  /// @param fromFrame - start transformation from frame
  /// @param toFrame - proceed transformation to frame
  /// @param outputTransform - General (linear) transform matrix fromFrame -> toFrame. Matrix is correct if return flag is true.  
  /// @param transformForBeam - calculate dynamic transformation for beam model or other models
  /// (e.g. transformation from Patient RAS frame to Collimation frame: RAS -> Patient -> TableTop -> Eccentric -> Patient Support -> Fixed reference -> Gantry -> Collimator)  //TODO: Deprecated
  /// \return Success flag (false on any error)
  bool GetTransformNodeBetween(vtkSlicerIECTransformLogic::CoordinateSystemIdentifier fromFrame, vtkSlicerIECTransformLogic::CoordinateSystemIdentifier toFrame,
    vtkGeneralTransform* outputTransform, bool transformForBeam = true);

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
  /// @brief Get coordinate system identifiers from frame system up to root system
  /// Root system = FixedReference system, see IEC 61217:2011 hierarchy
  bool GetPathToRoot(vtkSlicerIECTransformLogic::CoordinateSystemIdentifier frame, vtkSlicerIECTransformLogic::CoordinateSystemsList& path);

  /// @brief Get coordinate system identifiers from root system down to frame system
  /// Root system = FixedReference system, see IEC 61217:2011 hierarchy
  bool GetPathFromRoot(vtkSlicerIECTransformLogic::CoordinateSystemIdentifier frame, vtkSlicerIECTransformLogic::CoordinateSystemsList& path);

protected:
  vtkSlicerRoomsEyeViewModuleLogic();
  ~vtkSlicerRoomsEyeViewModuleLogic() override;

  /// Register MRML Node classes to scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  void SetMRMLSceneInternal(vtkMRMLScene * newScene) override;

private:
  vtkSlicerRoomsEyeViewModuleLogic(const vtkSlicerRoomsEyeViewModuleLogic&) = delete;
  void operator=(const vtkSlicerRoomsEyeViewModuleLogic&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;
};

#endif
