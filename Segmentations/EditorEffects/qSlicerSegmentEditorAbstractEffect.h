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
#include <QVector3D>

class qSlicerSegmentEditorAbstractEffectPrivate;

class vtkMRMLScene;
class vtkMRMLSegmentEditorNode;
class vtkMRMLAbstractViewNode;
class vtkMRMLSegmentationNode;
class vtkSegment;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkOrientedImageData;
class vtkProp;
class qMRMLWidget;
class qMRMLSliceWidget;
class QFrame;
class QColor;

/// \ingroup SlicerRt_QtModules_Segmentations
/// \brief Abstract class for segment editor effects
class Q_SLICER_SEGMENTATIONS_EFFECTS_EXPORT qSlicerSegmentEditorAbstractEffect : public QObject
{
public:
  Q_OBJECT

  /// This property stores the name of the effect
  /// Cannot be empty.
  /// \sa name()
  Q_PROPERTY(QString name READ name WRITE setName)

public:
  /// Property identifier for soring event tags in widgets
  static const char* observerTagIdentifier() { return "ObserverTags"; };

public:
  typedef QObject Superclass;
  qSlicerSegmentEditorAbstractEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorAbstractEffect();

// API: Methods that are to be reimplemented in the effect subclasses
public:  
  /// Get icon for effect to be displayed in segment editor
  virtual QIcon icon() { return QIcon(); };

  /// Get help text for effect to be displayed in the help box
  Q_INVOKABLE virtual const QString helpText()const { return QString(); };

  /// Clone editor effect. Override to return a new instance of the effect sub-class
  virtual qSlicerSegmentEditorAbstractEffect* clone() = 0;

  /// Perform actions to activate the effect (show options frame, etc.)
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  Q_INVOKABLE virtual void activate();

  /// Perform actions to deactivate the effect (hide options frame, destroy actors, etc.)
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  Q_INVOKABLE virtual void deactivate();

  /// Perform actions needed before the edited labelmap is applied back to the segment.
  /// NOTE: The default implementation only emits the signal. If the child classes override this function,
  /// they must call apply from the base class too, AFTER the effect-specific implementation
  Q_INVOKABLE virtual void apply();

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  /// NOTE: Base class implementation needs to be called BEFORE the effect-specific implementation
  virtual void setupOptionsFrame() { };

  /// Create a cursor customized for the given effect, potentially for each view
  virtual QCursor createCursor(qMRMLWidget* viewWidget);

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
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void setMRMLDefaults() = 0;

  /// Simple mechanism to let the effects know that edited labelmap has changed
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void editedLabelmapChanged() { };
  /// Simple mechanism to let the effects know that master volume has changed
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void masterVolumeNodeChanged() { };
  /// Simple mechanism to let the effects know that the layout has changed
  virtual void layoutChanged() { };

public slots:
  /// Update user interface from parameter set node
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void updateGUIFromMRML() = 0;

  /// Update parameter set node from user interface
  /// NOTE: Base class implementation needs to be called with the effect-specific implementation
  virtual void updateMRMLFromGUI() = 0;

// Get/set methods
public:
  /// Get segment editor parameter set node
  Q_INVOKABLE vtkMRMLSegmentEditorNode* parameterSetNode();
  /// Set segment editor parameter set node
  Q_INVOKABLE void setParameterSetNode(vtkMRMLSegmentEditorNode* node);

  /// Get MRML scene (from parameter set node)
  Q_INVOKABLE vtkMRMLScene* scene();

  /// Get effect options frame
  Q_INVOKABLE QFrame* optionsFrame();

  /// Add actor to container that needs to be cleared on deactivation
  Q_INVOKABLE void addActor(qMRMLWidget* viewWidget, vtkProp* actor);

  /// Add effect options widget to options frame layout
  /// The implemented effects need to create their options UI widget, make the connections,
  /// then call this function to add the options UI to the effect options frame
  Q_INVOKABLE void addOptionsWidget(QWidget* newOptionsWidget);

  /// Get name of effect
  virtual QString name()const;

  /// Set the name of the effect
  /// NOTE: name must be defined in constructor in C++ effects, this can only be used in python scripted ones
  virtual void setName(QString name);

  /// Turn off cursor and save cursor to restore later
  Q_INVOKABLE void cursorOff(qMRMLWidget* viewWidget);
  /// Restore saved cursor
  Q_INVOKABLE void cursorOn(qMRMLWidget* viewWidget);

