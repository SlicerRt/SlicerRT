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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerContourMorphologyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerContourMorphologyModuleLogic_h
#define __vtkSlicerContourMorphologyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerContourMorphologyModuleLogicExport.h"

class vtkMRMLContourMorphologyNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_CONTOURMORPHOLOGY_MODULE_LOGIC_EXPORT vtkSlicerContourMorphologyModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerContourMorphologyModuleLogic *New();
  vtkTypeMacro(vtkSlicerContourMorphologyModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  /// TODO
  void SetAndObserveContourMorphologyNode(vtkMRMLContourMorphologyNode* node);

  /// TODO
  vtkGetObjectMacro(ContourMorphologyNode, vtkMRMLContourMorphologyNode);

  /// TODO
  int SetContourARepresentationToLabelmap();

  /// TODO
  int SetContourBRepresentationToLabelmap();

  /// TODO
  int MorphContour();

protected:
  vtkSlicerContourMorphologyModuleLogic();
  virtual ~vtkSlicerContourMorphologyModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void OnMRMLSceneEndImport();
  virtual void OnMRMLSceneEndClose();

protected:
  /// Parameter set MRML node
  vtkMRMLContourMorphologyNode* ContourMorphologyNode;

private:
  vtkSlicerContourMorphologyModuleLogic(const vtkSlicerContourMorphologyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerContourMorphologyModuleLogic&);               // Not implemented
};

#endif
