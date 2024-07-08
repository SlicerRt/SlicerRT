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

// .NAME vtkSlicerBeamsModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerBeamsModuleLogic_h
#define __vtkSlicerBeamsModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include <vtkSlicerIECTransformLogic.h>

// Beams includes
#include "vtkSlicerBeamsModuleLogicExport.h"
#include "vtkMRMLRTBeamNode.h"

// VTK includes
#include <vtkNew.h>

class vtkSlicerMLCPositionLogic;

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerBeamsModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerBeamsModuleLogic *New();
  vtkTypeMacro(vtkSlicerBeamsModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Get transform node between two coordinate systems is exists
  /// \return Transform node if there is a direct transform between the specified coordinate frames, nullptr otherwise
  ///   Note: If IEC does not specify a transform between the given coordinate frames, then there will be no node with the returned name.
  vtkMRMLLinearTransformNode* GetTransformNodeBetween(
    vtkSlicerIECTransformLogic::CoordinateSystemIdentifier fromFrame, vtkSlicerIECTransformLogic::CoordinateSystemIdentifier toFrame);

  /// Update parent transform of a given beam using its parameters and the IEC logic
  void UpdateTransformForBeam(vtkMRMLRTBeamNode* beamNode);

  /// Update parent transform of a given beam using its parameters and the IEC logic
  /// without using plan node (only isocenter position)
  /// @param beamSequenceScene - inner scene of the beam sequence node
  /// @param beamNode - a current beam node (must be added to the beam sequence node beforehand)
  /// @param beamTransformNode - parent transform of the beam according to the beam parameters and isocenter
  /// @param isocenter - isocenter position
  /// \warning This method is used only in vtkSlicerDicomRtImportExportModuleLogic::vtkInternal::LoadDynamicBeamSequence
  void UpdateTransformForBeam(vtkMRMLScene* beamSequenceScene, vtkMRMLRTBeamNode* beamNode, 
    vtkMRMLLinearTransformNode* beamTransformNode, double isocenter[3]);

public:
  /// Update parent transform node of a given beam from the IEC transform hierarchy and the beam parameters
  void UpdateBeamTransform(vtkMRMLRTBeamNode* beamNode);
  /// Update parent transform node of a given beam from the IEC transform hierarchy and the beam parameters
  /// \warning This method is used only in vtkSlicerBeamsModuleLogic::UpdateTransformForBeam
  void UpdateBeamTransform(vtkMRMLRTBeamNode* beamNode, vtkMRMLLinearTransformNode* beamTransformNode, double* isocenter = nullptr);

  /// Update IEC transforms according to beam node. Function kept separate from \sa UpdateBeamTransform for usage in the DRR module
  void UpdateIECTransformsFromBeam(vtkMRMLRTBeamNode* beamNode, double* isocenter=nullptr);

public:
  /// Update FixedReference to RAS and RAS to Patient transforms based on isocenter and patient support transforms.
  /// \param planNode: Plan node to get the isocenter position from
  /// \param isocenter: Option to set any isocenter for dynamic beams
  void UpdateRASRelatedTransforms(vtkMRMLRTPlanNode* planNode=nullptr, double* isocenter=nullptr);

public:
  vtkGetObjectMacro(MLCPositionLogic, vtkSlicerMLCPositionLogic);

  /// Possibility to use an external IEC logic. This is useful for testing.
  void SetIECLogic(vtkSlicerIECTransformLogic* iecLogic);

protected:
  vtkSlicerBeamsModuleLogic();
  ~vtkSlicerBeamsModuleLogic() override;

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  void OnMRMLSceneEndImport() override;

  /// Handles events registered in the observer manager
  void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData) override;

private:
  vtkSlicerBeamsModuleLogic(const vtkSlicerBeamsModuleLogic&) = delete;
  void operator=(const vtkSlicerBeamsModuleLogic&) = delete;
  
  vtkSlicerMLCPositionLogic* MLCPositionLogic;

private:
  vtkSlicerIECTransformLogic* IECLogic;
};

#endif
