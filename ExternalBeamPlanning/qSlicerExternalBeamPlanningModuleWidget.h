/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
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
class QString;
class QTableWidgetItem;

/// \ingroup Slicer_QtModules_Contours
class Q_SLICER_QTMODULES_EXTERNALBEAMPLANNING_EXPORT qSlicerExternalBeamPlanningModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerExternalBeamPlanningModuleWidget(QWidget *parent=0);
  virtual ~qSlicerExternalBeamPlanningModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setExternalBeamPlanningNode(vtkMRMLNode*);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void onLogicModified();

  void referenceVolumeNodeChanged(vtkMRMLNode*);
  void rtPlanNodeChanged(vtkMRMLNode*);
  void addBeamClicked();
  void removeBeamClicked();

  void beamNameChanged(const QString &);
  void radiationTypeChanged(int);

  /* Task buttons */
  void beamPrescriptionButtonClicked();
  void beamGeometryButtonClicked();
  void beamModelButtonClicked();

  /* Prescription page */
  void beamTypeChanged(const QString &);

  /* Geometry page */
  void gantryAngleChanged(double);
  void collimatorTypeChanged(const QString &);
  void isocenterFiducialNodeChanged(vtkMRMLNode*);
  void protonTargetContourNodeChanged(vtkMRMLNode* node);

  void tableWidgetItemClicked(QTableWidgetItem *item);

  /* Calculation buttons */
  void calculateDoseClicked();
  void calculateWEDClicked();

protected:
  QScopedPointer<qSlicerExternalBeamPlanningModuleWidgetPrivate> d_ptr;

  virtual void setup();
  void onEnter();

  void updateRTBeamTableWidget();

  void updateBeamParameters();

private:
  Q_DECLARE_PRIVATE(qSlicerExternalBeamPlanningModuleWidget);
  Q_DISABLE_COPY(qSlicerExternalBeamPlanningModuleWidget);
};

#endif
