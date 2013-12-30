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

// .NAME vtkSlicerRTHierarchyModuleLogic
// .SECTION Description TODO

#ifndef __vtkSlicerRTHierarchyModuleLogic_h
#define __vtkSlicerRTHierarchyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// RTHierarchy Module Logic
#include "vtkSlicerRTHierarchyModuleLogicExport.h"

/// \ingroup SlicerRt_QtModules_RtHierarchy
/// \brief Class to wrap Plastimatch registration capability into the embedded Python shell in Slicer
class VTK_SLICER_RTHIERARCHY_MODULE_LOGIC_EXPORT vtkSlicerRTHierarchyModuleLogic : public vtkSlicerModuleLogic
{
public:
  /// Constructor
  static vtkSlicerRTHierarchyModuleLogic* New();
  vtkTypeMacro(vtkSlicerRTHierarchyModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerRTHierarchyModuleLogic();
  virtual ~vtkSlicerRTHierarchyModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();

protected:
  
private:
  vtkSlicerRTHierarchyModuleLogic(const vtkSlicerRTHierarchyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerRTHierarchyModuleLogic&);            // Not implemented
};

#endif
