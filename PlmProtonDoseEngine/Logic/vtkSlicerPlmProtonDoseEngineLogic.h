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
  and was supported through CANARIE.

==============================================================================*/

// .NAME vtkSlicerPlmProtonDoseEngineLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPlmProtonDoseEngineLogic_h
#define __vtkSlicerPlmProtonDoseEngineLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerPlmProtonDoseEngineModuleLogicExport.h"


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLMPROTONDOSEENGINE_MODULE_LOGIC_EXPORT vtkSlicerPlmProtonDoseEngineLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerPlmProtonDoseEngineLogic *New();
  vtkTypeMacro(vtkSlicerPlmProtonDoseEngineLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerPlmProtonDoseEngineLogic();
  virtual ~vtkSlicerPlmProtonDoseEngineLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
private:

  vtkSlicerPlmProtonDoseEngineLogic(const vtkSlicerPlmProtonDoseEngineLogic&); // Not implemented
  void operator=(const vtkSlicerPlmProtonDoseEngineLogic&); // Not implemented
};

#endif
