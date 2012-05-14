/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// .NAME vtkSlicerDoseAccumulationLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseAccumulationLogic_h
#define __vtkSlicerDoseAccumulationLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseAccumulationModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLDoseAccumulationNode;

/// \ingroup Slicer_QtModules_DoseAccumulation
class VTK_SLICER_DOSEACCUMULATION_MODULE_LOGIC_EXPORT vtkSlicerDoseAccumulationLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDoseAccumulationLogic *New();
  vtkTypeMacro(vtkSlicerDoseAccumulationLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Collect and return volume nodes (if in DoseAccumulationNode ShowDoseVolumesOnly is set to true, then only return dose volumes)
  vtkCollection* GetVolumeNodesFromScene();

  /// Accumulates dose volumes with the given IDs and corresponding weights
  int AccumulateDoseVolumes();

public:
  void SetAndObserveDoseAccumulationNode(vtkMRMLDoseAccumulationNode* node);
  vtkGetObjectMacro(DoseAccumulationNode, vtkMRMLDoseAccumulationNode);

protected:
  vtkSlicerDoseAccumulationLogic();
  virtual ~vtkSlicerDoseAccumulationLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerDoseAccumulationLogic(const vtkSlicerDoseAccumulationLogic&); // Not implemented
  void operator=(const vtkSlicerDoseAccumulationLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLDoseAccumulationNode* DoseAccumulationNode;
};

#endif

