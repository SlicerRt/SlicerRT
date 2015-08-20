/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QMessageBox>

// SlicerQt includes
#include "qSlicerSegmentMorphologyModuleWidget.h"
#include "ui_qSlicerSegmentMorphologyModule.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "qMRMLSegmentSelectorWidget.h"

// Segment morphology includes
#include "vtkSlicerSegmentMorphologyModuleLogic.h"
#include "vtkMRMLSegmentMorphologyNode.h"

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_SegmentMorphology
class qSlicerSegmentMorphologyModuleWidgetPrivate: public Ui_qSlicerSegmentMorphologyModule
{
  Q_DECLARE_PUBLIC(qSlicerSegmentMorphologyModuleWidget);
protected:
  qSlicerSegmentMorphologyModuleWidget* const q_ptr;
public:
  qSlicerSegmentMorphologyModuleWidgetPrivate(qSlicerSegmentMorphologyModuleWidget &object);
  ~qSlicerSegmentMorphologyModuleWidgetPrivate();
  vtkSlicerSegmentMorphologyModuleLogic* logic() const;
public:
  bool UniformExpandShrink;
};

//-----------------------------------------------------------------------------
// qSlicerSegmentMorphologyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModuleWidgetPrivate::qSlicerSegmentMorphologyModuleWidgetPrivate(qSlicerSegmentMorphologyModuleWidget& object)
  : q_ptr(&object)
{
  this->UniformExpandShrink = true;
}

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModuleWidgetPrivate::~qSlicerSegmentMorphologyModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerSegmentMorphologyModuleLogic*
qSlicerSegmentMorphologyModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerSegmentMorphologyModuleWidget);
  return vtkSlicerSegmentMorphologyModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerSegmentMorphologyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModuleWidget::qSlicerSegmentMorphologyModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSegmentMorphologyModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModuleWidget::~qSlicerSegmentMorphologyModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetSegmentMorphologyNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLSegmentMorphologyNode");
    if (node)
    {
      this->setSegmentMorphologyNode( vtkMRMLSegmentMorphologyNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::setup()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Error->setText("");

  QButtonGroup* buttonGroup = new QButtonGroup(d->CollapsibleButton_OperationParameters);
  buttonGroup->addButton(d->radioButton_Expand);
  buttonGroup->addButton(d->radioButton_Shrink);
  buttonGroup->addButton(d->radioButton_Union);
  buttonGroup->addButton(d->radioButton_Intersect);
  buttonGroup->addButton(d->radioButton_Subtract);

  d->checkBox_Uniform->setChecked(d->UniformExpandShrink);

  // Make connections
  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setSegmentMorphologyNode(vtkMRMLNode*)) );

  connect( d->SegmentSelectorWidget_SegmentA, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(segmentationANodeChanged(vtkMRMLNode*)) );
  connect( d->SegmentSelectorWidget_SegmentA, SIGNAL(currentSegmentChanged(QString)), this, SLOT(segmentAChanged(QString)) );
  connect( d->SegmentSelectorWidget_SegmentB, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(segmentationBNodeChanged(vtkMRMLNode*)) );
  connect( d->SegmentSelectorWidget_SegmentB, SIGNAL(currentSegmentChanged(QString)), this, SLOT(segmentBChanged(QString)) );
  connect( d->MRMLNodeComboBox_OutputSegmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(outputSegmentationNodeChanged(vtkMRMLNode*)) );

  connect( d->radioButton_Expand, SIGNAL(clicked()), this, SLOT(radioButtonExpandClicked()) );
  connect( d->radioButton_Shrink, SIGNAL(clicked()), this, SLOT(radioButtonShrinkClicked()) );
  connect( d->radioButton_Union, SIGNAL(clicked()), this, SLOT(radioButtonUnionClicked()) );
  connect( d->radioButton_Intersect, SIGNAL(clicked()), this, SLOT(radioButtonIntersectClicked()) );
  connect( d->radioButton_Subtract, SIGNAL(clicked()), this, SLOT(radioButtonSubtractClicked()) );

  connect( d->checkBox_Uniform, SIGNAL(stateChanged(int)), this, SLOT(checkBoxUniformCheckedStateChanged(int)) );
  connect( d->doubleSpinBox_XSize, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxXSizeChanged(double)) );
  connect( d->doubleSpinBox_YSize, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxYSizeChanged(double)) );
  connect( d->doubleSpinBox_ZSize, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxZSizeChanged(double)) );

  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerSegmentMorphologyModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLSegmentMorphologyNode");
    if (node)
    {
      paramNode = vtkMRMLSegmentMorphologyNode::SafeDownCast(node);
      d->logic()->SetAndObserveSegmentMorphologyNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLSegmentMorphologyNode> newNode = vtkSmartPointer<vtkMRMLSegmentMorphologyNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveSegmentMorphologyNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  d->radioButton_Expand->setChecked(false);
  d->radioButton_Shrink->setChecked(false);
  d->radioButton_Union->setChecked(false);
  d->radioButton_Intersect->setChecked(false);
  d->radioButton_Subtract->setChecked(false);

  bool sizeSpinboxesEnabled = false;

  // Set widget states according to type of operation
  int operation = paramNode->GetOperation();
  switch (operation)
  {
    case vtkMRMLSegmentMorphologyNode::Expand:
      d->radioButton_Expand->setChecked(true);
      d->SegmentSelectorWidget_SegmentB->setEnabled(false);
      sizeSpinboxesEnabled = true;
      break;
    case vtkMRMLSegmentMorphologyNode::Shrink:
      d->radioButton_Shrink->setChecked(true);
      d->SegmentSelectorWidget_SegmentB->setEnabled(false);
      sizeSpinboxesEnabled = true;
      break;
    case vtkMRMLSegmentMorphologyNode::Union:
      d->radioButton_Union->setChecked(true);
      d->SegmentSelectorWidget_SegmentB->setEnabled(true);
      break;
    case vtkMRMLSegmentMorphologyNode::Intersect:
      d->radioButton_Intersect->setChecked(true);
      d->SegmentSelectorWidget_SegmentB->setEnabled(true);
      break;
    case vtkMRMLSegmentMorphologyNode::Subtract:
      d->radioButton_Subtract->setChecked(true);
      d->SegmentSelectorWidget_SegmentB->setEnabled(true);
      break;
    default:
      vtkErrorWithObjectMacro(this->mrmlScene(), "updateWidgetFromMRML: Invalid morphology operation!");
      break;
  }

  d->doubleSpinBox_XSize->setEnabled(sizeSpinboxesEnabled);
  d->doubleSpinBox_YSize->setEnabled(sizeSpinboxesEnabled);
  d->doubleSpinBox_ZSize->setEnabled(sizeSpinboxesEnabled);

  // Apply parameter node parameters to the selector widgets
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);

  if (paramNode->GetSegmentationANode())
  {
    d->SegmentSelectorWidget_SegmentA->setCurrentNode(paramNode->GetSegmentationANode());
  }
  else
  {
    this->segmentationANodeChanged(d->SegmentSelectorWidget_SegmentA->currentNode());
  }
  if (paramNode->GetSegmentAID())
  {
    d->SegmentSelectorWidget_SegmentA->setCurrentSegmentID(paramNode->GetSegmentAID());
  }

  if (paramNode->GetSegmentationBNode())
  {
    d->SegmentSelectorWidget_SegmentB->setCurrentNode(paramNode->GetSegmentationBNode());
  }
  else
  {
    this->segmentationBNodeChanged(d->SegmentSelectorWidget_SegmentB->currentNode());
  }
  if (paramNode->GetSegmentBID())
  {
    d->SegmentSelectorWidget_SegmentB->setCurrentSegmentID(paramNode->GetSegmentBID());
  }
  if (paramNode->GetOutputSegmentationNode())
  {
    d->MRMLNodeComboBox_OutputSegmentation->setCurrentNode(paramNode->GetOutputSegmentationNode());
  }
  else
  {
    this->outputSegmentationNodeChanged(d->MRMLNodeComboBox_OutputSegmentation->currentNode());
  }

  d->doubleSpinBox_XSize->setValue(paramNode->GetXSize());
  d->doubleSpinBox_YSize->setValue(paramNode->GetYSize());
  d->doubleSpinBox_ZSize->setValue(paramNode->GetZSize());

  // Update buttons state according to other widgets states
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::updateButtonsState()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::updateButtonsState: Invalid scene!";
    return;
  }

  bool applyEnabled = d->logic()->GetSegmentMorphologyNode()
                   && d->logic()->GetSegmentMorphologyNode()->GetSegmentationANode()
                   && d->logic()->GetSegmentMorphologyNode()->GetSegmentAID()
                   && ( d->SegmentSelectorWidget_SegmentB->isEnabled()
                     ?  d->logic()->GetSegmentMorphologyNode()->GetSegmentationBNode()
                     && d->logic()->GetSegmentMorphologyNode()->GetSegmentBID()
                     : true );
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::setSegmentMorphologyNode(vtkMRMLNode *node)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::setSegmentMorphologyNode: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = vtkMRMLSegmentMorphologyNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetSegmentMorphologyNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveSegmentMorphologyNode(paramNode);

  // Set selected MRML nodes in comboboxes in the parameter set if it was NULL there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if ( !paramNode->GetSegmentationANode()
      && d->SegmentSelectorWidget_SegmentA->currentNode() )
    {
      paramNode->SetAndObserveSegmentationANode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_SegmentA->currentNode()));
    }
    if ( !paramNode->GetSegmentAID()
      && !d->SegmentSelectorWidget_SegmentA->currentSegmentID().isEmpty() )
    {
      paramNode->SetSegmentAID(d->SegmentSelectorWidget_SegmentA->currentSegmentID().toLatin1().constData());
    }
    if ( !paramNode->GetSegmentationBNode()
      && d->SegmentSelectorWidget_SegmentB->currentNode() )
    {
      paramNode->SetAndObserveSegmentationBNode(vtkMRMLSegmentationNode::SafeDownCast(d->SegmentSelectorWidget_SegmentB->currentNode()));
    }
    if ( !paramNode->GetSegmentBID()
      && !d->SegmentSelectorWidget_SegmentB->currentSegmentID().isEmpty() )
    {
      paramNode->SetSegmentBID(d->SegmentSelectorWidget_SegmentB->currentSegmentID().toLatin1().constData());
    }

    this->updateButtonsState();
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::segmentationANodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::segmentationANodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveSegmentationANode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::segmentAChanged(QString segmentID)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::segmentAChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode || segmentID.isEmpty())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetSegmentAID(segmentID.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::segmentationBNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::segmentationBNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveSegmentationBNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::segmentBChanged(QString segmentID)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::segmentBChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode || segmentID.isEmpty())
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetSegmentBID(segmentID.toLatin1().constData());
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::outputSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::outputSegmentationNodeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::radioButtonExpandClicked()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::radioButtonExpandClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLSegmentMorphologyNode::Expand);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::radioButtonShrinkClicked()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::radioButtonShrinkClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLSegmentMorphologyNode::Shrink);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::radioButtonUnionClicked()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::radioButtonUnionClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLSegmentMorphologyNode::Union);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::radioButtonIntersectClicked()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::radioButtonIntersectClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLSegmentMorphologyNode::Intersect);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::radioButtonSubtractClicked()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::radioButtonSubtractClicked: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLSegmentMorphologyNode::Subtract);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::checkBoxUniformCheckedStateChanged(int state)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  d->UniformExpandShrink = state;

  if (d->UniformExpandShrink)
  {
    d->doubleSpinBox_YSize->setValue(d->doubleSpinBox_XSize->value());
    d->doubleSpinBox_ZSize->setValue(d->doubleSpinBox_XSize->value());
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::doubleSpinBoxXSizeChanged(double value)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::doubleSpinBoxXSizeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetXSize(value);
  paramNode->DisableModifiedEventOff();

  if (d->UniformExpandShrink)
  {
    d->doubleSpinBox_YSize->setValue(value);
    d->doubleSpinBox_ZSize->setValue(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::doubleSpinBoxYSizeChanged(double value)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::doubleSpinBoxYSizeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetYSize(value);
  paramNode->DisableModifiedEventOff();

  if (d->UniformExpandShrink)
  {
    d->doubleSpinBox_XSize->setValue(value);
    d->doubleSpinBox_ZSize->setValue(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::doubleSpinBoxZSizeChanged(double value)
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::doubleSpinBoxZSizeChanged: Invalid scene!";
    return;
  }

  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetZSize(value);
  paramNode->DisableModifiedEventOff();

  if (d->UniformExpandShrink)
  {
    d->doubleSpinBox_XSize->setValue(value);
    d->doubleSpinBox_YSize->setValue(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModuleWidget::applyClicked()
{
  Q_D(qSlicerSegmentMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentMorphologyModuleWidget::applyClicked: Invalid scene!";
    return;
  }

  d->label_Error->setText("");

  // Warn user that output contains segments and data will be overwritten
  vtkMRMLSegmentMorphologyNode* paramNode = d->logic()->GetSegmentMorphologyNode();
  if (!paramNode)
  {
    return;
  }
  vtkMRMLSegmentationNode* outputSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(paramNode->GetOutputSegmentationNode());
  if (outputSegmentationNode && outputSegmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
  {
    QString message = QString("As the output segmentation will contain only the new output segment, the existing segments in the segmentation will be lost!\nWould you like to use it anyway?");
    QMessageBox::StandardButton answer =
      QMessageBox::question(NULL, tr("Output segmentation is not empty"), message,
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
    {
      return;
    }
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Perform morphological operation
  std::string errorMessage = d->logic()->ApplyMorphologyOperation();
  if (!errorMessage.empty())
  {
    d->label_Error->setText(QString(errorMessage.c_str()));
  }

  QApplication::restoreOverrideCursor();
}
