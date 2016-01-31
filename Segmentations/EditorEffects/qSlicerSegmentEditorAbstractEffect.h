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

#ifndef __qSlicerSegmentEditorAbstractEffect_h
#define __qSlicerSegmentEditorAbstractEffect_h

// Segmentations Editor Effects includes
#include "qSlicerSegmentationsEditorEffectsExport.h"

// CTK includes
#include <ctkVTKObject.h>
#include <ctkPimpl.h>

// Qt includes
#include <QIcon>
#include <QPair>
#include <QPoint>

class qSlicerSegmentEditorAbstractEffectPrivate;

class vtkMRMLScene;
class vtkMRMLSegmentEditorEffectNode;
class vtkMRMLAbstractViewNode;
class vtkMRMLVolumeNode;
class vtkMRMLSegmentationNode;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkOrientedImageData;
class vtkProp;
class qMRMLWidget;
class qMRMLSliceWidget;
class QFrame;

/// \ingroup SlicerRt_QtModules_Segmentations
/// \brief Abstract class for segment editor effects
class Q_SLICER_SEGMENTATIONS_EFFECTS_EXPORT qSlicerSegmentEditorAbstractEffect : public QObject
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

// API: Methods that are to be reimplemented in the effect subclasses
public:  
  /// Get name of effect
  Q_INVOKABLE virtual QString name() = 0;

  /// Get icon for effect to be displayed in segment editor
  virtual QIcon icon() { return QIcon(); };

  /// Get help text for effect to be displayed in the help box
  Q_INVOKABLE virtual const QString helpText()const { return QString(); };

  /// Clone editor effect. Override to return a new instance of the effect sub-class
  virtual qSlicerSegmentEditorAbstractEffect* clone() = 0;

  /// Perform actions to activate the effect (show options frame, etc.)
  Q_INVOKABLE virtual void activate();

  /// Perform actions to deactivate the effect (hide options frame, destroy actors, etc.)
  Q_INVOKABLE virtual void deactivate();

  /// Callback function invoked when interaction happens
  /// \param callerInteractor Interactor object that was observed to catch the event
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  virtual void processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLWidget* viewWidget) { };

  /// Callback function invoked when view node is modified
  /// \param callerViewNode View node that was observed to catch the event. Can be either \sa vtkMRMLSliceNode or \sa vtkMRMLViewNode
  /// \param eid Event identifier
  /// \param viewWidget Widget of the Slicer layout view. Can be \sa qMRMLSliceWidget or \sa qMRMLThreeDWidget
  virtual void processViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, qMRMLWidget* viewWidget) { };

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  virtual void setupOptionsFrame() { };

  /// Set default parameters in the parameter MRML node
  virtual void setMRMLDefaults() = 0;

public slots:
  /// Update user interface from parameter set node
  virtual void updateGUIFromMRML() = 0;

  /// Update parameter set node from user interface
  virtual void updateMRMLFromGUI() = 0;

signals:
  /// Signal that needs to be emitted from the effects when the edited labelmap is to be applied to the currently
  /// edited segment. Connected to \sa qMRMLSegmentEditorWidget::applyChangesToSelectedSegment
  void apply();

// Get/set methods
public:
  /// Set edited labelmap. Can be overridden to perform additional actions.
  Q_INVOKABLE virtual void setEditedLabelmap(vtkOrientedImageData* labelmap) { m_EditedLabelmap = labelmap; };

  /// Set MRML scene
  Q_INVOKABLE void setScene(vtkMRMLScene* scene) { m_Scene = scene; };

  /// Set master volume node ID
  Q_INVOKABLE void setMasterVolumeNodeID(QString id) { m_MasterVolumeNodeID = id; };
  // Get master volume node
  Q_INVOKABLE vtkMRMLVolumeNode* masterVolumeNode();

  /// Set segmentation node ID
  Q_INVOKABLE void setSegmentationNodeID(QString id) { m_SegmentationNodeID = id; };
  // Get segmentation node
  Q_INVOKABLE vtkMRMLSegmentationNode* segmentationNode();

  /// Get effect options frame
  Q_INVOKABLE QFrame* optionsFrame();

  /// Add actor to container that needs to be cleared on deactivation
  void addActor(qMRMLWidget* viewWidget, vtkProp* actor);

protected:
  /// Add effect options widget to options frame layout
  /// The implemented effects need to create their options UI widget, make the connections,
  /// then call this function to add the options UI to the effect options frame
  void addOptionsWidget(QWidget* newOptionsWidget);

// Effect parameter functions
public:
  /// Get effect parameter set node
  vtkMRMLSegmentEditorEffectNode* parameterSetNode();

  /// Get effect parameter from effect parameter set node
  QString parameter(QString name);

  /// Convenience function to get integer parameter
  int integerParameter(QString name);

  /// Convenience function to get double parameter
  double doubleParameter(QString name);

  /// Set effect parameter in effect parameter set node. This function is called by both convenience functions.
  /// \param name Parameter name string
  /// \param value Parameter value string
  /// \param emitModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  void setParameter(QString name, QString value, bool emitModifiedEvent=false);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param value Parameter value integer
  /// \param emitModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  void setParameter(QString name, int value, bool emitModifiedEvent=false);

  /// Convenience function to set double parameter
  /// \param name Parameter name string
  /// \param value Parameter value double
  /// \param emitModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  void setParameter(QString name, double value, bool emitModifiedEvent=false);

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
  /// Convert XYZ slice view position to RAS position:
  /// x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  /// slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  /// coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  static void xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  static void xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to image IJK position, \sa xyzToRas
  static void xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);
  /// Convert XY in-slice position to image IJK position
  static void xyToIjk(QPoint xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);

protected:
  /// MRML scene
  vtkMRMLScene* m_Scene;

  /// Edited binary labelmap
  vtkOrientedImageData* m_EditedLabelmap;

  /// Master volume node ID to conveniently and get the master volume for certain operations
  /// (the alternative would be to go through the slice logic and the layers)
  QString m_MasterVolumeNodeID;
 
  /// Segmentation node ID to conveniently and get the master volume for certain operations
  /// (the alternative would be to go through the slice logic and the layers)
  QString m_SegmentationNodeID;

protected:
  QScopedPointer<qSlicerSegmentEditorAbstractEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorAbstractEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorAbstractEffect);
};

#endif
