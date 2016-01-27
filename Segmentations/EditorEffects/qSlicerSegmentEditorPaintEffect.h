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

#ifndef __qSlicerSegmentEditorPaintEffect_h
#define __qSlicerSegmentEditorPaintEffect_h

// Segmentations Widgets includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

#include "qSlicerSegmentEditorLabelEffect.h"

class qSlicerSegmentEditorPaintEffectPrivate;
class vtkPolyData;

/// \ingroup SlicerRt_QtModules_Segmentations
/// \brief TODO:
class Q_SLICER_SEGMENTATIONS_EFFECTS_EXPORT qSlicerSegmentEditorPaintEffect :
  public qSlicerSegmentEditorLabelEffect
{
public:
  Q_OBJECT

public:
  typedef qSlicerSegmentEditorLabelEffect Superclass;
  qSlicerSegmentEditorPaintEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorPaintEffect(); 

public:
  /// Get name of effect
  virtual QString name();

  /// Clone editor effect
  virtual qSlicerSegmentEditorAbstractEffect* clone();

  /// Get icon for effect to be displayed in segment editor
  virtual QIcon icon();

  /// Perform actions to deactivate the effect (such as destroy actors, etc.)
  virtual void deactivate();

  /// Callback function invoked when interaction happens
  /// \param callerInteractor Interactor object that was observed to catch the event
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  virtual void processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLWidget* viewWidget);

  /// Callback function invoked when view node is modified
  /// \param callerViewNode View node that was observed to catch the event. Can be either \sa vtkMRMLSliceNode or \sa vtkMRMLViewNode
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  virtual void processViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, qMRMLWidget* viewWidget);

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  virtual void setupOptionsFrame();

  /// Set default parameters in the parameter MRML node
  virtual void setMRMLDefaults();

  /// Set edited labelmap. Can be overridden to perform additional actions.
  virtual void setEditedLabelmap(vtkOrientedImageData* labelmap);

public slots:
  /// Update user interface from parameter set node
  virtual void updateGUIFromMRML();

  /// Update parameter set node from user interface
  virtual void updateMRMLFromGUI();

public:
  /// Apply paint operation
  void apply();

protected:
  QScopedPointer<qSlicerSegmentEditorPaintEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorPaintEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorPaintEffect);
};

#endif
