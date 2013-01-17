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
#include "qSlicerDoseComparisonModuleWidget.h"
#include "ui_qSlicerDoseComparisonModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// DoseComparison includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseComparison
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
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerDoseComparisonModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
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
    if ( (SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetReferenceDoseVolumeNodeId()))
      && d->MRMLNodeComboBox_ReferenceDoseVolume->currentNode() )
    {
      paramNode->SetAndObserveReferenceDoseVolumeNodeId(d->MRMLNodeComboBox_ReferenceDoseVolume->currentNodeId().toLatin1());
    }
    if ( (SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetCompareDoseVolumeNodeId()))
      && d->MRMLNodeComboBox_CompareDoseVolume->currentNode() )
    {
      paramNode->SetAndObserveCompareDoseVolumeNodeId(d->MRMLNodeComboBox_CompareDoseVolume->currentNodeId().toLatin1());
    }
    if ( (SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetGammaVolumeNodeId()))
      && d->MRMLNodeComboBox_GammaVolume->currentNode())
    {
      paramNode->SetAndObserveGammaVolumeNodeId(d->MRMLNodeComboBox_GammaVolume->currentNodeId().toLatin1());
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
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetReferenceDoseVolumeNodeId()))
    {
      d->MRMLNodeComboBox_ReferenceDoseVolume->setCurrentNode(paramNode->GetReferenceDoseVolumeNodeId());
    }
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetCompareDoseVolumeNodeId()))
    {
      d->MRMLNodeComboBox_CompareDoseVolume->setCurrentNode(paramNode->GetCompareDoseVolumeNodeId());
    }
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetGammaVolumeNodeId()))
    {
      d->MRMLNodeComboBox_GammaVolume->setCurrentNode(paramNode->GetGammaVolumeNodeId());
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
  connect( d->MRMLNodeComboBox_GammaVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(gammaVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->doubleSpinBox_DtaDistanceTolerance, SIGNAL(valueChanged(double)), this, SLOT(dtaDistanceToleranceChanged(double)) );
  connect( d->doubleSpinBox_DoseDifferenceTolerance, SIGNAL(valueChanged(double)), this, SLOT(doseDifferenceToleranceChanged(double)) );
  connect( d->doubleSpinBox_ReferenceDose, SIGNAL(valueChanged(double)), this, SLOT(referenceDoseChanged(double)) );
  connect( d->doubleSpinBox_AnalysisThreshold, SIGNAL(valueChanged(double)), this, SLOT(analysisThresholdChanged(double)) );
  connect( d->doubleSpinBox_MaximumGamma, SIGNAL(valueChanged(double)), this, SLOT(maximumGammaChanged(double)) );
  connect( d->radioButton_ReferenceDose_MaximumDose, SIGNAL(toggled(bool)), this, SLOT(referenceDoseUseMaximumDoseChanged(bool)) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setDoseComparisonNode(vtkMRMLNode*)) );

  // TODO: re-enable when it is implemented in Plastimatch (#135)
  d->label_7->setVisible(false);
  d->doubleSpinBox_AnalysisThreshold->setVisible(false);

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  bool applyEnabled = d->logic()->GetDoseComparisonNode()
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetDoseComparisonNode()->GetReferenceDoseVolumeNodeId())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetDoseComparisonNode()->GetCompareDoseVolumeNodeId())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetDoseComparisonNode()->GetGammaVolumeNodeId());
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

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->refreshOutputBaseName();
  this->updateButtonsState();
  this->checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::compareDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveCompareDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->refreshOutputBaseName();
  this->updateButtonsState();
  this->checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::gammaVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveGammaVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::dtaDistanceToleranceChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDtaDistanceToleranceMm(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::doseDifferenceToleranceChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDoseDifferenceTolerancePercent(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::referenceDoseUseMaximumDoseChanged(bool state)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  d->doubleSpinBox_ReferenceDose->setEnabled(!state);

  paramNode->DisableModifiedEventOn();
  paramNode->SetUseMaximumDose(state);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::referenceDoseChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetReferenceDoseGy(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::analysisThresholdChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAnalysisThresholdPercent(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::maximumGammaChanged(double value)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetMaximumGamma(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::applyClicked()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  d->logic()->ComputeGammaDoseDifference();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::checkDoseVolumeAttributes()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !d->ModuleWindowInitialized)
  {
    return;
  }

  vtkMRMLVolumeNode* referenceDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetReferenceDoseVolumeNodeId()));
  vtkMRMLVolumeNode* compareDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetCompareDoseVolumeNodeId()));

  if (referenceDoseVolumeNode && !d->logic()->DoseVolumeContainsDose(referenceDoseVolumeNode))
  {
    d->label_Warning->setText(tr(" Selected reference volume is not a dose"));
  }
  else if (compareDoseVolumeNode && !d->logic()->DoseVolumeContainsDose(compareDoseVolumeNode))
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
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QString newBaseName(SlicerRtCommon::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX.c_str());

  vtkMRMLVolumeNode* referenceDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetReferenceDoseVolumeNodeId()));
  if (referenceDoseVolumeNode)
  {
    newBaseName.append(referenceDoseVolumeNode->GetName());
  }

  vtkMRMLVolumeNode* compareDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetCompareDoseVolumeNodeId()));
  if (compareDoseVolumeNode)
  {
    newBaseName.append(compareDoseVolumeNode->GetName());
  }

  d->MRMLNodeComboBox_GammaVolume->setBaseName( newBaseName.toLatin1() );
}
