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

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationModuleLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"

// SlicerRt includes
#include "vtkSlicerRtCommon.h"

// Qt includes
#include <QCheckBox>
#include <QDebug>

// SlicerQt includes
#include "qSlicerDoseAccumulationModuleWidget.h"
#include "ui_qSlicerDoseAccumulationModule.h"

// SubjectHierarchy MRML includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DoseAccumulation
class qSlicerDoseAccumulationModuleWidgetPrivate: public Ui_qSlicerDoseAccumulationModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseAccumulationModuleWidget);
protected:
  qSlicerDoseAccumulationModuleWidget* const q_ptr;
public:
  qSlicerDoseAccumulationModuleWidgetPrivate(qSlicerDoseAccumulationModuleWidget& object);
  ~qSlicerDoseAccumulationModuleWidgetPrivate();
  vtkSlicerDoseAccumulationModuleLogic* logic() const;
public:
  /// Map that associates dose volume checkboxes to the corresponding MRML node IDs and the dose unit name
  std::map<QCheckBox*, std::pair<std::string, std::string> > CheckboxToVolumeIdMap;

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
vtkSlicerDoseAccumulationModuleLogic*
qSlicerDoseAccumulationModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseAccumulationModuleWidget);
  return vtkSlicerDoseAccumulationModuleLogic::SafeDownCast(q->logic());
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
  if (scene && d->MRMLNodeComboBox_ParameterSet->currentNode() == nullptr)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseAccumulationNode> newNode = vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
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
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerDoseAccumulationModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == nullptr)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  // If we have a parameter node select it
  if (paramNode == nullptr)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseAccumulationNode> newNode = vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
  else
  {
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  //TODO: Set parameters from UI to new node if UI selection was valid but param node selection empty (nullptr, etc.)?

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (paramNode && this->mrmlScene())
  {
    d->checkBox_ShowDoseVolumesOnly->setChecked(paramNode->GetShowDoseVolumesOnly());
    if (paramNode->GetAccumulatedDoseVolumeNode())
    {
      d->MRMLNodeComboBox_AccumulatedDoseVolume->setCurrentNode(paramNode->GetAccumulatedDoseVolumeNode());
    }
    if (paramNode->GetReferenceDoseVolumeNode())
    {
      d->MRMLNodeComboBox_ReferenceDoseVolume->setCurrentNode(paramNode->GetReferenceDoseVolumeNode());
    }
    else
    {
      this->referenceDoseVolumeNodeChanged(d->MRMLNodeComboBox_ReferenceDoseVolume->currentNode());
    }
  }

  this->refreshOutputBaseName();
  this->refreshVolumesTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::setup()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Warning->setVisible(false);
  d->label_Error->setVisible(false);

  d->tableWidget_Volumes->setColumnWidth(0, 20);
  d->tableWidget_Volumes->setColumnWidth(1, 300);

  // Make connections
  connect( d->MRMLNodeComboBox_ReferenceDoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( referenceDoseVolumeNodeChanged(vtkMRMLNode*) ) );

  connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseOnlyChanged(int) ) );

  connect( d->tableWidget_Volumes, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(onTableItemChanged(QTableWidgetItem*)) );
  connect( d->tableWidget_Volumes, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(storeSelectedTableItemText(QTableWidgetItem*,QTableWidgetItem*)) );

  connect( d->MRMLNodeComboBox_AccumulatedDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(accumulatedDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setParameterNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::referenceDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();

  d->label_Warning->setVisible(!paramNode->GetReferenceDoseVolumeNode() || !vtkSlicerRtCommon::IsDoseVolumeNode(paramNode->GetReferenceDoseVolumeNode()));
  d->label_Warning->setText(QString("Volume is not a dose volume!"));
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::accumulatedDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !node)
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveAccumulatedDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  bool applyEnabled = paramNode
                   && paramNode->GetAccumulatedDoseVolumeNode()
                   && paramNode->GetNumberOfSelectedInputVolumeNodes() > 0;
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::refreshVolumesTable()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  // Clear the table
  d->tableWidget_Volumes->clearContents();

  // Clear checkboxes map and save previous weights
  std::map<QCheckBox*, std::pair<std::string, std::string> >::iterator it;
  for (it = d->CheckboxToVolumeIdMap.begin(); it != d->CheckboxToVolumeIdMap.end(); ++it)
  {
    QCheckBox* checkbox = it->first;
    disconnect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( includeVolumeCheckStateChanged(int) ) );
    delete checkbox;
  }
  d->CheckboxToVolumeIdMap.clear();

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->mrmlScene());
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  // Get dose volumes from scene (or all volumes if requested)
  std::vector<vtkMRMLNode*> shownVolumeNodes;
  std::vector<vtkMRMLNode*> volumeNodes;
  this->mrmlScene()->GetNodesByClass("vtkMRMLScalarVolumeNode", volumeNodes);
  for (std::vector<vtkMRMLNode*>::iterator nodeIt=volumeNodes.begin(); nodeIt!=volumeNodes.end(); ++nodeIt)
  {
    vtkMRMLNode* node = (*nodeIt);
    if (vtkSlicerRtCommon::IsDoseVolumeNode(node) || !paramNode->GetShowDoseVolumesOnly())
    {
      shownVolumeNodes.push_back(node);
    }
  }

  // If number of nodes is the same in the table and the list of nodes, then we don't need refreshing the table
  // (this function is called after each node event, so it cannot occur that a node has been removed and another added)
  if ( d->CheckboxToVolumeIdMap.size() == shownVolumeNodes.size()
    || shownVolumeNodes.size() == 0 )
  {
    return;
  }

  d->tableWidget_Volumes->setRowCount(shownVolumeNodes.size());

  std::map<std::string,double>* oldVolumeNodeIdsToWeightsMap = paramNode->GetVolumeNodeIdsToWeightsMap();
  std::map<std::string,double> newVolumeNodeIdsToWeightsMap;

  // Fill the table
  int row=0;
  for (std::vector<vtkMRMLNode*>::iterator nodeIt=shownVolumeNodes.begin(); nodeIt!=shownVolumeNodes.end(); ++nodeIt, ++row)
  {
    vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(*nodeIt);
    if (!volumeNode)
    {
      continue;
    }

    // Get dose unit name
    std::string doseUnitName("");
    vtkIdType volumeShItemID = shNode->GetItemByDataNode(volumeNode);
    if (volumeShItemID)
    {
      shNode->GetAttributeFromItemAncestor(volumeShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    }
    std::string doseUnitStr = ( !doseUnitName.empty() ? doseUnitName : "N/A" );

    // Create checkbox
    QCheckBox* checkbox = new QCheckBox(d->tableWidget_Volumes);
    checkbox->setToolTip(tr("Include this volume in accumulated dose volume"));
    checkbox->setProperty(vtkSlicerDoseAccumulationModuleLogic::DOSEACCUMULATION_DOSE_VOLUME_NODE_NAME_ATTRIBUTE_NAME.c_str(), QString(volumeNode->GetName()));
    d->CheckboxToVolumeIdMap[checkbox] = std::pair<std::string, std::string>(volumeNode->GetID(), doseUnitStr);

    // Set previous checked state of the checkbox
    for (unsigned int referenceIndex=0; referenceIndex<paramNode->GetNumberOfSelectedInputVolumeNodes(); ++referenceIndex)
    {
      if (paramNode->GetNthSelectedInputVolumeNode(referenceIndex) == volumeNode)
      {
        checkbox->setChecked(true);
        break;
      }
    }

    connect( checkbox, SIGNAL( stateChanged(int) ), this, SLOT( includeVolumeCheckStateChanged(int) ) );

    // Store checkbox with the volume node ID and row number (to be able to retrieve the weight)
    double weight = 1.0;
    if (oldVolumeNodeIdsToWeightsMap->find(volumeNode->GetID()) != oldVolumeNodeIdsToWeightsMap->end())
    {
      weight = (*oldVolumeNodeIdsToWeightsMap)[volumeNode->GetID()];
    }

    d->tableWidget_Volumes->setCellWidget(row, 0, checkbox);
    d->tableWidget_Volumes->setItem(row, 1, new QTableWidgetItem( QString(volumeNode->GetName()) ) );    
    d->tableWidget_Volumes->setItem(row, 2, new QTableWidgetItem( QString::number(weight,'f',2) ) );

    newVolumeNodeIdsToWeightsMap[volumeNode->GetID()] = weight;
  }

  // Set new weights map
  paramNode->GetVolumeNodeIdsToWeightsMap()->swap(newVolumeNodeIdsToWeightsMap);

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::storeSelectedTableItemText(QTableWidgetItem* selectedItem, QTableWidgetItem* previousItem)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);
  Q_UNUSED(previousItem);

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

  // Do nothing if nothing is selected
  if (!changedItem)
  {
    return;
  }

  // Set weight if the selected cell is "editable"
  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (d->SelectedTableItemText.isNull() && paramNode)
  {
    std::string volumeID = d->CheckboxToVolumeIdMap[(QCheckBox*)d->tableWidget_Volumes->cellWidget(changedItem->row(), 0)].first;
    (*paramNode->GetVolumeNodeIdsToWeightsMap())[volumeID] = changedItem->text().toDouble();
    return;
  }

  // Revert the edited text if the cell is "non-editable"
  d->tableWidget_Volumes->blockSignals(true);
  changedItem->setText( d->SelectedTableItemText );
  d->tableWidget_Volumes->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::applyClicked()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage = d->logic()->AccumulateDoseVolumes(paramNode);

  d->label_Error->setVisible( !errorMessage.empty() );
  if (!errorMessage.empty())
  {
    d->label_Error->setText( QString(errorMessage.c_str()) );
  }

  this->refreshVolumesTable();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::includeVolumeCheckStateChanged(int aState)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  d->label_Error->setVisible(false);

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!this->mrmlScene() || !paramNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene or parameter set node!";
    return;
  }

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());
  if (!senderCheckbox)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid sender checkbox for show/hide in chart checkbox state change";
    return;
  }

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(d->CheckboxToVolumeIdMap[senderCheckbox].first) );

  // Add or delete node to/from the list
  if (aState)
  {
    paramNode->AddSelectedInputVolumeNode(volumeNode);
  }
  else
  {
    paramNode->RemoveSelectedInputVolumeNode(volumeNode);
  }

  this->refreshOutputBaseName();
  this->checkDoseUnitsInSelectedVolumes();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::showDoseOnlyChanged(int aState)
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseAccumulationNode* paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetShowDoseVolumesOnly(aState);
  paramNode->DisableModifiedEventOff();

  this->refreshVolumesTable();
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::checkDoseUnitsInSelectedVolumes()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  QSet<QString> doseUnits;
  std::map<QCheckBox*, std::pair<std::string, std::string> >::iterator it;
  for (it=d->CheckboxToVolumeIdMap.begin(); it!=d->CheckboxToVolumeIdMap.end(); ++it)
  {
    if (it->first->isChecked())
    {
      doseUnits.insert( QString(it->second.second.c_str()).trimmed().toLower() );
    }
  }

  d->label_Warning->setVisible( doseUnits.count() > 1 );
  d->label_Warning->setText(QString("Dose units do not match!"));
}

//-----------------------------------------------------------------------------
void qSlicerDoseAccumulationModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerDoseAccumulationModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  QString newBaseName(vtkSlicerDoseAccumulationModuleLogic::DOSEACCUMULATION_OUTPUT_BASE_NAME_PREFIX.c_str());
  std::map<QCheckBox*, std::pair<std::string, std::string> >::iterator it;
  for (it=d->CheckboxToVolumeIdMap.begin(); it!=d->CheckboxToVolumeIdMap.end(); ++it)
  {
    if (it->first->isChecked())
    {
      vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
        this->mrmlScene()->GetNodeByID(it->second.first.c_str()) );
      if (!volumeNode)
      {
        continue;
      }

      newBaseName.append(volumeNode->GetName());
    }
  }

  d->MRMLNodeComboBox_AccumulatedDoseVolume->setBaseName( newBaseName );
}
