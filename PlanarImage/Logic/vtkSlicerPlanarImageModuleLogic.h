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

// .NAME vtkSlicerPlanarImageModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPlanarImageModuleLogic_h
#define __vtkSlicerPlanarImageModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerPlanarImageModuleLogicExport.h"

// STD includes
#include <map>

class vtkMRMLPlanarImageNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkImageMapToWindowLevelColors;
class vtkPoints;

/// \ingroup SlicerRt_QtModules_PlanarImage
class VTK_SLICER_PLANARIMAGE_LOGIC_EXPORT vtkSlicerPlanarImageModuleLogic : public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPlanarImageModuleLogic *New();
  vtkTypeMacro(vtkSlicerPlanarImageModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

public:
  /// Show planar image as a rectangular model with the input volume as the texture
  void CreateModelForPlanarImage(vtkMRMLPlanarImageNode* planarImageNode);

protected:
  /// Compute image plane corners in the world coordinate system based on the transforms related to the planar image
  void ComputeImagePlaneCorners(vtkMRMLScalarVolumeNode* planarImageVolume, vtkPoints* sliceCornerPoints);

  /// Create texture pipeline for the planar image so that model node texture is updated
  /// according to the current display state of the volume
  void SetTextureForPlanarImage(vtkMRMLScalarVolumeNode* planarImageVolumeNode, vtkMRMLModelNode* displayedModelNode);

protected:
  vtkSlicerPlanarImageModuleLogic();
  ~vtkSlicerPlanarImageModuleLogic() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  void RegisterNodes() override;

  /// Update texture and if display properties change in an observed planar image volume node,
  /// and geometry if transform is changed
  void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData) override;
  /// Removes texture pipeline and displayed model if planar image volume node is about to be removed,
  /// and creates texture pipeline after scene is imported
  void ProcessMRMLSceneEvents(vtkObject* caller, unsigned long event, void* callData) override;

protected:
  /// Mappers that apply window/level from the volume nodes to the texture images
  std::map<vtkMRMLScalarVolumeNode*, vtkImageMapToWindowLevelColors*> TextureWindowLevelMappers;

private:
  vtkSlicerPlanarImageModuleLogic(const vtkSlicerPlanarImageModuleLogic&); // Not implemented
  void operator=(const vtkSlicerPlanarImageModuleLogic&);               // Not implemented
};

#endif
