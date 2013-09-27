/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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
#include "qSlicerBeamsModuleWidget.h"
#include "ui_qSlicerBeamsModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// Beams includes
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLBeamsNode.h"

// MRML includes
#include <vtkMRMLMarkupsFiducialNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Beams
class qSlicerBeamsModuleWidgetPrivate: public Ui_qSlicerBeamsModule
{
  Q_DECLARE_PUBLIC(qSlicerBeamsModuleWidget);
protected:
  qSlicerBeamsModuleWidget* const q_ptr;
public:
  qSlicerBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object);
  ~qSlicerBeamsModuleWidgetPrivate();
  vtkSlicerBeamsModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidgetPrivate::qSlicerBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidgetPrivate::~qSlicerBeamsModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic*
qSlicerBeamsModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerBeamsModuleWidget);
  return vtkSlicerBeamsModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidget::qSlicerBeamsModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerBeamsModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidget::~qSlicerBeamsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerBeamsModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetBeamsNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLBeamsNode");
    if (node)
    {
      this->setBeamsNode( vtkMRMLBeamsNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerBeamsModuleWidget::onEnter: Invalid MRML scene!";
    return;
  }

  Q_D(qSlicerBeamsModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << "qSlicerBeamsModuleWidget::onEnter: Invalid logic!";
    return;
  }
  vtkMRMLBeamsNode* paramNode = d->logic()->GetBeamsNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLBeamsNode");
    if (node)
    {
      paramNode = vtkMRMLBeamsNode::SafeDownCast(node);
      d->logic()->SetAndObserveBeamsNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLBeamsNode> newNode = vtkSmartPointer<vtkMRMLBeamsNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveBeamsNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setBeamsNode(vtkMRMLNode *node)
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLBeamsNode* paramNode = vtkMRMLBeamsNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetBeamsNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveBeamsNode(paramNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetBeamsNode());
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetIsocenterMarkupsNodeId()))
    {
      d->MRMLNodeComboBox_IsocenterMarkups->setCurrentNodeID(paramNode->GetIsocenterMarkupsNodeId());
    }
    else
    {
      this->isocenterMarkupsNodeChanged(d->MRMLNodeComboBox_IsocenterMarkups->currentNode());
    }
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetBeamModelNodeId()))
    {
      d->MRMLNodeComboBox_BeamModel->setCurrentNodeID(paramNode->GetBeamModelNodeId());
    }
    else
    {
      this->beamModelNodeChanged(d->MRMLNodeComboBox_BeamModel->currentNode());
    }
  }

  this->refreshOutputBaseName();
  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setup()
{
  Q_D(qSlicerBeamsModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Error->setVisible(false);

  // Make connections
  connect( d->MRMLNodeComboBox_IsocenterMarkups, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( isocenterMarkupsNodeChanged(vtkMRMLNode*) ) );

  connect( d->MRMLNodeComboBox_BeamModel, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(beamModelNodeChanged(vtkMRMLNode*)) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamsNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::isocenterMarkupsNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    qCritical() << "qSlicerBeamsModuleWidget::isocenterMarkupsNodeChanged: Invalid MRML scene, parameter set node or input node!";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveIsocenterMarkupsNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
  this->refreshOutputBaseName();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamModelNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    qCritical() << "qSlicerBeamsModuleWidget::beamModelNodeChanged: Invalid MRML scene, parameter set node or input node!";
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveBeamModelNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateButtonsState()
{
  Q_D(qSlicerBeamsModuleWidget);

  bool applyEnabled = d->logic()->GetBeamsNode()
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetBeamsNode()->GetIsocenterMarkupsNodeId())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetBeamsNode()->GetBeamModelNodeId());
  d->pushButton_Apply->setEnabled(applyEnabled);

  d->label_Error->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::applyClicked()
{
  Q_D(qSlicerBeamsModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage;
  d->logic()->CreateBeamModel(errorMessage);

  d->label_Error->setVisible( !errorMessage.empty() );
  d->label_Error->setText( QString(errorMessage.c_str()) );

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene())
  {
    qCritical() << "qSlicerBeamsModuleWidget::refreshOutputBaseName: Invalid MRML scene or parameter set node!";
    return;
  }

  QString newBeamModelBaseName(SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str());

  vtkMRMLMarkupsFiducialNode* isocenterNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetIsocenterMarkupsNodeId()) );
  if (isocenterNode)
  {
    newBeamModelBaseName.append(isocenterNode->GetName());
  }

  d->MRMLNodeComboBox_BeamModel->setBaseName( newBeamModelBaseName );
}
