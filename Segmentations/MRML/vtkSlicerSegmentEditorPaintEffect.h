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
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerSegmentEditorPaintEffect - Logic class for segmentation handling
// .SECTION Description
// TODO

#ifndef __vtkSlicerSegmentEditorPaintEffect_h
#define __vtkSlicerSegmentEditorPaintEffect_h

// Slicer includes
#include "vtkMRMLAbstractLogic.h"

#include "vtkSlicerSegmentationsModuleMRMLExport.h"

class vtkCollection;
class vtkPolyData;

/// \ingroup SlicerRt_QtModules_Segmentations
class VTK_SLICER_SEGMENTATIONS_MODULE_MRML_EXPORT vtkSlicerSegmentEditorPaintEffect :
  public vtkMRMLAbstractLogic
{
public:
  static vtkSlicerSegmentEditorPaintEffect *New();
  vtkTypeMacro(vtkSlicerSegmentEditorPaintEffect,vtkMRMLAbstractLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

//TODO: Create abstract functions in base effect class
  /// Activate paint effect
  void Activate();

  /// Draw paint circle glyph
  void CreateGlyph(vtkPolyData* polyData);
  
  /// Apply paint operation
  void Apply();

  /// TODO
//  vtkCollection* GetInteractionCallbackCommandCollection() { return this->InteractionCallbackCommandCollection; };

protected:
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /// Callback function invoked when interaction happens
//  static void ProcessEvents(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  vtkSlicerSegmentEditorPaintEffect();
  virtual ~vtkSlicerSegmentEditorPaintEffect();

  /// TODO:
  vtkPolyData* Brush;

private:
  vtkSlicerSegmentEditorPaintEffect(const vtkSlicerSegmentEditorPaintEffect&); // Not implemented
  void operator=(const vtkSlicerSegmentEditorPaintEffect&);               // Not implemented
};

#endif
