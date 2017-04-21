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
#include <QTableWidgetItem>

// SlicerQt includes
#include "qSlicerBeamsModuleWidget.h"
#include "ui_qSlicerBeamsModule.h"

// Slicer includes
#include <qSlicerApplication.h>
#include <qSlicerSubjectHierarchyAbstractPlugin.h>

// Beams includes
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Beams
class qSlicerBeamsModuleWidgetPrivate: public Ui_qSlicerBeamsModule
{
  Q_DECLARE_PUBLIC(qSlicerBeamsModuleWidget);
protected:
  qSlicerBeamsModuleWidget* const q_ptr;
public:
  qSlicerBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object);
  vtkSlicerBeamsModuleLogic* logic() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamsModuleWidgetPrivate::qSlicerBeamsModuleWidgetPrivate(qSlicerBeamsModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic* qSlicerBeamsModuleWidgetPrivate::logic() const
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
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  Q_D(qSlicerBeamsModuleWidget);

  // First check the logic if it has a parameter node
  if (!d->logic())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid logic!";
    return;
  }

  d->ModuleWindowInitialized = true;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setup()
{
  Q_D(qSlicerBeamsModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect( d->MRMLNodeComboBox_RtBeam, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(setBeamNode(vtkMRMLNode*)) );
  connect( d->pushButton_SwitchToParentPlan, SIGNAL(clicked()), this, SLOT(switchToParentPlanButtonClicked()) );

  // Main parameters
  connect( d->lineEdit_BeamName, SIGNAL(textChanged(const QString &)), this, SLOT(beamNameChanged(const QString &)) );
  connect( d->doubleSpinBox_BeamWeight, SIGNAL(valueChanged(double)), this, SLOT(beamWeightChanged(double)) );

  // Handle scene change event if occurs
  qvtkConnect( d->logic(), vtkCommand::ModifiedEvent, this, SLOT( onLogicModified() ) );

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateButtonsState()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  bool applyEnabled = beamNode
                   && beamNode->GetParentPlanNode();

  d->pushButton_SwitchToParentPlan->setText(beamNode ? "Switch to parent plan" : "Go to External Beam Planning");
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  // Get active beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());

  // Enable beam parameters section only if there is a valid beam node
  d->CollapsibleButton_BeamParameters->setEnabled(beamNode);

  // If node is empty, disable controls
  d->BeamParametersTabWidget->setEnabled(beamNode);
  if (!beamNode)
  {
    d->lineEdit_BeamName->setText("");
    d->doubleSpinBox_BeamWeight->setValue(0.0);
  }
  else
  {
    // Main parameters
    d->lineEdit_BeamName->setText(QString::fromStdString(beamNode->GetName()));
    d->doubleSpinBox_BeamWeight->setValue(beamNode->GetBeamWeight());
  }

  this->updateButtonsState();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::setBeamNode(vtkMRMLNode* node)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  vtkMRMLRTBeamNode* rtBeamNode = vtkMRMLRTBeamNode::SafeDownCast(node);

  // Each time the node is modified, the qt widgets are updated
  qvtkReconnect(rtBeamNode, vtkCommand::ModifiedEvent, this, SLOT(onBeamNodeModified()));

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onBeamNodeModified()
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::switchToParentPlanButtonClicked()
{
  Q_D(qSlicerBeamsModuleWidget);

  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    qSlicerSubjectHierarchyAbstractPlugin::switchToModule("ExternalBeamPlanning");
    return;
  }

  // Open ExternalBeamPlanning module and select parent plan
  qSlicerApplication::application()->openNodeModule(beamNode->GetParentPlanNode());
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::onLogicModified()
{
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamNameChanged(const QString &text)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (!beamNode)
  {
    return;
  }
  beamNode->DisableModifiedEventOn();
  beamNode->SetName(text.toStdString().c_str());
  beamNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModuleWidget::beamWeightChanged(double value)
{
  Q_D(qSlicerBeamsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene!";
    return;
  }

  // Update in beam node
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(d->MRMLNodeComboBox_RtBeam->currentNode());
  if (beamNode == NULL)
  {
    return;
  }

  beamNode->DisableModifiedEventOn();
  beamNode->SetBeamWeight(value);
  beamNode->DisableModifiedEventOff();
}

//-----------------------------------------------------------------------------
bool qSlicerBeamsModuleWidget::setEditedNode(vtkMRMLNode* node, QString role/*=QString()*/, QString context/*=QString()*/)
{
  Q_D(qSlicerBeamsModuleWidget);
  if (vtkMRMLRTBeamNode::SafeDownCast(node))
  {
    d->MRMLNodeComboBox_RtBeam->setCurrentNode(node);
    return true;
  }
  return false;
}

//-----------------------------------------------------------
double qSlicerBeamsModuleWidget::nodeEditable(vtkMRMLNode* node)
{
  /// Return a higher confidence value (0.6) for beam nodes to prevent beams to be opened by Models
  if (vtkMRMLRTBeamNode::SafeDownCast(node))
  {
    return 0.6;
  }

  return 0.0;
}
