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
#include "qSlicerBeamVisualizerModuleWidget.h"
#include "ui_qSlicerBeamVisualizerModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// BeamVisualizer includes
#include "vtkSlicerBeamVisualizerModuleLogic.h"
#include "vtkMRMLBeamVisualizerNode.h"

// MRML includes
#include <vtkMRMLAnnotationFiducialNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_BeamVisualizer
class qSlicerBeamVisualizerModuleWidgetPrivate: public Ui_qSlicerBeamVisualizerModule
{
  Q_DECLARE_PUBLIC(qSlicerBeamVisualizerModuleWidget);
protected:
  qSlicerBeamVisualizerModuleWidget* const q_ptr;
public:
  qSlicerBeamVisualizerModuleWidgetPrivate(qSlicerBeamVisualizerModuleWidget& object);
  ~qSlicerBeamVisualizerModuleWidgetPrivate();
  vtkSlicerBeamVisualizerModuleLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerBeamVisualizerModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidgetPrivate::qSlicerBeamVisualizerModuleWidgetPrivate(qSlicerBeamVisualizerModuleWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidgetPrivate::~qSlicerBeamVisualizerModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerBeamVisualizerModuleLogic*
qSlicerBeamVisualizerModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerBeamVisualizerModuleWidget);
  return vtkSlicerBeamVisualizerModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerBeamVisualizerModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidget::qSlicerBeamVisualizerModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerBeamVisualizerModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModuleWidget::~qSlicerBeamVisualizerModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is no one in the scene
  if (scene &&  d->logic()->GetBeamVisualizerNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLBeamVisualizerNode");
    if (node)
    {
      this->setBeamVisualizerNode( vtkMRMLBeamVisualizerNode::SafeDownCast(node) );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::onSceneImportedEvent()
{
  this->onEnter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerBeamVisualizerModuleWidget);

  // First check the logic if it has a parameter node
  if (d->logic() == NULL)
  {
    return;
  }
  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();

  // If we have a parameter node select it
  if (paramNode == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLBeamVisualizerNode");
    if (node)
    {
      paramNode = vtkMRMLBeamVisualizerNode::SafeDownCast(node);
      d->logic()->SetAndObserveBeamVisualizerNode(paramNode);
      return;
    }
    else 
    {
      vtkSmartPointer<vtkMRMLBeamVisualizerNode> newNode = vtkSmartPointer<vtkMRMLBeamVisualizerNode>::New();
      this->mrmlScene()->AddNode(newNode);
      d->logic()->SetAndObserveBeamVisualizerNode(newNode);
    }
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::setBeamVisualizerNode(vtkMRMLNode *node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = vtkMRMLBeamVisualizerNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect( d->logic()->GetBeamVisualizerNode(), paramNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()) );

  d->logic()->SetAndObserveBeamVisualizerNode(paramNode);

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
  if (paramNode && this->mrmlScene())
  {
    d->MRMLNodeComboBox_ParameterSet->setCurrentNode(d->logic()->GetBeamVisualizerNode());
    if (paramNode->GetIsocenterFiducialNodeId() && strcmp(paramNode->GetIsocenterFiducialNodeId(),""))
    {
      d->MRMLNodeComboBox_IsocenterFiducial->setCurrentNode(paramNode->GetIsocenterFiducialNodeId());
    }
    else
    {
      this->isocenterFiducialNodeChanged(d->MRMLNodeComboBox_IsocenterFiducial->currentNode());
    }
    if (paramNode->GetSourceFiducialNodeId() && strcmp(paramNode->GetSourceFiducialNodeId(),""))
    {
      d->MRMLNodeComboBox_SourceFiducial->setCurrentNode(paramNode->GetSourceFiducialNodeId());
    }
    if (paramNode->GetBeamModelNodeId() && strcmp(paramNode->GetBeamModelNodeId(),""))
    {
      d->MRMLNodeComboBox_BeamModel->setCurrentNode(paramNode->GetBeamModelNodeId());
    }
  }

  this->refreshOutputBaseName();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::setup()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_IsocenterFiducial, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( isocenterFiducialNodeChanged(vtkMRMLNode*) ) );
  connect( d->MRMLNodeComboBox_SourceFiducial, SIGNAL( currentNodeChanged(vtkMRMLNode*) ), this, SLOT( sourceFiducialNodeChanged(vtkMRMLNode*) ) );

  connect( d->MRMLNodeComboBox_BeamModel, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(beamModelNodeChanged(vtkMRMLNode*)) );
  connect( d->pushButton_Apply, SIGNAL(clicked()), this, SLOT(applyClicked()) );

  connect( d->MRMLNodeComboBox_ParameterSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamVisualizerNode(vtkMRMLNode*)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::isocenterFiducialNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
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
void qSlicerBeamVisualizerModuleWidget::sourceFiducialNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
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
void qSlicerBeamVisualizerModuleWidget::beamModelNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
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
void qSlicerBeamVisualizerModuleWidget::updateButtonsState()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  bool applyEnabled = d->logic()->GetBeamVisualizerNode()
                   && d->logic()->GetBeamVisualizerNode()->GetIsocenterFiducialNodeId()
                   && strcmp(d->logic()->GetBeamVisualizerNode()->GetIsocenterFiducialNodeId(), "")
                   && d->logic()->GetBeamVisualizerNode()->GetSourceFiducialNodeId()
                   && strcmp(d->logic()->GetBeamVisualizerNode()->GetSourceFiducialNodeId(), "");
  d->pushButton_Apply->setEnabled(applyEnabled);
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::onLogicModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::applyClicked()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  std::string errorMessage;
  d->logic()->ComputeSourceFiducialPosition(errorMessage);
  d->logic()->CreateBeamModel(errorMessage);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModuleWidget::refreshOutputBaseName()
{
  Q_D(qSlicerBeamVisualizerModuleWidget);

  vtkMRMLBeamVisualizerNode* paramNode = d->logic()->GetBeamVisualizerNode();
  if (!paramNode || !this->mrmlScene())
  {
    return;
  }

  QString newBeamModelBaseName(SlicerRtCommon::BEAMVISUALIZER_OUTPUT_BEAM_MODEL_BASE_NAME_PREFIX.c_str());
  QString newSourceFiducialBaseName(SlicerRtCommon::BEAMVISUALIZER_OUTPUT_SOURCE_FIDUCIAL_PREFIX.c_str());

  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->mrmlScene()->GetNodeByID(paramNode->GetIsocenterFiducialNodeId()) );
  if (isocenterNode)
  {
    newBeamModelBaseName.append("_");
    newBeamModelBaseName.append(isocenterNode->GetName());
    newSourceFiducialBaseName.append("_");
    newSourceFiducialBaseName.append(isocenterNode->GetName());
  }

  d->MRMLNodeComboBox_BeamModel->setBaseName( newBeamModelBaseName.toLatin1() );
  d->MRMLNodeComboBox_SourceFiducial->setBaseName( newSourceFiducialBaseName.toLatin1() );
}
