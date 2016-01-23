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
#include "ctkVTKObject.h"
#include <ctkPimpl.h>

// Qt includes
#include <QIcon>
#include <QPair>
#include <QPoint>

class qSlicerSegmentEditorAbstractEffectPrivate;

class vtkMRMLScene;
class vtkMRMLSegmentEditorEffectNode;
class vtkMRMLAbstractViewNode;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkOrientedImageData;
class qMRMLWidget;
class qMRMLSliceWidget;

/// \ingroup SlicerRt_QtModules_Segmentations
class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qSlicerSegmentEditorAbstractEffect : public QObject
{
public:
  Q_OBJECT
  QVTK_OBJECT

public:
  /// Property identifier for soring event tags in widgets
  static const char* observerTagIdentifier() { return "ObserverTags"; };

public:
  typedef QObject Superclass;
  qSlicerSegmentEditorAbstractEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorAbstractEffect();

// API methods
public:  
  /// Get name of effect
  virtual QString name() = 0;

  /// Clone editor effect
  virtual qSlicerSegmentEditorAbstractEffect* clone() = 0;

  /// Get icon for effect to be displayed in segment editor
  virtual QIcon icon() { return QIcon(); };

  /// Perform actions to activate the effect
  virtual void activate() { };

  /// Perform actions to deactivate the effect (such as destroy actors, etc.)
  virtual void deactivate() { };

  /// Callback function invoked when interaction happens
  /// \param callerInteractor Interactor object that was observed to catch the event
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  virtual void processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLWidget* viewWidget) { };

  /// Update user interface from parameter set node
  virtual void updateGUIFromMRML(vtkObject* caller, void* callData) = 0;

// Get/set methods
  /// Set MRML scene
  void setScene(vtkMRMLScene* scene) { m_Scene = scene; };

  /// Set edited labelmap
  void setEditedLabelmap(vtkOrientedImageData* labelmap) { m_EditedLabelmap = labelmap; };

// Effect parameter functions
public:
  /// Get effect parameter set node
  vtkMRMLSegmentEditorEffectNode* parameterSetNode();

  /// Get effect parameter from effect parameter set node
  QString parameter(QString name);

  /// Set effect parameter in effect parameter set node
  void setParameter(QString name, QString value);

// Utility functions
public:
  /// Turn off cursor and save cursor to restore later
  void cursorOff(qMRMLWidget* viewWidget);
  /// Restore saved cursor
  void cursorOn(qMRMLWidget* viewWidget);

  /// Set the AbortFlag on the vtkCommand associated with the event.
  /// Causes other things listening to the interactor not to receive the events
  void abortEvent(vtkRenderWindowInteractor* interactor, unsigned long eventId, qMRMLWidget* viewWidget);

  /// Get render window for view widget
  static vtkRenderWindow* renderWindow(qMRMLWidget* viewWidget);
  /// Get renderer for view widget
  static vtkRenderer* renderer(qMRMLWidget* viewWidget);
  /// Get node for view widget
  static vtkMRMLAbstractViewNode* viewNode(qMRMLWidget* viewWidget);

  /// Convert RAS position to XY in-slice position
  static QPoint rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  static void xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget);

protected:
  /// MRML scene
  vtkMRMLScene* m_Scene;

  /// Edited binary labelmap
  vtkOrientedImageData* m_EditedLabelmap;
 
protected:
  QScopedPointer<qSlicerSegmentEditorAbstractEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorAbstractEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorAbstractEffect);
};

#endif
