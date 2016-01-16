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

// .NAME qSlicerSegmentEditorPaintEffect - Logic class for segmentation handling
// .SECTION Description
// TODO

#ifndef __qSlicerSegmentEditorPaintEffect_h
#define __qSlicerSegmentEditorPaintEffect_h

// Segmentations Widgets includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

#include "qSlicerSegmentEditorAbstractEffect.h"

class qSlicerSegmentEditorPaintEffectPrivate;
class vtkPolyData;

/// \ingroup SlicerRt_QtModules_Segmentations
class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qSlicerSegmentEditorPaintEffect :
  public qSlicerSegmentEditorAbstractEffect
{
public:
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerSegmentEditorAbstractEffect Superclass;
  qSlicerSegmentEditorPaintEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorPaintEffect(); 

public:  
//TODO: Create abstract functions in base effect class
  /// Activate paint effect
  virtual void activate();

  /// Draw paint circle glyph
  void createGlyph(vtkPolyData* polyData);
  
  /// Apply paint operation
  void apply();

protected:
  /// Callback function invoked when interaction happens
//  static void ProcessEvents(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  QScopedPointer<qSlicerSegmentEditorPaintEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorPaintEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorPaintEffect);
};

#endif
