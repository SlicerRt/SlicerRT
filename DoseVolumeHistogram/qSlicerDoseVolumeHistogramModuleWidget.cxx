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

// Qt includes
#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>
#include <QProgressDialog>
#include <QMainWindow>

// SlicerRt includes
#include "SlicerRtCommon.h"

// SlicerQt includes
#include "qSlicerDoseVolumeHistogramModuleWidget.h"
#include "ui_qSlicerDoseVolumeHistogramModule.h"

// QSlicer includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h" 

// DoseVolumeHistogram includes
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLLayoutNode.h>

// VTK includes
#include <vtkStringArray.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

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
  /// Map that associates the vtkMRMLDoubleArrayNode id to the show/hide in chart checkboxes
  QMap<QCheckBox*, QString> PlotCheckboxToStructureNameMap;

  /// Flag whether show/hide all checkbox has been clicked - some operations are not necessary when it was clicked
  bool ShowHideAllClicked;
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget& object)
 : q_ptr(&object)
{
  this->PlotCheckboxToStructureNameMap.clear();
  this->ShowHideAllClicked = false;
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::~qSlicerDoseVolumeHistogramModuleWidgetPrivate()
{
  this->PlotCheckboxToStructureNameMap.clear();
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

  if( scene )
  {
    this->updateChartCheckboxesState();
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
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::onEnter: Invalid logic!";
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

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setDoseVolumeHistogramNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = vtkMRMLDoseVolumeHistogramNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetDoseVolumeHistogramNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveDoseVolumeHistogramNode(paramNode);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
    if (paramNode->GetDoseVolumeNode())
    {
      d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNode());
    }
    else
    {
      this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
    }
    if (paramNode->GetContourSetContourNode())
    {
      d->ContourSelectorWidget->setCurrentNode(paramNode->GetContourSetContourNode());
    }
    else
    {
      this->contourSetNodeChanged(d->ContourSelectorWidget->currentNode());
    }
    if (paramNode->GetChartNode())
    {
      d->MRMLNodeComboBox_Chart->setCurrentNode(paramNode->GetChartNode());
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
  }

  this->refreshDvhTable();
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

  d->label_NotDoseVolumeWarning->setVisible(false);
  d->label_Error->setVisible(false);

  d->pushButton_SwitchToFourUpQuantitativeLayout->setEnabled(false);
  d->pushButton_SwitchToOneUpQuantitativeLayout->setEnabled(false);

  // Show only dose volumes in the dose volume combobox by default
  d->MRMLNodeComboBox_DoseVolume->addAttribute( QString("vtkMRMLScalarVolumeNode"), SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());

  // Set up contour selector widget
  d->ContourSelectorWidget->setAcceptContourHierarchies(true);
  d->ContourSelectorWidget->setRequiredRepresentation(vtkMRMLContourNode::IndexedLabelmap);

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( setDoseVolumeHistogramNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->ContourSelectorWidget, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( contourSetNodeChanged(vtkMRMLNode*) ) );
  connect( d->ContourSelectorWidget, SIGNAL( selectionValidityChanged() ), this, SLOT( updateButtonsState() ) );
  connect( d->MRMLNodeComboBox_Chart, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( chartNodeChanged(vtkMRMLNode*) ) );

  connect( d->pushButton_ComputeDVH, SIGNAL( clicked() ), this, SLOT( computeDvhClicked() ) );
  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseVolumesOnlyCheckboxChanged(int) ) );
  connect( d->pushButton_ExportDvhToCsv, SIGNAL( clicked() ), this, SLOT( exportDvhToCsvClicked() ) );
  connect( d->pushButton_ExportMetricsToCsv, SIGNAL( clicked() ), this, SLOT( exportMetricsToCsv() ) );
  connect( d->checkBox_ShowHideAll, SIGNAL( stateChanged(int) ), this, SLOT( showHideAllCheckedStateChanged(int) ) );
  connect( d->lineEdit_VDose, SIGNAL( textEdited(QString) ), this, SLOT( lineEditVDoseEdited(QString) ) );
  connect( d->checkBox_ShowVMetricsCc, SIGNAL( stateChanged(int) ), this, SLOT( showVMetricsCcCheckedStateChanged(int) ) );
  connect( d->checkBox_ShowVMetricsPercent, SIGNAL( stateChanged(int) ), this, SLOT( showVMetricsPercentCheckedStateChanged(int) ) );
  connect( d->lineEdit_DVolumeCc, SIGNAL( textEdited(QString) ), this, SLOT( lineEditDVolumeCcEdited(QString) ) );
  connect( d->lineEdit_DVolumePercent, SIGNAL( textEdited(QString) ), this, SLOT( lineEditDVolumePercentEdited(QString) ) );
  connect( d->checkBox_ShowDMetrics, SIGNAL( stateChanged(int) ), this, SLOT( showDMetricsCheckedStateChanged(int) ) );
  connect( d->pushButton_SwitchToFourUpQuantitativeLayout, SIGNAL( clicked() ), this, SLOT( switchToFourUpQuantitativeLayout() ) );
  connect( d->pushButton_SwitchToOneUpQuantitativeLayout, SIGNAL( clicked() ), this, SLOT( switchToOneUpQuantitativeLayout() ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (paramNode)
  {
    // Enable/disable ComputeDVH button
    bool dvhCanBeComputed = paramNode->GetDoseVolumeNode()
                         && paramNode->GetContourSetContourNode()
                         && d->ContourSelectorWidget->isSelectionValid();
    d->pushButton_ComputeDVH->setEnabled(dvhCanBeComputed);

    // Enable/disable Export DVH to file button
    bool dvhIsPlotted = false;
    if (paramNode->GetChartNode())
    {
      std::vector<bool> checkStates;
      paramNode->GetShowInChartCheckStates(checkStates);
      for (std::vector<bool>::iterator stateIt = checkStates.begin(); stateIt != checkStates.end(); ++stateIt)
      {
        // If at lease one DVH is displayed in the chart view
        if (*stateIt)
        {
          dvhIsPlotted = true;
          break;
        }
      }
      d->pushButton_ExportDvhToCsv->setToolTip( dvhIsPlotted ? tr("Export DVH values from the selected structures in the selected chart to CSV file") :
        tr("Only the shown structures are saved in the file. No structures are selected, so none can be saved!"));
    }
    else
    {
      d->pushButton_ExportDvhToCsv->setToolTip(tr("Chart node needs to be selected first before saving DVH values to file!"));
    }
    d->pushButton_ExportDvhToCsv->setEnabled(dvhIsPlotted);
    d->pushButton_SwitchToFourUpQuantitativeLayout->setEnabled(dvhIsPlotted);
    d->pushButton_SwitchToOneUpQuantitativeLayout->setEnabled(dvhIsPlotted);

    // Enable/disable Export metrics button
    std::vector<vtkMRMLNode*> dvhNodes;
    paramNode->GetDvhDoubleArrayNodes(dvhNodes);
    bool dvhMetricsCanBeExported = (dvhNodes.size() > 0);
    d->pushButton_ExportMetricsToCsv->setEnabled(dvhMetricsCanBeExported);
  }

  // Enable/disable V and D metric control
  bool vdMetricCanBeShown = (d->tableWidget_ChartStatistics->rowCount() > 0);
  d->checkBox_ShowVMetricsCc->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowVMetricsPercent->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowDMetrics->setEnabled(vdMetricCanBeShown);

  d->label_Error->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::updateChartCheckboxesState()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::updateChartCheckboxesState: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  vtkMRMLChartNode* chartNode = paramNode->GetChartNode();

  // If there is no chart node selected, disable all checkboxes
  if (chartNode == NULL)
  {
    QMapIterator<QCheckBox*, QString> it(d->PlotCheckboxToStructureNameMap);
    while (it.hasNext())
    {
      it.next();
      it.key()->setEnabled(false);
    }
    d->checkBox_ShowHideAll->setEnabled(false);

    return;
  }

  // Set state of the checkboxes of the individual structures and create list of these states to store in the parameter set node
  vtkStringArray* arraysInSelectedChart = chartNode->GetArrays();
  std::vector<bool> checkStates;

  QMapIterator<QCheckBox*, QString> it(d->PlotCheckboxToStructureNameMap);
  while (it.hasNext())
  {
    it.next();

    it.key()->setEnabled(true);
    it.key()->blockSignals(true); // block signals for the checkboxes so that changing it do not toggle the visibility of the plot
    it.key()->setChecked(false);

    for (int i=0; i<arraysInSelectedChart->GetNumberOfValues(); ++i)
    {
      // Check checkbox if the plot for the array is present in the chart
      if (arraysInSelectedChart->GetValue(i).compare(it.value().toLatin1().constData()) == 0)
      {
        it.key()->setChecked(true);
        break;
      }
    }

    // Add checkbox state to the list
    checkStates.push_back(it.key()->isChecked());

    it.key()->blockSignals(false); // unblock signal for the checkbox in question
  }

  // Set check states list to the parameter node
  paramNode->SetShowInChartCheckStates(checkStates);

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
  else if (arraysInSelectedChart->GetNumberOfValues() == d->PlotCheckboxToStructureNameMap.size())
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

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::doseVolumeNodeChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();

  d->label_NotDoseVolumeWarning->setVisible(!d->logic()->DoseVolumeContainsDose());

  // Set forced reference volume in contour selector widget to ensure this dose volume is selected as reference
  d->ContourSelectorWidget->setForcedReferenceVolumeNodeID(node->GetID());
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::contourSetNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::contourSetNodeChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveContourSetContourNode(node);
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::chartNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::chartNodeChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveChartNode(vtkMRMLChartNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateChartCheckboxesState();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::refreshDvhTable()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // Clear the table
  d->tableWidget_ChartStatistics->setRowCount(0);
  d->tableWidget_ChartStatistics->setColumnCount(0);
  d->tableWidget_ChartStatistics->clearContents();

  // Clear checkbox to contour set name map
  QMapIterator<QCheckBox*, QString> it(d->PlotCheckboxToStructureNameMap);
  while (it.hasNext())
  {
    it.next();

    QCheckBox* checkbox = it.key();
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );
    delete checkbox;
  }

  d->PlotCheckboxToStructureNameMap.clear();

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  // Get DVH double array nodes for parameter set node
  std::vector<vtkMRMLNode*> dvhNodes;
  paramNode->GetDvhDoubleArrayNodes(dvhNodes);
  if (dvhNodes.size() < 1)
  {
    return;
  }

  // Collect metrics for found DVH nodes from their attributes
  std::vector<std::string> metricList;
  d->logic()->CollectMetricsForDvhNodes(dvhNodes, metricList);

  // Get requested V metrics
  std::vector<double> vDoseValues;
  if (d->checkBox_ShowVMetricsCc->isChecked() || d->checkBox_ShowVMetricsPercent->isChecked())
  {
    this->getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValues);
  }
  int vColumnCount = (d->checkBox_ShowVMetricsCc->isChecked() ? vDoseValues.size() : 0)
    + (d->checkBox_ShowVMetricsPercent->isChecked() ? vDoseValues.size() : 0);

  // Get requested D metrics
  std::vector<double> dVolumeValuesCc;
  std::vector<double> dVolumeValuesPercent;
  if (d->checkBox_ShowDMetrics->isChecked())
  {
    this->getNumbersFromLineEdit(d->lineEdit_DVolumeCc, dVolumeValuesCc);
    this->getNumbersFromLineEdit(d->lineEdit_DVolumePercent, dVolumeValuesPercent);
  }

  // Set up the table columns
  d->tableWidget_ChartStatistics->setColumnCount(3 + metricList.size()
    + vColumnCount + dVolumeValuesCc.size() + dVolumeValuesPercent.size());
  QStringList headerLabels;
  headerLabels << "" << "Structure" << "Volume name";
  for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
  {
    QString metricName(it->c_str());
    metricName = metricName.right( metricName.length()
      - SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.size() );
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
  d->tableWidget_ChartStatistics->setRowCount(dvhNodes.size());

  // Fill the table
  std::vector<vtkMRMLNode*>::iterator dvhIt;
  int dvhIndex = 0;
  for (dvhIt = dvhNodes.begin(); dvhIt != dvhNodes.end(); ++dvhIt, ++dvhIndex)
  {
    if (!(*dvhIt))
    {
      continue;
    }

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_ChartStatistics);
    checkbox->setToolTip(tr("Show/hide DVH plot of structure '%1' in selected chart").arg(
      QString((*dvhIt)->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ));
    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( showInChartCheckStateChanged(int) ) );

    // Store checkbox with the double array ID
    d->PlotCheckboxToStructureNameMap[checkbox] = QString((*dvhIt)->GetID());

    d->tableWidget_ChartStatistics->setCellWidget(dvhIndex, 0, checkbox);

    d->tableWidget_ChartStatistics->setItem(dvhIndex, 1, new QTableWidgetItem(
      QString((*dvhIt)->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str())) ));    

    vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
      (*dvhIt)->GetNodeReference(SlicerRtCommon::DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE.c_str()) );
    if (volumeNode)
    {
      d->tableWidget_ChartStatistics->setItem(dvhIndex, 2, new QTableWidgetItem( QString(volumeNode->GetName()) ));    
    }

    // Add default metric values
    int col = 3;
    for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
    {
      std::vector<std::string> attributeNames = (*dvhIt)->GetAttributeNames();
      std::string foundAttributeName;
      for (std::vector<std::string>::iterator attributeIt = attributeNames.begin(); attributeIt != attributeNames.end(); ++attributeIt)
      {
        if (vtksys::SystemTools::LowerCase(*attributeIt).compare( vtksys::SystemTools::LowerCase(*it) ) == 0)
        {
          foundAttributeName = *attributeIt;
        }
      }
      QString metricValue( (*dvhIt)->GetAttribute(foundAttributeName.c_str()) );
      if (foundAttributeName.empty() || metricValue.isEmpty())
      {
        ++col;
        continue;
      }

      d->tableWidget_ChartStatistics->setItem(dvhIndex, col, new QTableWidgetItem(metricValue));
      ++col;
    }

    // Add V metric values
    if (vDoseValues.size() > 0)
    {
      std::vector<double> volumes;
      std::vector<double> percents;
      d->logic()->ComputeVMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), vDoseValues, volumes, percents);
      if (volumes.size() != percents.size())
      {
        std::cerr << "Error: V metric result mismatch!" << std::endl;
        continue;
      }
      col = 3 + metricList.size();

      for (unsigned int volumeIndex=0; volumeIndex<volumes.size(); ++volumeIndex)
      {
        if (d->checkBox_ShowVMetricsCc->isChecked())
        {
          QString metricValue;
          metricValue.setNum(volumes[volumeIndex], 'f', 2);
          d->tableWidget_ChartStatistics->setItem(dvhIndex, col, new QTableWidgetItem(metricValue));
          ++col;
        }
        if (d->checkBox_ShowVMetricsPercent->isChecked())
        {
          QString metricValue;
          metricValue.setNum(percents[volumeIndex], 'f', 2);
          d->tableWidget_ChartStatistics->setItem(dvhIndex, col, new QTableWidgetItem(metricValue));
          ++col;
        }
      }
    }

    // Add D metric values
    if (dVolumeValuesCc.size() > 0)
    {
      std::vector<double> doses;
      d->logic()->ComputeDMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), dVolumeValuesCc, doses, false);
      col = 3 + metricList.size() + vColumnCount;
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        QString metricValue;
        metricValue.setNum((*it), 'f', 2);
        d->tableWidget_ChartStatistics->setItem(dvhIndex, col, new QTableWidgetItem(metricValue));
        ++col;
      }
    }
    if (dVolumeValuesPercent.size() > 0)
    {
      std::vector<double> doses;
      d->logic()->ComputeDMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), dVolumeValuesPercent, doses, true);
      col = 3 + metricList.size() + vColumnCount + dVolumeValuesCc.size();
      for (std::vector<double>::iterator doseIt = doses.begin(); doseIt != doses.end(); ++doseIt)
      {
        QString metricValue;
        metricValue.setNum((*doseIt), 'f', 2);
        d->tableWidget_ChartStatistics->setItem(dvhIndex, col, new QTableWidgetItem(metricValue));
        ++col;
      }
    }
  }

  d->tableWidget_ChartStatistics->resizeColumnsToContents();

  this->updateChartCheckboxesState();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::computeDvhClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Get selected contours
  std::vector<vtkMRMLContourNode*> structureContourNodes = d->ContourSelectorWidget->selectedContourNodes();

  // Initialize progress bar
  QProgressDialog *convertProgress = new QProgressDialog(qSlicerApplication::application()->mainWindow());
  convertProgress->setModal(true);
  convertProgress->setMinimumDuration(150);
  convertProgress->setLabelText("Computing DVH for all selected contours...");
  convertProgress->show();
  QApplication::processEvents();
  unsigned int numberOfContours = structureContourNodes.size();
  unsigned int currentContour = 0;

  // Compute the DVH for each structure in the selected contour set using the selected dose volume
  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = structureContourNodes.begin(); contourIt != structureContourNodes.end(); ++contourIt)
  {
    std::string errorMessage;
    d->logic()->ComputeDvh((*contourIt), errorMessage);

    if (!errorMessage.empty())
    {
      d->label_Error->setVisible(true);
      d->label_Error->setText( QString(errorMessage.c_str()) );
      break;
    }

    // Set progress
    ++currentContour;
    convertProgress->setValue(currentContour/(double)numberOfContours * 100.0);
  }

  delete convertProgress;
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showInChartCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::showInChartCheckStateChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
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
    d->logic()->AddDvhToSelectedChart(d->PlotCheckboxToStructureNameMap[senderCheckbox].toLatin1().constData());
  }
  else
  {
    d->logic()->RemoveDvhFromSelectedChart(d->PlotCheckboxToStructureNameMap[senderCheckbox].toLatin1().constData());
  }

  if (!d->ShowHideAllClicked)
  {
    std::vector<bool> checkStates;
    paramNode->GetShowInChartCheckStates(checkStates);

    // Update states vector with the one checked state and set it back to the parameter node
    QMap<QCheckBox*, QString>::const_iterator checkboxIt;
    std::vector<bool>::iterator stateIt;
    for (checkboxIt=d->PlotCheckboxToStructureNameMap.begin(), stateIt=checkStates.begin();
      checkboxIt!=d->PlotCheckboxToStructureNameMap.end(); ++checkboxIt, ++stateIt)
    {
      if (checkboxIt.key() == senderCheckbox)
      {
        (*stateIt) = (bool)aState;
        break;
      }
    }
    paramNode->SetShowInChartCheckStates(checkStates);

    // Change show/hide all checkbox state
    d->checkBox_ShowHideAll->blockSignals(true);
    bool isThereChecked = false;
    bool isThereUnchecked = false;
    QMapIterator<QCheckBox*, QString> it(d->PlotCheckboxToStructureNameMap);
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

  this->updateChartCheckboxesState();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showHideAllCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::showHideAllCheckedStateChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowHideAll(aState);
  paramNode->DisableModifiedEventOff();

  QMap<QCheckBox*, QString>::const_iterator checkboxIt;

  d->ShowHideAllClicked = true;

  if (aState == Qt::PartiallyChecked)
  {
    std::vector<bool> checkStates;
    paramNode->GetShowInChartCheckStates(checkStates);
    std::vector<bool>::iterator stateIt;

    bool noneIsOn = true;
    bool noneIsOff = true;
    for (stateIt = checkStates.begin(); stateIt != checkStates.end(); ++stateIt)
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
      for (checkboxIt=d->PlotCheckboxToStructureNameMap.begin(), stateIt=checkStates.begin();
        checkboxIt!=d->PlotCheckboxToStructureNameMap.end(); ++checkboxIt, ++stateIt)
      {
        checkboxIt.key()->setChecked(*stateIt);
      }
    }
  }
  else
  {
    bool state = (aState==Qt::Checked ? true : false);
    for (checkboxIt=d->PlotCheckboxToStructureNameMap.begin(); checkboxIt!=d->PlotCheckboxToStructureNameMap.end(); ++checkboxIt)
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
    this->getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValuesCc);
  }
  std::vector<double> vDoseValuesPercent;
  if (d->checkBox_ShowVMetricsPercent->isChecked())
  {
    this->getNumbersFromLineEdit(d->lineEdit_VDose, vDoseValuesPercent);
  }

  // Get requested D metrics
  std::vector<double> dVolumeValuesCc;
  std::vector<double> dVolumeValuesPercent;
  if (d->checkBox_ShowDMetrics->isChecked())
  {
    this->getNumbersFromLineEdit(d->lineEdit_DVolumeCc, dVolumeValuesCc);
    this->getNumbersFromLineEdit(d->lineEdit_DVolumePercent, dVolumeValuesPercent);
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

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::lineEditVDoseEdited: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetVDoseValues(aText.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showVMetricsCcCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::showVMetricsCcCheckedStateChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowVMetricsCc(aState);
  paramNode->DisableModifiedEventOff();

  this->refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showVMetricsPercentCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::showVMetricsPercentCheckedStateChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowVMetricsPercent(aState);
  paramNode->DisableModifiedEventOff();

  this->refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumeCcEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumeCcEdited: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDVolumeValuesCc(aText.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumePercentEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumePercentEdited: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDVolumeValuesPercent(aText.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showDMetricsCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::showDMetricsCheckedStateChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowDMetrics(aState);
  paramNode->DisableModifiedEventOff();

  this->refreshDvhTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showDoseVolumesOnlyCheckboxChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseVolumeHistogramModuleWidget::showDoseVolumesOnlyCheckboxChanged: Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowDoseVolumesOnly(aState);
  paramNode->DisableModifiedEventOff();

  if (aState)
  {
    d->MRMLNodeComboBox_DoseVolume->addAttribute("vtkMRMLScalarVolumeNode", SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  }
  else
  {
    // The row has to be replaced, so replace with an attribute that matches all desired volumes
    d->MRMLNodeComboBox_DoseVolume->addAttribute("vtkMRMLScalarVolumeNode", "LabelMap", 0);
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::switchToFourUpQuantitativeLayout()
{
  qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutFourUpQuantitativeView);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::switchToOneUpQuantitativeLayout()
{
  qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutOneUpQuantitativeView);
}
