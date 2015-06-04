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
qSlicerSegmentComparisonModuleWidget::~qSlicerSegmentComparisonModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetSegmentComparisonNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLSegmentComparisonNode");
    if (node)
    {
      this->setSegmentComparisonNode( vtkMRMLSegmentComparisonNode::SafeDownCast(node) );
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerSegmentComparisonModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << "qSlicerSegmentComparisonModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLSegmentComparisonNode");
    if (node)
    {
      paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(node);
      d->logic()->SetAndObserveSegmentComparisonNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLSegmentComparisonNode> newNode = vtkSmartPointer<vtkMRMLSegmentComparisonNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveSegmentComparisonNode(newNode);
    }
  }

  d->ModuleWindowInitialized = true;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::setSegmentComparisonNode(vtkMRMLNode *node)
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  vtkMRMLSegmentComparisonNode* paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetSegmentComparisonNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveSegmentComparisonNode(paramNode);

  // Set selected MRML nodes in comboboxes in the parameter set if it was NULL there
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::updateWidgetFromMRML: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetSegmentComparisonNode());

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

  connect( d->pushButton_ComputeDice, SIGNAL(clicked()), this, SLOT(computeDiceClicked()) );
  connect( d->pushButton_ComputeHausdorff, SIGNAL(clicked()), this, SLOT(computeHausdorffClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setSegmentComparisonNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  bool computeEnabled = d->logic()->GetSegmentComparisonNode()
                   && d->logic()->GetSegmentComparisonNode()->GetReferenceSegmentationNode()
                   && d->logic()->GetSegmentComparisonNode()->GetReferenceSegmentID()
                   && d->logic()->GetSegmentComparisonNode()->GetCompareSegmentationNode()
                   && d->logic()->GetSegmentComparisonNode()->GetCompareSegmentID();
  d->pushButton_ComputeDice->setEnabled(computeEnabled);
  d->pushButton_ComputeHausdorff->setEnabled(computeEnabled);

  this->updateWidgetFromMRML();
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::referenceSegmentationNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::referenceSegmentChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::compareSegmentationNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::compareSegmentChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::computeHausdorffClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::string errorMessage = d->logic()->ComputeHausdorffDistances();

  if (paramNode->GetHausdorffResultsValid())
  {
    d->label_Error->setText("");

    d->lineEdit_MaximumHausdorffDistanceForBoundary->setText(
      QString("%1 mm").arg(paramNode->GetMaximumHausdorffDistanceForBoundaryMm(),0,'f',2) );
    d->lineEdit_AverageHausdorffDistanceForBoundary->setText(
      QString("%1 mm").arg(paramNode->GetAverageHausdorffDistanceForBoundaryMm(),0,'f',2) );
    d->lineEdit_95PercentHausdorffDistanceForBoundary->setText(
      QString("%1 mm").arg(paramNode->GetPercent95HausdorffDistanceForBoundaryMm(),0,'f',2) );
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
    qCritical() << "qSlicerSegmentComparisonModuleWidget::computeDiceClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::string errorMessage = d->logic()->ComputeDiceStatistics();

  if (paramNode->GetDiceResultsValid())
  {
    d->label_Error->setText("");

    d->lineEdit_DiceCoefficient->setText(
      QString("%1").arg(paramNode->GetDiceCoefficient()) );
    d->lineEdit_TruePositives->setText(
      QString("%1 %").arg(paramNode->GetTruePositivesPercent(),0,'f',2) );
    d->lineEdit_TrueNegatives->setText(
      QString("%1 %").arg(paramNode->GetTrueNegativesPercent(),0,'f',2) );
    d->lineEdit_FalsePositives->setText(
      QString("%1 %").arg(paramNode->GetFalsePositivesPercent(),0,'f',2) );
    d->lineEdit_FalseNegatives->setText(
      QString("%1 %").arg(paramNode->GetFalseNegativesPercent(),0,'f',2) );
    d->lineEdit_ReferenceCenter->setText(
      QString("(%1, %2, %3 )")
      .arg(paramNode->GetReferenceCenter()[0],7,'f',2,QLatin1Char(' '))
      .arg(paramNode->GetReferenceCenter()[1],7,'f',2,QLatin1Char(' '))
      .arg(paramNode->GetReferenceCenter()[2],7,'f',2,QLatin1Char(' ')) );
    d->lineEdit_CompareCenter->setText(
      QString("(%1, %2, %3 )")
      .arg(paramNode->GetCompareCenter()[0],7,'f',2,QLatin1Char(' '))
      .arg(paramNode->GetCompareCenter()[1],7,'f',2,QLatin1Char(' '))
      .arg(paramNode->GetCompareCenter()[2],7,'f',2,QLatin1Char(' ')) );
    d->lineEdit_ReferenceVolume->setText(
      QString("%1 cc").arg(paramNode->GetReferenceVolumeCc()) );
    d->lineEdit_CompareVolume->setText(
      QString("%1 cc").arg(paramNode->GetCompareVolumeCc()) );
  }
  else
  {
    d->label_Error->setText(QString(errorMessage.c_str()));
    this->invalidateDiceResults();
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::invalidateDiceResults()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentComparisonModuleWidget::invalidateDiceResults: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DiceResultsValidOff();

  d->lineEdit_DiceCoefficient->setText(tr("N/A"));
  d->lineEdit_TruePositives->setText(tr("N/A"));
  d->lineEdit_TrueNegatives->setText(tr("N/A"));
  d->lineEdit_FalsePositives->setText(tr("N/A"));
  d->lineEdit_FalseNegatives->setText(tr("N/A"));
  d->lineEdit_ReferenceCenter->setText(tr("N/A"));
  d->lineEdit_CompareCenter->setText(tr("N/A"));
  d->lineEdit_ReferenceVolume->setText(tr("N/A"));
  d->lineEdit_CompareVolume->setText(tr("N/A"));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModuleWidget::invalidateHausdorffResults()
{
  Q_D(qSlicerSegmentComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentComparisonModuleWidget::invalidateHausdorffResults: Invalid scene!";
    return;
  }

  vtkMRMLSegmentComparisonNode* paramNode = d->logic()->GetSegmentComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->HausdorffResultsValidOff();

  d->lineEdit_MaximumHausdorffDistanceForBoundary->setText(tr("N/A"));
  d->lineEdit_AverageHausdorffDistanceForBoundary->setText(tr("N/A"));
  d->lineEdit_95PercentHausdorffDistanceForBoundary->setText(tr("N/A"));
}
