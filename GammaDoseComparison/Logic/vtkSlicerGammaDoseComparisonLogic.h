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

// .NAME vtkSlicerGammaDoseComparisonLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerGammaDoseComparisonLogic_h
#define __vtkSlicerGammaDoseComparisonLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerGammaDoseComparisonModuleLogicExport.h"


/// \ingroup Slicer_QtModules_GammaDoseComparison
class VTK_SLICER_GAMMADOSECOMPARISON_MODULE_LOGIC_EXPORT vtkSlicerGammaDoseComparisonLogic :
  public vtkSlicerModuleLogic
{
public:
  
  static vtkSlicerGammaDoseComparisonLogic *New();
  vtkTypeMacro(vtkSlicerGammaDoseComparisonLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerGammaDoseComparisonLogic();
  virtual ~vtkSlicerGammaDoseComparisonLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
private:

  vtkSlicerGammaDoseComparisonLogic(const vtkSlicerGammaDoseComparisonLogic&); // Not implemented
  void operator=(const vtkSlicerGammaDoseComparisonLogic&);               // Not implemented
};

#endif

