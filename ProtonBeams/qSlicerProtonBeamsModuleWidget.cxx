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
#include <QCheckBox>

// SlicerQt includes
#include "qSlicerProtonBeamsModuleWidget.h"
#include "ui_qSlicerProtonBeamsModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// Beams includes
#include "vtkSlicerProtonBeamsModuleLogic.h"
#include "vtkMRMLProtonBeamsNode.h"

// MRML includes
#include <vtkMRMLAnnotationFiducialNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Beams
class qSlicerProtonBeamsModuleWidgetPrivate: public Ui_qSlicerProtonBeamsModule
{
  Q_DECLARE_PUBLIC(qSlicerProtonBeamsModuleWidget);
protected:
  qSlicerProtonBeamsModuleWidget* const q_ptr;
public:
  qSlicerProtonBeamsModuleWidgetPrivate(qSlicerProtonBeamsModuleWidget& object);
  ~qSlicerProtonBeamsModuleWidgetPrivate();
  vtkSlicerProtonBeamsModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerProtonBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModuleWidgetPrivate::qSlicerProtonBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModuleWidgetPrivate::~qSlicerProtonBeamsModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerProtonBeamsModuleLogic*
qSlicerProtonBeamsModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerProtonBeamsModuleWidget);
  return vtkSlicerProtonBeamsModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerProtonBeamsModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModuleWidget::qSlicerProtonBeamsModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerProtonBeamsModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModuleWidget::~qSlicerProtonBeamsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetBeamsNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLProtonBeamsNode");
    if (node)
    {
      this->setBeamsNode( vtkMRMLProtonBeamsNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerProtonBeamsModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLProtonBeamsNode* paramNode = d->logic()->GetBeamsNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLProtonBeamsNode");
    if (node)
    {
      paramNode = vtkMRMLProtonBeamsNode::SafeDownCast(node);
      d->logic()->SetAndObserveBeamsNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLProtonBeamsNode> newNode = vtkSmartPointer<vtkMRMLProtonBeamsNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveBeamsNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::setBeamsNode(vtkMRMLNode *node)
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  vtkMRMLProtonBeamsNode* paramNode = vtkMRMLBeamsNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetBeamsNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveBeamsNode(paramNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  vtkMRMLProtonBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetBeamsNode());
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetIsocenterFiducialNodeId()))
    {
      d->MRMLNodeComboBox_IsocenterFiducial->setCurrentNode(paramNode->GetIsocenterFiducialNodeId());
    }
    else
    {
      this->isocenterFiducialNodeChanged(d->MRMLNodeComboBox_IsocenterFiducial->currentNode());
    }
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetSourceFiducialNodeId()))
    {
      d->MRMLNodeComboBox_SourceFiducial->setCurrentNode(paramNode->GetSourceFiducialNodeId());
    }
    else
    {
      this->sourceFiducialNodeChanged(d->MRMLNodeComboBox_SourceFiducial->currentNode());
    }
    if (!SlicerRtCommon::IsStringNullOrEmpty(paramNode->GetBeamModelNodeId()))
    {
      d->MRMLNodeComboBox_BeamModel->setCurrentNode(paramNode->GetBeamModelNodeId());
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
void qSlicerProtonBeamsModuleWidget::setup()
{
  Q_D(qSlicerProtonBeamsModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->label_Error->setVisible(false);

  // Make connections
  connect( d->MRMLNodeComboBox_IsocenterFiducial, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( isocenterFiducialNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_SourceFiducial, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( sourceFiducialNodeChanged(vtkMRMLNode*) ) );

  connect( d->MRMLNodeComboBox_BeamModel, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(beamModelNodeChanged(vtkMRMLNode*)) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamsNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::isocenterFiducialNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  vtkMRMLProtonBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveIsocenterFiducialNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
  this->refreshOutputBaseName();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::sourceFiducialNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  vtkMRMLProtonBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveSourceFiducialNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::beamModelNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  vtkMRMLProtonBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene() || !node)
  {
    return;
  }

  paramNode->DisableModifiedEventOn();
  paramNode->SetAndObserveBeamModelNodeId(node->GetID());
  paramNode->DisableModifiedEventOff();

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::updateButtonsState()
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  bool applyEnabled = d->logic()->GetBeamsNode()
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetBeamsNode()->GetIsocenterFiducialNodeId())
                   && !SlicerRtCommon::IsStringNullOrEmpty(d->logic()->GetBeamsNode()->GetSourceFiducialNodeId());
  d->pushButton_Apply->setEnabled(applyEnabled);

  d->label_Error->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::applyClicked()
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage;
  d->logic()->CreateBeamModel(errorMessage);

  d->label_Error->setVisible( !errorMessage.empty() );
  d->label_Error->setText( QString(errorMessage.c_str()) );

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerProtonBeamsModuleWidget);

  vtkMRMLProtonBeamsNode* paramNode = d->logic()->GetBeamsNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QString newBeamModelBaseName(SlicerRtCommon::BEAMS_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str());
  QString newSourceFiducialBaseName(SlicerRtCommon::BEAMS_OUTPUT_SOURCE_FIDUCIAL_PREFIX.c_str());

  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetIsocenterFiducialNodeId()) );
  if (isocenterNode)
  {
    newBeamModelBaseName.append(isocenterNode->GetName());
    newSourceFiducialBaseName.append(isocenterNode->GetName());
  }

  d->MRMLNodeComboBox_BeamModel->setBaseName( newBeamModelBaseName );
  d->MRMLNodeComboBox_SourceFiducial->setBaseName( newSourceFiducialBaseName );
}
