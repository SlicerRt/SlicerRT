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
class vtkMRMLNode;
class vtkMRMLRTBeamNode;
class vtkMRMLRTPlanNode;
class QString;
class QItemSelection;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class Q_SLICER_QTMODULES_EXTERNALBEAMPLANNING_EXPORT qSlicerExternalBeamPlanningModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerExternalBeamPlanningModuleWidget(QWidget *parent=nullptr);
  ~qSlicerExternalBeamPlanningModuleWidget() override;

  /// Support of node editing. Selects node in user interface that the user wants to edit
  bool setEditedNode(vtkMRMLNode* node, QString role=QString(), QString context=QString()) override;

public slots:
  /// Set the current MRML scene to the widget
  void setMRMLScene(vtkMRMLScene*) override;

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Update the entire widget based on the current parameter node
  void updateWidgetFromMRML();

public:
  /// Get selected beam node. If there are multiple selections then return the first one
  vtkMRMLRTBeamNode* currentBeamNode();

protected slots:
  /// Plan node serves as parameter set node for this module
  void setPlanNode(vtkMRMLNode*);

  // Plan parameters section
  void referenceVolumeNodeChanged(vtkMRMLNode*);
  void segmentationNodeChanged(vtkMRMLNode*);
  void poisMarkupsNodeChanged(vtkMRMLNode*);

  void targetSegmentChanged(const QString& segment);
  void isocenterAtTargetCenterCheckboxStateChanged(int state);
  void isocenterCoordinatesChanged(double* isocenterCoordinates);
  void centerViewToIsocenterClicked();

  void inversePlanningCheckboxStateChanged(int state);

  /// Update isocenter controls from isocenter markups fiducial
  void updateIsocenterPosition();

  void rxDoseChanged(double);
  void doseEngineChanged(const QString &);

  // Output section
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void doseROINodeChanged(vtkMRMLNode*);
  void doseGridSpacingChanged(const QString &);
  
  // Calculation buttons
  void calculateDoseClicked();
  void calculateWEDClicked();
  void clearDoseClicked();

  // Dose Optimization buttons
  void PlanOptimizerChanged(const QString&);
  void optimizePlanClicked();

  // Beams section
  void addBeamClicked();
  void removeBeamClicked();

  // Update functions
  void onLogicModified();
  void onProgressUpdated(double progress);

protected:
  void setup() override;
  void enter() override;
  void onEnter();

  // Populate dose engines combobox from registered dose engines
  void updateDoseEngines();

  // Populate optimization engine combobox
  void updatePlanOptimizers();

protected:
  QScopedPointer<qSlicerExternalBeamPlanningModuleWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerExternalBeamPlanningModuleWidget);
  Q_DISABLE_COPY(qSlicerExternalBeamPlanningModuleWidget);
};

#endif
