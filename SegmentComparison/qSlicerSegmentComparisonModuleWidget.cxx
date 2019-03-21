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

// SlicerQt includes
#include "qSlicerSegmentComparisonModuleWidget.h"
#include "ui_qSlicerSegmentComparisonModule.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "qMRMLSegmentSelectorWidget.h"

// Qt includes
#include <QDebug>

// SegmentComparison includes
#include "vtkSlicerSegmentComparisonModuleLogic.h"
#include "vtkMRMLSegmentComparisonNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLTableNode.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_SegmentComparison
class qSlicerSegmentComparisonModuleWidgetPrivate: public Ui_qSlicerSegmentComparisonModule
{
  Q_DECLARE_PUBLIC(qSlicerSegmentComparisonModuleWidget);
protected:
  qSlicerSegmentComparisonModuleWidget* const q_ptr;
public:
  qSlicerSegmentComparisonModuleWidgetPrivate(qSlicerSegmentComparisonModuleWidget& object);
  vtkSlicerSegmentComparisonModuleLogic* logic() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerSegmentComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentComparisonModuleWidgetPrivate::qSlicerSegmentComparisonModuleWidgetPrivate(qSlicerSegmentComparisonModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
vtkSlicerSegmentComparisonModuleLogic*
qSlicerSegmentComparisonModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerSegmentComparisonModuleWidget);
  return vtkSlicerSegmentComparisonModuleLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerSegmentComparisonModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSegmentComparisonModuleWidget::qSlicerSegmentComparisonModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSegmentComparisonModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentComparisonModuleWidget::~qSlicerSegmentComparisonModuleWidget() = default;

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene && d->MRMLNodeComboBox_ParameterSet->currentNode() == nullptr)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLSegmentComparisonNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLSegmentComparisonNode> newNode = vtkSmartPointer<vtkMRMLSegmentComparisonNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerSegmentComparisonModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }
  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  // If we have a parameter node select it
  if (!paramNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLSegmentComparisonNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLSegmentComparisonNode> newNode = vtkSmartPointer<vtkMRMLSegmentComparisonNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
  else
  {
    this->updateWidgetFromMRML();
  }

  d->ModuleWindowInitialized = true;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  // Set selected MRML nodes in comboboxes in the parameter set if it was nullptr there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if ( !paramNode->GetReferenceSegmentationNode()
      && d->SegmentSelectorWidget_Reference->currentNode() )
    {
      paramNode->SetAndObserveReferenceSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_Reference->currentNode()));
    }
    if ( !paramNode->GetReferenceSegmentID()
      && !d->SegmentSelectorWidget_Reference->currentSegmentID().isEmpty() )
    {
      paramNode->SetReferenceSegmentID(d->SegmentSelectorWidget_Reference->currentSegmentID().toLatin1().constData());
    }
    if ( !paramNode->GetCompareSegmentationNode()
      && d->SegmentSelectorWidget_Compare->currentNode() )
    {
      paramNode->SetAndObserveCompareSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_Compare->currentNode()));
    }
    if ( !paramNode->GetCompareSegmentID()
      && !d->SegmentSelectorWidget_Compare->currentSegmentID().isEmpty() )
    {
      paramNode->SetCompareSegmentID(d->SegmentSelectorWidget_Compare->currentSegmentID().toLatin1().constData());
    }
    this->updateButtonsState();
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  // Make sure there are table nodes referenced from the parameter node
  if (!paramNode->GetDiceTableNode())
  {
    vtkSmartPointer<vtkMRMLTableNode> diceTableNode = vtkSmartPointer<vtkMRMLTableNode>::New();
    std::string diceTableNodeName = this->mrmlScene()->GenerateUniqueName("Dice");
    diceTableNode->SetName(diceTableNodeName.c_str());
    this->mrmlScene()->AddNode(diceTableNode);
    paramNode->SetAndObserveDiceTableNode(diceTableNode);
  }
  d->MRMLTableView_Dice->setMRMLTableNode(paramNode->GetDiceTableNode());

  if (!paramNode->GetHausdorffTableNode())
  {
    vtkSmartPointer<vtkMRMLTableNode> hausdorffTableNode = vtkSmartPointer<vtkMRMLTableNode>::New();
    std::string hausdorffTableNodeName = this->mrmlScene()->GenerateUniqueName("Hausdorff");
    hausdorffTableNode->SetName(hausdorffTableNodeName.c_str());
    this->mrmlScene()->AddNode(hausdorffTableNode);
    paramNode->SetAndObserveHausdorffTableNode(hausdorffTableNode);
  }
  d->MRMLTableView_Hausdorff->setMRMLTableNode(paramNode->GetHausdorffTableNode());

  // Set input selection
  if (paramNode->GetReferenceSegmentationNode())
  {
    d->SegmentSelectorWidget_Reference->setCurrentNode(paramNode->GetReferenceSegmentationNode());
  }
  if (paramNode->GetReferenceSegmentID())
  {
    d->SegmentSelectorWidget_Reference->setCurrentSegmentID(paramNode->GetReferenceSegmentID());
  }

  if (paramNode->GetCompareSegmentationNode())
  {
    d->SegmentSelectorWidget_Compare->setCurrentNode(paramNode->GetCompareSegmentationNode());
  }
  if (paramNode->GetCompareSegmentID())
  {
    d->SegmentSelectorWidget_Compare->setCurrentSegmentID(paramNode->GetCompareSegmentID());
  }

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::setup()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Error->setText("");

  // Make connections
  connect( d->SegmentSelectorWidget_Reference, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceSegmentationNodeChanged(vtkMRMLNode*)) );
  connect( d->SegmentSelectorWidget_Reference, SIGNAL(currentSegmentChanged(QString)), this, SLOT(referenceSegmentChanged(QString)) );
  connect( d->SegmentSelectorWidget_Compare, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(compareSegmentationNodeChanged(vtkMRMLNode*)) );
  connect( d->SegmentSelectorWidget_Compare, SIGNAL(currentSegmentChanged(QString)), this, SLOT(compareSegmentChanged(QString)) );

  connect( d->pushButton_ComputeHausdorff, SIGNAL(clicked()), this, SLOT(computeHausdorffClicked()) );
  connect( d->pushButton_ComputeDice, SIGNAL(clicked()), this, SLOT(computeDiceClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setParameterNode(vtkMRMLNode*)) );

  // Setup table views
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  d->MRMLTableView_Dice->verticalHeader()->setResizeMode(QHeaderView::Fixed); // Set default row height
  d->MRMLTableView_Hausdorff->verticalHeader()->setResizeMode(QHeaderView::Fixed); // Set default row height
