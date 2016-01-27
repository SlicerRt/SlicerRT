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

#ifndef __qSlicerSegmentEditorPaintEffect_p_h
#define __qSlicerSegmentEditorPaintEffect_p_h

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Slicer API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// Segmentations Widgets includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

#include "qSlicerSegmentEditorPaintEffect.h"

// Qt includes
#include <QObject>
#include <QList>
#include <QMap>

class PaintEffectBrush;
class QPoint;
class QIcon;
class QFrame;
class QCheckBox;
class ctkDoubleSlider;
class qMRMLSliceWidget;
class qMRMLSpinBox;
class vtkActor2D;
class vtkImageSlicePaint;

/// \ingroup SlicerRt_QtModules_Segmentations
/// \brief TODO:
class qSlicerSegmentEditorPaintEffectPrivate: public QObject
{
  Q_OBJECT
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorPaintEffect);
protected:
  qSlicerSegmentEditorPaintEffect* const q_ptr;
public:
  typedef QObject Superclass;
  qSlicerSegmentEditorPaintEffectPrivate(qSlicerSegmentEditorPaintEffect& object);
  ~qSlicerSegmentEditorPaintEffectPrivate();

  /// Depending on the \sa DelayedPaint mode, either paint the given point or queue
  /// it up with a marker for later painting
  void paintAddPoint(qMRMLSliceWidget* sliceWidget, int x, int y);

  /// Draw paint circle glyph
  void createBrushGlyph(qMRMLSliceWidget* sliceWidget, PaintEffectBrush* brush);

  /// Update brushes
  void updateBrushes();

protected:
  /// Get brush object for widget. Create if does not exist
  PaintEffectBrush* brushForWidget(qMRMLSliceWidget* sliceWidget);

  /// Add a feedback actor (copy of the paint radius actor) for any points that don't
  /// have one yet. If the list is empty, clear out the old actors
  void paintFeedback(qMRMLSliceWidget* sliceWidget);

  /// Paint labelmap
  void paintApply(qMRMLSliceWidget* sliceWidget);

  /// Paint with a brush that is circular (or optionally spherical) in XY space
  /// (could be stretched or rotate when transformed to IJK)
  /// - Make sure to hit every pixel in IJK space
  /// - Apply the threshold if selected
  void paintBrush(qMRMLSliceWidget* sliceWidget, QPoint xy);

  /// Paint one pixel to coordinate
  void paintPixel(qMRMLSliceWidget* sliceWidget, QPoint xy);

  /// Scale brush radius and save it in parameter node
  void scaleRadius(double scaleFactor);

public slots:
  void onRadiusUnitsToggled(bool checked);
  void onQuickRadiusButtonClicked();
  void onRadiusValueChanged(double value);

public:
  QIcon EffectIcon;

  QList<QPoint> PaintCoordinates;
  QList<vtkActor2D*> FeedbackActors;
  QMap<qMRMLSliceWidget*, PaintEffectBrush*> Brushes;
  vtkImageSlicePaint* Painter;
  bool DelayedPaint;
  bool IsPainting;

  QFrame* RadiusFrame;
  qMRMLSpinBox* RadiusSpinBox;
  ctkDoubleSlider* RadiusSlider; 
  QCheckBox* SphereCheckbox;
  QCheckBox* SmudgeCheckbox;
  QCheckBox* PixelModeCheckbox;
};

#endif
