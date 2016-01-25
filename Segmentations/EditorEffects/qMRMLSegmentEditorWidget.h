/*==============================================================================

  Program: 3D Slicer

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

#ifndef __qMRMLSegmentEditorWidget_h
#define __qMRMLSegmentEditorWidget_h

// Segmentations Editor Effects includes
#include "qSlicerSegmentationsEditorEffectsExport.h"

// MRMLWidgets includes
#include "qMRMLWidget.h"

// CTK includes
#include <ctkPimpl.h>

// STD includes
#include <cstdlib>

class vtkMRMLNode;
class vtkObject;
class qMRMLSegmentEditorWidgetPrivate;
class QItemSelection;
class QAbstractButton;
class qSlicerSegmentEditorAbstractEffect;

/// \brief Qt widget for editing a segment from a segmentation using Editor effects.
/// \ingroup SlicerRt_QtModules_Segmentations_Widgets
/// 
/// Widget for editing segmentations that can be re-used in any module.
/// 
/// IMPORTANT: The embedding module is responsible to call \sa setupSliceObservations
///   in e.g. the enter function to set up observations enabling interaction event
///   processing, and to call \sa removeSliceObservations in e.g. the exit function to
///   clear the observations to prevent unnecessary function calls.
/// 
class Q_SLICER_SEGMENTATIONS_EFFECTS_EXPORT qMRMLSegmentEditorWidget : public qMRMLWidget
{
  Q_OBJECT

public:
  Q_PROPERTY(qSlicerSegmentEditorAbstractEffect* activeEffect READ activeEffect WRITE setActiveEffect)

public:
  typedef qMRMLWidget Superclass;
  /// Constructor
  explicit qMRMLSegmentEditorWidget(QWidget* parent = 0);
  /// Destructor
  virtual ~qMRMLSegmentEditorWidget();

  /// Get currently selected segmentation MRML node
  Q_INVOKABLE vtkMRMLNode* segmentationNode();
  /// Get ID of currently selected segmentation node
  Q_INVOKABLE QString segmentationNodeID();

  /// Get segment ID of selected segment
  Q_INVOKABLE QString currentSegmentID();

  /// Return active effect if selected, NULL otherwise
  /// \sa m_ActiveEffect, setActiveEffect()
  qSlicerSegmentEditorAbstractEffect* activeEffect()const;
  /// Set active effect
  /// \sa m_ActiveEffect, activeEffect()
  void setActiveEffect(qSlicerSegmentEditorAbstractEffect* effect);

  /// Get an effect object by name
  /// \return The effect instance if exists, NULL otherwise
  Q_INVOKABLE qSlicerSegmentEditorAbstractEffect* effectByName(QString name);

  /// Create observations between slice view interactor and the widget.
  /// The captured events are propagated to the active effect if any.
  /// NOTE: This method should be called from the enter function of the
  ///   embedding module widget so that the events are correctly processed.
  void setupSliceObservations();

  /// Remove observations
  /// NOTE: This method should be called from the exit function of the
  ///   embedding module widget so that events are not processed unnecessarily.
  void removeSliceObservations();

public slots:
  /// Set the MRML \a scene associated with the widget
  virtual void setMRMLScene(vtkMRMLScene* newScene);

  /// Set segmentation MRML node
  Q_INVOKABLE void setSegmentationNode(vtkMRMLNode* node);
  /// Set segmentation MRML node by its ID
  Q_INVOKABLE void setSegmentationNodeID(const QString& nodeID);

protected slots:
  /// Handles changing of current segmentation MRML node
  Q_INVOKABLE void onSegmentationNodeChanged(vtkMRMLNode* node);
  /// Handles changing of the current master volume MRML node
  void onReferenceVolumeNodeChanged(vtkMRMLNode* node);
  /// Handles segment selection changes
  void segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

  /// Activate/deactivate effect on clicking its button
  void onEffectButtonClicked(QAbstractButton* button);

protected:  
  /// Callback function invoked when interaction happens
  static void processEvents(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  QScopedPointer<qMRMLSegmentEditorWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSegmentEditorWidget);
  Q_DISABLE_COPY(qMRMLSegmentEditorWidget);
};

#endif
