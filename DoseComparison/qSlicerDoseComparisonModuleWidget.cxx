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

// SlicerQt includes
#include "qSlicerDoseComparisonModuleWidget.h"
#include "ui_qSlicerDoseComparisonModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

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
};

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidgetPrivate::qSlicerDoseComparisonModuleWidgetPrivate(qSlicerDoseComparisonModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
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
  if (scene &&  d->logic()->GetDoseComparisonNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
    if (node)
    {
      this->setDoseComparisonNode( vtkMRMLDoseComparisonNode::SafeDownCast(node) );
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
    qCritical() << "qSlicerDoseComparisonModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerDoseComparisonModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();

  // If we have a parameter node select it
  if (!paramNode)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
    if (node)
    {
      paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);
      d->logic()->SetAndObserveDoseComparisonNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLDoseComparisonNode> newNode = vtkSmartPointer<vtkMRMLDoseComparisonNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveDoseComparisonNode(newNode);
    }
  }

  d->ModuleWindowInitialized = true;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::setDoseComparisonNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetDoseComparisonNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveDoseComparisonNode(paramNode);

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
    if (!paramNode->GetMaskContourNode() && d->MRMLNodeComboBox_MaskContour->currentNode())
    {
      paramNode->SetAndObserveMaskContourNode(vtkMRMLContourNode::SafeDownCast(d->MRMLNodeComboBox_MaskContour->currentNode()));
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

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetDoseComparisonNode());
    if (paramNode->GetReferenceDoseVolumeNode())
    {
      d->MRMLNodeComboBox_ReferenceDoseVolume->setCurrentNode(paramNode->GetReferenceDoseVolumeNode());
    }
    if (paramNode->GetCompareDoseVolumeNode())
    {
      d->MRMLNodeComboBox_CompareDoseVolume->setCurrentNode(paramNode->GetCompareDoseVolumeNode());
    }
    if (paramNode->GetMaskContourNode())
    {
      d->MRMLNodeComboBox_MaskContour->setCurrentNode(paramNode->GetMaskContourNode());
    }
    if (paramNode->GetGammaVolumeNode())
    {
      d->MRMLNodeComboBox_GammaVolume->setCurrentNode(paramNode->GetGammaVolumeNode());
    }
    d->doubleSpinBox_DtaDistanceTolerance->setValue(paramNode->GetDtaDistanceToleranceMm());
    d->doubleSpinBox_DoseDifferenceTolerance->setValue(paramNode->GetDoseDifferenceTolerancePercent());
    d->doubleSpinBox_ReferenceDose->setValue(paramNode->GetReferenceDoseGy());
    d->doubleSpinBox_AnalysisThreshold->setValue(paramNode->GetAnalysisThresholdPercent());
    d->doubleSpinBox_MaximumGamma->setValue(paramNode->GetMaximumGamma());
    if (paramNode->GetUseMaximumDose())
    {
      d->radioButton_ReferenceDose_MaximumDose->setChecked(true);
    }
    else
    {
      d->radioButton_ReferenceDose_CustomValue->setChecked(true);
    }
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

  // Make connections
  connect( d->MRMLNodeComboBox_ReferenceDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_CompareDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(compareDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_MaskContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(maskContourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_GammaVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(gammaVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->doubleSpinBox_DtaDistanceTolerance, SIGNAL(valueChanged(double)), this, SLOT(dtaDistanceToleranceChanged(double)) );
  connect( d->doubleSpinBox_DoseDifferenceTolerance, SIGNAL(valueChanged(double)), this, SLOT(doseDifferenceToleranceChanged(double)) );
  connect( d->doubleSpinBox_ReferenceDose, SIGNAL(valueChanged(double)), this, SLOT(referenceDoseChanged(double)) );
  connect( d->doubleSpinBox_AnalysisThreshold, SIGNAL(valueChanged(double)), this, SLOT(analysisThresholdChanged(double)) );
  connect( d->doubleSpinBox_MaximumGamma, SIGNAL(valueChanged(double)), this, SLOT(maximumGammaChanged(double)) );
  connect( d->radioButton_ReferenceDose_MaximumDose, SIGNAL(toggled(bool)), this, SLOT(referenceDoseUseMaximumDoseChanged(bool)) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setDoseComparisonNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  bool applyEnabled = d->logic()->GetDoseComparisonNode()
                   && d->logic()->GetDoseComparisonNode()->GetReferenceDoseVolumeNode()
                   && d->logic()->GetDoseComparisonNode()->GetCompareDoseVolumeNode()
                   && d->logic()->GetDoseComparisonNode()->GetGammaVolumeNode();
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
    qCritical() << "qSlicerDoseComparisonModuleWidget::referenceDoseVolumeNodeChanged: Invalid scene!";
    return;
  }
  if (!node)
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::referenceDoseVolumeNodeChanged: Invalid reference dose volume node set!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
    qCritical() << "qSlicerDoseComparisonModuleWidget::compareDoseVolumeNodeChanged: Invalid scene!";
    return;
  }
  if (!node)
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::compareDoseVolumeNodeChanged: Invalid compare dose volume node set!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
void qSlicerDoseComparisonModuleWidget::maskContourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::maskContourNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveMaskContourNode(vtkMRMLContourNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::gammaVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::gammaVolumeNodeChanged: Invalid scene!";
    return;
  }
  if (!node)
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::gammaVolumeNodeChanged: Invalid output gamma volume node set!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
    qCritical() << "qSlicerDoseComparisonModuleWidget::dtaDistanceToleranceChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
    qCritical() << "qSlicerDoseComparisonModuleWidget::doseDifferenceToleranceChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
    qCritical() << "qSlicerDoseComparisonModuleWidget::referenceDoseUseMaximumDoseChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  d->doubleSpinBox_ReferenceDose->setEnabled(!state);

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
    qCritical() << "qSlicerDoseComparisonModuleWidget::referenceDoseChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetReferenceDoseGy(value);
  paramNode->DisableModifiedEventOff();

  this->invalidateResults();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::analysisThresholdChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::analysisThresholdChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
void qSlicerDoseComparisonModuleWidget::maximumGammaChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::maximumGammaChanged: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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
void qSlicerDoseComparisonModuleWidget::applyClicked()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerDoseComparisonModuleWidget::applyClicked: Invalid scene!";
    return;
  }

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  d->logic()->ComputeGammaDoseDifference();

  if (paramNode->GetResultsValid())
  {
    d->lineEdit_PassFraction->setText(
      QString("%1 %").arg(paramNode->GetPassFractionPercent(),0,'f',2) );
  }

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::checkDoseVolumeAttributes()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
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

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!this->mrmlScene() || !paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  QString newBaseName(SlicerRtCommon::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX.c_str());

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

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!this->mrmlScene() || !paramNode || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->ResultsValidOff();

  d->lineEdit_PassFraction->setText(tr("N/A"));
}
