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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerContourMorphologyModuleWidget.h"
#include "ui_qSlicerContourMorphologyModule.h"

// Isodose includes
#include "vtkSlicerContourMorphologyModuleLogic.h"
#include "vtkMRMLContourMorphologyNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLContourNode.h>

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "qMRMLContourSelectorWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ContourMorphology
class qSlicerContourMorphologyModuleWidgetPrivate: public Ui_qSlicerContourMorphologyModule
{
  Q_DECLARE_PUBLIC(qSlicerContourMorphologyModuleWidget);
protected:
  qSlicerContourMorphologyModuleWidget* const q_ptr;
public:
  qSlicerContourMorphologyModuleWidgetPrivate(qSlicerContourMorphologyModuleWidget &object);
  ~qSlicerContourMorphologyModuleWidgetPrivate();
  vtkSlicerContourMorphologyModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerContourMorphologyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidgetPrivate::qSlicerContourMorphologyModuleWidgetPrivate(qSlicerContourMorphologyModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidgetPrivate::~qSlicerContourMorphologyModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerContourMorphologyModuleLogic*
qSlicerContourMorphologyModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerContourMorphologyModuleWidget);
  return vtkSlicerContourMorphologyModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerContourMorphologyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidget::qSlicerContourMorphologyModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerContourMorphologyModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModuleWidget::~qSlicerContourMorphologyModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetContourMorphologyNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLContourMorphologyNode");
    if (node)
    {
      this->setContourMorphologyNode( vtkMRMLContourMorphologyNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setup()
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Set up contour selector widgets
  d->ContourSelectorWidget_ContourA->setAcceptContourHierarchies(false);
  d->ContourSelectorWidget_ContourA->setRequiredRepresentation(vtkMRMLContourNode::IndexedLabelmap);
  d->ContourSelectorWidget_ContourB->setAcceptContourHierarchies(false);
  d->ContourSelectorWidget_ContourB->setRequiredRepresentation(vtkMRMLContourNode::IndexedLabelmap);

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setContourMorphologyNode(vtkMRMLNode*)) );

