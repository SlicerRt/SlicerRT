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
#include <QProgressDialog>

// SlicerQt includes
#include "qSlicerDoseComparisonModuleWidget.h"
#include "ui_qSlicerDoseComparisonModule.h"

// QSlicer includes
#include "qSlicerApplication.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// DoseComparison includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DoseComparison
class qSlicerDoseComparisonModuleWidgetPrivate: public Ui_qSlicerDoseComparisonModule
{
  Q_DECLARE_PUBLIC(qSlicerDoseComparisonModuleWidget);
protected:
  qSlicerDoseComparisonModuleWidget* const q_ptr;
public:
  qSlicerDoseComparisonModuleWidgetPrivate(qSlicerDoseComparisonModuleWidget& object);
  vtkSlicerDoseComparisonModuleLogic* logic() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  /// Progress dialog for tracking DVH calculation progress
  QProgressDialog* GammaProgressDialog;
};

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidgetPrivate::qSlicerDoseComparisonModuleWidgetPrivate(qSlicerDoseComparisonModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , GammaProgressDialog(NULL)
{
}

//-----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic*
qSlicerDoseComparisonModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseComparisonModuleWidget);
  return vtkSlicerDoseComparisonModuleLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidget::qSlicerDoseComparisonModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerDoseComparisonModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidget::~qSlicerDoseComparisonModuleWidget()
{
}


