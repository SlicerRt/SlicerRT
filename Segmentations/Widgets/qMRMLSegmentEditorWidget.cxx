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

// Segmentations includes
#include "qMRMLSegmentEditorWidget.h"

#include "ui_qMRMLSegmentEditorWidget.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkSegmentation.h"
#include "vtkSegment.h"

// Segment editor effects includes
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorEffectHandler.h"

// VTK includes
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorObserver.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkSmartPointer.h>

// Slicer includes
#include "qSlicerApplication.h"
#include "vtkSlicerApplicationLogic.h"
#include "qSlicerLayoutManager.h"
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "vtkMRMLSliceLogic.h"

// MRML includes
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>

// Qt includes
#include <QDebug>
#include <QPushButton>
#include <QButtonGroup>

//-----------------------------------------------------------------------------
class qMRMLSegmentEditorWidgetPrivate: public Ui_qMRMLSegmentEditorWidget
{
  Q_DECLARE_PUBLIC(qMRMLSegmentEditorWidget);

protected:
  qMRMLSegmentEditorWidget* const q_ptr;
public:
  qMRMLSegmentEditorWidgetPrivate(qMRMLSegmentEditorWidget& object);
  ~qMRMLSegmentEditorWidgetPrivate();
  void init();
  void createEffects();

public:
  /// Selected segmentation MRML node
  vtkMRMLSegmentationNode* SegmentationNode;

  /// Selected segment ID
  QString SelectedSegmentID;
  
  /// Commands for each slice view handling interactions
  std::vector<vtkCallbackCommand*> InteractionCallbackCommands;
  
