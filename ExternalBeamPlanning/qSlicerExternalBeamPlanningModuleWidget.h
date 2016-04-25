/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerExternalBeamPlanningModuleWidget_h
#define __qSlicerExternalBeamPlanningModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerExternalBeamPlanningModuleExport.h"

class qSlicerExternalBeamPlanningModuleWidgetPrivate;
class vtkMRMLExternalBeamPlanningNode;
class vtkMRMLNode;
class vtkMRMLRTBeamNode;
class vtkMRMLRTPlanNode;
class QString;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class Q_SLICER_QTMODULES_EXTERNALBEAMPLANNING_EXPORT qSlicerExternalBeamPlanningModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerExternalBeamPlanningModuleWidget(QWidget *parent=0);
  virtual ~qSlicerExternalBeamPlanningModuleWidget();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

// Update functions
protected:
  /// Update the entire widget based on the current parameter node
  void updateWidgetFromMRML();

  void updateRTBeamTableWidget();

  /// Update widget GUI from a beam node
  void updateWidgetFromRTBeam(vtkMRMLRTBeamNode* beamNode);

  void updateBeamParameters();

  void updateCurrentBeamTransform();

  void updateBeamGeometryModel();

// Utility functions
protected:
  vtkMRMLRTBeamNode* currentBeamNode();

protected slots:
  /// RT plan was modified programmatically
  void onRTPlanNodeModified();

  // Logic modified
  void onLogicModified();

  // Currently selected RTBeam was modified
  void onRTBeamNodeModifiedEvent();

  // RT plan page
  void referenceVolumeNodeChanged(vtkMRMLNode*);
  void planSegmentationNodeChanged(vtkMRMLNode*);
  void rtPlanNodeChanged(vtkMRMLNode*);
  void planPOIsNodeChanged(vtkMRMLNode*);
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void doseROINodeChanged(vtkMRMLNode*);
  void doseGridSpacingChanged(const QString &);
  void doseEngineTypeChanged(const QString &);
  
  // RT Beams page
  void tableWidgetCellClicked(int, int);
  void addBeamClicked();
  void removeBeamClicked();

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
  
  // Calculation buttons
  void calculateDoseClicked();
  void calculateWEDClicked();
  void clearDoseClicked();

  void collimatorTypeChanged(const QString &);

protected:
  virtual void setup();
  virtual void enter();
  void onEnter();

protected:
  QScopedPointer<qSlicerExternalBeamPlanningModuleWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerExternalBeamPlanningModuleWidget);
  Q_DISABLE_COPY(qSlicerExternalBeamPlanningModuleWidget);
};

#endif
