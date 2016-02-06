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

#ifndef __qSlicerSegmentEditorLabelEffect_h
#define __qSlicerSegmentEditorLabelEffect_h

// Segmentations Editor Effects includes
#include "qSlicerSegmentationsEditorEffectsExport.h"

#include "qSlicerSegmentEditorAbstractEffect.h"

class qSlicerSegmentEditorLabelEffectPrivate;

class vtkMatrix4x4;
class vtkOrientedImageData;
class vtkMRMLVolumeNode;
class vtkMRMLSegmentationNode;

/// \ingroup SlicerRt_QtModules_Segmentations
/// \brief Base class for all "label" effects.
/// 
/// This base class provides GUI and MRML for the options PaintOver and Threshold.
class Q_SLICER_SEGMENTATIONS_EFFECTS_EXPORT qSlicerSegmentEditorLabelEffect :
  public qSlicerSegmentEditorAbstractEffect
{
public:
  Q_OBJECT

public:
  typedef qSlicerSegmentEditorAbstractEffect Superclass;
  qSlicerSegmentEditorLabelEffect(QObject* parent = NULL);
  virtual ~qSlicerSegmentEditorLabelEffect(); 

  static QString paintOverParameterName() { return QString("PaintOver"); };
  static QString paintThresholdParameterName() { return QString("PaintThreshold"); };
  static QString paintThresholdMinParameterName() { return QString("PaintThresholdMin"); };
  static QString paintThresholdMaxParameterName() { return QString("PaintThresholdMax"); };
  static QString thresholdAvailableParameterName() { return QString("ThresholdAvailable"); };
  static QString paintOverAvailableParameterName() { return QString("PaintOverAvailable"); };

public:
  /// Get name of effect
  /// (redefinition of pure virtual function to allow python wrapper to identify this as abstract class)
  Q_INVOKABLE virtual QString name() = 0;

  /// Clone editor effect
  /// (redefinition of pure virtual function to allow python wrapper to identify this as abstract class)
  virtual qSlicerSegmentEditorAbstractEffect* clone() = 0;

  /// Perform actions needed before the edited labelmap is applied back to the segment
  virtual void apply();

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  virtual void setupOptionsFrame();

  /// Set default parameters in the parameter MRML node
  virtual void setMRMLDefaults();

  /// Perform actions needed on edited labelmap change
  virtual void editedLabelmapChanged();

  /// Perform actions needed on master volume change
  virtual void masterVolumeNodeChanged();

public slots:
  /// Update user interface from parameter set node
  virtual void updateGUIFromMRML();

  /// Update parameter set node from user interface
  virtual void updateMRMLFromGUI();

// Utility functions
public:
  /// Return matrix for volume node that takes into account the IJKToRAS
  /// and any linear transforms that have been applied
  static void ijkToRasMatrix(vtkMRMLVolumeNode* node, vtkMatrix4x4* ijkToRas);

  /// Return matrix for volume node that takes into account the IJKToRAS
  /// and any linear transforms that have been applied
  static void ijkToRasMatrix(vtkOrientedImageData* image, vtkMRMLSegmentationNode* node, vtkMatrix4x4* ijkToRas);

protected:
  QScopedPointer<qSlicerSegmentEditorLabelEffectPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSegmentEditorLabelEffect);
  Q_DISABLE_COPY(qSlicerSegmentEditorLabelEffect);
};

#endif
