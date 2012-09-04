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

  updateWidgetFromMRML();
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
    if ((!paramNode->GetReferenceDoseVolumeNodeId() || strcmp(paramNode->GetReferenceDoseVolumeNodeId(), ""))
      && d->MRMLNodeComboBox_ReferenceDoseVolume->currentNode())
    {
      paramNode->SetAndObserveReferenceDoseVolumeNodeId(d->MRMLNodeComboBox_ReferenceDoseVolume->currentNodeId().toLatin1());
    }
    if ((!paramNode->GetCompareDoseVolumeNodeId() || strcmp(paramNode->GetCompareDoseVolumeNodeId(), ""))
      && d->MRMLNodeComboBox_CompareDoseVolume->currentNode())
    {
      paramNode->SetAndObserveCompareDoseVolumeNodeId(d->MRMLNodeComboBox_CompareDoseVolume->currentNodeId().toLatin1());
    }
    if ((!paramNode->GetGammaVolumeNodeId() || strcmp(paramNode->GetGammaVolumeNodeId(), ""))
      && d->MRMLNodeComboBox_GammaVolume->currentNode())
    {
      paramNode->SetAndObserveGammaVolumeNodeId(d->MRMLNodeComboBox_GammaVolume->currentNodeId().toLatin1());
    }
    updateButtonsState();
  }

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetContourComparisonNode());
    if (paramNode->GetReferenceDoseVolumeNodeId() && strcmp(paramNode->GetReferenceDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_ReferenceDoseVolume->setCurrentNode(paramNode->GetReferenceDoseVolumeNodeId());
    }
    if (paramNode->GetCompareDoseVolumeNodeId() && strcmp(paramNode->GetCompareDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_CompareDoseVolume->setCurrentNode(paramNode->GetCompareDoseVolumeNodeId());
    }
    if (paramNode->GetGammaVolumeNodeId() && strcmp(paramNode->GetGammaVolumeNodeId(),""))
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
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::setup()
{
  Q_D(qSlicerContourComparisonModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Warning->setText("");

  // Make connections
  connect( d->MRMLNodeComboBox_ReferenceDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_CompareDoseVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(compareDoseVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_GammaVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(GammaVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->doubleSpinBox_DtaDistanceTolerance, SIGNAL(valueChanged(double)), this, SLOT(dtaDistanceToleranceChanged(double)) );
  connect( d->doubleSpinBox_DoseDifferenceTolerance, SIGNAL(valueChanged(double)), this, SLOT(doseDifferenceToleranceChanged(double)) );
  connect( d->doubleSpinBox_ReferenceDose, SIGNAL(valueChanged(double)), this, SLOT(referenceDoseChanged(double)) );
  connect( d->doubleSpinBox_AnalysisThreshold, SIGNAL(valueChanged(double)), this, SLOT(analysisThresholdChanged(double)) );
  connect( d->doubleSpinBox_MaximumGamma, SIGNAL(valueChanged(double)), this, SLOT(maximumGammaChanged(double)) );
  connect( d->radioButton_ReferenceDose_MaximumDose, SIGNAL(toggled(bool)), this, SLOT(referenceDoseUseMaximumDoseChanged(bool)) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setContourComparisonNode(vtkMRMLNode*)) );

  // TODO: re-enable when it is implemented in Plastimatch (#135)
  d->label_7->setVisible(false);
  d->doubleSpinBox_AnalysisThreshold->setVisible(false);

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  bool applyEnabled = d->logic()->GetContourComparisonNode()
                   && d->logic()->GetContourComparisonNode()->GetReferenceDoseVolumeNodeId()
                   && strcmp(d->logic()->GetContourComparisonNode()->GetReferenceDoseVolumeNodeId(), "")
                   && d->logic()->GetContourComparisonNode()->GetCompareDoseVolumeNodeId()
                   && strcmp(d->logic()->GetContourComparisonNode()->GetCompareDoseVolumeNodeId(), "")
                   && d->logic()->GetContourComparisonNode()->GetGammaVolumeNodeId()
                   && strcmp(d->logic()->GetContourComparisonNode()->GetGammaVolumeNodeId(), "");
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::onLogicModified()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::referenceDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();

  checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::compareDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveCompareDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();

  checkDoseVolumeAttributes();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::GammaVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveGammaVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::dtaDistanceToleranceChanged(double value)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDtaDistanceToleranceMm(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::doseDifferenceToleranceChanged(double value)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetDoseDifferenceTolerancePercent(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::referenceDoseUseMaximumDoseChanged(bool state)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
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
void qSlicerContourComparisonModuleWidget::referenceDoseChanged(double value)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetReferenceDoseGy(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::analysisThresholdChanged(double value)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAnalysisThresholdPercent(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::maximumGammaChanged(double value)
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetMaximumGamma(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::applyClicked()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  d->logic()->ComputeGammaDoseDifference();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModuleWidget::checkDoseVolumeAttributes()
{
  Q_D(qSlicerContourComparisonModuleWidget);

  vtkMRMLContourComparisonNode* paramNode = d->logic()->GetContourComparisonNode();
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
