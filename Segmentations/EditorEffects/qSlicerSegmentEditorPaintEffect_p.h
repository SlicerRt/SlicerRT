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

// Segmentations Editor Effects includes
#include "qSlicerSegmentationsEditorEffectsExport.h"

#include "qSlicerSegmentEditorPaintEffect.h"

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QObject>
#include <QList>
#include <QMap>

class BrushPipeline;
class ctkDoubleSlider;
class QPoint;
class QIcon;
class QFrame;
class QCheckBox;
class QPushButton;
class qMRMLSliceWidget;
class qMRMLSpinBox;
class vtkActor2D;
class vtkImageSlicePaint;
class vtkPoints;

/// \ingroup SlicerRt_QtModules_Segmentations
/// \brief Private implementation of the segment editor paint effect
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
  void paintAddPoint(qMRMLSliceWidget* sliceWidget, int pixelPositionXy[2]);

  /// Update paint circle glyph
  void updateBrush(qMRMLSliceWidget* sliceWidget, BrushPipeline* brush);

  /// Update brushes
  void updateBrushes();

protected:
  /// Get brush object for widget. Create if does not exist
  BrushPipeline* brushForWidget(qMRMLSliceWidget* sliceWidget);

  /// Paint labelmap
  void paintApply(qMRMLSliceWidget* sliceWidget);

  /// Paint with a brush that is circular (or optionally spherical) in XY space
  /// (could be stretched or rotate when transformed to IJK)
  /// - Make sure to hit every pixel in IJK space
  /// - Apply the threshold if selected
  void paintBrush(qMRMLSliceWidget* sliceWidget, double brushCenterXy[2]);

  /// Paint one pixel to coordinate
  void paintPixel(qMRMLSliceWidget* sliceWidget, double pixelPositionXy[2]);
  void paintPixels(qMRMLSliceWidget* sliceWidget, vtkPoints* pixelPositions);

  /// Scale brush radius and save it in parameter node
  void scaleRadius(double scaleFactor);

public slots:
  void onRadiusUnitsClicked();
  void onQuickRadiusButtonClicked();
  void onRadiusValueChanged(double);

public:
  QIcon PaintIcon;

  vtkSmartPointer<vtkPoints> PaintCoordinates;
  QList<vtkActor2D*> FeedbackActors;
  QMap<qMRMLSliceWidget*, BrushPipeline*> Brushes;
  vtkImageSlicePaint* Painter;
  bool DelayedPaint;
  bool IsPainting;

  QFrame* RadiusFrame;
  qMRMLSpinBox* RadiusSpinBox;
  ctkDoubleSlider* RadiusSlider;
  QPushButton* RadiusUnitsToggle;
  QCheckBox* SphereCheckbox;
  QCheckBox* SmudgeCheckbox;
  QCheckBox* PixelModeCheckbox;
};

#endif
