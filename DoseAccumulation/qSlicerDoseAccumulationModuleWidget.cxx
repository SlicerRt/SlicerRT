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
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerDoseAccumulationModuleWidget.h"
#include "ui_qSlicerDoseAccumulationModule.h"

// ModuleTemplate includes
#include "vtkSlicerDoseAccumulationLogic.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseAccumulation
class qSlicerDoseAccumulationModuleWidgetPrivate: public Ui_qSlicerDoseAccumulationModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseAccumulationModuleWidget);
protected:
  qSlicerDoseAccumulationModuleWidget* const q_ptr;
public:
  qSlicerDoseAccumulationModuleWidgetPrivate(qSlicerDoseAccumulationModuleWidget& object);
  vtkSlicerDoseAccumulationLogic* logic() const;
public:
  /// Text of the attribute table item that is being edited
  QString SelectedTableItemText;
};

//-----------------------------------------------------------------------------
// qSlicerDoseAccumulationModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidgetPrivate::qSlicerDoseAccumulationModuleWidgetPrivate(qSlicerDoseAccumulationModuleWidget& object)
  : q_ptr(&object)
  , SelectedTableItemText(QString())
{
}

//-----------------------------------------------------------------------------
vtkSlicerDoseAccumulationLogic*
qSlicerDoseAccumulationModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseAccumulationModuleWidget);
  return vtkSlicerDoseAccumulationLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerDoseAccumulationModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidget::qSlicerDoseAccumulationModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseAccumulationModuleWidgetPrivate(*this) )
{
  m_CheckboxToVolumeIdMap.clear();
  m_SelectedVolumeIds.clear();
}

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidget::~qSlicerDoseAccumulationModuleWidget()
{
  m_CheckboxToVolumeIdMap.clear();
  m_SelectedVolumeIds.clear();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setup()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->tableWidget_Volumes->setColumnWidth(0, 24);

  // Make connections
  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseOnlyChanged(int) ) );

  connect( d->tableWidget_Volumes, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(onTableItemChanged(QTableWidgetItem*)) );
  connect( d->tableWidget_Volumes, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(storeSelectedTableItemText(QTableWidgetItem*,QTableWidgetItem*)) );

  connect( d->MRMLNodeComboBox_AccumulatedDoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( accumulatedDoseVolumeNodeChanged(vtkMRMLNode*) ) );
  connect( d->pushButton_Apply, SIGNAL( clicked() ), this, SLOT( applyClicked() ) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::accumulatedDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);
  vtkMRMLVolumeNode* volumeNode = dynamic_cast<vtkMRMLVolumeNode*>(node);
  if (volumeNode)
  {
    d->logic()->SetAccumulatedDoseVolumeNode(volumeNode);
    updateButtonsState();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  bool applyEnabled = d->logic()->GetAccumulatedDoseVolumeNode() && m_SelectedVolumeIds.size();
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::onLogicModified()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (d->logic()->GetSceneChanged())
  {
    refreshVolumesTable();

    d->logic()->SceneChangedOff();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::refreshVolumesTable()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkCollection* volumeNodes = d->logic()->GetVolumeNodes(d->checkBox_ShowDoseVolumesOnly->isChecked());

  // Clear the table
  d->tableWidget_Volumes->clear();

  std::map< QCheckBox*, std::pair<std::string,int> >::iterator it;
  for (it = m_CheckboxToVolumeIdMap.begin(); it != m_CheckboxToVolumeIdMap.end(); ++it)
  {
    QCheckBox* checkbox = it->first;
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( includeVolumeCheckStateChanged(int) ) );
    delete checkbox;
  }

  m_CheckboxToVolumeIdMap.clear();
  m_SelectedVolumeIds.clear();

  if (volumeNodes == NULL)
  {
    return;
  }
  else if (volumeNodes->GetNumberOfItems() < 1)
  {
    volumeNodes->Delete();
    return;
  }

  d->tableWidget_Volumes->setRowCount(volumeNodes->GetNumberOfItems());

  // Fill the table
  for (int i=0; i<volumeNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast( volumeNodes->GetItemAsObject(i) );
    if (!volumeNode)
    {
      continue;
    }

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_Volumes);
    checkbox->setToolTip(tr("Include this volume in accumulated dose volume"));
    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( includeVolumeCheckStateChanged(int) ) );

    // Store checkbox with the augmented structure set name and the double array ID
    std::pair<std::string,int> volumeIdAndRowNumberPair(volumeNode->GetID(), i);
    m_CheckboxToVolumeIdMap[checkbox] = volumeIdAndRowNumberPair;

    d->tableWidget_Volumes->setCellWidget(i, 0, checkbox);
    d->tableWidget_Volumes->setItem(i, 1, new QTableWidgetItem( QString(volumeNode->GetName()) ) );    
    d->tableWidget_Volumes->setItem(i, 2, new QTableWidgetItem( QString("1.0") ) );    
  }

  volumeNodes->Delete();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  // Store only if the selected item is in a "non-editable" column
  if (selectedItem && selectedItem->column() != 2)
  {
    d->SelectedTableItemText = selectedItem->text();
    return;
  }

  // Set to null string otherwise
  d->SelectedTableItemText = QString();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::onTableItemChanged(QTableWidgetItem* changedItem)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  // Do nothing if unselected or the selected cell is "editable"
  if (!changedItem || d->SelectedTableItemText.isNull())
  {
    return;
  }

  // Revert the edited text if the cell is "non-editable"
  d->tableWidget_Volumes->blockSignals(true);
  changedItem->setText( d->SelectedTableItemText.toLatin1() );
  d->tableWidget_Volumes->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::applyClicked()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  std::vector< std::pair<std::string,double> > volumeIdsAndWeights;
  for (std::set<std::string>::iterator it = m_SelectedVolumeIds.begin(); it != m_SelectedVolumeIds.end(); ++it)
  {
    std::pair<std::string,double> volumeIdAndWeightPair;
    volumeIdAndWeightPair.first = (*it);

    std::map< QCheckBox*, std::pair<std::string,int> >::iterator checkboxIt;
    for (checkboxIt = m_CheckboxToVolumeIdMap.begin(); checkboxIt != m_CheckboxToVolumeIdMap.end(); ++checkboxIt)
    {
      if (checkboxIt->second.first.compare(*it) == 0)
      {
        volumeIdAndWeightPair.second = d->tableWidget_Volumes->item(checkboxIt->second.second, 2)->text().toDouble();
        break;
      }
    }

    volumeIdsAndWeights.push_back(volumeIdAndWeightPair);
  }

  d->logic()->AccumulateDoseVolumes(volumeIdsAndWeights);
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::includeVolumeCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());

  if (!senderCheckbox)
  {
    std::cerr << "Error: Invalid sender checkbox for show/hide in chart checkbox state change" << std::endl;
    return;
  }

  if (aState)
  {
    m_SelectedVolumeIds.insert(m_CheckboxToVolumeIdMap[senderCheckbox].first);
  }
  else
  {
    m_SelectedVolumeIds.erase(m_CheckboxToVolumeIdMap[senderCheckbox].first);
  }

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::showDoseOnlyChanged(int aState)
{
  refreshVolumesTable();
}
