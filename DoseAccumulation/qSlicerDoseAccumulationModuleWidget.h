/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

#ifndef __qSlicerDoseAccumulationModuleWidget_h
#define __qSlicerDoseAccumulationModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDoseAccumulationModuleExport.h"

class qSlicerDoseAccumulationModuleWidgetPrivate;
class vtkMRMLNode;
class QTableWidgetItem;

// STD includes
#include <map>

/// \ingroup SlicerRt_QtModules_DoseAccumulation
class Q_SLICER_QTMODULES_DOSEACCUMULATION_EXPORT qSlicerDoseAccumulationModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDoseAccumulationModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDoseAccumulationModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setDoseAccumulationNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void referenceDoseVolumeNodeChanged(vtkMRMLNode*);

  void accumulatedDoseVolumeNodeChanged(vtkMRMLNode*);

  void onTableItemChanged(QTableWidgetItem* changedItem);
  void storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem);

  void applyClicked();

  void showDoseOnlyChanged(int aState);
  void includeVolumeCheckStateChanged(int aState);

  void onLogicModified();

protected:
  QScopedPointer<qSlicerDoseAccumulationModuleWidgetPrivate> d_ptr;

protected:
  virtual void setup();
  void onEnter();

protected:
  /// Updates button states
  void updateButtonsState();

  /// Refresh volumes table
  void refreshVolumesTable();

  /// Checks the dose units and displays warning if they do not match
  void checkDoseUnitsInSelectedVolumes();

  /// Assemble new output volume base name from the selected input volume names
  void refreshOutputBaseName();

private:
  Q_DECLARE_PRIVATE(qSlicerDoseAccumulationModuleWidget);
  Q_DISABLE_COPY(qSlicerDoseAccumulationModuleWidget);
};

#endif
