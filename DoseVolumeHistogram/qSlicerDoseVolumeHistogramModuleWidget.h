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

#ifndef __qSlicerDoseVolumeHistogramModuleWidget_h
#define __qSlicerDoseVolumeHistogramModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDoseVolumeHistogramModuleExport.h"

class qSlicerDoseVolumeHistogramModuleWidgetPrivate;
class vtkMRMLNode;
class QLineEdit;
class QItemSelection;

/// \ingroup SlicerRt_QtModules_DoseVolumeHistogram
class Q_SLICER_QTMODULES_DOSEVOLUMEHISTOGRAM_EXPORT qSlicerDoseVolumeHistogramModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDoseVolumeHistogramModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDoseVolumeHistogramModuleWidget();

  virtual void enter();

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setDoseVolumeHistogramNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void segmentationNodeChanged(vtkMRMLNode*);
  void segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
  void chartNodeChanged(vtkMRMLNode*);
  void automaticOversampingCheckedStateChanged(int aState);

  /// Updates button states
  void updateButtonsState();

  void computeDvhClicked();
  void showInChartCheckStateChanged(int aState);
  void exportDvhToCsvClicked();
  void exportMetricsToCsv();
  void showHideAllCheckedStateChanged(int aState);
  void lineEditVDoseEdited(QString aText);
  void showVMetricsCcCheckedStateChanged(int aState);
  void showVMetricsPercentCheckedStateChanged(int aState);
  void lineEditDVolumeCcEdited(QString aText);
  void lineEditDVolumePercentEdited(QString aText);
  void showDMetricsCheckedStateChanged(int aState);
  void showDoseVolumesOnlyCheckboxChanged(int aState);
  void switchToFourUpQuantitativeLayout();
  void switchToOneUpQuantitativeLayout();

  void onLogicModified();

  void onProgressUpdated(vtkObject*, void*, unsigned long, void*);

protected:
  QScopedPointer<qSlicerDoseVolumeHistogramModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  void onEnter();

protected:
  /// Updates state of show/hide chart checkboxes according to the currently selected chart
  void updateChartCheckboxesState();

  /// Refresh DVH statistics table
  /// \param force Flag indicating if refresh is to be done in any case
  void refreshDvhTable(bool force=false);

  /// Get value list from text in given line edit (empty list if unsuccessful)
  void getNumbersFromLineEdit(QLineEdit* aLineEdit, std::vector<double> &aValues);

private:
  Q_DECLARE_PRIVATE(qSlicerDoseVolumeHistogramModuleWidget);
  Q_DISABLE_COPY(qSlicerDoseVolumeHistogramModuleWidget);
};

#endif
