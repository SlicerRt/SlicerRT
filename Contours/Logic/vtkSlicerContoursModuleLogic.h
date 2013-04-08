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

// .NAME vtkSlicerContoursModuleLogic - Logic class for contour handling
// .SECTION Description
// This class manages the logic associated with converting and handling
// contour node objects.


#ifndef __vtkSlicerContoursModuleLogic_h
#define __vtkSlicerContoursModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerContoursModuleLogicExport.h"

/// \ingroup Slicer_QtModules_Contours
class VTK_SLICER_CONTOURS_LOGIC_EXPORT vtkSlicerContoursModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerContoursModuleLogic *New();
  vtkTypeMacro(vtkSlicerContoursModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();

protected:
  vtkSlicerContoursModuleLogic();
  virtual ~vtkSlicerContoursModuleLogic();

private:
  vtkSlicerContoursModuleLogic(const vtkSlicerContoursModuleLogic&); // Not implemented
  void operator=(const vtkSlicerContoursModuleLogic&);               // Not implemented
};

#endif
