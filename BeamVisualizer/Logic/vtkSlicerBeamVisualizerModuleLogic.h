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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerBeamVisualizerModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerBeamVisualizerModuleLogic_h
#define __vtkSlicerBeamVisualizerModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerBeamVisualizerModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLBeamVisualizerNode;

/// \ingroup Slicer_QtModules_BeamVisualizer
class VTK_SLICER_BEAMVISUALIZER_LOGIC_EXPORT vtkSlicerBeamVisualizerModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerBeamVisualizerModuleLogic *New();
  vtkTypeMacro(vtkSlicerBeamVisualizerModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Collect and return volume nodes (if in BeamVisualizerNode ShowDoseVolumesOnly is set to true, then only return dose volumes)
  vtkCollection* GetVolumeNodesFromScene();

  /// 
  bool ReferenceDoseVolumeContainsDose();

  /// Accumulates dose volumes with the given IDs and corresponding weights
  void AccumulateDoseVolumes(std::string &errorMessage);

public:
  /// Set and observe dose accumulation parameter node 
  void SetAndObserveBeamVisualizerNode(vtkMRMLBeamVisualizerNode* node);

  /// Get dose accumulation parameter node 
  vtkGetObjectMacro(BeamVisualizerNode, vtkMRMLBeamVisualizerNode);

protected:
  vtkSlicerBeamVisualizerModuleLogic();
  virtual ~vtkSlicerBeamVisualizerModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:
  vtkSlicerBeamVisualizerModuleLogic(const vtkSlicerBeamVisualizerModuleLogic&); // Not implemented
  void operator=(const vtkSlicerBeamVisualizerModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLBeamVisualizerNode* BeamVisualizerNode;
};

#endif

