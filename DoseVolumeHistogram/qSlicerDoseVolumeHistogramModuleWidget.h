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

==============================================================================*/

#ifndef __qSlicerDoseVolumeHistogramModuleWidget_h
#define __qSlicerDoseVolumeHistogramModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerDoseVolumeHistogramModuleExport.h"

// STD includes
#include <map>

class qSlicerDoseVolumeHistogramModuleWidgetPrivate;
class vtkMRMLNode;
class QCheckBox;
class QTimer;
class QLineEdit;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_DOSEVOLUMEHISTOGRAM_EXPORT qSlicerDoseVolumeHistogramModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerDoseVolumeHistogramModuleWidget(QWidget *parent=0);
  virtual ~qSlicerDoseVolumeHistogramModuleWidget();

public slots:

protected slots:
  void doseVolumeNodeChanged(vtkMRMLNode*);
  void structureSetNodeChanged(vtkMRMLNode*);
  void chartNodeChanged(vtkMRMLNode*);

  void computeDvhClicked();
  void showInChartCheckStateChanged(int aState);
  void exportDvhToCsvClicked();
  void exportMetricsToCsv();
  void showHideAllCheckedStateChanged(int aState);
  void showVMetricsCheckedStateChanged(int aState);
  void lineEditVDoseEdited(QString aText);
  void showDMetricsCheckedStateChanged(int aState);
  void lineEditDVolumeEdited(QString aText);

  void checkSceneChange();

protected:
  QScopedPointer<qSlicerDoseVolumeHistogramModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

protected:
  /// Updates button states
  void updateButtonsState();

  /// Updates state of show/hide chart checkboxes according to the currently selected chart
  void updateChartCheckboxesState();

  /// Refresh DVH statistics table
  void refreshDvhTable();

  /// Get value list from text in given line edit (empty list if unsuccessful)
  void getNumbersFromLineEdit(QLineEdit* aLineEdit, std::vector<double> &aValues);

protected:
  /// Map that associates a string pair containing the structure set plot name (including table row number) and the vtkMRMLDoubleArrayNode id (respectively) to the show/hide in chart checkboxes
  std::map<QCheckBox*, std::pair<std::string, std::string>> m_ChartCheckboxToStructureSetNameMap;

  /// Vector of checkbox states for the case the user makes the show/hide all checkbox state partially checked. Then the last configuration is restored
  std::vector<bool> m_ShowInChartCheckStates;

  /// Flag whether show/hide all checkbox has been clicked - some operations are not necessary when it was clicked
  bool m_ShowHideAllClicked;

  /// Timer that invokes checking if the scene has recently changed (refresh DVH table needed)
  QTimer* m_CheckSceneChangeTimer; 

private:
  Q_DECLARE_PRIVATE(qSlicerDoseVolumeHistogramModuleWidget);
  Q_DISABLE_COPY(qSlicerDoseVolumeHistogramModuleWidget);
};

#endif
