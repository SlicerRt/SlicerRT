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

// Qt includes
#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerDoseVolumeHistogramModuleWidget.h"
#include "ui_qSlicerDoseVolumeHistogramModule.h"

// DoseVolumeHistogram includes
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLDoubleArrayNode.h>

// VTK includes
#include <vtkStringArray.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <set>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseVolumeHistogram
class qSlicerDoseVolumeHistogramModuleWidgetPrivate: public Ui_qSlicerDoseVolumeHistogramModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseVolumeHistogramModuleWidget);
protected:
  qSlicerDoseVolumeHistogramModuleWidget* const q_ptr;
public:
  qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget &object);
  ~qSlicerDoseVolumeHistogramModuleWidgetPrivate();
  vtkSlicerDoseVolumeHistogramModuleLogic* logic() const;
public:
  /// Map that associates a string pair containing the structure set plot name (including table row number) and the vtkMRMLDoubleArrayNode id (respectively) to the show/hide in chart checkboxes
  QMap<QCheckBox*, QPair<QString, QString> > ChartCheckboxToStructureSetNameMap;

  /// Flag whether show/hide all checkbox has been clicked - some operations are not necessary when it was clicked
  bool ShowHideAllClicked;
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget& object)
 : q_ptr(&object)
{
  this->ChartCheckboxToStructureSetNameMap.clear();
  this->ShowHideAllClicked = false;
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::~qSlicerDoseVolumeHistogramModuleWidgetPrivate()
{
  this->ChartCheckboxToStructureSetNameMap.clear();
}

//-----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic*
qSlicerDoseVolumeHistogramModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseVolumeHistogramModuleWidget);
  return vtkSlicerDoseVolumeHistogramModuleLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::qSlicerDoseVolumeHistogramModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseVolumeHistogramModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidget::~qSlicerDoseVolumeHistogramModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetDoseVolumeHistogramNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLDoseVolumeHistogramNode");
    if (node)
    {
      this->setDoseVolumeHistogramNode( vtkMRMLDoseVolumeHistogramNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    return;
  }

  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLDoseVolumeHistogramNode");
    if (node)
    {
      paramNode = vtkMRMLDoseVolumeHistogramNode::SafeDownCast(node);
      d->logic()->SetAndObserveDoseVolumeHistogramNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode> newNode = vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveDoseVolumeHistogramNode(newNode);
    }
  }

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setDoseVolumeHistogramNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = vtkMRMLDoseVolumeHistogramNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetDoseVolumeHistogramNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveDoseVolumeHistogramNode(paramNode);
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetDoseVolumeNodeId() && strcmp(paramNode->GetDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNodeId());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }
    if (paramNode->GetStructureSetModelNodeId() && strcmp(paramNode->GetStructureSetModelNodeId(),""))
    {
      d->MRMLNodeComboBox_StructureSet->setCurrentNode(paramNode->GetStructureSetModelNodeId());
    }
    else
    {
      this->structureSetNodeChanged(d->MRMLNodeComboBox_StructureSet->currentNode());
    }
    if (paramNode->GetChartNodeId() && strcmp(paramNode->GetChartNodeId(),""))
    {
      d->MRMLNodeComboBox_Chart->setCurrentNode(paramNode->GetChartNodeId());
    }
    else
    {
      this->chartNodeChanged(d->MRMLNodeComboBox_Chart->currentNode());
    }
    d->lineEdit_VDose->setText(paramNode->GetVDoseValues());
    d->checkBox_ShowVMetricsCc->setChecked(paramNode->GetShowVMetricsCc());
    d->checkBox_ShowVMetricsPercent->setChecked(paramNode->GetShowVMetricsPercent());
    d->lineEdit_DVolumeCc->setText(paramNode->GetDVolumeValuesCc());
    d->lineEdit_DVolumePercent->setText(paramNode->GetDVolumeValuesPercent());
    d->checkBox_ShowDMetrics->setChecked(paramNode->GetShowDMetrics());
    d->checkBox_SaveLabelmaps->setChecked(paramNode->GetSaveLabelmaps());
  }

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setup()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Hide widgets whose functions have not been implemented yet
  d->label_ImportCSV->setVisible(false);
  d->pushButton_ImportCSV->setVisible(false);

  d->tableWidget_ChartStatistics->setSortingEnabled(false);
  d->tableWidget_ChartStatistics->setSelectionMode(QAbstractItemView::NoSelection);

  d->label_NotDoseVolumeWarning->setText("");

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setDoseVolumeHistogramNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_StructureSet, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( structureSetNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_Chart, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( chartNodeChanged(vtkMRMLNode*) ) );

  connect( d->pushButton_ComputeDVH, SIGNAL( clicked() ), this, SLOT( computeDvhClicked() ) );
  connect( d->pushButton_ExportDvhToCsv, SIGNAL( clicked() ), this, SLOT( exportDvhToCsvClicked() ) );
  connect( d->pushButton_ExportMetricsToCsv, SIGNAL( clicked() ), this, SLOT( exportMetricsToCsv() ) );
  connect( d->checkBox_ShowHideAll, SIGNAL( stateChanged(int) ), this, SLOT( showHideAllCheckedStateChanged(int) ) );
  connect( d->lineEdit_VDose, SIGNAL( textEdited(QString) ), this, SLOT( lineEditVDoseEdited(QString) ) );
  connect( d->checkBox_ShowVMetricsCc, SIGNAL( stateChanged(int) ), this, SLOT( showVMetricsCcCheckedStateChanged(int) ) );
  connect( d->checkBox_ShowVMetricsPercent, SIGNAL( stateChanged(int) ), this, SLOT( showVMetricsPercentCheckedStateChanged(int) ) );
  connect( d->lineEdit_DVolumeCc, SIGNAL( textEdited(QString) ), this, SLOT( lineEditDVolumeCcEdited(QString) ) );
  connect( d->lineEdit_DVolumePercent, SIGNAL( textEdited(QString) ), this, SLOT( lineEditDVolumePercentEdited(QString) ) );
  connect( d->checkBox_ShowDMetrics, SIGNAL( stateChanged(int) ), this, SLOT( showDMetricsCheckedStateChanged(int) ) );
  connect( d->checkBox_SaveLabelmaps, SIGNAL( stateChanged(int) ), this, SLOT( saveLabelmapsCheckedStateChanged(int) ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (paramNode)
  {
    // Enable/disable ComputeDVH button
    bool dvhCanBeComputed = paramNode->GetDoseVolumeNodeId()
                     && strcmp(paramNode->GetDoseVolumeNodeId(), "")
                     && paramNode->GetStructureSetModelNodeId()
                     && strcmp(paramNode->GetStructureSetModelNodeId(), "");
    d->pushButton_ComputeDVH->setEnabled(dvhCanBeComputed);

    // Enable/disable Export DVH to file button
    bool dvhCanBeExported = false;
    if (paramNode->GetChartNodeId() && strcmp(paramNode->GetChartNodeId(),""))
    {
      for (std::vector<bool>::iterator stateIt=paramNode->GetShowInChartCheckStates()->begin();
        stateIt!=paramNode->GetShowInChartCheckStates()->end(); ++stateIt)
      {
        if (*stateIt)
        {
          dvhCanBeExported = true;
          break;
        }
      }
      d->pushButton_ExportDvhToCsv->setToolTip( dvhCanBeExported ? tr("Export DVH values from the selected structures in the selected chart to CSV file") :
        tr("Only the shown structures are saved in the file. No structures are selected, so none can be saved!"));
    }
    else
    {
      d->pushButton_ExportDvhToCsv->setToolTip(tr("Chart node needs to be selected first before saving DVH values to file!"));
    }
    d->pushButton_ExportDvhToCsv->setEnabled(dvhCanBeExported);

    // Enable/disable Export metrics button
    bool dvhMetricsCanBeExported = (paramNode->GetDvhDoubleArrayNodeIds()->size() > 0);
    d->pushButton_ExportMetricsToCsv->setEnabled(dvhMetricsCanBeExported);
  }

  // Enable/disable V and D metric control
  bool vdMetricCanBeShown = (d->tableWidget_ChartStatistics->rowCount() > 0);
  d->checkBox_ShowVMetricsCc->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowVMetricsPercent->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowDMetrics->setEnabled(vdMetricCanBeShown);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateChartCheckboxesState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetChartNodeId()));

  // If there is no chart node selected, disable all checkboxes
  if (chartNode == NULL)
  {
    QMapIterator<QCheckBox*, QPair<QString, QString> > it(d->ChartCheckboxToStructureSetNameMap);
    while (it.hasNext())
    {
      it.next();
      it.key()->setEnabled(false);
    }
    d->checkBox_ShowHideAll->setEnabled(false);

    return;
  }

  vtkStringArray* arraysInSelectedChart = chartNode->GetArrays();
  paramNode->GetShowInChartCheckStates()->clear();

  QMapIterator<QCheckBox*, QPair<QString, QString> > it(d->ChartCheckboxToStructureSetNameMap);
  while (it.hasNext())
  {
    it.next();

    it.key()->setEnabled(true);
    it.key()->blockSignals(true); // block signals for the checkboxes so that changing it do not toggle the visibility of the plot
    it.key()->setChecked(false);

    for (int i=0; i<arraysInSelectedChart->GetNumberOfValues(); ++i)
    {
      if (arraysInSelectedChart->GetValue(i).compare(it.value().second.toLatin1()) == 0)
      {
        it.key()->setChecked(true);
        break;
      }
    }

    paramNode->GetShowInChartCheckStates()->push_back(it.key()->isChecked());

    it.key()->blockSignals(false); // unblock signal for the checkbox in question
  }

  // Change show/hide all checkbox state
  d->checkBox_ShowHideAll->setEnabled(true);
  d->checkBox_ShowHideAll->blockSignals(true);
  if (arraysInSelectedChart->GetNumberOfValues() == 0)
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::Unchecked);
    paramNode->DisableModifiedEventOn();
    paramNode->SetShowHideAll(Qt::Unchecked);
    paramNode->DisableModifiedEventOff();
  }
  else if (arraysInSelectedChart->GetNumberOfValues() == d->ChartCheckboxToStructureSetNameMap.size())
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
    paramNode->DisableModifiedEventOn();
    paramNode->SetShowHideAll(Qt::Checked);
    paramNode->DisableModifiedEventOff();
  }
  else
  {
    d->checkBox_ShowHideAll->setCheckState(Qt::PartiallyChecked);
    paramNode->DisableModifiedEventOn();
    paramNode->SetShowHideAll(Qt::PartiallyChecked);
    paramNode->DisableModifiedEventOff();
  }
  d->checkBox_ShowHideAll->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();

  if (d->logic()->DoseVolumeContainsDose())
  {
    d->label_NotDoseVolumeWarning->setText("");
  }
  else
  {
    d->label_NotDoseVolumeWarning->setText(tr(" Selected volume is not a dose"));
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::structureSetNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveStructureSetModelNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::chartNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveChartNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onLogicModified()
{
  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::refreshDvhTable()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // Clear the table
  d->tableWidget_ChartStatistics->setRowCount(0);
  d->tableWidget_ChartStatistics->setColumnCount(0);
  d->tableWidget_ChartStatistics->clearContents();

  QMapIterator<QCheckBox*, QPair<QString, QString> > it(d->ChartCheckboxToStructureSetNameMap);
  while (it.hasNext())
  {
    it.next();

    QCheckBox* checkbox = it.key();
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );
    delete checkbox;
  }

  d->ChartCheckboxToStructureSetNameMap.clear();

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  std::vector<std::string>* dvhNodes = paramNode->GetDvhDoubleArrayNodeIds();


  if (dvhNodes->size() < 1)
  {
    return;
  }

  std::vector<std::string> metricList;
  d->logic()->CollectMetricsForDvhNodes(dvhNodes, metricList);

  // Get requested V metrics
  std::vector<double> vDoseValues;
  if (d->checkBox_ShowVMetricsCc->isChecked() || d->checkBox_ShowVMetricsPercent->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValues);
  }
  int vColumnCount = (d->checkBox_ShowVMetricsCc->isChecked() ? vDoseValues.size() : 0)
    + (d->checkBox_ShowVMetricsPercent->isChecked() ? vDoseValues.size() : 0);

  // Get requested D metrics
  std::vector<double> dVolumeValuesCc;
  std::vector<double> dVolumeValuesPercent;
  if (d->checkBox_ShowDMetrics->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_DVolumeCc, dVolumeValuesCc);
    getNumbersFromLineEdit(d->lineEdit_DVolumePercent, dVolumeValuesPercent);
  }

  // Set up the table
  d->tableWidget_ChartStatistics->setColumnCount(3 + metricList.size()
    + vColumnCount + dVolumeValuesCc.size() + dVolumeValuesPercent.size());
  QStringList headerLabels;
  headerLabels << "" << "Structure" << "Volume name";
  for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
  {
    QString metricName(it->c_str());
    metricName = metricName.right( metricName.length()
      - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.size() );
    headerLabels << metricName;
  }
  for (std::vector<double>::iterator it = vDoseValues.begin(); it != vDoseValues.end(); ++it)
  {
    if (d->checkBox_ShowVMetricsCc->isChecked())
    {
      QString metricName = QString("V%1 (cc)").arg(*it);
      headerLabels << metricName;
      d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
    }
    if (d->checkBox_ShowVMetricsPercent->isChecked())
    {
      QString metricName = QString("V%1 (%)").arg(*it);
      headerLabels << metricName;
      d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
    }
  }
  for (std::vector<double>::iterator it = dVolumeValuesCc.begin(); it != dVolumeValuesCc.end(); ++it)
  {
    QString metricName = QString("D%1cc (Gy)").arg(*it);
    headerLabels << metricName;
    d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
  }
  for (std::vector<double>::iterator it = dVolumeValuesPercent.begin(); it != dVolumeValuesPercent.end(); ++it)
  {
    QString metricName = QString("D%1% (Gy)").arg(*it);
    headerLabels << metricName;
    d->tableWidget_ChartStatistics->setColumnWidth(headerLabels.size()-1, 64);
  }
  d->tableWidget_ChartStatistics->setColumnWidth(0, 24);
  d->tableWidget_ChartStatistics->setHorizontalHeaderLabels(headerLabels);
  d->tableWidget_ChartStatistics->setRowCount(dvhNodes->size());

  // Fill the table
  std::vector<std::string>::iterator dvhIt;
  int i;
  QList<QString> structureNames;
  for (i=0, dvhIt = dvhNodes->begin(); dvhIt != dvhNodes->end(); ++dvhIt, ++i)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      this->mrmlScene()->GetNodeByID(dvhIt->c_str()));
    if (!dvhNode)
    {
      continue;
    }

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_ChartStatistics);
    checkbox->setToolTip(tr("Show/hide DVH plot of structure '%1' in selected chart").arg(
      QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ));
    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );

    // Assign line style and plot name
    QString plotName( dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) );
    int numberOfStructuresWithSameName = structureNames.count(plotName);
    structureNames << plotName;

    plotName.append( QString(" (%1)").arg(i+1) );

    if (numberOfStructuresWithSameName % 4 == 1)
    {
      dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dashed");
      plotName.append( " [- -]" );
    }
    else if (numberOfStructuresWithSameName % 4 == 2)
    {
      dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dotted");
      plotName.append( " [...]" );
    }
    else if (numberOfStructuresWithSameName % 4 == 3)
    {
      dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dashed-dotted");
      plotName.append( " [-.-]" );
    }
    else
    {
      dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "solid");
    }

    // Store checkbox with the augmented structure set name and the double array ID
    dvhNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME.c_str(), plotName.toLatin1());
    d->ChartCheckboxToStructureSetNameMap[checkbox] = QPair<QString, QString>(plotName, dvhNode->GetID());

    d->tableWidget_ChartStatistics->setCellWidget(i, 0, checkbox);

    d->tableWidget_ChartStatistics->setItem(i, 1, new QTableWidgetItem(
      QString(dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ));    

    vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast( this->mrmlScene()->GetNodeByID(
      dvhNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME.c_str()) ) );
    if (volumeNode)
    {
      d->tableWidget_ChartStatistics->setItem(i, 2, new QTableWidgetItem( QString(volumeNode->GetName()) ));    
    }

    // Add default metric values
    int col = 3;
    for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
    {
      std::vector<std::string> attributeNames = dvhNode->GetAttributeNames();
      std::string foundAttributeName;
      for (std::vector<std::string>::iterator attributeIt = attributeNames.begin(); attributeIt != attributeNames.end(); ++attributeIt)
      {
        if (vtksys::SystemTools::LowerCase(*attributeIt).compare( vtksys::SystemTools::LowerCase(*it) ) == 0)
        {
          foundAttributeName = *attributeIt;
        }
      }
      QString metricValue( dvhNode->GetAttribute(foundAttributeName.c_str()) );
      if (foundAttributeName.empty() || metricValue.isEmpty())
      {
        ++col;
        continue;
      }

      d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
      ++col;
    }

    // Add V metric values
    if (vDoseValues.size() > 0)
    {
      std::vector<double> volumes;
      std::vector<double> percents;
      d->logic()->ComputeVMetrics(dvhNode, vDoseValues, volumes, percents);
      if (volumes.size() != percents.size())
      {
        std::cerr << "Error: V metric result mismatch!" << std::endl;
        continue;
      }
      col = 3 + metricList.size();

      for (unsigned int j=0; j<volumes.size(); ++j)
      {
        if (d->checkBox_ShowVMetricsCc->isChecked())
        {
          QString metricValue;
          metricValue.setNum(volumes[j], 'f', 2);
          d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
          ++col;
        }
        if (d->checkBox_ShowVMetricsPercent->isChecked())
        {
          QString metricValue;
          metricValue.setNum(percents[j], 'f', 2);
          d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
          ++col;
        }
      }
    }

    // Add D metric values
    if (dVolumeValuesCc.size() > 0)
    {
      std::vector<double> doses;
      d->logic()->ComputeDMetrics(dvhNode, dVolumeValuesCc, doses, false);
      col = 3 + metricList.size() + vColumnCount;
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        QString metricValue;
        metricValue.setNum((*it), 'f', 2);
        d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
        ++col;
      }
    }
    if (dVolumeValuesPercent.size() > 0)
    {
      std::vector<double> doses;
      d->logic()->ComputeDMetrics(dvhNode, dVolumeValuesPercent, doses, true);
      col = 3 + metricList.size() + vColumnCount + dVolumeValuesCc.size();
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        QString metricValue;
        metricValue.setNum((*it), 'f', 2);
        d->tableWidget_ChartStatistics->setItem(i, col, new QTableWidgetItem(metricValue));
        ++col;
      }
    }
  }

  updateButtonsState();
  updateChartCheckboxesState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::computeDvhClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Compute the DVH for the selected structure set using the selected dose volume
  d->logic()->ComputeDvh();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showInChartCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());
  if (!senderCheckbox)
  {
    std::cerr << "Error: Invalid sender checkbox for show/hide in chart checkbox state change" << std::endl;
    return;
  }

  if (aState)
  {
    d->logic()->AddDvhToSelectedChart(d->ChartCheckboxToStructureSetNameMap[senderCheckbox].first.toLatin1(), d->ChartCheckboxToStructureSetNameMap[senderCheckbox].second.toLatin1());
  }
  else
  {
    d->logic()->RemoveDvhFromSelectedChart(d->ChartCheckboxToStructureSetNameMap[senderCheckbox].first.toLatin1());
  }

  if (!d->ShowHideAllClicked)
  {
    // Update states vector
    QMap<QCheckBox*, QPair<QString, QString> >::const_iterator checkboxIt;
    std::vector<bool>::iterator stateIt;
    for (checkboxIt=d->ChartCheckboxToStructureSetNameMap.begin(),
      stateIt=paramNode->GetShowInChartCheckStates()->begin();
      checkboxIt!=d->ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt, ++stateIt)
    {
      if (checkboxIt.key() == senderCheckbox)
      {
        (*stateIt) = (bool)aState;
      }
    }

    // Change show/hide all checkbox state
    d->checkBox_ShowHideAll->blockSignals(true);
    bool isThereChecked = false;
    bool isThereUnchecked = false;
    QMapIterator<QCheckBox*, QPair<QString, QString> > it(d->ChartCheckboxToStructureSetNameMap);
    while (it.hasNext())
    {
      it.next();

      if (it.key()->isChecked())
      {
        isThereChecked = true;
      }
      else
      {
        isThereUnchecked = true;
      }
    }
    if (isThereChecked && isThereUnchecked)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::PartiallyChecked);
    }
    else if (isThereChecked && !isThereUnchecked)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
    }
    else
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Unchecked);
    }
    d->checkBox_ShowHideAll->blockSignals(false);
  }

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showHideAllCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowHideAll(aState);
  paramNode->DisableModifiedEventOff();

  QMap<QCheckBox*, QPair<QString, QString> >::const_iterator checkboxIt;

  d->ShowHideAllClicked = true;

  if (aState == Qt::PartiallyChecked)
  {
    std::vector<bool>::iterator stateIt;

    bool noneIsOn = true;
    bool noneIsOff = true;
    for (stateIt=paramNode->GetShowInChartCheckStates()->begin(); stateIt!=paramNode->GetShowInChartCheckStates()->end(); ++stateIt)
    {
      if (*stateIt)
      {
        noneIsOn = false;
      }
      else
      {
        noneIsOff = false;
      }
    }
    // If all the states are the same then advance to Checked state
    if (noneIsOn || noneIsOff)
    {
      d->checkBox_ShowHideAll->setCheckState(Qt::Checked);
      return;
    }
    else // Else set the states one by one and leave it PartiallyChecked
    {
      for (checkboxIt=d->ChartCheckboxToStructureSetNameMap.begin(),
        stateIt=paramNode->GetShowInChartCheckStates()->begin();
        checkboxIt!=d->ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt, ++stateIt)
      {
        checkboxIt.key()->setChecked(*stateIt);
      }
    }
  }
  else
  {
    bool state = (aState==Qt::Checked ? true : false);
    for (checkboxIt=d->ChartCheckboxToStructureSetNameMap.begin(); checkboxIt!=d->ChartCheckboxToStructureSetNameMap.end(); ++checkboxIt)
    {
      checkboxIt.key()->setChecked(state);
    }
  }

  // Workaround for a bug
  d->checkBox_ShowHideAll->blockSignals(true);
  d->checkBox_ShowHideAll->setCheckState((Qt::CheckState)aState);
  d->checkBox_ShowHideAll->blockSignals(false);

  d->ShowHideAllClicked = false;
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportDvhToCsvClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // User selects file and format
  QString selectedFilter;

	QString fileName = QFileDialog::getSaveFileName( NULL, QString( tr( "Save DVH values to CSV file" ) ), tr(""), QString( tr( "CSV with COMMA ( *.csv );;CSV with TAB ( *.csv )" ) ), &selectedFilter );
	if ( fileName.isNull() )
	{
		return;
	}

	bool comma = true;
	if ( selectedFilter.compare( "CSV with TAB ( *.csv )" ) == 0 )
	{
		comma = false;
	}

  // Export
  if (! d->logic()->ExportDvhToCsv(fileName.toAscii().data(), comma) )
  {
    std::cerr << "Error occurred while exporting DVH to CSV!" << std::endl;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::exportMetricsToCsv()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // User selects file and format
  QString selectedFilter;

	QString fileName = QFileDialog::getSaveFileName( NULL, QString( tr( "Save DVH metrics to CSV file" ) ), tr(""), QString( tr( "CSV with COMMA ( *.csv );;CSV with TAB ( *.csv )" ) ), &selectedFilter );
	if ( fileName.isNull() )
	{
		return;
	}

	bool comma = true;
	if ( selectedFilter.compare( "CSV with TAB ( *.csv )" ) == 0 )
	{
		comma = false;
	}

  // Get requested V metrics
  std::vector<double> vDoseValuesCc;
  if (d->checkBox_ShowVMetricsCc->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValuesCc);
  }
  std::vector<double> vDoseValuesPercent;
  if (d->checkBox_ShowVMetricsPercent->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValuesPercent);
  }

  // Get requested D metrics
  std::vector<double> dVolumeValuesCc;
  std::vector<double> dVolumeValuesPercent;
  if (d->checkBox_ShowDMetrics->isChecked())
  {
    getNumbersFromLineEdit(d->lineEdit_DVolumeCc, dVolumeValuesCc);
    getNumbersFromLineEdit(d->lineEdit_DVolumePercent, dVolumeValuesPercent);
  }

  // Export
  if (! d->logic()->ExportDvhMetricsToCsv(fileName.toAscii().data(), vDoseValuesCc, vDoseValuesPercent, dVolumeValuesCc, dVolumeValuesPercent, comma) )
  {
    std::cerr << "Error occurred while exporting DVH metrics to CSV!" << std::endl;
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::getNumbersFromLineEdit(QLineEdit* aLineEdit, std::vector<double> &aValues)
{
  aValues.clear();

  if (!aLineEdit)
  {
    return;
  }

  QStringList valuesStringList = aLineEdit->text().split(',');
  QStringListIterator it(valuesStringList);
  while (it.hasNext())
  {
    QString nextValue( it.next() );

    bool toDoubleOk;
    double value = nextValue.toDouble(&toDoubleOk);
    if (toDoubleOk)
    {
      aValues.push_back(value);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditVDoseEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetVDoseValues(aText.toLatin1());
  paramNode->DisableModifiedEventOff();

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showVMetricsCcCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowVMetricsCc(aState);
  paramNode->DisableModifiedEventOff();

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showVMetricsPercentCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowVMetricsPercent(aState);
  paramNode->DisableModifiedEventOff();

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumeCcEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDVolumeValuesCc(aText.toLatin1());
  paramNode->DisableModifiedEventOff();

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumePercentEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDVolumeValuesPercent(aText.toLatin1());
  paramNode->DisableModifiedEventOff();

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showDMetricsCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowDMetrics(aState);
  paramNode->DisableModifiedEventOff();

  refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::saveLabelmapsCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetSaveLabelmaps(aState);
  paramNode->DisableModifiedEventOff();
}
