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

  /// Allows other modules to select input and output nodes in the module's GUI.
  /// There may be multiple node selectors in a module widget, you can select between them
  /// using the role argument.
  /// Context can be specified to make a selection within that node (for example, a markup list
  /// node may contain multiple markups; context can be used to select a specific item).
  /// Returns true if the selection was successful.
  virtual bool setEditedNode(vtkMRMLNode* node, QString role = QString(), QString context = QString());

public slots:
  /// Set the current MRML scene to the widget
  virtual void setMRMLScene(vtkMRMLScene*);

  /// Process loaded scene
  void onSceneImportedEvent();

  /// Set current parameter node
  void setParameterNode(vtkMRMLNode *node);

  /// Update widget GUI from parameter node
  void updateWidgetFromMRML();

protected slots:
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void segmentationNodeChanged(vtkMRMLNode*);
  void segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
  void automaticOversampingCheckedStateChanged(int aState);

  /// Updates button states
  void updateButtonsState();

  void computeDvhClicked();

  void exportDvhToCsvClicked();
  void exportMetricsToCsv();

  void lineEditVDoseEdited(QString aText);
  void showVMetricsCcCheckedStateChanged(int aState);
  void showVMetricsPercentCheckedStateChanged(int aState);
  void lineEditDVolumeCcEdited(QString aText);
  void lineEditDVolumePercentEdited(QString aText);
  void showDMetricsCheckedStateChanged(int aState);
  void showDoseVolumesOnlyCheckboxChanged(int aState);
  void doseSurfaceHistogramCheckboxChanged(int aState);

  void showAllClicked();
  void hideAllClicked();
  void showHideLegendClicked(bool checked);
  void switchToFourUpQuantitativeLayout();
  void switchToOneUpQuantitativeLayout();

  void onLogicModified();

  void onProgressUpdated(vtkObject*, void*, unsigned long, void*);

protected:
  virtual void setup();
  void onEnter();

  void setVisibleAll(bool on);

protected:
  QScopedPointer<qSlicerDoseVolumeHistogramModuleWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDoseVolumeHistogramModuleWidget);
  Q_DISABLE_COPY(qSlicerDoseVolumeHistogramModuleWidget);
};

#endif
