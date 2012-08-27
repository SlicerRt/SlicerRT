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

// .NAME vtkSlicerProtonDoseModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerProtonDoseModuleLogic_h
#define __vtkSlicerProtonDoseModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerProtonDoseModuleLogicExport.h"

class vtkMRMLVolumeNode;
class vtkMRMLProtonDoseNode;

/// \ingroup Slicer_QtModules_ProtonDose
class VTK_SLICER_PROTONDOSE_LOGIC_EXPORT vtkSlicerProtonDoseModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerProtonDoseModuleLogic *New();
  vtkTypeMacro(vtkSlicerProtonDoseModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Accumulates dose volumes with the given IDs and corresponding weights
  int ComputeProtonDose();

  void SetAndObserveProtonDoseNode(vtkMRMLProtonDoseNode* node);
  vtkGetObjectMacro(ProtonDoseNode, vtkMRMLProtonDoseNode);

protected:
  vtkSlicerProtonDoseModuleLogic();
  virtual ~vtkSlicerProtonDoseModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

private:

  vtkSlicerProtonDoseModuleLogic(const vtkSlicerProtonDoseModuleLogic&); // Not implemented
  void operator=(const vtkSlicerProtonDoseModuleLogic&);               // Not implemented

protected:
  /// Parameter set MRML node
  vtkMRMLProtonDoseNode* ProtonDoseNode;
};

#endif

