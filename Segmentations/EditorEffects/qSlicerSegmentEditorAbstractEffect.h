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

// Qt includes
#include <QIcon>
#include <QPoint>

class qSlicerSegmentEditorAbstractEffectPrivate;

class vtkMRMLScene;
class vtkMRMLSegmentEditorNode;
class vtkMRMLAbstractViewNode;
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

  /// Perform actions needed before the edited labelmap is applied back to the segment.
  /// The default implementation only emits the signal. If the child classes override this function,
  /// they must call apply from the base class too
  virtual void apply();

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  virtual void setupOptionsFrame() { };

  /// Create a cursor customized for the given effect, potentially for each view
  virtual QCursor createCursor(qMRMLWidget* viewWidget);
  /// Turn off cursor and save cursor to restore later
  virtual void cursorOff(qMRMLWidget* viewWidget);
  /// Restore saved cursor
  virtual void cursorOn(qMRMLWidget* viewWidget);

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

  /// Set default parameters in the parameter MRML node
  virtual void setMRMLDefaults() = 0;

public slots:
  /// Update user interface from parameter set node
  virtual void updateGUIFromMRML() = 0;

  /// Update parameter set node from user interface
  virtual void updateMRMLFromGUI() = 0;

// Get/set methods
public:
  /// Get segment editor parameter set node
  vtkMRMLSegmentEditorNode* parameterSetNode();
  /// Set segment editor parameter set node
  Q_INVOKABLE void setParameterSetNode(vtkMRMLSegmentEditorNode* node);

  /// Get MRML scene (from parameter set node)
  Q_INVOKABLE vtkMRMLScene* scene();

  /// Connect signal that is emitted when the edited labelmap is to be applied to the currently
  /// edited segment.
  void connectApply(QObject* receiver, const char* method);

  /// Simple mechanism to let the effects know that edited labelmap has changed
  virtual void editedLabelmapChanged() { };
  /// Simple mechanism to let the effects know that master volume has changed
  virtual void masterVolumeNodeChanged() { };

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
  /// Get effect parameter from effect parameter set node
  Q_INVOKABLE QString parameter(QString name);

  /// Convenience function to get integer parameter
  Q_INVOKABLE int integerParameter(QString name);

  /// Convenience function to get double parameter
  Q_INVOKABLE double doubleParameter(QString name);

  /// Set effect parameter in effect parameter set node. This function is called by both convenience functions.
  /// \param name Parameter name string
  /// \param value Parameter value string
  /// \param emitParameterModifiedEvent Flag determining whether parameter modified event is emitted
  ///   when setting the parameter. It triggers UI update of the effect option widgets only.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  Q_INVOKABLE void setParameter(QString name, QString value, bool emitParameterModifiedEvent=false);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param value Parameter value integer
  /// \param emitParameterModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  Q_INVOKABLE void setParameter(QString name, int value, bool emitParameterModifiedEvent=false);

  /// Convenience function to set double parameter
  /// \param name Parameter name string
  /// \param value Parameter value double
  /// \param emitParameterModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  Q_INVOKABLE void setParameter(QString name, double value, bool emitParameterModifiedEvent=false);

// Utility functions
public:
  /// Set the AbortFlag on the vtkCommand associated with the event.
  /// Causes other things listening to the interactor not to receive the events
  void abortEvent(vtkRenderWindowInteractor* interactor, unsigned long eventId, qMRMLWidget* viewWidget);

  /// Get render window for view widget
  Q_INVOKABLE static vtkRenderWindow* renderWindow(qMRMLWidget* viewWidget);
  /// Get renderer for view widget
  Q_INVOKABLE static vtkRenderer* renderer(qMRMLWidget* viewWidget);
  /// Get node for view widget
  Q_INVOKABLE static vtkMRMLAbstractViewNode* viewNode(qMRMLWidget* viewWidget);

  /// Convert RAS position to XY in-slice position
  Q_INVOKABLE static QPoint rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to RAS position:
  /// x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  /// slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  /// coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  Q_INVOKABLE static void xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  Q_INVOKABLE static void xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to image IJK position, \sa xyzToRas
  Q_INVOKABLE static void xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);
  /// Convert XY in-slice position to image IJK position
  Q_INVOKABLE static void xyToIjk(QPoint xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);

protected:
  QScopedPointer<qSlicerSegmentEditorAbstractEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorAbstractEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorAbstractEffect);
};

#endif
