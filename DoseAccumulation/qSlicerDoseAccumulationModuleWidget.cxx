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
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerDoseAccumulationModuleWidget.h"
#include "ui_qSlicerDoseAccumulationModule.h"

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"

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
  ~qSlicerDoseAccumulationModuleWidgetPrivate();
  vtkSlicerDoseAccumulationLogic* logic() const;
public:
  /// Map that associates dose volume checkboxes to the corresponding MRML node IDs
  std::map<QCheckBox*, std::string> CheckboxToVolumeIdMap;

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
  this->CheckboxToVolumeIdMap.clear();
}

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidgetPrivate::~qSlicerDoseAccumulationModuleWidgetPrivate()
{
  this->CheckboxToVolumeIdMap.clear();
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
}

//-----------------------------------------------------------------------------
qSlicerDoseAccumulationModuleWidget::~qSlicerDoseAccumulationModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetDoseAccumulationNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
    if (node)
    {
      this->setDoseAccumulationNode( vtkMRMLDoseAccumulationNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerDoseAccumulationModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLDoseAccumulationNode* paramNode = d->logic()->GetDoseAccumulationNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
    if (node)
    {
      paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(node);
      d->logic()->SetAndObserveDoseAccumulationNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseAccumulationNode> newNode = vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveDoseAccumulationNode(newNode);
    }
  }

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setDoseAccumulationNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetDoseAccumulationNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveDoseAccumulationNode(paramNode);
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = d->logic()->GetDoseAccumulationNode();

  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetDoseAccumulationNode());
    d->checkBox_ShowDoseVolumesOnly->setChecked(paramNode->GetShowDoseVolumesOnly());
    if (paramNode->GetAccumulatedDoseVolumeNodeId() && stricmp(paramNode->GetAccumulatedDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_AccumulatedDoseVolume->setCurrentNode(paramNode->GetAccumulatedDoseVolumeNodeId());
    }
  }

  refreshVolumesTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setup()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Warning->setText("");

  d->tableWidget_Volumes->setColumnWidth(0, 24);
  d->tableWidget_Volumes->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents); 

  // Make connections
  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseOnlyChanged(int) ) );

  connect( d->tableWidget_Volumes, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(onTableItemChanged(QTableWidgetItem*)) );
  connect( d->tableWidget_Volumes, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(storeSelectedTableItemText(QTableWidgetItem*,QTableWidgetItem*)) );

  connect( d->MRMLNodeComboBox_AccumulatedDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(accumulatedDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setDoseAccumulationNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::accumulatedDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = d->logic()->GetDoseAccumulationNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->SetAccumulatedDoseVolumeNodeId(node->GetID());
  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  bool applyEnabled = d->logic()->GetDoseAccumulationNode()
                   && d->logic()->GetDoseAccumulationNode()->GetAccumulatedDoseVolumeNodeId()
                   && stricmp(d->logic()->GetDoseAccumulationNode()->GetAccumulatedDoseVolumeNodeId(), "")
                   && d->logic()->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->size() > 0;
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::onLogicModified()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::refreshVolumesTable()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkCollection* volumeNodes = d->logic()->GetVolumeNodesFromScene();

  // Clear the table
  d->tableWidget_Volumes->clearContents();

  // Clear checkboxes map and save previous weights
  for (std::map<QCheckBox*, std::string>::iterator it = d->CheckboxToVolumeIdMap.begin(); it != d->CheckboxToVolumeIdMap.end(); ++it)
  {
    QCheckBox* checkbox = it->first;
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( includeVolumeCheckStateChanged(int) ) );
    delete checkbox;
  }
  d->CheckboxToVolumeIdMap.clear();

  if (volumeNodes == NULL)
  {
    return;
  }

  d->tableWidget_Volumes->setRowCount(volumeNodes->GetNumberOfItems());
  if (volumeNodes->GetNumberOfItems() < 1 || !d->logic()->GetDoseAccumulationNode())
  {
    volumeNodes->Delete();
    return;
  }

  std::map<std::string,double>* oldVolumeNodeIdsToWeightsMap = d->logic()->GetDoseAccumulationNode()->GetVolumeNodeIdsToWeightsMap();
  std::map<std::string,double> newVolumeNodeIdsToWeightsMap;

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
    d->CheckboxToVolumeIdMap[checkbox] = volumeNode->GetID();

    // Set previous checked state of the checkbox
    std::set<std::string>* selectedVolumeIds = d->logic()->GetDoseAccumulationNode()->GetSelectedInputVolumeIds();
    if (selectedVolumeIds->find(volumeNode->GetID()) != selectedVolumeIds->end())
    {
      checkbox->setChecked(true);
    }

    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( includeVolumeCheckStateChanged(int) ) );

    // Store checkbox with the volume node ID and row number (to be able to retrieve the weight)
    double weight = 1.0;
    if (oldVolumeNodeIdsToWeightsMap->find(volumeNode->GetID()) != oldVolumeNodeIdsToWeightsMap->end())
    {
      weight = (*oldVolumeNodeIdsToWeightsMap)[volumeNode->GetID()];
    }

    d->tableWidget_Volumes->setCellWidget(i, 0, checkbox);
    d->tableWidget_Volumes->setItem(i, 1, new QTableWidgetItem( QString(volumeNode->GetName()) ) );    
    d->tableWidget_Volumes->setItem(i, 2, new QTableWidgetItem( QString::number(weight,'f',2) ) );

    newVolumeNodeIdsToWeightsMap[volumeNode->GetID()] = weight;
  }

  // Set new weights map
  d->logic()->GetDoseAccumulationNode()->GetVolumeNodeIdsToWeightsMap()->swap(newVolumeNodeIdsToWeightsMap);

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

  // Do nothing if unselected
  if (!changedItem)
  {
    return;
  }

  // Set weight if the selected cell is "editable"
  if (d->SelectedTableItemText.isNull() && d->logic()->GetDoseAccumulationNode())
  {
    std::string volumeID = d->CheckboxToVolumeIdMap[(QCheckBox*)d->tableWidget_Volumes->cellWidget(changedItem->row(), 0)];
    (*d->logic()->GetDoseAccumulationNode()->GetVolumeNodeIdsToWeightsMap())[volumeID] = changedItem->text().toDouble();
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

  d->logic()->AccumulateDoseVolumes();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::includeVolumeCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (!d->logic()->GetDoseAccumulationNode())
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
    d->logic()->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->insert(d->CheckboxToVolumeIdMap[senderCheckbox]);
  }
  else
  {
    d->logic()->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->erase(d->CheckboxToVolumeIdMap[senderCheckbox]);
  }

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::showDoseOnlyChanged(int aState)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (!d->logic()->GetDoseAccumulationNode())
  {
    return;
  }

  d->logic()->GetDoseAccumulationNode()->SetShowDoseVolumesOnly(aState);

  refreshVolumesTable();
}
