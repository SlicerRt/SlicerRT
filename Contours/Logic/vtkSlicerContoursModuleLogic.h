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
  /// Create a default structure set node so that contours can be created from potential representations without having
  /// loaded a DICOM-RT study. This method becomes obsolete when creating new patient hierarchy nodes feature is implemented.
  void CreateDefaultStructureSetNode();

  /// Creates an empty ribbon model as a default representation for the new contours
  void CreateEmptyRibbonModelForContour(vtkMRMLNode* node);

protected:
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();

  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndClose();

protected:
  vtkSlicerContoursModuleLogic();
  virtual ~vtkSlicerContoursModuleLogic();

private:
  vtkSlicerContoursModuleLogic(const vtkSlicerContoursModuleLogic&); // Not implemented
  void operator=(const vtkSlicerContoursModuleLogic&);               // Not implemented
};

#endif