  /// Button group for the effects
  QButtonGroup EffectButtonGroup;
};

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::qMRMLSegmentEditorWidgetPrivate(qMRMLSegmentEditorWidget& object)
  : q_ptr(&object)
  , SelectedSegmentID(QString())
  , SegmentationNode(NULL)
{
  this->InteractionCallbackCommands.clear();
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::~qMRMLSegmentEditorWidgetPrivate()
{
  std::vector<vtkCallbackCommand*>::iterator commandIt;
  for (commandIt = this->InteractionCallbackCommands.begin(); commandIt != this->InteractionCallbackCommands.end(); ++commandIt)
  {
    vtkCallbackCommand* command = (*commandIt);
    command->Delete();
  }
  this->InteractionCallbackCommands.clear();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::init()
{
  Q_Q(qMRMLSegmentEditorWidget);
  this->setupUi(q);

  // Make connections
  QObject::connect( this->MRMLNodeComboBox_Segmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onSegmentationNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onReferenceVolumeNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->SegmentsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
    q, SLOT(segmentSelectionChanged(QItemSelection,QItemSelection) ) );
  
  // Widget properties
  this->SegmentsTableView->setMode(qMRMLSegmentsTableView::EditorMode);
  this->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);

  this->EffectButtonGroup.setExclusive(true);
  QObject::connect(&this->EffectButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), q, SLOT(onEffectButtonClicked(QAbstractButton*) ) );

  // Instantiate and expose effects
  this->createEffects();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::createEffects()
{
  Q_Q(qMRMLSegmentEditorWidget);

  // Clear possible previous buttons
  QList<QAbstractButton*> effectButtons = this->EffectButtonGroup.buttons();
  foreach (QAbstractButton* button, effectButtons)
  {
    this->EffectButtonGroup.removeButton(button);
    button->deleteLater();
  }

  // Setup layout
  QHBoxLayout* effectsGroupLayout = new QHBoxLayout();
  effectsGroupLayout->setContentsMargins(4,4,4,4);
  effectsGroupLayout->setSpacing(4);
  this->EffectsGroupBox->setLayout(effectsGroupLayout);

  // Instantiate effects and create buttons for activating effects
  QList<qSlicerSegmentEditorAbstractEffect*> effects =
    qSlicerSegmentEditorEffectHandler::instance()->registeredEffects();
  foreach (qSlicerSegmentEditorAbstractEffect* currentEffect, effects)
  {
    QPushButton* effectButton = new QPushButton(this->EffectsGroupBox);
    effectButton->setObjectName(currentEffect->name());
    effectButton->setCheckable(true);
    effectButton->setIcon(currentEffect->icon());
    effectButton->setProperty("Effect", QVariant::fromValue<QObject*>(currentEffect));

    this->EffectButtonGroup.addButton(effectButton);
    effectsGroupLayout->addWidget(effectButton);
  }
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qMRMLSegmentEditorWidget methods

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidget::qMRMLSegmentEditorWidget(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLSegmentEditorWidgetPrivate(*this))
{
  Q_D(qMRMLSegmentEditorWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidget::~qMRMLSegmentEditorWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  qMRMLWidget::setMRMLScene(newScene);
  
  // Set scene to effect handler
  qSlicerSegmentEditorEffectHandler::instance()->setScene(newScene);

  // Create observations between slice view interactor and the widget.
  // The captured events are propagated to the active effect if any.
  this->setupSliceObservations();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node);
  if (d->SegmentationNode != segmentationNode)
  {
    d->SegmentationNode = segmentationNode;
  }
  
  // Select reference volume node in combobox if any
  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(d->SegmentationNode != NULL);
  if (d->SegmentationNode)
  {
    vtkMRMLNode* refereceVolumeNode = d->SegmentationNode->GetNodeReference(
      vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str() );
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(refereceVolumeNode);
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
  }
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MRMLNodeComboBox_Segmentation->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentEditorWidget::segmentationNode()
{
  Q_D(qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_Segmentation->currentNode();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNodeID(const QString& nodeID)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MRMLNodeComboBox_Segmentation->setCurrentNodeID(nodeID);
}

//------------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::segmentationNodeID()
{
  Q_D(qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_Segmentation->currentNodeID();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::segmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qMRMLSegmentEditorWidget);

  std::vector<std::string> selectedSegmentIDs;
  if (selectedSegmentIDs.size() != 1)
  {
    qCritical() << "qMRMLSegmentEditorWidget::segmentSelectionChanged: One segment should be selected!";
    return;
  }

  d->SelectedSegmentID = QString(selectedSegmentIDs[0].c_str());
  
  // Show segmentation in label layer of slice viewers
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qMRMLSegmentEditorWidget::segmentSelectionChanged: Unable to get selection node to show segmentation node " << d->SegmentationNode->GetName();
    return;
  }
  selectionNode->SetReferenceActiveLabelVolumeID(d->SegmentationNode->GetID());
  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
 
  // Set binary labelmap representation to effects
  //TODO:
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onReferenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!volumeNode)
  {
    return;
  }

  // Set reference image connection and conversion parameter in selected segmentation
  if (d->SegmentationNode->GetNodeReference(vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str()) != volumeNode)
  {
    d->SegmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(volumeNode);
  }

  // Show reference volume in background layer of slice viewers
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qMRMLSegmentEditorWidget::onReferenceVolumeNodeChanged: Unable to get selection node to show volume node " << volumeNode->GetName();
    return;
  }
  //selectionNode->SetActiveVolumeID(volumeNode->GetID());
  selectionNode->SetReferenceActiveVolumeID(volumeNode->GetID());
  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
}

//-----------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::currentSegmentID()
{
  Q_D(qMRMLSegmentEditorWidget);

  return d->SelectedSegmentID;
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setupSliceObservations()
{
  Q_D(qMRMLSegmentEditorWidget);

  vtkMRMLScene* scene = this->mrmlScene();
  if (!scene)
  {
    qCritical() << "qMRMLSegmentEditorWidget::setupSliceObservations: Invalid MRML scene!";
    return;
  }

  // Clear previous observations before setting up the new ones
  std::vector<vtkCallbackCommand*>::iterator commandIt;
  for (commandIt = d->InteractionCallbackCommands.begin(); commandIt != d->InteractionCallbackCommands.end(); ++commandIt)
  {
    vtkCallbackCommand* command = (*commandIt);
    command->Delete();
  }
  d->InteractionCallbackCommands.clear();

  // Set up interactor observations
  //TODO: Include 3D view too
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  int sliceNodeCount = scene->GetNumberOfNodesByClass("vtkMRMLSliceNode");
  for (int nodeIndex=0; nodeIndex<sliceNodeCount; ++nodeIndex)
  {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(
      scene->GetNthNodeByClass(nodeIndex, "vtkMRMLSliceNode") );
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceNode->GetLayoutName());

//    vtkMRMLSliceLogic* sliceLogic = sliceWidget->sliceLogic();
    qMRMLSliceView* sliceView = sliceWidget->sliceView();
    
    vtkRenderWindowInteractor* interactor = sliceView->interactorStyle()->GetInteractor();
//    vtkRenderWindow* renderWindow = sliceView->renderWindow();
//    vtkRenderer* renderer = vtkRenderer::SafeDownCast(
//      renderWindow->GetRenderers()->GetItemAsObject(0) );

    // Create command for slice view
    vtkCallbackCommand* interactionCallbackCommand = vtkCallbackCommand::New();
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(sliceWidget) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processInteractionEvents );

    // Connect events
    unsigned long leftButtonPressTag = interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactionCallbackCommand, 1.0);
    unsigned long leftButtonReleaseTag = interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactionCallbackCommand, 1.0);
    unsigned long mouseMoveReleaseTag = interactor->AddObserver(vtkCommand::MouseMoveEvent, interactionCallbackCommand, 1.0);
    unsigned long enterTag = interactor->AddObserver(vtkCommand::EnterEvent, interactionCallbackCommand, 1.0);
    unsigned long leaveTag = interactor->AddObserver(vtkCommand::LeaveEvent, interactionCallbackCommand, 1.0);
    
    d->InteractionCallbackCommands.push_back(interactionCallbackCommand);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onEffectButtonClicked(QAbstractButton* button)
{
  Q_D(qMRMLSegmentEditorWidget);

  // Get effect that was just clicked
  qSlicerSegmentEditorAbstractEffect* clickedEffect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
    button->property("Effect").value<QObject*>() );
  // Get currently active effect
  qSlicerSegmentEditorAbstractEffect* activeEffect = qSlicerSegmentEditorEffectHandler::instance()->activeEffect();

  // If the selected effect was clicked again, then de-select
  if (activeEffect == clickedEffect)
  {
    d->EffectButtonGroup.setExclusive(false);
    button->blockSignals(true);

    button->setChecked(false);
    qSlicerSegmentEditorEffectHandler::instance()->setActiveEffect(NULL);
    d->ActiveEffectLabel->setText("None");

    button->blockSignals(false);
    d->EffectButtonGroup.setExclusive(true);
  }
  else
  {
    // Deactivate previously selected effect
    //activeEffect->deactivate(); //TODO: needed?

    // Set selected effect as current and activate it
    qSlicerSegmentEditorEffectHandler::instance()->setActiveEffect(clickedEffect);
    //d->ActiveEffect->activate(); //TODO: needed?
    //TODO: Setup parameter node: set current effect name, allow effect to populate parameter set node with (default) parameters
    d->ActiveEffectLabel->setText(clickedEffect->name());

    // Create effect options widget
    //TODO:
    //d->EffectOptionsFrame
  }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::processInteractionEvents(vtkObject* caller,
                                        unsigned long eid,
                                        void* clientData,
                                        void* vtkNotUsed(callData))
{
  // Get active effect
  qSlicerSegmentEditorAbstractEffect* activeEffect = qSlicerSegmentEditorEffectHandler::instance()->activeEffect();
  if (!activeEffect)
  {
    return;
  }
  // Get slice widget and interactor for slice view where the event happened
  qMRMLSliceWidget* sliceWidget = reinterpret_cast<qMRMLSliceWidget*>(clientData);
  vtkRenderWindowInteractor* callerInteractor = reinterpret_cast<vtkRenderWindowInteractor*>(caller);
  if (!sliceWidget || !callerInteractor)
  {
    return;
  }

  // Call processing function of active effect if any
  activeEffect->processInteractionEvents(callerInteractor, eid, sliceWidget);
}
