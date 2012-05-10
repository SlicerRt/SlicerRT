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
#include <set>

#include "vtkSlicerDoseAccumulationModuleLogicExport.h"

class vtkMRMLVolumeNode;

/// \ingroup Slicer_QtModules_DoseAccumulation
class VTK_SLICER_DOSEACCUMULATION_MODULE_LOGIC_EXPORT vtkSlicerDoseAccumulationLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDoseAccumulationLogic *New();
  vtkTypeMacro(vtkSlicerDoseAccumulationLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Collect and return dose volume nodes
  /// \param doseVolumesOnly If true, then collect only dose volume nodes, all volume nodes otherwise
  vtkCollection* GetVolumeNodes(bool doseVolumesOnly);

  /// Accumulates dose volumes with the given IDs and corresponding weights
  void AccumulateDoseVolumes(std::vector< std::pair<std::string,double> > volumeIdsAndWeights);

public:
  void SetAccumulatedDoseVolumeNode( vtkMRMLVolumeNode* );
  vtkGetObjectMacro( AccumulatedDoseVolumeNode, vtkMRMLVolumeNode );

  vtkSetMacro( SceneChanged, bool );
  vtkGetMacro( SceneChanged, bool );
  vtkBooleanMacro( SceneChanged, bool );

protected:
  vtkSlicerDoseAccumulationLogic();
  virtual ~vtkSlicerDoseAccumulationLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

private:
  vtkSlicerDoseAccumulationLogic(const vtkSlicerDoseAccumulationLogic&); // Not implemented
  void operator=(const vtkSlicerDoseAccumulationLogic&);               // Not implemented

protected:
  /// Selected accumulated dose volume MRML node object
  vtkMRMLVolumeNode* AccumulatedDoseVolumeNode;

  /// Flag indicating if the scene has recently changed (update of the module GUI needed)
  bool SceneChanged;
};

#endif