  this->connect( d->ContourSelectorWidget_ContourA, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setContourANode(vtkMRMLNode*)) );
  this->connect( d->ContourSelectorWidget_ContourB, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setContourBNode(vtkMRMLNode*)) );
  this->connect( d->MRMLNodeComboBox_OutputContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setOutputContourNode(vtkMRMLNode*)) );

  this->connect( d->radioButton_Expand, SIGNAL(clicked()), this, SLOT(radioButtonExpandClicked()) );
  this->connect( d->radioButton_Shrink, SIGNAL(clicked()), this, SLOT(radioButtonShrinkClicked()) );
  this->connect( d->radioButton_Union, SIGNAL(clicked()), this, SLOT(radioButtonUnionClicked()) );
  this->connect( d->radioButton_Intersect, SIGNAL(clicked()), this, SLOT(radioButtonIntersectClicked()) );
  this->connect( d->radioButton_Subtract, SIGNAL(clicked()), this, SLOT(radioButtonSubtractClicked()) );

  this->connect( d->doubleSpinBox_XSize, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxXSizeChanged(double)) );
  this->connect( d->doubleSpinBox_YSize, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxYSizeChanged(double)) );
  this->connect( d->doubleSpinBox_ZSize, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxZSizeChanged(double)) );

  this->connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerContourMorphologyModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLContourMorphologyNode");
    if (node)
    {
      paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);
      d->logic()->SetAndObserveContourMorphologyNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLContourMorphologyNode> newNode = vtkSmartPointer<vtkMRMLContourMorphologyNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveContourMorphologyNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::updateWidgetFromMRML: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
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
  vtkMRMLContourMorphologyNode::ContourMorphologyOperationType operation = paramNode->GetOperation();
  switch (operation)
  {
    case vtkMRMLContourMorphologyNode::Expand:
      d->radioButton_Expand->setChecked(true);
      sizeSpinboxesEnabled = true;
      if (d->ContourSelectorWidget_ContourB->isEnabled()) // Only group/ungroup if it actually changed
      {
        this->groupContourSelectorWidgets(false);
      }
      break;
    case vtkMRMLContourMorphologyNode::Shrink:
      d->radioButton_Shrink->setChecked(true);
      sizeSpinboxesEnabled = true;
      if (d->ContourSelectorWidget_ContourB->isEnabled())
      {
        this->groupContourSelectorWidgets(false);
      }
      break;
    case vtkMRMLContourMorphologyNode::Union:
      d->radioButton_Union->setChecked(true);
      if (!d->ContourSelectorWidget_ContourB->isEnabled())
      {
        this->groupContourSelectorWidgets(true);
      }
      break;
    case vtkMRMLContourMorphologyNode::Intersect:
      d->radioButton_Intersect->setChecked(true);
      if (!d->ContourSelectorWidget_ContourB->isEnabled())
      {
        this->groupContourSelectorWidgets(true);
      }
      break;
    case vtkMRMLContourMorphologyNode::Subtract:
      d->radioButton_Subtract->setChecked(true);
      if (!d->ContourSelectorWidget_ContourB->isEnabled())
      {
        this->groupContourSelectorWidgets(true);
      }
      break;
    default:
      vtkErrorWithObjectMacro(this->mrmlScene(), "updateWidgetFromMRML: Invalid morphology operation!");
      break;
  }

  d->doubleSpinBox_XSize->setEnabled(sizeSpinboxesEnabled);
  d->doubleSpinBox_YSize->setEnabled(sizeSpinboxesEnabled);
  d->doubleSpinBox_ZSize->setEnabled(sizeSpinboxesEnabled);

  // Apply parameter node parameters to the widgets
  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
  if (paramNode->GetContourANode())
  {
    d->ContourSelectorWidget_ContourA->setCurrentNode(paramNode->GetContourANode());
  }
  else
  {
    this->setContourANode(d->ContourSelectorWidget_ContourA->currentNode());
  }
  if (paramNode->GetContourBNode())
  {
    d->ContourSelectorWidget_ContourB->setCurrentNode(paramNode->GetContourBNode());
  }
  else
  {
    this->setContourBNode(d->ContourSelectorWidget_ContourB->currentNode());
  }

  d->doubleSpinBox_XSize->setValue(paramNode->GetXSize());
  d->doubleSpinBox_YSize->setValue(paramNode->GetYSize());
  d->doubleSpinBox_ZSize->setValue(paramNode->GetZSize());

  // Update buttons state according to other widgets states
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::updateButtonsState()
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::updateButtonsState: Invalid scene!";
    return;
  }

  bool applyEnabled = d->logic()->GetContourMorphologyNode()
                   && d->logic()->GetContourMorphologyNode()->GetContourANode()
                   && (d->ContourSelectorWidget_ContourB->isEnabled() ? d->logic()->GetContourMorphologyNode()->GetContourBNode() != 0 : true)
                   && d->logic()->GetContourMorphologyNode()->GetOutputContourNode()
                   //&& d->logic()->GetContourMorphologyNode()->GetReferenceVolumeNode()
                   && (d->ContourSelectorWidget_ContourB->isEnabled() ? d->ContourSelectorWidget_ContourB->isSelectionValid() : d->ContourSelectorWidget_ContourA->isSelectionValid());
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::groupContourSelectorWidgets(bool group)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (group)
  {
    // This call propagates the attributes set to the widget to the slave
    d->ContourSelectorWidget_ContourB->addSlaveContourSelectorWidget(d->ContourSelectorWidget_ContourA);

    // Connections are made to the master contour selector node
    this->disconnect( d->ContourSelectorWidget_ContourA, SIGNAL(selectionValidityChanged(vtkMRMLNode*)), this, SLOT(updateButtonsState(vtkMRMLNode*)) );
    this->disconnect( d->ContourSelectorWidget_ContourA, SIGNAL(currentReferenceVolumeNodeChanged(vtkMRMLNode*)), this, SLOT(setReferenceVolumeNode(vtkMRMLNode*)) );
    this->connect( d->ContourSelectorWidget_ContourB, SIGNAL(selectionValidityChanged(vtkMRMLNode*)), this, SLOT(updateButtonsState(vtkMRMLNode*)) );
    this->connect( d->ContourSelectorWidget_ContourB, SIGNAL(currentReferenceVolumeNodeChanged(vtkMRMLNode*)), this, SLOT(setReferenceVolumeNode(vtkMRMLNode*)) );

    d->ContourSelectorWidget_ContourB->updateWidgetState();
  }
  else
  {
    // Ungroup contour selector widgets
    d->ContourSelectorWidget_ContourB->ungroup();
    d->ContourSelectorWidget_ContourB->setCurrentNodeID(QString());

    // Connections are made to the only active contour selector node
    this->disconnect( d->ContourSelectorWidget_ContourB, SIGNAL(selectionValidityChanged(vtkMRMLNode*)), this, SLOT(updateButtonsState(vtkMRMLNode*)) );
    this->disconnect( d->ContourSelectorWidget_ContourB, SIGNAL(currentReferenceVolumeNodeChanged(vtkMRMLNode*)), this, SLOT(setReferenceVolumeNode(vtkMRMLNode*)) );
    this->connect( d->ContourSelectorWidget_ContourA, SIGNAL(selectionValidityChanged(vtkMRMLNode*)), this, SLOT(updateButtonsState(vtkMRMLNode*)) );
    this->connect( d->ContourSelectorWidget_ContourA, SIGNAL(currentReferenceVolumeNodeChanged(vtkMRMLNode*)), this, SLOT(setReferenceVolumeNode(vtkMRMLNode*)) );

    d->ContourSelectorWidget_ContourA->updateWidgetState();
  }

  d->ContourSelectorWidget_ContourB->setEnabled(group);
}


