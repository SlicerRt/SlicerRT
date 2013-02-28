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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Qt includes

// SlicerQt includes
#include "qSlicerContourComparisonModuleWidget.h"
#include "ui_qSlicerContourComparisonModule.h"

// ContourComparison includes
#include "vtkSlicerContourComparisonModuleLogic.h"
#include "vtkMRMLContourComparisonNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ContourComparison
class qSlicerContourComparisonModuleWidgetPrivate: public Ui_qSlicerContourComparisonModule
{
  Q_DECLARE_PUBLIC(qSlicerContourComparisonModuleWidget);
protected:
  qSlicerContourComparisonModuleWidget* const q_ptr;
public:
  qSlicerContourComparisonModuleWidgetPrivate(qSlicerContourComparisonModuleWidget& object);
  vtkSlicerContourComparisonModuleLogic* logic() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerContourComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerContourComparisonModuleWidgetPrivate::qSlicerContourComparisonModuleWidgetPrivate(qSlicerContourComparisonModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogic*
qSlicerContourComparisonModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerContourComparisonModuleWidget);
  return vtkSlicerContourComparisonModuleLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerContourComparisonModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerContourComparisonModuleWidget::qSlicerContourComparisonModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerContourComparisonModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerContourComparisonModuleWidget::~qSlicerContourComparisonModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetContourComparisonNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLContourComparisonNode");
    if (node)
    {
      this->setContourComparisonNode( vtkMRMLContourComparisonNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerContourComparisonModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLContourComparisonNode");
    if (node)
    {
      paramNode = vtkMRMLContourComparisonNode::SafeDownCast(node);
      d->logic()->SetAndObserveContourComparisonNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLContourComparisonNode> newNode = vtkSmartPointer<vtkMRMLContourComparisonNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveContourComparisonNode(newNode);
    }
  }

  d->ModuleWindowInitialized = true;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::setContourComparisonNode(vtkMRMLNode *node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = vtkMRMLContourComparisonNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetContourComparisonNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveContourComparisonNode(paramNode);

  // Set selected MRML nodes in comboboxes in the parameter set if it was NULL there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if ( SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetReferenceContourNodeId())
      && d->MRMLNodeComboBox_ReferenceContour->currentNode() )
    {
      paramNode->SetAndObserveReferenceContourNodeId(d->MRMLNodeComboBox_ReferenceContour->currentNodeId().toLatin1());
    }
    if ( SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetCompareContourNodeId())
      && d->MRMLNodeComboBox_CompareContour->currentNode() )
    {
      paramNode->SetAndObserveCompareContourNodeId(d->MRMLNodeComboBox_CompareContour->currentNodeId().toLatin1());
    }
    if ( SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetRasterizationReferenceVolumeNodeId())
      && d->MRMLNodeComboBox_ReferenceVolume->currentNode() )
    {
      paramNode->SetAndObserveReferenceVolumeNodeId(d->MRMLNodeComboBox_ReferenceVolume->currentNodeId().toLatin1());
    }
    this->updateButtonsState();
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetContourComparisonNode());
  if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetReferenceContourNodeId()))
  {
    d->MRMLNodeComboBox_ReferenceContour->setCurrentNode(paramNode->GetReferenceContourNodeId());
  }
  if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetCompareContourNodeId()))
  {
    d->MRMLNodeComboBox_CompareContour->setCurrentNode(paramNode->GetCompareContourNodeId());
  }

  bool referenceVolumeNeeded = d->logic()->IsReferenceVolumeNeeded();

  bool referenceVolumeSelected = ( !SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetRasterizationReferenceVolumeNodeId()) );
  d->label_NoReferenceVolumeIsSelected->setVisible( referenceVolumeNeeded && !referenceVolumeSelected );
  d->label_NoReferenceVolumeIsNeeded->setVisible( !referenceVolumeNeeded );
  d->MRMLNodeComboBox_ReferenceVolume->setVisible( referenceVolumeNeeded );
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::setup()
{
  Q_D(qSlicerContourComparisonModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_NoReferenceVolumeIsSelected->setVisible(false);
  d->label_NoReferenceVolumeIsNeeded->setVisible(false);
  d->label_Error->setText("");

  // Make connections
  connect( d->MRMLNodeComboBox_ReferenceContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceContourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_CompareContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(compareContourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->pushButton_ComputeDice, SIGNAL(clicked()), this, SLOT(computeDiceClicked()) );
  connect( d->pushButton_ComputeHausdorff, SIGNAL(clicked()), this, SLOT(computeHausdorffClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setContourComparisonNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  bool computeEnabled = d->logic()->GetContourComparisonNode()
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourComparisonNode()->GetReferenceContourNodeId())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourComparisonNode()->GetCompareContourNodeId())
                   && ( ! d->logic()->IsReferenceVolumeNeeded()
                     || ( !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourComparisonNode()->GetRasterizationReferenceVolumeNodeId()) ) );
  d->pushButton_ComputeDice->setEnabled(computeEnabled);
  d->pushButton_ComputeHausdorff->setEnabled(computeEnabled);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::onLogicModified()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::referenceContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceContourNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::compareContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveCompareContourNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->invalidateDiceResults();
  this->invalidateHausdorffResults();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::computeHausdorffClicked()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::string errorMessage;
  d->logic()->ComputeHausdorffDistances(errorMessage);

  if (paramNode->GetHausdorffResultsValid())
  {
    d->label_Error->setText("");

    d->lineEdit_MaximumHausdorffDistance->setText(
      QString("%1 mm").arg(paramNode->GetMaximumHausdorffDistanceMm(),0,'f',2) );
    d->lineEdit_AverageHausdorffDistanceForVolume->setText(
      QString("%1 mm").arg(paramNode->GetAverageHausdorffDistanceForVolumeMm(),0,'f',2) );
    d->lineEdit_AverageHausdorffDistanceForBoundary->setText(
      QString("%1 mm").arg(paramNode->GetAverageHausdorffDistanceForBoundaryMm(),0,'f',2) );
    d->lineEdit_95PercentHausdorffDistanceForVolume->setText(
      QString("%1 mm").arg(paramNode->GetPercent95HausdorffDistanceForVolumeMm(),0,'f',2) );
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
void qSlicerContourComparisonModuleWidget::computeDiceClicked()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  std::string errorMessage;
  d->logic()->ComputeDiceStatistics(errorMessage);

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
void qSlicerContourComparisonModuleWidget::invalidateDiceResults()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
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
void qSlicerContourComparisonModuleWidget::invalidateHausdorffResults()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->HausdorffResultsValidOff();

  d->lineEdit_MaximumHausdorffDistance->setText(tr("N/A"));
  d->lineEdit_AverageHausdorffDistanceForVolume->setText(tr("N/A"));
  d->lineEdit_AverageHausdorffDistanceForBoundary->setText(tr("N/A"));
  d->lineEdit_95PercentHausdorffDistanceForVolume->setText(tr("N/A"));
  d->lineEdit_95PercentHausdorffDistanceForBoundary->setText(tr("N/A"));
}
