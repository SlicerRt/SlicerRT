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

// .NAME qSlicerSegmentEditorAbstractEffect - Logic class for segmentation handling
// .SECTION Description
// TODO

#ifndef __qSlicerSegmentEditorAbstractEffect_h
#define __qSlicerSegmentEditorAbstractEffect_h

// Segmentations Widgets includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class vtkRenderWindowInteractor;
class vtkMRMLSliceNode;

/// \ingroup SlicerRt_QtModules_Segmentations
class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qSlicerSegmentEditorAbstractEffect : public QObject
{
public:
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef QObject Superclass;
  qSlicerSegmentEditorAbstractEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorAbstractEffect(); 

public:  
  /// Activate paint effect
  virtual void activate() = 0;

protected:
  // virtual void setMRMLScene(vtkMRMLScene* newScene);

  /// Callback function invoked when interaction happens
  //TODO: Support events form 3D view with extra parameter
  virtual void processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, vtkMRMLSliceNode* sliceNode/*, 3D view*/) { };

private:
  qSlicerSegmentEditorAbstractEffect(const qSlicerSegmentEditorAbstractEffect&); // Not implemented
  void operator=(const qSlicerSegmentEditorAbstractEffect&); // Not implemented 
};

#endif
