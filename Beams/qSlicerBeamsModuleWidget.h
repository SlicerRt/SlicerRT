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

#ifndef __qSlicerBeamsModuleWidget_h
#define __qSlicerBeamsModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerBeamsModuleExport.h"

class qSlicerBeamsModuleWidgetPrivate;
class vtkMRMLNode;
class vtkMRMLRTBeamNode;

/// \ingroup SlicerRt_QtModules_Beams
class Q_SLICER_QTMODULES_BEAMS_EXPORT qSlicerBeamsModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerBeamsModuleWidget(QWidget *parent=0);
  virtual ~qSlicerBeamsModuleWidget();

  virtual void enter();

  virtual bool setEditedNode(vtkMRMLNode* node, QString role=QString(), QString context=QString());

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Update the entire widget based on the current parameter node
  void updateWidgetFromMRML();

// Update functions
protected:
  /// TODO
  void updateCurrentBeamTransform();

  /// TODO
  void updateBeamGeometryModel();

protected slots:
  /// Logic modified
  void onLogicModified();

  /// Handles active beam selection changed event
  void rtBeamNodeChanged(vtkMRMLNode* node);
  /// Currently selected RTBeam was modified
  void onRTBeamNodeModified();

  void goToParentPlanButtonClicked();

  // Beam global parameters
  void beamNameChanged(const QString &);
  void radiationTypeChanged(int);

  // Prescription page
  void beamTypeChanged(const QString &);
  void targetSegmentationNodeChanged(vtkMRMLNode* node);
  void targetSegmentChanged(const QString& segment);
  void rxDoseChanged(double);
  void isocenterSpecChanged(const QString &);
  void isocenterCoordinatesChanged(double*);
  void isocenterFiducialNodeChangedfromCoordinates(double*);
  void dosePointFiducialNodeChangedfromCoordinates(double*);

  // Energy page
  void proximalMarginChanged(double);
  void distalMarginChanged(double);
  void beamLineTypeChanged(const QString &);
  void manualEnergyPrescriptionChanged(bool);
  void minimumEnergyChanged(double);
  void maximumEnergyChanged(double);

  // Geometry page
  void mlcPositionDoubleArrayNodeChanged(vtkMRMLNode* node);
  void sourceDistanceChanged(double);
  void xJawsPositionValuesChanged(double, double);
  void yJawsPositionValuesChanged(double, double);
  void collimatorAngleChanged(double);
  void gantryAngleChanged(double);
  void couchAngleChanged(double);
  void smearingChanged(double);
  void beamWeightChanged(double);

  // Proton beam model
  void apertureDistanceChanged(double);
  void algorithmChanged(const QString &);
  void pencilBeamSpacingChanged(double);
  void sourceSizeChanged(double);
  void energyResolutionChanged(double);
  void energySpreadChanged(double);
  void stepLengthChanged(double);
  void wedApproximationChanged(bool);
  void rangeCompensatorHighlandChanged(bool);

  // Beam visualization
  void updateDRRClicked();
  void beamEyesViewClicked(bool);
  void contoursInBEWClicked(bool);

  //TODO: Unused function
  void collimatorTypeChanged(const QString &);

protected:
  QScopedPointer<qSlicerBeamsModuleWidgetPrivate> d_ptr;

protected:
  virtual void setup();
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerBeamsModuleWidget);
  Q_DISABLE_COPY(qSlicerBeamsModuleWidget);
};

#endif