  /// Connect signal that is emitted when the edited labelmap is to be applied to the currently
  /// edited segment.
  void connectApply(QObject* receiver, const char* method);

// Effect parameter functions
public:
  /// Get effect or common parameter from effect parameter set node
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
  /// Set parameters that are common for multiple effects. Typically used by base class effects, such
  ///   as label or morphology \sa setParameter
  /// By default the parameter names are prefixed for each effect, so they are unique for effects.
  /// This method does not prefix the parameter, so can be the same for multiple effects.
  /// Note: Parameter getter functions look for effect parameters first, then common parameter if the
  ///   effect-specific is not found.
  Q_INVOKABLE void setCommonParameter(QString name, QString value, bool emitParameterModifiedEvent=false);

  /// Convenience function to set integer parameter
  /// \param name Parameter name string
  /// \param value Parameter value integer
  /// \param emitParameterModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  Q_INVOKABLE void setParameter(QString name, int value, bool emitParameterModifiedEvent=false);
  /// Convenience function to set integer common parameter \sa setCommonParameter
  Q_INVOKABLE void setCommonParameter(QString name, int value, bool emitParameterModifiedEvent=false);

  /// Convenience function to set double parameter
  /// \param name Parameter name string
  /// \param value Parameter value double
  /// \param emitParameterModifiedEvent Flag determining whether modified event is emitted when setting the parameter.
  ///   It is false by default, as in most cases disabling modified events in the effects is desirable,
  ///   as they are mostly called from functions \sa setMRMLDefaults and \sa updateMRMLFromGUI
  Q_INVOKABLE void setParameter(QString name, double value, bool emitParameterModifiedEvent=false);
  /// Convenience function to set double common parameter \sa setCommonParameter
  Q_INVOKABLE void setCommonParameter(QString name, double value, bool emitParameterModifiedEvent=false);

// Utility functions
public:
  /// Set the AbortFlag on the vtkCommand associated with the event.
  /// Causes other things listening to the interactor not to receive the events
  Q_INVOKABLE void abortEvent(vtkRenderWindowInteractor* interactor, unsigned long eventId, qMRMLWidget* viewWidget);

  /// Get image data of master volume.
  /// Use argument as the image data is temporary, because it involves the geometry of the volume
  /// \return Success flag
  Q_INVOKABLE bool masterVolumeImageData(vtkOrientedImageData* masterImageData);
  /// Get scalar range for master volume
  /// \return Success flag
  bool masterVolumeScalarRange(double& low, double& high);

  /// Get render window for view widget
  Q_INVOKABLE static vtkRenderWindow* renderWindow(qMRMLWidget* viewWidget);
  /// Get renderer for view widget
  Q_INVOKABLE static vtkRenderer* renderer(qMRMLWidget* viewWidget);
  /// Get node for view widget
  Q_INVOKABLE static vtkMRMLAbstractViewNode* viewNode(qMRMLWidget* viewWidget);

  /// Convert RAS position to XY in-slice position
  static QPoint rasToXy(double ras[3], qMRMLSliceWidget* sliceWidget);
  /// Convert RAS position to XY in-slice position, python accessor method
  Q_INVOKABLE static QPoint rasToXy(QVector3D ras, qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to RAS position:
  /// x,y uses slice (canvas) coordinate system and actually has a 3rd z component (index into the
  /// slice you're looking at), hence xyToRAS is really performing xyzToRAS. RAS is patient world
  /// coordinate system. Note the 1 is because the transform uses homogeneous coordinates.
  static void xyzToRas(double inputXyz[3], double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to RAS position, python accessor method
  Q_INVOKABLE static QVector3D xyzToRas(QVector3D inputXyz, qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  static void xyToRas(QPoint xy, double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position
  static void xyToRas(int xy[2], double outputRas[3], qMRMLSliceWidget* sliceWidget);
  /// Convert XY in-slice position to RAS position, python accessor method
  Q_INVOKABLE static QVector3D xyToRas(QPoint xy, qMRMLSliceWidget* sliceWidget);
  /// Convert XYZ slice view position to image IJK position, \sa xyzToRas
  static void xyzToIjk(double inputXyz[3], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);
  /// Convert XYZ slice view position to image IJK position, python accessor method, \sa xyzToRas
  Q_INVOKABLE static QVector3D xyzToIjk(QVector3D inputXyz, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);
  /// Convert XY in-slice position to image IJK position
  static void xyToIjk(QPoint xy, int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);
  /// Convert XY in-slice position to image IJK position
  static void xyToIjk(int xy[2], int outputIjk[3], qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);
  /// Convert XY in-slice position to image IJK position, python accessor method
  Q_INVOKABLE static QVector3D xyToIjk(QPoint xy, qMRMLSliceWidget* sliceWidget, vtkOrientedImageData* image);

protected:
  /// Name of the effect
  QString m_Name;

protected:
  QScopedPointer<qSlicerSegmentEditorAbstractEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorAbstractEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorAbstractEffect);
};

#endif
