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

// VTK includes

// STD includes
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
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
    return;
    }

  Q_D(qSlicerContourMorphologyModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
    {
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

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  int op = paramNode->GetOperation();
  switch (op)
  {
    case SLICERRT_EXPAND:
      d->radioButton_Expand->setChecked(true);
      d->radioButton_Shrink->setChecked(false);
      d->radioButton_Union->setChecked(false);
      d->radioButton_Intersect->setChecked(false);
      d->radioButton_Subtract->setChecked(false);

      d->MRMLNodeComboBox_ContourB->setEnabled(false);

      d->doubleSpinBox_XSize->setEnabled(true);
      d->doubleSpinBox_YSize->setEnabled(true);
      d->doubleSpinBox_ZSize->setEnabled(true);
      break;
    case SLICERRT_SHRINK:
      d->radioButton_Expand->setChecked(false);
      d->radioButton_Shrink->setChecked(true);
      d->radioButton_Union->setChecked(false);
      d->radioButton_Intersect->setChecked(false);
      d->radioButton_Subtract->setChecked(false);

      d->MRMLNodeComboBox_ContourB->setEnabled(false);

      d->doubleSpinBox_XSize->setEnabled(true);
      d->doubleSpinBox_YSize->setEnabled(true);
      d->doubleSpinBox_ZSize->setEnabled(true);
      break;
    case SLICERRT_UNION:
      d->radioButton_Expand->setChecked(false);
      d->radioButton_Shrink->setChecked(false);
      d->radioButton_Union->setChecked(true);
      d->radioButton_Intersect->setChecked(false);
      d->radioButton_Subtract->setChecked(false);

      d->MRMLNodeComboBox_ContourB->setEnabled(true);

      d->doubleSpinBox_XSize->setEnabled(false);
      d->doubleSpinBox_YSize->setEnabled(false);
      d->doubleSpinBox_ZSize->setEnabled(false);
      break;
    case SLICERRT_INTERSECT:
      d->radioButton_Expand->setChecked(false);
      d->radioButton_Shrink->setChecked(false);
      d->radioButton_Union->setChecked(false);
      d->radioButton_Intersect->setChecked(true);
      d->radioButton_Subtract->setChecked(false);

      d->MRMLNodeComboBox_ContourB->setEnabled(true);

      d->doubleSpinBox_XSize->setEnabled(false);
      d->doubleSpinBox_YSize->setEnabled(false);
      d->doubleSpinBox_ZSize->setEnabled(false);
      break;
    case SLICERRT_SUBTRACT:
      d->radioButton_Expand->setChecked(false);
      d->radioButton_Shrink->setChecked(false);
      d->radioButton_Union->setChecked(false);
      d->radioButton_Intersect->setChecked(false);
      d->radioButton_Subtract->setChecked(true);

      d->MRMLNodeComboBox_ContourB->setEnabled(true);

      d->doubleSpinBox_XSize->setEnabled(false);
      d->doubleSpinBox_YSize->setEnabled(false);
      d->doubleSpinBox_ZSize->setEnabled(false);
      break;
  }

  d->MRMLNodeComboBox_ParameterSet->setCurrentNode(paramNode);
  if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetContourANodeID()))
  {
    d->MRMLNodeComboBox_ContourA->setCurrentNode(paramNode->GetContourANodeID());
  }
  else
  {
    this->setContourANode(d->MRMLNodeComboBox_ContourA->currentNode());
  }
  if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetContourBNodeID()))
  {
    d->MRMLNodeComboBox_ContourB->setCurrentNode(paramNode->GetContourBNodeID());
  }
  else
  {
    this->setContourBNode(d->MRMLNodeComboBox_ContourB->currentNode());
  }
  if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetReferenceVolumeNodeID()))
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(paramNode->GetReferenceVolumeNodeID());
  }
  else
  {
    this->setReferenceVolumeNode(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
  }

  d->doubleSpinBox_XSize->setValue(paramNode->GetXSize());
  d->doubleSpinBox_YSize->setValue(paramNode->GetYSize());
  d->doubleSpinBox_ZSize->setValue(paramNode->GetZSize());
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setup()
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  this->connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setContourMorphologyNode(vtkMRMLNode*) ) );

  this->connect( d->MRMLNodeComboBox_ContourA, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setContourANode(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_ContourB, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setContourBNode(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setReferenceVolumeNode(vtkMRMLNode*) ) );
  this->connect( d->MRMLNodeComboBox_OutputContour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT( setOutputContourNode(vtkMRMLNode*) ) );

  this->connect( d->radioButton_Expand, SIGNAL(clicked()), this, SLOT(radioButtonExpandClicked()));
  this->connect( d->radioButton_Shrink, SIGNAL(clicked()), this, SLOT(radioButtonShrinkClicked()));
  this->connect( d->radioButton_Union, SIGNAL(clicked()), this, SLOT(radioButtonUnionClicked()));
  this->connect( d->radioButton_Intersect, SIGNAL(clicked()), this, SLOT(radioButtonIntersectClicked()));
  this->connect( d->radioButton_Subtract, SIGNAL(clicked()), this, SLOT(radioButtonSubtractClicked()));

  this->connect( d->doubleSpinBox_XSize, SIGNAL(valueChanged(double value)), this, SLOT(doubleSpinBoxXSizeChanged(double value)));
  this->connect( d->doubleSpinBox_YSize, SIGNAL(valueChanged(double value)), this, SLOT(doubleSpinBoxYSizeChanged(double value)));
  this->connect( d->doubleSpinBox_ZSize, SIGNAL(valueChanged(double value)), this, SLOT(doubleSpinBoxZSizeChanged(double value)));

  this->connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourMorphologyNode(vtkMRMLNode *node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetContourMorphologyNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveContourMorphologyNode(paramNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourANode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveContourANodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setContourBNode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveContourBNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setReferenceVolumeNode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveReferenceVolumeNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::setOutputContourNode(vtkMRMLNode* node)
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveOutputContourNodeID(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonExpandClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToExpand();
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonShrinkClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToShrink();
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonUnionClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToUnion();
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonIntersectClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToIntersect();
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::radioButtonSubtractClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetOperationToSubtract();
  paramNode->DisableModifiedEventOff();

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::doubleSpinBoxXSizeChanged(double value)
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
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
  
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
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
  
  vtkMRMLContourMorphologyNode* paramNode = d->logic()->GetContourMorphologyNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }
  paramNode->DisableModifiedEventOn();
  paramNode->SetZSize(value);
  paramNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::updateButtonsState()
{
  Q_D(qSlicerContourMorphologyModuleWidget);
  if (!this->mrmlScene())
  {
    return;
  }

  bool applyEnabled = d->logic()->GetContourMorphologyNode()
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetContourANodeID())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetContourBNodeID())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetOutputContourNodeID())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetContourMorphologyNode()->GetReferenceVolumeNodeID());
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModuleWidget::applyClicked()
{
  Q_D(qSlicerContourMorphologyModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  // 
  d->logic()->MorphContour();

  QApplication::restoreOverrideCursor();
}