//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourMorphologyNode(vtkMRMLNode *node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::setContourMorphologyNode: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetContourMorphologyNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveContourMorphologyNode(paramNode);

  // Set selected MRML nodes in comboboxes in the parameter set if it was NULL there
  // (then in the meantime the comboboxes selected the first one from the scene and we have to set that)
  if (paramNode)
  {
    if ( !paramNode->GetContourANode()
      && d->ContourSelectorWidget_ContourA->currentNode() )
    {
      paramNode->SetAndObserveContourANode(vtkMRMLContourNode::SafeDownCast(d->ContourSelectorWidget_ContourA->currentNode()));
    }
    if ( !paramNode->GetContourBNode()
      && d->ContourSelectorWidget_ContourB->currentNode() )
    {
      paramNode->SetAndObserveContourBNode(vtkMRMLContourNode::SafeDownCast(d->ContourSelectorWidget_ContourB->currentNode()));
    }
    if (!paramNode->GetReferenceVolumeNode())
    {
      // If binary operator es selected, then contour selector B is the one in charge
      if (d->ContourSelectorWidget_ContourB->isEnabled())
      {
        if (d->ContourSelectorWidget_ContourB->currentReferenceVolumeNode())
        {
          paramNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(d->ContourSelectorWidget_ContourB->currentReferenceVolumeNode()));
        }
      }
      else
      {
        if (d->ContourSelectorWidget_ContourA->currentReferenceVolumeNode())
        {
          paramNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(d->ContourSelectorWidget_ContourA->currentReferenceVolumeNode()));
        }
      }
    }
    this->updateButtonsState();
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourANode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::setContourANode: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveContourANode(vtkMRMLContourNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourBNode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::setContourBNode: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveContourBNode(vtkMRMLContourNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setReferenceVolumeNode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::setReferenceVolumeNode: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setOutputContourNode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::setOutputContourNode: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputContourNode(vtkMRMLContourNode::SafeDownCast(node));
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonExpandClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::radioButtonExpandClicked: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLContourMorphologyNode::Expand);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonShrinkClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::radioButtonShrinkClicked: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLContourMorphologyNode::Shrink);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonUnionClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::radioButtonUnionClicked: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLContourMorphologyNode::Union);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonIntersectClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::radioButtonIntersectClicked: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLContourMorphologyNode::Intersect);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonSubtractClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::radioButtonSubtractClicked: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetOperation(vtkMRMLContourMorphologyNode::Subtract);
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::doubleSpinBoxXSizeChanged(double value)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::doubleSpinBoxXSizeChanged: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetXSize(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::doubleSpinBoxYSizeChanged(double value)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::doubleSpinBoxYSizeChanged: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetYSize(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::doubleSpinBoxZSizeChanged(double value)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::doubleSpinBoxZSizeChanged: Invalid scene!";
    return;
  }

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetZSize(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::applyClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContourMorphologyModuleWidget::applyClicked: Invalid scene!";
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // Perform morphological operation
  d->logic()->MorphContour();

  QApplication::restoreOverrideCursor();
}
