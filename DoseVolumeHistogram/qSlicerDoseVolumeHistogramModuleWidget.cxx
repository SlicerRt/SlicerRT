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

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

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
#include <vtkMRMLTableNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkStringArray.h>
#include <vtkBitArray.h>
#include <vtkTable.h>

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
  /// Flag whether show/hide all checkbox has been clicked - some operations are not necessary when it was clicked

  /// Progress dialog for tracking DVH calculation progress
  QProgressDialog* ConvertProgressDialog;
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::qSlicerDoseVolumeHistogramModuleWidgetPrivate(qSlicerDoseVolumeHistogramModuleWidget& object)
 : q_ptr(&object)
{
  this->ConvertProgressDialog = NULL;
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModuleWidgetPrivate::~qSlicerDoseVolumeHistogramModuleWidgetPrivate()
{
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
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
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

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
  if (paramNode->GetDoseVolumeNode())
  {
    d->MRMLNodeComboBox_DoseVolume->setCurrentNode(paramNode->GetDoseVolumeNode());
  }
  else
  {
    this->doseVolumeNodeChanged(d->MRMLNodeComboBox_DoseVolume->currentNode());
  }
  if (paramNode->GetSegmentationNode())
  {
    d->MRMLNodeComboBox_Segmentation->setCurrentNode(paramNode->GetSegmentationNode());
  }
  else
  {
    this->segmentationNodeChanged(d->MRMLNodeComboBox_Segmentation->currentNode());
  }
  d->lineEdit_VDose->setText(paramNode->GetVDoseValues());
  d->checkBox_ShowVMetricsCc->setChecked(paramNode->GetShowVMetricsCc());
  d->checkBox_ShowVMetricsPercent->setChecked(paramNode->GetShowVMetricsPercent());
  d->lineEdit_DVolumeCc->setText(paramNode->GetDVolumeValuesCc());
  d->lineEdit_DVolumePercent->setText(paramNode->GetDVolumeValuesPercent());
  d->checkBox_ShowDMetrics->setChecked(paramNode->GetShowDMetrics());
  d->checkBox_AutomaticOversampling->setChecked(paramNode->GetAutomaticOversampling());

  // Set metrics table to table view
  if (d->MRMLTableView->mrmlTableNode() != paramNode->GetMetricsTableNode())
  {
    d->MRMLTableView->setMRMLTableNode(paramNode->GetMetricsTableNode());
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setup()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->SegmentsTableView->setMode(qMRMLSegmentsTableView::SimpleListMode);

  d->MRMLTableView->setSelectionMode(QAbstractItemView::NoSelection);

  d->label_NotDoseVolumeWarning->setVisible(false);
  d->label_Error->setVisible(false);

  d->pushButton_ShowAll->setEnabled(false);
  d->pushButton_HideAll->setEnabled(false);
  d->pushButton_SwitchToFourUpQuantitativeLayout->setEnabled(false);
  d->pushButton_SwitchToOneUpQuantitativeLayout->setEnabled(false);

  // Show only dose volumes in the dose volume combobox by default
  d->MRMLNodeComboBox_DoseVolume->addAttribute( QString("vtkMRMLScalarVolumeNode"), SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( setDoseVolumeHistogramNode(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_DoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( doseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->checkBox_AutomaticOversampling, SIGNAL( stateChanged(int) ), this, SLOT( automaticOversampingCheckedStateChanged(int) ) );
  connect( d->MRMLNodeComboBox_Segmentation, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( segmentationNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_Segmentation, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), d->SegmentsTableView, SLOT( setSegmentationNode(vtkMRMLNode*) ) );
  connect( d->SegmentsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(segmentSelectionChanged(QItemSelection,QItemSelection) ) );

  connect( d->pushButton_ComputeDVH, SIGNAL( clicked() ), this, SLOT( computeDvhClicked() ) );
  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseVolumesOnlyCheckboxChanged(int) ) );
  connect( d->pushButton_ExportDvhToCsv, SIGNAL( clicked() ), this, SLOT( exportDvhToCsvClicked() ) );
  connect( d->pushButton_ExportMetricsToCsv, SIGNAL( clicked() ), this, SLOT( exportMetricsToCsv() ) );
  connect( d->lineEdit_VDose, SIGNAL( textEdited(QString) ), this, SLOT( lineEditVDoseEdited(QString) ) );
  connect( d->checkBox_ShowVMetricsCc, SIGNAL( stateChanged(int) ), this, SLOT( showVMetricsCcCheckedStateChanged(int) ) );
  connect( d->checkBox_ShowVMetricsPercent, SIGNAL( stateChanged(int) ), this, SLOT( showVMetricsPercentCheckedStateChanged(int) ) );
  connect( d->lineEdit_DVolumeCc, SIGNAL( textEdited(QString) ), this, SLOT( lineEditDVolumeCcEdited(QString) ) );
  connect( d->lineEdit_DVolumePercent, SIGNAL( textEdited(QString) ), this, SLOT( lineEditDVolumePercentEdited(QString) ) );
  connect( d->checkBox_ShowDMetrics, SIGNAL( stateChanged(int) ), this, SLOT( showDMetricsCheckedStateChanged(int) ) );
  connect( d->pushButton_ShowAll, SIGNAL( clicked() ), this, SLOT( showAllClicked() ) );
  connect( d->pushButton_HideAll, SIGNAL( clicked() ), this, SLOT( hideAllClicked() ) );
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
    // Enable/disable Compute DVH button
    bool dvhCanBeComputed = paramNode->GetDoseVolumeNode()
                         && paramNode->GetSegmentationNode();
    d->pushButton_ComputeDVH->setEnabled(dvhCanBeComputed);

    // Enable/disable visibility related buttons
    bool dvhComputed = paramNode->GetMetricsTableNode()->GetTable()->GetNumberOfRows();
    d->pushButton_ShowAll->setEnabled(dvhComputed);
    d->pushButton_HideAll->setEnabled(dvhComputed);
    d->pushButton_SwitchToFourUpQuantitativeLayout->setEnabled(dvhComputed);
    d->pushButton_SwitchToOneUpQuantitativeLayout->setEnabled(dvhComputed);

    // Enable/disable Export DVH to file button
    /* //TODO:
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

    // Enable/disable Export metrics button
    std::vector<vtkMRMLNode*> dvhNodes;
    paramNode->GetDvhDoubleArrayNodes(dvhNodes);
    bool dvhMetricsCanBeExported = (dvhNodes.size() > 0);
    d->pushButton_ExportMetricsToCsv->setEnabled(dvhMetricsCanBeExported);
    */
  }

  // Enable/disable V and D metric control
  /*
  bool vdMetricCanBeShown = (d->tableWidget_ChartStatistics->rowCount() > 0);
  d->checkBox_ShowVMetricsCc->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowVMetricsPercent->setEnabled(vdMetricCanBeShown);
  d->checkBox_ShowDMetrics->setEnabled(vdMetricCanBeShown);
  */
  d->label_Error->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::doseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::automaticOversampingCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAutomaticOversampling(aState);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::segmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  std::vector<std::string> selectedSegmentIDs;
  foreach (QString segmentId, d->SegmentsTableView->selectedSegmentIDs())
  {
    selectedSegmentIDs.push_back(segmentId.toLatin1().constData());
  }
  paramNode->SetSelectedSegmentIDs(selectedSegmentIDs);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::computeDvhClicked()
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Initialize progress bar
  qvtkConnect( d->logic(), SlicerRtCommon::ProgressUpdated, this, SLOT( onProgressUpdated(vtkObject*,void*,unsigned long,void*) ) );
  d->ConvertProgressDialog = new QProgressDialog(qSlicerApplication::application()->mainWindow());
  d->ConvertProgressDialog->setModal(true);
  d->ConvertProgressDialog->setMinimumDuration(150);
  d->ConvertProgressDialog->setLabelText("Computing DVH for all selected segments...");
  d->ConvertProgressDialog->show();
  QApplication::processEvents();

  // Compute the DVH for each selected segment set using the selected dose volume
  std::string errorMessage = d->logic()->ComputeDvh();
  if (!errorMessage.empty())
  {
    d->label_Error->setVisible(true);
    d->label_Error->setText( QString(errorMessage.c_str()) );
  }

  qvtkDisconnect( d->logic(), SlicerRtCommon::ProgressUpdated, this, SLOT( onProgressUpdated(vtkObject*,void*,unsigned long,void*) ) );
  delete d->ConvertProgressDialog;

  d->MRMLTableView->setFirstRowLocked(true);
  this->updateButtonsState();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::onProgressUpdated(vtkObject* vtkNotUsed(caller), void* callData, unsigned long vtkNotUsed(eid), void* vtkNotUsed(clientData))
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!d->ConvertProgressDialog)
  {
    return;
  }

  double* progress = reinterpret_cast<double*>(callData);
  d->ConvertProgressDialog->setValue((int)((*progress)*100.0));
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
{/* //TODO:
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
  }*/
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditVDoseEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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

  if (!d->logic()->ComputeVMetrics())
  {
    qCritical() << Q_FUNC_INFO << ": Failed to compute DVH metrics!";
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showVMetricsCcCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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

  if (!d->logic()->ComputeVMetrics())
  {
    qCritical() << Q_FUNC_INFO << ": Failed to compute DVH metrics!";
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showVMetricsPercentCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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

  if (!d->logic()->ComputeVMetrics())
  {
    qCritical() << Q_FUNC_INFO << ": Failed to compute DVH metrics!";
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumeCcEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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

  if (!d->logic()->ComputeDMetrics())
  {
    qCritical() << Q_FUNC_INFO << ": Failed to compute DVH metrics!";
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::lineEditDVolumePercentEdited(QString aText)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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

  if (!d->logic()->ComputeDMetrics())
  {
    qCritical() << Q_FUNC_INFO << ": Failed to compute DVH metrics!";
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showDMetricsCheckedStateChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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

  if (!d->logic()->ComputeDMetrics())
  {
    qCritical() << Q_FUNC_INFO << ": Failed to compute DVH metrics!";
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showDoseVolumesOnlyCheckboxChanged(int aState)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
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
    d->MRMLNodeComboBox_DoseVolume->removeAttribute("vtkMRMLScalarVolumeNode", SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::showAllClicked()
{
  this->setVisibleAll(true);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::hideAllClicked()
{
  this->setVisibleAll(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModuleWidget::setVisibleAll(bool on)
{
  Q_D(qSlicerDoseVolumeHistogramModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  vtkMRMLDoseVolumeHistogramNode* paramNode = d->logic()->GetDoseVolumeHistogramNode();
  if (!paramNode)
  {
    return;
  }

  vtkMRMLTableNode* metricsTableNode = paramNode->GetMetricsTableNode();
  if (!metricsTableNode)
  {
    return;
  }

  vtkBitArray* visibilityColumn = vtkBitArray::SafeDownCast(
    metricsTableNode->GetTable()->GetColumn(vtkMRMLDoseVolumeHistogramNode::MetricColumnVisible) );
  for (int row=0; row<metricsTableNode->GetNumberOfRows(); ++row)
  {
    visibilityColumn->SetValue(row, on);
  }
  visibilityColumn->Modified();
  metricsTableNode->Modified();
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
