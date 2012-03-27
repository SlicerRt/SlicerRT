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
  void exportStatisticsToCsv();

protected:
  QScopedPointer<qSlicerDoseVolumeHistogramModuleWidgetPrivate> d_ptr;
  
  virtual void setup();
  virtual void updateButtonsState();
  virtual void updateChartCheckboxesState();

protected:
  /// Map that associates a string pair containing the structure set name and the vtkMRMLDoubleArrayNode id to the show/hide in chart checkboxes
  std::map<QCheckBox*, std::pair<std::string, std::string>> m_ChartCheckboxToStructureSetNameMap;

private:
  Q_DECLARE_PRIVATE(qSlicerDoseVolumeHistogramModuleWidget);
  Q_DISABLE_COPY(qSlicerDoseVolumeHistogramModuleWidget);
};

#endif
