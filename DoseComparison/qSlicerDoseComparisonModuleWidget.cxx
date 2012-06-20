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

// DoseComparison includes
#include "vtkSlicerDoseComparisonLogic.h"
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
  vtkSlicerDoseComparisonLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModuleWidgetPrivate::qSlicerDoseComparisonModuleWidgetPrivate(qSlicerDoseComparisonModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
vtkSlicerDoseComparisonLogic*
qSlicerDoseComparisonModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDoseComparisonModuleWidget);
  return vtkSlicerDoseComparisonLogic::SafeDownCast(q->logic());
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

  // Find parameters node or create it if there is no one in the scene
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

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::setDoseComparisonNode(vtkMRMLNode *node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetDoseComparisonNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveDoseComparisonNode(paramNode);
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (paramNode && this->mrmlScene())
  {/*
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetDoseComparisonNode());
    d->checkBox_ShowDoseVolumesOnly->setChecked(paramNode->GetShowDoseVolumesOnly());
    if (paramNode->GetAccumulatedDoseVolumeNodeId() && stricmp(paramNode->GetAccumulatedDoseVolumeNodeId(),""))
    {
      d->MRMLNodeComboBox_AccumulatedDoseVolume->setCurrentNode(paramNode->GetAccumulatedDoseVolumeNodeId());
    }*/
  }
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
  connect( d->MRMLNodeComboBox_GammaVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(gammaDoseVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->doubleSpinBox_DtaDistanceTolerance, SIGNAL(valueChanged(double)), this, SLOT(dtaDistanceToleranceChanged(double)) );
  connect( d->doubleSpinBox_DoseDifferenceTolerance, SIGNAL(valueChanged(double)), this, SLOT(doseDifferenceToleranceChanged(double)) );
  connect( d->doubleSpinBox_ReferenceDose, SIGNAL(valueChanged(double)), this, SLOT(referenceDoseChanged(double)) );
  connect( d->doubleSpinBox_AnalysisThreshold, SIGNAL(valueChanged(double)), this, SLOT(analysisThresholdChanged(double)) );
  connect( d->doubleSpinBox_MaximumGamma, SIGNAL(valueChanged(double)), this, SLOT(maximumGammaChanged(double)) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setDoseComparisonNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::updateButtonsState()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  bool applyEnabled = d->logic()->GetDoseComparisonNode()
                   && d->logic()->GetDoseComparisonNode()->GetReferenceDoseVolumeNodeId()
                   && stricmp(d->logic()->GetDoseComparisonNode()->GetReferenceDoseVolumeNodeId(), "")
                   && d->logic()->GetDoseComparisonNode()->GetCompareDoseVolumeNodeId()
                   && stricmp(d->logic()->GetDoseComparisonNode()->GetCompareDoseVolumeNodeId(), "")
                   && d->logic()->GetDoseComparisonNode()->GetGammaDoseVolumeNodeId()
                   && stricmp(d->logic()->GetDoseComparisonNode()->GetGammaDoseVolumeNodeId(), "");
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::onLogicModified()
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::referenceDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetReferenceDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::compareDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetCompareDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModuleWidget::gammaDoseVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerDoseComparisonModuleWidget);

  vtkMRMLDoseComparisonNode* paramNode = d->logic()->GetDoseComparisonNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetGammaDoseVolumeNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  updateButtonsState();
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

  d->logic()->ComputeGammaDoseDifference();
}