//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is none in the scene
  if (scene && d->MRMLNodeComboBox_ParameterSet->currentNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseComparisonNode> newNode = vtkSmartPointer<vtkMRMLDoseComparisonNode>::New();
      this->mrmlScene()->AddNode(newNode);
      this->setParameterNode(newNode);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerDoseComparisonModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  // If we have a parameter node select it
  if (!paramNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
    if (node)
    {
      this->setParameterNode(node);
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseComparisonNode> newNode = vtkSmartPointer<vtkMRMLDoseComparisonNode>::New();
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
void qSlicerDoseComparisonModuleWidget::setParameterNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);

  // Make sure the parameter set node is selected (in case the function was not called by the selector combobox signal)
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  // Set selected MRML nodes in comboboxes in the parameter set if it was NULL there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if (!paramNode->GetReferenceDoseVolumeNode() && d->MRMLNodeComboBox_ReferenceDoseVolume->currentNode())
    {
      paramNode->SetAndObserveReferenceDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_ReferenceDoseVolume->currentNode()));
    }
    if (!paramNode->GetCompareDoseVolumeNode() && d->MRMLNodeComboBox_CompareDoseVolume->currentNode())
    {
      paramNode->SetAndObserveCompareDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_CompareDoseVolume->currentNode()));
    }
    if (!paramNode->GetMaskSegmentationNode() && d->SegmentSelectorWidget_Mask->currentNode())
    {
      paramNode->SetAndObserveMaskSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_Mask->currentNode()));
    }
    if ( !paramNode->GetMaskSegmentID() && !d->SegmentSelectorWidget_Mask->currentSegmentID().isEmpty() )
    {
      paramNode->SetMaskSegmentID(d->SegmentSelectorWidget_Mask->currentSegmentID().toLatin1().constData());
    }
    if (!paramNode->GetGammaVolumeNode() && d->MRMLNodeComboBox_GammaVolume->currentNode())
    {
      paramNode->SetAndObserveGammaVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_GammaVolume->currentNode()));
    }

    this->updateButtonsState();
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());

  if (paramNode && this->mrmlScene())
  {
    if (paramNode->GetReferenceDoseVolumeNode())
    {
      d->MRMLNodeComboBox_ReferenceDoseVolume->setCurrentNode(paramNode->GetReferenceDoseVolumeNode());
    }
    if (paramNode->GetCompareDoseVolumeNode())
    {
      d->MRMLNodeComboBox_CompareDoseVolume->setCurrentNode(paramNode->GetCompareDoseVolumeNode());
    }
    if (paramNode->GetMaskSegmentationNode())
    {
      d->SegmentSelectorWidget_Mask->setCurrentNode(paramNode->GetMaskSegmentationNode());
    }
    if (paramNode->GetMaskSegmentID())
    {
      d->SegmentSelectorWidget_Mask->setCurrentSegmentID(paramNode->GetMaskSegmentID());
    }
    if (paramNode->GetGammaVolumeNode())
    {
      d->MRMLNodeComboBox_GammaVolume->setCurrentNode(paramNode->GetGammaVolumeNode());
    }
    d->doubleSpinBox_DtaDistanceTolerance->setValue(paramNode->GetDtaDistanceToleranceMm());
    d->doubleSpinBox_DoseDifferenceTolerance->setValue(paramNode->GetDoseDifferenceTolerancePercent());
    d->doubleSpinBox_ReferenceDose_cGy->setValue(paramNode->GetReferenceDoseGy() * 100.0); // Spinbox shows cGy
    d->doubleSpinBox_AnalysisThreshold->setValue(paramNode->GetAnalysisThresholdPercent());
    d->checkBox_LinearInterpolation->setChecked(paramNode->GetUseLinearInterpolation());
    d->checkBox_Local->setChecked(paramNode->GetLocalDoseDifference());
    d->doubleSpinBox_MaximumGamma->setValue(paramNode->GetMaximumGamma());
    if (paramNode->GetUseMaximumDose())
    {
      d->radioButton_ReferenceDose_MaximumDose->setChecked(true);
    }
    else
    {
      d->radioButton_ReferenceDose_CustomValue->setChecked(true);
    }
    d->checkBox_ThresholdReferenceOnly->setChecked(paramNode->GetDoseThresholdOnReferenceOnly());
  }

  this->refreshOutputBaseName();
  this->checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::setup()
{
  Q_D(qSlicerDoseComparisonModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Warning->setText("");

  d->SegmentSelectorWidget_Mask->setNoneEnabled(true);

  // Make connections
  connect( d->MRMLNodeComboBox_ReferenceDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_CompareDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(compareDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->SegmentSelectorWidget_Mask, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(maskSegmentationNodeChanged(vtkMRMLNode*)) );
  connect( d->SegmentSelectorWidget_Mask, SIGNAL(currentSegmentChanged(QString)), this, SLOT(maskSegmentChanged(QString)) );
  connect( d->MRMLNodeComboBox_GammaVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(gammaVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->doubleSpinBox_DtaDistanceTolerance, SIGNAL(valueChanged(double)), this, SLOT(dtaDistanceToleranceChanged(double)) );
  connect( d->doubleSpinBox_DoseDifferenceTolerance, SIGNAL(valueChanged(double)), this, SLOT(doseDifferenceToleranceChanged(double)) );
  connect( d->doubleSpinBox_ReferenceDose_cGy, SIGNAL(valueChanged(double)), this, SLOT(referenceDoseChanged(double)) );
  connect( d->doubleSpinBox_AnalysisThreshold, SIGNAL(valueChanged(double)), this, SLOT(analysisThresholdChanged(double)) );
  connect( d->checkBox_LinearInterpolation, SIGNAL(stateChanged(int)), this, SLOT(linearInterpolationCheckedStateChanged(int)) );
  connect( d->checkBox_Local, SIGNAL(stateChanged(int)), this, SLOT(localDoseDifferenceCheckedStateChanged(int)) );
  connect( d->doubleSpinBox_MaximumGamma, SIGNAL(valueChanged(double)), this, SLOT(maximumGammaChanged(double)) );
  connect( d->radioButton_ReferenceDose_MaximumDose, SIGNAL(toggled(bool)), this, SLOT(referenceDoseUseMaximumDoseChanged(bool)) );
  connect( d->checkBox_ThresholdReferenceOnly, SIGNAL(stateChanged(int)), this, SLOT(doseThresholdOnReferenceOnlyCheckedStateChanged(int)) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setParameterNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  bool applyEnabled = paramNode
                   && paramNode->GetReferenceDoseVolumeNode()
                   && paramNode->GetCompareDoseVolumeNode()
                   && paramNode->GetGammaVolumeNode();
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::referenceDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid reference dose volume node set!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->refreshOutputBaseName();
  this->invalidateResults();
  this->updateButtonsState();
  this->checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::compareDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid compare dose volume node set!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveCompareDoseVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->refreshOutputBaseName();
  this->invalidateResults();
  this->updateButtonsState();
  this->checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::maskSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveMaskSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::maskSegmentChanged(QString segmentID)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetMaskSegmentID(segmentID.isEmpty() ? NULL : segmentID.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::gammaVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }
  if (!node)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid output gamma volume node set!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveGammaVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::dtaDistanceToleranceChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDtaDistanceToleranceMm(value);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::doseDifferenceToleranceChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDoseDifferenceTolerancePercent(value);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::referenceDoseUseMaximumDoseChanged(bool state)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  d->doubleSpinBox_ReferenceDose_cGy->setEnabled(!state);

  paramNode->DisableModifiedEventOn();
  paramNode->SetUseMaximumDose(state);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::referenceDoseChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetReferenceDoseGy(value / 100.0);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::analysisThresholdChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAnalysisThresholdPercent(value);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::linearInterpolationCheckedStateChanged(int state)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetUseLinearInterpolation(state);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::localDoseDifferenceCheckedStateChanged(int state)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetLocalDoseDifference(state);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::maximumGammaChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetMaximumGamma(value);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::doseThresholdOnReferenceOnlyCheckedStateChanged(int state)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDoseThresholdOnReferenceOnly(state);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::applyClicked()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  d->label_Warning->setText("");

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // Initialize progress bar
  qvtkConnect( d->logic(), SlicerRtCommon::ProgressUpdated, this, SLOT( onProgressUpdated(vtkObject*,void*,unsigned long,void*) ) );
  d->GammaProgressDialog = new QProgressDialog((QWidget*)qSlicerApplication::application()->mainWindow());
  d->GammaProgressDialog->setModal(true);
  d->GammaProgressDialog->setMinimumDuration(150);
  d->GammaProgressDialog->setLabelText("Computing gamma dose difference...");
  d->GammaProgressDialog->show();
  QApplication::processEvents();

  std::string errorMessage = d->logic()->ComputeGammaDoseDifference(paramNode);
  if (!errorMessage.empty())
  {
    d->label_Warning->setText( QString(errorMessage.c_str()) );
  }

  if (paramNode->GetResultsValid())
  {
    d->lineEdit_PassFraction->setText(
      QString("%1 %").arg(paramNode->GetPassFractionPercent(),0,'f',2) );
  }

  qvtkDisconnect( d->logic(), SlicerRtCommon::ProgressUpdated, this, SLOT( onProgressUpdated(vtkObject*,void*,unsigned long,void*) ) );
  delete d->GammaProgressDialog;
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::onProgressUpdated(vtkObject* caller, void* callData, unsigned long eid, void* clientData)
{
  Q_D(qSlicerDoseComparisonModuleWidget);
  Q_UNUSED(caller);
  Q_UNUSED(eid);
  Q_UNUSED(clientData);

  if (!d->GammaProgressDialog)
  {
    return;
  }

  double* progress = reinterpret_cast<double*>(callData);
  d->GammaProgressDialog->setValue((int)((*progress)*100.0));
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::checkDoseVolumeAttributes()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!this->mrmlScene() || !paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = paramNode->GetReferenceDoseVolumeNode();
  vtkMRMLScalarVolumeNode* compareDoseVolumeNode = paramNode->GetCompareDoseVolumeNode();

  if (referenceDoseVolumeNode && !SlicerRtCommon::IsDoseVolumeNode(referenceDoseVolumeNode))
  {
    d->label_Warning->setText(tr(" Selected reference volume is not a dose"));
  }
  else if (compareDoseVolumeNode && !SlicerRtCommon::IsDoseVolumeNode(compareDoseVolumeNode))
  {
    d->label_Warning->setText(tr(" Selected compare volume is not a dose"));
  }
  else
  {
    d->label_Warning->setText("");
  }
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!this->mrmlScene() || !paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QString newBaseName(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX.c_str());

  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = paramNode->GetReferenceDoseVolumeNode();
  if (referenceDoseVolumeNode)
  {
    newBaseName.append(referenceDoseVolumeNode->GetName());
  }

  vtkMRMLScalarVolumeNode* compareDoseVolumeNode = paramNode->GetCompareDoseVolumeNode();
  if (compareDoseVolumeNode)
  {
    newBaseName.append(compareDoseVolumeNode->GetName());
  }

  d->MRMLNodeComboBox_GammaVolume->setBaseName( newBaseName );
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::invalidateResults()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(d->MRMLNodeComboBox_ParameterSet->currentNode());
  if (!this->mrmlScene() || !paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->ResultsValidOff();

  d->lineEdit_PassFraction->setText(tr("N/A"));
}
