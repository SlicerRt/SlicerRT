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
/// \brief TODO:
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
  static QString thresholdEnabledParameterName() { return QString("ThresholdEnabled"); };
  static QString paintOverEnabledParameterName() { return QString("PaintOverEnabled"); };

public:
  /// Get name of effect
  /// (redefinition of pure virtual function to allow python wrapper to identify this as abstract class)
  Q_INVOKABLE virtual QString name() = 0;

  /// Clone editor effect
  /// (redefinition of pure virtual function to allow python wrapper to identify this as abstract class)
  virtual qSlicerSegmentEditorAbstractEffect* clone() = 0;

  /// Create options frame widgets, make connections, and add them to the main options frame using \sa addOptionsWidget
  virtual void setupOptionsFrame();

  /// Set default parameters in the parameter MRML node
  virtual void setMRMLDefaults();

  /// Set master volume node ID. Override to set threshold minimum and maximum
  Q_INVOKABLE virtual void setMasterVolumeNodeID(QString id);

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
