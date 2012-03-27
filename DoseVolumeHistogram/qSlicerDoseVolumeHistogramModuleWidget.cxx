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

// Qt includes
#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerDoseVolumeHistogramModuleWidget.h"
#include "ui_qSlicerDoseVolumeHistogramModule.h"

// ModuleTemplate includes
#include "vtkSlicerDoseVolumeHistogramLogic.h"

// MRML includes
#include "vtkMRMLVolumeNode.h"
#include "vtkMRMLChartNode.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerDoseVolumeHistogramModuleWidgetPrivate: public Ui_qSlicerDoseVolumeHistogramModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseVolumeHistogramModuleWidget);
protected:
  qSlicerDoseVolumeHistogramModuleWidget* const q_ptr;
public:
  qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget &object);
  vtkSlicerDoseVolumeHistogramLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget& object)
 : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic*
qSlicerDoseVolumeHistogramModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseVolumeHistogramModuleWidget);
  return vtkSlicerDoseVolumeHistogramLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::qSlicerDoseVolumeHistogramModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseVolumeHistogramModuleWidgetPrivate(*this) )
{
  m_ChartCheckboxToStructureSetNameMap.clear();
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::~qSlicerDoseVolumeHistogramModuleWidget()
{
  m_ChartCheckboxToStructureSetNameMap.clear();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setup()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Hide widgets whose functions have not been implemented yet
  d->pushButton_ExportStatisticsToCsv->setVisible(false);
  d->label_ImportCSV->setVisible(false);
  d->pushButton_ImportCSV->setVisible(false);

  d->tableWidget_ChartStatistics->setSortingEnabled(false);
  d->tableWidget_ChartStatistics->setSelectionMode(QAbstractItemView::NoSelection);

  // Make connections
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_StructureSet, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( structureSetNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_Chart, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( chartNodeChanged(vtkMRMLNode*) ) );

  connect( d->pushButton_ComputeDVH, SIGNAL( clicked() ), this, SLOT( computeDvhClicked() ) );
  connect( d->pushButton_ExportDvhToCsv, SIGNAL( clicked() ), this, SLOT( exportDvhToCsvClicked() ) );
  connect( d->pushButton_ExportStatisticsToCsv, SIGNAL( clicked() ), this, SLOT( exportStatisticsToCsv() ) );
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  bool dvhCanBeComputed=d->logic()->GetDoseVolumeNode() && d->logic()->GetStructureSetModelNode();
  d->pushButton_ComputeDVH->setEnabled(dvhCanBeComputed);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateChartCheckboxesState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  //d->logic()->GetChartNode();

  std::cerr << "ERROR: NOT IMPLEMENTED" << std::endl; // TODO:
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  vtkMRMLVolumeNode* volumeNode = dynamic_cast<vtkMRMLVolumeNode*>(node);
  if (volumeNode)
  {
    d->logic()->SetDoseVolumeNode(volumeNode);
    updateButtonsState();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::structureSetNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->logic()->SetStructureSetModelNode(node);
  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::chartNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  vtkMRMLChartNode* chartNode = dynamic_cast<vtkMRMLChartNode*>(node);
  if (chartNode)
  {
    d->logic()->SetChartNode(chartNode);
    updateButtonsState();
    updateChartCheckboxesState();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::computeDvhClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  std::vector<std::string> names;
  std::vector<std::string> dvhArrayIDs;
  std::vector<double> counts;
  std::vector<double> meanDoses;
  std::vector<double> totalVolumeCCs;
  std::vector<double> maxDoses;
  std::vector<double> minDoses;

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  d->logic()->ComputeDvh(names, dvhArrayIDs, counts, meanDoses, totalVolumeCCs, maxDoses, minDoses);

  // Set up the table
  d->tableWidget_ChartStatistics->setColumnCount(6);
  int rowCount = d->tableWidget_ChartStatistics->rowCount();
  QString doseUnit( d->logic()->GetDoseVolumeNode()->GetAttribute("DoseUnits") );
  QStringList headerLabels;
  headerLabels << "Structure" << "Total volume (cc)" << QString("Mean dose (%1)").arg(doseUnit) << QString("Max dose (%1)").arg(doseUnit) << QString("Min dose (%1)").arg(doseUnit) << "Show in Chart";
  d->tableWidget_ChartStatistics->setHorizontalHeaderLabels(headerLabels);

  QTableWidgetItem* toggleAllInChartHeaderItem = new QTableWidgetItem("Show/hide");
  toggleAllInChartHeaderItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate);
  toggleAllInChartHeaderItem->setToolTip(tr("Click here to show/hide all structures in selected chart. Use the checkboxes below to toggle structures individually"));
  toggleAllInChartHeaderItem->setCheckState(Qt::PartiallyChecked);
  d->tableWidget_ChartStatistics->setHorizontalHeaderItem(5, toggleAllInChartHeaderItem);

  d->tableWidget_ChartStatistics->setRowCount(rowCount + names.size());

  // Fill the table
  for (unsigned int i=0; i<names.size(); ++i)
  {
    d->tableWidget_ChartStatistics->setItem(rowCount+i, 0, new QTableWidgetItem( QString::fromStdString(names[i]) ) );
    d->tableWidget_ChartStatistics->setItem(rowCount+i, 1, new QTableWidgetItem( QString::number(totalVolumeCCs[i]) ) );
    d->tableWidget_ChartStatistics->setItem(rowCount+i, 2, new QTableWidgetItem( QString::number(meanDoses[i],'f',4) ) );
    d->tableWidget_ChartStatistics->setItem(rowCount+i, 3, new QTableWidgetItem( QString::number(maxDoses[i],'f',4) ) );
    d->tableWidget_ChartStatistics->setItem(rowCount+i, 4, new QTableWidgetItem( QString::number(minDoses[i],'f',4) ) );

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_ChartStatistics);
    checkbox->setToolTip(tr("Show/hide DVH plot of structure '%1' in selected chart").arg(QString(names[i].c_str())));
    m_ChartCheckboxToStructureSetNameMap[checkbox] = std::pair<std::string, std::string>(names[i], dvhArrayIDs[i]);
    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );

    d->tableWidget_ChartStatistics->setCellWidget(rowCount+i, 5, checkbox);
  }

  updateButtonsState();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showInChartCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());

  if (!senderCheckbox)
  {
    std::cerr << "Error: Invalid sender checkbox for show/hide in chart checkbox state change" << std::endl;
    return;
  }

  if (aState)
  {
    d->logic()->AddDvhToSelectedChart(m_ChartCheckboxToStructureSetNameMap[senderCheckbox].first.c_str(), m_ChartCheckboxToStructureSetNameMap[senderCheckbox].second.c_str());
  }
  else
  {
    d->logic()->RemoveDvhFromSelectedChart(m_ChartCheckboxToStructureSetNameMap[senderCheckbox].first.c_str());
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportDvhToCsvClicked()
{
  QString filter = QString( tr( "Comma Separated Values (*.csv);;" ) );
  QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save DVH to CSV"), QString("."), filter);

	if (! fileName.isNull() )
  {
    // TODO
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportStatisticsToCsv()
{
  QString filter = QString( tr( "Comma Separated Values (*.csv);;" ) );
  QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save statistics to CSV"), QString("."), filter);

	if (! fileName.isNull() )
  {
    // print comma separated value file with header keys in quotes

    //csv = ""
    //header = ""
    //for k in self.keys[:-1]:
    //  header += "\"%s\"" % k + ","
    //header += "\"%s\"" % self.keys[-1] + "\n"
    //csv = header
    //for i in self.labelStats["Labels"]:
    //  line = ""
    //  for k in self.keys[:-1]:
    //    line += str(self.labelStats[i,k]) + ","
    //  line += str(self.labelStats[i,self.keys[-1]]) + "\n"
    //  csv += line

    //fp = open(fileName, "w")
    //fp.write(csv)
    //fp.close()
  }
}
