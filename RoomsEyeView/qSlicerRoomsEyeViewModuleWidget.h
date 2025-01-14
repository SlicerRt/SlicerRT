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

  The collision detection module was partly supported by Conselleria de
  Educación, Investigación, Cultura y Deporte (Generalitat Valenciana), Spain
  under grant number CDEIGENT/2019/011.

==============================================================================*/

#ifndef __qSlicerRoomsEyeViewModuleWidget_h
#define __qSlicerRoomsEyeViewModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerRoomsEyeViewModuleExport.h"

class qSlicerRoomsEyeViewModuleWidgetPrivate;
class vtkMRMLNode;

/// \ingroup SlicerRt_QtModules_RoomsEyeView
class Q_SLICER_QTMODULES_ROOMSEYEVIEW_EXPORT qSlicerRoomsEyeViewModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerRoomsEyeViewModuleWidget(QWidget *parent=nullptr);
  ~qSlicerRoomsEyeViewModuleWidget() override;

  void enter() override;

public slots:
  virtual void setMRMLScene(vtkMRMLScene*);
  void setParameterNode(vtkMRMLNode*);
  void onSceneImportedEvent();
  void onSceneClosedEvent();

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

  /// Load treatment machine by file
  /// \param descriptorFilePath Descriptor JSON file full path
  /// \param treatmentMachineType Treatment machine identifier string, can be omitted.
  ///        Used for setting hard-coded machine specific motion ranges. The argument
  ///        should be removed and the JSON file content used for this instead.
  void loadTreatmentMachineFromFile(QString descriptorFilePath, QString treatmentMachineType="");

  /// Check for collisions and update UI to indicate result
  void checkForCollisions();

  void updateTreatmentOrientationMarker();

  void setFixedReferenceCameraEnabled(bool);

protected slots:
  void onLoadTreatmentMachineButtonClicked();

  void onCollimatorRotationSliderValueChanged(double);
  void onGantryRotationSliderValueChanged(double);
  void onImagingPanelMovementSliderValueChanged(double);
  void onPatientSupportRotationSliderValueChanged(double);
  void onVerticalTableTopDisplacementSliderValueChanged(double);
  void onLongitudinalTableTopDisplacementSliderValueChanged(double);
  void onLateralTableTopDisplacementSliderValueChanged(double);

  void onBeamsEyeViewButtonClicked();

  void onBeamNodeChanged(vtkMRMLNode*);
  void onPatientBodySegmentationNodeChanged(vtkMRMLNode*);
  void onPatientBodySegmentChanged(QString);

  void onLogicModified();

protected:
  QScopedPointer<qSlicerRoomsEyeViewModuleWidgetPrivate> d_ptr;

protected:
  void setup() override;
  void onEnter();

private:
  Q_DECLARE_PRIVATE(qSlicerRoomsEyeViewModuleWidget);
  Q_DISABLE_COPY(qSlicerRoomsEyeViewModuleWidget);
};

#endif
