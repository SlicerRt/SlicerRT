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
#include "qSlicerBeamVisualizerModuleWidget.h"
#include "ui_qSlicerBeamVisualizerModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// BeamVisualizer includes
#include "vtkSlicerBeamVisualizerModuleLogic.h"
#include "vtkMRMLBeamVisualizerNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_BeamVisualizer
class qSlicerBeamVisualizerModuleWidgetPrivate: public Ui_qSlicerBeamVisualizerModule
{
  Q_DECLARE_PUBLIC(qSlicerBeamVisualizerModuleWidget);
protected:
  qSlicerBeamVisualizerModuleWidget* const q_ptr;
public:
  qSlicerBeamVisualizerModuleWidgetPrivate(qSlicerBeamVisualizerModuleWidget& object);
  ~qSlicerBeamVisualizerModuleWidgetPrivate();
  vtkSlicerBeamVisualizerModuleLogic* logic() const;
public:
  /// Map that associates dose volume checkboxes to the corresponding MRML node IDs and the dose unit name
  //std::map<QCheckBox*, std::pair<std::string, std::string> > CheckboxToVolumeIdMap;

  /// Text of the attribute table item that is being edited
  //QString SelectedTableItemText;
};

//-----------------------------------------------------------------------------
// qSlicerBeamVisualizerModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidgetPrivate::qSlicerBeamVisualizerModuleWidgetPrivate(qSlicerBeamVisualizerModuleWidget& object)
  : q_ptr(&object)
  //, SelectedTableItemText(QString())
{
  //this->CheckboxToVolumeIdMap.clear();
}

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidgetPrivate::~qSlicerBeamVisualizerModuleWidgetPrivate()
{
  //this->CheckboxToVolumeIdMap.clear();
}

//-----------------------------------------------------------------------------
vtkSlicerBeamVisualizerModuleLogic*
qSlicerBeamVisualizerModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerBeamVisualizerModuleWidget);
  return vtkSlicerBeamVisualizerModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerBeamVisualizerModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidget::qSlicerBeamVisualizerModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerBeamVisualizerModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidget::~qSlicerBeamVisualizerModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetBeamVisualizerNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLBeamVisualizerNode");
    if (node)
    {
      this->setBeamVisualizerNode( vtkMRMLBeamVisualizerNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerBeamVisualizerModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLBeamVisualizerNode");
    if (node)
    {
      paramNode = vtkMRMLBeamVisualizerNode::SafeDownCast(node);
      d->logic()->SetAndObserveBeamVisualizerNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLBeamVisualizerNode> newNode = vtkSmartPointer<vtkMRMLBeamVisualizerNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveBeamVisualizerNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::setBeamVisualizerNode(vtkMRMLNode *node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = vtkMRMLBeamVisualizerNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetBeamVisualizerNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveBeamVisualizerNode(paramNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  //vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
  //if (paramNode && this->mrmlScene())
  //{
  //  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetBeamVisualizerNode());
  //  d->checkBox_ShowDoseVolumesOnly->setChecked(paramNode->GetShowDoseVolumesOnly());
  //  if (paramNode->GetAccumulatedDoseVolumeNodeId() && strcmp(paramNode->GetAccumulatedDoseVolumeNodeId(),""))
  //  {
  //    d->MRMLNodeComboBox_AccumulatedDoseVolume->setCurrentNode(paramNode->GetAccumulatedDoseVolumeNodeId());
  //  }
  //}

  //this->refreshOutputBaseName();
  //this->refreshVolumesTable();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::setup()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  //d->label_Warning->setVisible(false);
  //d->label_Error->setVisible(false);

  //d->tableWidget_Volumes->setColumnWidth(0, 20);
  //d->tableWidget_Volumes->setColumnWidth(1, 300);

  //// Make connections
  //connect( d->MRMLNodeComboBox_ReferenceDoseVolume, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( referenceDoseVolumeNodeChanged(vtkMRMLNode*) ) );

  //connect( d->checkBox_ShowDoseVolumesOnly, SIGNAL( stateChanged(int) ), this, SLOT( showDoseOnlyChanged(int) ) );

  //connect( d->tableWidget_Volumes, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(onTableItemChanged(QTableWidgetItem*)) );
  //connect( d->tableWidget_Volumes, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(storeSelectedTableItemText(QTableWidgetItem*,QTableWidgetItem*)) );

  //connect( d->MRMLNodeComboBox_AccumulatedDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(accumulatedDoseVolumeNodeChanged(vtkMRMLNode*)) );
  //connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamVisualizerNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::referenceDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  //paramNode->DisableModifiedEventOn();
  //paramNode->SetAndObserveReferenceDoseVolumeNodeId(node->GetID());
  //paramNode->DisableModifiedEventOff();

  //this->updateButtonsState();

  //d->label_Warning->setVisible(!d->logic()->ReferenceDoseVolumeContainsDose());
  //d->label_Warning->setText(QString("Volume is not a dose volume!"));
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::accumulatedDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  //paramNode->DisableModifiedEventOn();
  //paramNode->SetAndObserveAccumulatedDoseVolumeNodeId(node->GetID());
  //paramNode->DisableModifiedEventOff();

  //this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::updateButtonsState()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  //bool applyEnabled = d->logic()->GetBeamVisualizerNode()
  //                 && d->logic()->GetBeamVisualizerNode()->GetAccumulatedDoseVolumeNodeId()
  //                 && strcmp(d->logic()->GetBeamVisualizerNode()->GetAccumulatedDoseVolumeNodeId(), "")
  //                 && d->logic()->GetBeamVisualizerNode()->GetSelectedInputVolumeIds()->size() > 0;
  //d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::applyClicked()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage;
  d->logic()->AccumulateDoseVolumes(errorMessage);

  //d->label_Error->setVisible( !errorMessage.empty() );
  //d->label_Error->setText( QString(errorMessage.c_str()) );

  //this->refreshVolumesTable();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  //QString newBaseName(SlicerRtCommon::BeamVisualizer_OUTPUT_BASE_NAME_PREFIX.c_str());
  //std::map<QCheckBox*, std::pair<std::string, std::string> >::iterator it;
  //for (it=d->CheckboxToVolumeIdMap.begin(); it!=d->CheckboxToVolumeIdMap.end(); ++it)
  //{
  //  if (it->first->isChecked())
  //  {
  //    vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(
  //      this->mrmlScene()->GetNodeByID(it->second.first.c_str()) );
  //    if (!volumeNode)
  //    {
  //      continue;
  //    }

  //    newBaseName.append("_");
  //    newBaseName.append(volumeNode->GetName());
  //  }
  //}

  //d->MRMLNodeComboBox_AccumulatedDoseVolume->setBaseName( newBaseName.toLatin1() );
}
