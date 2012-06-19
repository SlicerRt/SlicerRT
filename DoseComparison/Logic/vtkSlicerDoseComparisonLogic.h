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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerDoseComparisonLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDoseComparisonLogic_h
#define __vtkSlicerDoseComparisonLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseComparisonModuleLogicExport.h"


/// \ingroup Slicer_QtModules_DoseComparison
class VTK_SLICER_DOSECOMPARISON_MODULE_LOGIC_EXPORT vtkSlicerDoseComparisonLogic :
  public vtkSlicerModuleLogic
{
public:
  
  static vtkSlicerDoseComparisonLogic *New();
  vtkTypeMacro(vtkSlicerDoseComparisonLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerDoseComparisonLogic();
  virtual ~vtkSlicerDoseComparisonLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
private:

  vtkSlicerDoseComparisonLogic(const vtkSlicerDoseComparisonLogic&); // Not implemented
  void operator=(const vtkSlicerDoseComparisonLogic&);               // Not implemented
};

#endif