#else
  d->MRMLTableView_Dice->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); // Set default row height
  d->MRMLTableView_Hausdorff->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); // Set default row height
#endif
  d->MRMLTableView_Dice->verticalHeader()->setDefaultSectionSize(22);
  d->MRMLTableView_Hausdorff->verticalHeader()->setDefaultSectionSize(22);

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  bool computeEnabled = paramNode
                   && paramNode->GetReferenceSegmentationNode()
                   && paramNode->GetReferenceSegmentID()
                   && paramNode->GetCompareSegmentationNode()
                   && paramNode->GetCompareSegmentID();
  d->pushButton_ComputeDice->setEnabled(computeEnabled);
  d->pushButton_ComputeHausdorff->setEnabled(computeEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::referenceSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::referenceSegmentChanged(QString segmentID)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || segmentID.isEmpty() || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetReferenceSegmentID(segmentID.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::compareSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveCompareSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::compareSegmentChanged(QString segmentID)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || segmentID.isEmpty() || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetCompareSegmentID(segmentID.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::computeHausdorffClicked()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::string errorMessage = d->logic()->ComputeHausdorffDistances(paramNode);

  if (paramNode->GetHausdorffResultsValid())
  {
    d->label_Error->setText("");

    // Hide input information that is in the tables for exporting, no need to show on UI
    d->MRMLTableView_Hausdorff->hideRow(0);
    d->MRMLTableView_Hausdorff->hideRow(1);
    d->MRMLTableView_Hausdorff->hideRow(2);
    d->MRMLTableView_Hausdorff->hideRow(3);
    d->MRMLTableView_Hausdorff->setColumnWidth(0,120);
    d->MRMLTableView_Dice->setColumnWidth(0,120); // For some reason the other table's columns are resized if this is not explicitly called
  }
  else
  {
    d->label_Error->setText(QString(errorMessage.c_str()));
    this->invalidateHausdorffResults();
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::computeDiceClicked()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::string errorMessage = d->logic()->ComputeDiceStatistics(paramNode);

  if (paramNode->GetDiceResultsValid())
  {
    d->label_Error->setText("");

    // Hide input information that is in the tables for exporting, no need to show on UI
    d->MRMLTableView_Dice->hideRow(0);
    d->MRMLTableView_Dice->hideRow(1);
    d->MRMLTableView_Dice->hideRow(2);
    d->MRMLTableView_Dice->hideRow(3);
    d->MRMLTableView_Dice->setColumnWidth(0,120);
    d->MRMLTableView_Hausdorff->setColumnWidth(0,120); // For some reason the other table's columns are resized if this is not explicitly called
  }
  else
  {
    d->label_Error->setText(QString(errorMessage.c_str()));
    this->invalidateDiceResults();
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::invalidateHausdorffResults()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->HausdorffResultsValidOff();

  if (paramNode->GetHausdorffTableNode())
  {
    paramNode->GetHausdorffTableNode()->RemoveAllColumns();
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::invalidateDiceResults()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DiceResultsValidOff();

  if (paramNode->GetDiceTableNode())
  {
    paramNode->GetDiceTableNode()->RemoveAllColumns();
  }
}
