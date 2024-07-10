/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkSlicerDrrImageComputationLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDrrImageComputationLogic_h
#define __vtkSlicerDrrImageComputationLogic_h

// Slicer includes
#include <vtkSlicerModuleLogic.h>

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDrrImageComputationModuleLogicExport.h"
#include <vtkSlicerRoomsEyeViewModuleLogic.h>

class vtkMRMLVolumeNode;
class vtkMRMLScalarVolumeNode;

class vtkMRMLDrrImageComputationNode;
class vtkMRMLRTBeamNode;
class vtkMRMLMarkupsPlaneNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLMarkupsLineNode;
class vtkMRMLTableNode;

class vtkMRMLLinearTransformNode;

class vtkSlicerBeamsModuleLogic;
class vtkSlicerPlanarImageModuleLogic;
class vtkSlicerCLIModuleLogic;

class vtkMatrix4x4;

/// \ingroup Slicer_QtModules_DrrImageComputation
class VTK_SLICER_DRRIMAGECOMPUTATION_MODULE_LOGIC_EXPORT vtkSlicerDrrImageComputationLogic :
  public vtkSlicerModuleLogic
{
public:
  static const char* IMAGER_BOUNDARY_MARKUPS_NODE_NAME; // plane
  static const char* IMAGE_WINDOW_MARKUPS_NODE_NAME; // plane
  static const char* FIDUCIALS_MARKUPS_NODE_NAME; // fiducial
  static const char* NORMAL_VECTOR_MARKUPS_NODE_NAME; // line
  static const char* VUP_VECTOR_MARKUPS_NODE_NAME; // line
  static const char* RTIMAGE_TRANSFORM_NODE_NAME;

  static vtkSlicerDrrImageComputationLogic *New();
  vtkTypeMacro(vtkSlicerDrrImageComputationLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Update normal and view up vectors of RT Image
  void UpdateNormalAndVupVectors(vtkMRMLDrrImageComputationNode* parameterNode);

  /// Create markups nodes for visualization
  void CreateMarkupsNodes(vtkMRMLDrrImageComputationNode* parameterNode);
  /// Update markups nodes using parameter node data
  void UpdateMarkupsNodes(vtkMRMLDrrImageComputationNode* parameterNode);
  /// Show/hide markups
  void ShowMarkupsNodes(bool toggled = false);

  /// Set Planar Image module logic
  void SetPlanarImageLogic(vtkSlicerPlanarImageModuleLogic* planarImageLogic);
  /// Set Plastimatch DRR CLI module logic
  void SetDRRComputationLogic(vtkSlicerCLIModuleLogic* plastimatchDrrLogic);
  /// Set Beams module logic
  void SetBeamsLogic(vtkSlicerBeamsModuleLogic* beamsLogic);

  /// Compute DRR image
  /// \param parameterNode - parameters of DRR image computation
  /// \param ctInputVolume - CT volume
  /// \return valid computed DRR volume node or nullptr otherwise 
  vtkMRMLScalarVolumeNode* ComputePlastimatchDRR(vtkMRMLDrrImageComputationNode* parameterNode, vtkMRMLScalarVolumeNode* ctInputVolume);

  /// Update Beam node from 3D view camera position
  /// \param parameterNode - parameters of DRR image computation
  /// \return true if beam was updated, false otherwise
  bool UpdateBeamFromCamera(vtkMRMLDrrImageComputationNode* parameterNode);

  /// Get transformation matrix for RT-Image
  /// \param parameterNode - parameters of DRR image computation
  /// \param mat - linear transformation matrix
  /// \return - true if matrix is valid, false otherwise 
  bool GetRtImageTransformMatrix(vtkMRMLDrrImageComputationNode* parameterNode, vtkMatrix4x4* mat);

  /// Get plastimatch intrinsic matrix component
  /// \param parameterNode - parameters of DRR image computation
  /// \param mat - linear transformation matrix
  /// \return - true if matrix is valid, false otherwise 
  bool GetPlastimatchIntrinsicMatrix(vtkMRMLDrrImageComputationNode* parameterNode, vtkMatrix4x4* mat);

  /// Get plastimatch extrinsic matrix component
  /// \param parameterNode - parameters of DRR image computation
  /// \param mat - linear transformation matrix
  /// \return - true if matrix is valid, false otherwise 
  bool GetPlastimatchExtrinsicMatrix(vtkMRMLDrrImageComputationNode* parameterNode, vtkMatrix4x4* mat);

  /// Get plastimatch projection matrix
  /// \param parameterNode - parameters of DRR image computation
  /// \param mat - linear transformation matrix
  /// \return - true if matrix is valid, false otherwise 
  bool GetPlastimatchProjectionMatrix(vtkMRMLDrrImageComputationNode* parameterNode, vtkMatrix4x4* mat);

  /// Get plastimatch projection matrix
  /// \param parameterNode - parameters of DRR image computation
  /// \param mat - linear transformation matrix
  /// \return - true if matrix is valid, false otherwise 
  bool GetPointOffsetFromImagerOrigin(vtkMRMLDrrImageComputationNode* parameterNode, const double pointRAS[3],
    double offsetWidthHeight[2], double offsetColumnRow[2]);

  /// Get intersection point between a ray and imager plane
  /// Origin of the ray (first point of the line) is the x-ray source from a RTBeamNode
  /// Second point of the line is the point itself
  /// \param parameterNode - parameters of DRR image computation
  /// \param point - second ray point coordinates in RAS
  /// \param pointIntersect - intersection coordinates of point ray and imager plane in RAS
  /// \return - true if there is an intersection, false otherwise 
  bool GetRayIntersectWithImagerPlane(vtkMRMLDrrImageComputationNode* parameterNode,
    const double pointRAS[3], double pointIntersectRAS[3]);

  /// Get IJK to RAS transformation matrix for current parameter node
  /// \param parameterNode - parameters of DRR image computation
  /// \param mat - linear transformation matrix
  /// \return - true if matrix is valid, false otherwise
  bool GetRtImageIJKToRASMatrix(vtkMRMLDrrImageComputationNode* parameterNode, vtkMatrix4x4* mat);

  /// Get transformation matrix, IEC Transformation from Gantry -> RAS (without collimator)
  bool GetRtImageTransformMatrixFromBeam(vtkMRMLRTBeamNode* node, vtkMatrix4x4* mat);

  /// Get RT Imager origin position in RAS
  bool GetRtImagerOriginPosition(vtkMRMLDrrImageComputationNode* parameterNode, double originPositionRAS[3]);

  /// Create projections table node
  vtkMRMLTableNode* CreateProjectionsTableNode(vtkMRMLDrrImageComputationNode* parameterNode,
    vtkMRMLScalarVolumeNode* ctInputVolume);

protected:
  vtkSlicerDrrImageComputationLogic();
  ~vtkSlicerDrrImageComputationLogic() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;
  void UpdateFromMRMLScene() override;
  void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;

  /// Handles events registered in the observer manager
  void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData) override;

private:
  vtkSlicerDrrImageComputationLogic(const vtkSlicerDrrImageComputationLogic&) = delete; // Not implemented
  void operator=(const vtkSlicerDrrImageComputationLogic&) = delete; // Not implemented

  /// Create markups for imager normal vector 
  vtkMRMLMarkupsLineNode* CreateImagerNormal(vtkMRMLDrrImageComputationNode* node); // n
  /// Create markups for view up normal vector
  vtkMRMLMarkupsLineNode* CreateImagerVUP(vtkMRMLDrrImageComputationNode* node); // vup
  /// Create markups for imager boundary
  vtkMRMLMarkupsPlaneNode* CreateImagerBoundary(vtkMRMLDrrImageComputationNode* node); // Imager == Reciever == Detector
  /// Create markups for image window within imager
  vtkMRMLMarkupsPlaneNode* CreateImageWindow(vtkMRMLDrrImageComputationNode* node); // subwindow
  /// Create fiducial markups: (0,0), Center, etc
  vtkMRMLMarkupsFiducialNode* CreateFiducials(vtkMRMLDrrImageComputationNode* node);
  /// Setup nodes for calculated DRR image
  /// \param parameterNode - parameters of DRR image computation
  /// \param drrVolumeNode - RTImage DRR volume
  bool SetupDisplayAndSubjectHierarchyNodes( vtkMRMLDrrImageComputationNode* parameterNode, vtkMRMLScalarVolumeNode* drrVolumeNode);
  /// Setup geometry for calculated DRR image
  /// \param parameterNode - parameters of DRR image computation
  /// \param drrVolumeNode - RTImage DRR volume
  bool SetupGeometry( vtkMRMLDrrImageComputationNode* parameterNode, vtkMRMLScalarVolumeNode* drrVolumeNode);
  /// IEC Transformation from Gantry -> RAS (without collimator)
  vtkMRMLLinearTransformNode* UpdateImageTransformFromBeam(vtkMRMLRTBeamNode* node);

  /// Planar Image logic instance
  vtkSlicerPlanarImageModuleLogic* PlanarImageLogic;
  /// Plastimatch DRR computation logic instance
  vtkSlicerCLIModuleLogic* PlastimatchDRRComputationLogic;
  /// Beams logic instance
  vtkSlicerBeamsModuleLogic* BeamsLogic;
  /// Rooms eye view logic instance
  vtkSlicerRoomsEyeViewModuleLogic* REVLogic;
};

#endif
