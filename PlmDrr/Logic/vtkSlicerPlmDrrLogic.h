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

// .NAME vtkSlicerPlmDrrLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPlmDrrLogic_h
#define __vtkSlicerPlmDrrLogic_h

// Slicer includes
#include <vtkSlicerModuleLogic.h>

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerPlmDrrModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLScalarVolumeNode;

class vtkMRMLPlmDrrNode;
class vtkMRMLRTBeamNode;
class vtkMRMLMarkupsClosedCurveNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLMarkupsLineNode;
class vtkSlicerPlanarImageModuleLogic;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLMDRR_MODULE_LOGIC_EXPORT vtkSlicerPlmDrrLogic :
  public vtkSlicerModuleLogic
{
public:
  static const char* IMAGER_BOUNDARY_MARKUPS_NODE_NAME; // curve
  static const char* IMAGE_WINDOW_MARKUPS_NODE_NAME; // curve
  static const char* FIDUCIALS_MARKUPS_NODE_NAME; // fiducial
  static const char* NORMAL_VECTOR_MARKUPS_NODE_NAME; // line
  static const char* VUP_VECTOR_MARKUPS_NODE_NAME; // line

  static vtkSlicerPlmDrrLogic *New();
  vtkTypeMacro(vtkSlicerPlmDrrLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create markups nodes for visualization
  void CreateMarkupsNodes(vtkMRMLPlmDrrNode* parameterNode);

  /// Update markups nodes using parameter node data
  void UpdateMarkupsNodes(vtkMRMLPlmDrrNode* parameterNode);
  void ShowMarkupsNodes( vtkMRMLPlmDrrNode* parameterNode, bool toggled);

  bool LoadRawImageFromFile( vtkMRMLPlmDrrNode* paramNode, vtkMRMLScalarVolumeNode* drrVolumeNode, const std::string& filename);
  bool LoadRtImage( vtkMRMLPlmDrrNode* paramNode, const std::string& filename);
  bool SetupRtImageGeometry( vtkMRMLPlmDrrNode* paramNode, vtkMRMLScalarVolumeNode* drrVolumeNode, vtkIdType shNodeID);

  /// Set Planar Image module logic
  void SetPlanarImageLogic(vtkSlicerPlanarImageModuleLogic* planarImageLogic);

protected:
  vtkSlicerPlmDrrLogic();
  virtual ~vtkSlicerPlmDrrLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Handles events registered in the observer manager
  void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData) override;

private:

  vtkSlicerPlmDrrLogic(const vtkSlicerPlmDrrLogic&); // Not implemented
  void operator=(const vtkSlicerPlmDrrLogic&); // Not implemented

  vtkMRMLMarkupsLineNode* CreateImagerNormal(vtkMRMLPlmDrrNode* node); // n
  vtkMRMLMarkupsLineNode* CreateImagerVUP(vtkMRMLPlmDrrNode* node); // vup
  vtkMRMLMarkupsClosedCurveNode* CreateImagerBoundary(vtkMRMLPlmDrrNode* node);
  vtkMRMLMarkupsClosedCurveNode* CreateImageWindow(vtkMRMLPlmDrrNode* node);
  vtkMRMLMarkupsFiducialNode* CreateFiducials(vtkMRMLPlmDrrNode* node);

  /// Planar Image logic instance
  vtkSlicerPlanarImageModuleLogic* PlanarImageLogic;
};

#endif
