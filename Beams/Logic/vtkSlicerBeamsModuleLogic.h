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

// Beams includes
#include "vtkSlicerBeamsModuleLogicExport.h"
#include "vtkMRMLRTBeamNode.h"

/// \ingroup SlicerRt_QtModules_Beams
class VTK_SLICER_BEAMS_LOGIC_EXPORT vtkSlicerBeamsModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerBeamsModuleLogic *New();
  vtkTypeMacro(vtkSlicerBeamsModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Update the beam model for a new isocenter, gantry angle, etc.
  void UpdateBeamTransform(vtkMRMLRTBeamNode* beamNode);

  /// Update beam model based on current properties
  void UpdateBeamGeometry(vtkMRMLRTBeamNode* beamNode);

protected:
  vtkSlicerBeamsModuleLogic();
  virtual ~vtkSlicerBeamsModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();

  /// Handles events registered in the observer manager
  virtual void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData);

private:
  vtkSlicerBeamsModuleLogic(const vtkSlicerBeamsModuleLogic&); // Not implemented
  void operator=(const vtkSlicerBeamsModuleLogic&);            // Not implemented
};

#endif

