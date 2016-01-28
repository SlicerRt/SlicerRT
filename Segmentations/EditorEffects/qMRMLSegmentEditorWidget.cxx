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
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Segment editor effects includes
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorEffectFactory.h"

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
#include "vtkMRMLSliceLogic.h"
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLThreeDView.h"

// MRML includes
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLViewNode.h>

// Qt includes
#include <QDebug>
#include <QPushButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
class SegmentEditorInteractionEventInfo: public QObject
{
public:
  /// Segment editor widget observing the event
  qMRMLSegmentEditorWidget* EditorWidget;
  /// Slice widget or 3D widget
  qMRMLWidget* ViewWidget;
};

//-----------------------------------------------------------------------------
// qMRMLSegmentEditorWidgetPrivate methods

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

  /// Create local effect clones for per-editor effect handling and create effect buttons
  void createEffects();

  /// Set MRML scene to all local effects
  void setSceneToEffects(vtkMRMLScene* scene);

  /// Create edited labelmap from selected segment, using the bounds of the master volume
  /// \return Success flag
  bool createEditedLabelmapFromSelectedSegment();

  /// Set edited labelmap to all local effects
  void setEditedLabelmapToEffects();

public:
  /// Selected segmentation MRML node
  vtkMRMLSegmentationNode* SegmentationNode;

  /// Selected segment ID
  QString SelectedSegmentID;

  /// Selected master volume MRML node
  vtkMRMLScalarVolumeNode* MasterVolumeNode;
  
  /// Active segmentLabelmap for editing. Mainly needed because the segment binary labelmaps are shrunk
  /// to the smallest possible extent, but the user wants to draw on the whole master volume.
  vtkOrientedImageData* EditedLabelmap;

  /// Mask segmentLabelmap
  vtkOrientedImageData* MaskLabelmap;

  /// List of registered effect instances
  QList<qSlicerSegmentEditorAbstractEffect*> RegisteredEffects;

  /// Active effect
  qSlicerSegmentEditorAbstractEffect* ActiveEffect;

  /// Commands for each slice and 3D view handling interactions
  QList<vtkCallbackCommand*> InteractionCallbackCommands;

  /// Structure containing necessary objects for each slice and 3D view handling interactions
  QList<SegmentEditorInteractionEventInfo*> InteractionCallbackEventInfos;
  
  /// Button group for the effects
  QButtonGroup EffectButtonGroup;
};

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::qMRMLSegmentEditorWidgetPrivate(qMRMLSegmentEditorWidget& object)
  : q_ptr(&object)
  , SelectedSegmentID(QString())
  , SegmentationNode(NULL)
  , MasterVolumeNode(NULL)
  , ActiveEffect(NULL)
  , EditedLabelmap(NULL)
  , MaskLabelmap(NULL)
{
  this->EditedLabelmap = vtkOrientedImageData::New();
  this->MaskLabelmap = vtkOrientedImageData::New();

  this->InteractionCallbackCommands.clear();
  this->InteractionCallbackEventInfos.clear();
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::~qMRMLSegmentEditorWidgetPrivate()
{
  if (this->EditedLabelmap)
  {
    this->EditedLabelmap->Delete();
    this->EditedLabelmap = NULL;
  }
  if (this->MaskLabelmap)
  {
    this->MaskLabelmap->Delete();
    this->MaskLabelmap = NULL;
  }

  foreach (vtkCallbackCommand* command, this->InteractionCallbackCommands)
  {
    command->Delete();
  }
  this->InteractionCallbackCommands.clear();

  foreach (SegmentEditorInteractionEventInfo* eventInfo, this->InteractionCallbackEventInfos)
  {
    delete eventInfo;
  }
  this->InteractionCallbackEventInfos.clear();

  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    delete effect;
  }
  this->RegisteredEffects.clear();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::init()
{
  Q_Q(qMRMLSegmentEditorWidget);
  this->setupUi(q);

  // Make connections
  QObject::connect( this->MRMLNodeComboBox_Segmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onSegmentationNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MRMLNodeComboBox_MasterVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onMasterVolumeNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->SegmentsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
    q, SLOT(onSegmentSelectionChanged(QItemSelection,QItemSelection) ) );
  
  // Widget properties
  this->SegmentsTableView->setMode(qMRMLSegmentsTableView::EditorMode);
  this->MRMLNodeComboBox_MasterVolume->setEnabled(false);

  this->EffectButtonGroup.setExclusive(true);
  QObject::connect(&this->EffectButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), q, SLOT(onEffectButtonClicked(QAbstractButton*) ) );

  // Create layout for effect options
  QVBoxLayout* layout = new QVBoxLayout(this->EffectsOptionsFrame);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

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

  // Create local copy of factory effects, so that
  // - Effects can have different parameters
  // - Segment editors can have different active effects
  qSlicerSegmentEditorEffectFactory::instance()->copyEffects(this->RegisteredEffects);

  // Setup layout
  QHBoxLayout* effectsGroupLayout = new QHBoxLayout();
  effectsGroupLayout->setContentsMargins(4,4,4,4);
  effectsGroupLayout->setSpacing(4);
  this->EffectsGroupBox->setLayout(effectsGroupLayout);

  // Initialize effects
  foreach (qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    // Create button for activating effect
    QPushButton* effectButton = new QPushButton(this->EffectsGroupBox);
    effectButton->setObjectName(effect->name());
    effectButton->setCheckable(true);
    effectButton->setIcon(effect->icon());
    effectButton->setProperty("Effect", QVariant::fromValue<QObject*>(effect));

    this->EffectButtonGroup.addButton(effectButton);
    effectsGroupLayout->addWidget(effectButton);

    // Add effect options frame to the options widget and hide them
    effect->setupOptionsFrame();
    QFrame* effectOptionsFrame = effect->optionsFrame();
    this->EffectsOptionsFrame->layout()->addWidget(effectOptionsFrame);
    effectOptionsFrame->setVisible(false);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::setSceneToEffects(vtkMRMLScene* scene)
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    effect->setScene(scene);
    effect->setMRMLDefaults();
  }
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::createEditedLabelmapFromSelectedSegment()
{
  // Clear edited labelmap
  this->EditedLabelmap->Initialize();

  if (!this->SegmentationNode || !this->MasterVolumeNode)
  {
    return false;
  }
  else if (this->SelectedSegmentID.isEmpty())
  {
    return false;
  }

  // Get binary labelmap representation of selected segment
  vtkSegment* selectedSegment = this->SegmentationNode->GetSegmentation()->GetSegment(
    this->SelectedSegmentID.toLatin1().constData() );
  vtkOrientedImageData* segmentLabelmap = vtkOrientedImageData::SafeDownCast(
    selectedSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
  if (!segmentLabelmap)
  {
    qCritical() << "qMRMLSegmentEditorWidgetPrivate::createEditedLabelmapFromSelectedSegment: Failed to get binary labelmap representation in segmentation " << this->SegmentationNode->GetName();
    return false;
  }

  // Convert master volume to a temporary oriented image data
  vtkSmartPointer<vtkOrientedImageData> masterVolumeOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(this->MasterVolumeNode) );

  // Pad segment binary labelmap to contain master volume to enable editing on its bounds
  if (!vtkOrientedImageDataResample::PadImageToContainImage(
    segmentLabelmap, masterVolumeOrientedImageData, this->EditedLabelmap) )
  {
    qCritical() << "qMRMLSegmentEditorWidgetPrivate::createEditedLabelmapFromSelectedSegment: Failed to pad binary labelmap of selected segment "
      << this->SelectedSegmentID << " in segmentation " << this->SegmentationNode->GetName() << " to contain master volume " << this->MasterVolumeNode->GetName();
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::setEditedLabelmapToEffects()
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    effect->setEditedLabelmap(this->EditedLabelmap);
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
  Q_D(qMRMLSegmentEditorWidget);

  // Set scene to effects
  d->setSceneToEffects(newScene);
  // Call base class method second, as it emits scene changed event, which resets MRML selections
  // and ultimately causes effects to perform updates before having access to the new scene.
  Superclass::setMRMLScene(newScene);

  // Create observations between view interactors and the editor widget.
  // The captured events are propagated to the active effect if any.
  this->setupSliceObservations();
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qMRMLSegmentEditorWidget::activeEffect()const
{
  Q_D(const qMRMLSegmentEditorWidget);

  return d->ActiveEffect;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setActiveEffect(qSlicerSegmentEditorAbstractEffect* effect)
{
  Q_D(qMRMLSegmentEditorWidget);

  // Deactivate previously selected effect
  if (d->ActiveEffect)
  {
    d->ActiveEffect->deactivate();
  }

  d->ActiveEffect = effect;
  if (d->ActiveEffect)
  {
    // Activate newly selected effect
    d->ActiveEffect->activate();
    d->ActiveEffectLabel->setText(d->ActiveEffect->name());
  }
  else
  {
    d->ActiveEffectLabel->setText("None");
  }
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
  
  // Select master volume node in combobox if any
  d->MRMLNodeComboBox_MasterVolume->setEnabled(d->SegmentationNode != NULL);
  if (d->SegmentationNode)
  {
    vtkMRMLNode* refereceVolumeNode = d->SegmentationNode->GetNodeReference(
      vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str() );
    d->MRMLNodeComboBox_MasterVolume->setCurrentNode(refereceVolumeNode);
  }

  // Make sure binary segmentLabelmap representation exists
  if (d->SegmentationNode)
  {
    if (!d->SegmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) )
    {
      QString message = QString("Failed to create binary segmentLabelmap representation in segmentation %1 for editing!\nPlease see Segmentations module for details.").
        arg(d->SegmentationNode->GetName());
      QMessageBox::critical(NULL, tr("Failed to create binary segmentLabelmap for editing"), message);
      qCritical() << "qMRMLSegmentEditorWidget::onSegmentationNodeChanged: " << message;
      return;
    }
  }

  // Select first segment
  if ( d->SegmentationNode
    && d->SegmentationNode->GetSegmentation()->GetNumberOfSegments() > 0 )
  {
    std::vector<std::string> segmentIDs;
    d->SegmentationNode->GetSegmentation()->GetSegmentIDs(segmentIDs);
    QStringList firstSegmentID;
    firstSegmentID << QString(segmentIDs[0].c_str());
    d->SegmentsTableView->setSelectedSegmentIDs(firstSegmentID);
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
void qMRMLSegmentEditorWidget::onSegmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qMRMLSegmentEditorWidget);

  QStringList selectedSegmentIDs = d->SegmentsTableView->selectedSegmentIDs();
  if (selectedSegmentIDs.size() != 1)
  {
    qCritical() << "qMRMLSegmentEditorWidget::onSegmentSelectionChanged: One segment should be selected!";
    return;
  }

  d->SelectedSegmentID = selectedSegmentIDs[0];
  
  // Show segmentation in label layer of slice viewers
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << "qMRMLSegmentEditorWidget::onSegmentSelectionChanged: Unable to get selection node to show segmentation node " << d->SegmentationNode->GetName();
    return;
  }
  selectionNode->SetReferenceActiveLabelVolumeID(d->SegmentationNode->GetID());
  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
 
  // Create edited segmentLabelmap from selected segment, using the bounds of the master volume
  if (d->createEditedLabelmapFromSelectedSegment())
  {
    // Set binary segmentLabelmap representation to effects
    d->setEditedLabelmapToEffects();
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (d->MasterVolumeNode != volumeNode)
  {
    d->MasterVolumeNode = volumeNode;
  }

  // Disable editing if no master volume node is set:
  // master volume determines the extent of editing, so even though the segmentation is valid
  // without a master volume, editing is not possible until it is selected.
  d->EffectsGroupBox->setEnabled(volumeNode);
  d->OptionsGroupBox->setEnabled(volumeNode);
  if (!volumeNode)
  {
    this->setActiveEffect(NULL);
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
    qCritical() << "qMRMLSegmentEditorWidget::onMasterVolumeNodeChanged: Unable to get selection node to show volume node " << volumeNode->GetName();
    return;
  }
  //selectionNode->SetActiveVolumeID(volumeNode->GetID());
  selectionNode->SetReferenceActiveVolumeID(volumeNode->GetID());
  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();

  // Create edited segmentLabelmap from selected segment, using the bounds of the master volume
  if (d->createEditedLabelmapFromSelectedSegment())
  {
    // Set binary segmentLabelmap representation to effects
    d->setEditedLabelmapToEffects();
  }
}

//-----------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::currentSegmentID()
{
  Q_D(qMRMLSegmentEditorWidget);

  return d->SelectedSegmentID;
}

//---------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qMRMLSegmentEditorWidget::effectByName(QString name)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (name.isEmpty())
    {
    return NULL;
    }

  // Find effect with name
  qSlicerSegmentEditorAbstractEffect* currentEffect = NULL;
  foreach (currentEffect, d->RegisteredEffects)
    {
    if (currentEffect->name().compare(name) == 0)
      {
      return currentEffect;
      }
    }

  qWarning() << "qMRMLSegmentEditorWidget::effectByName: Effect named '" << name << "' cannot be found!";
  return NULL;
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setupSliceObservations()
{
  Q_D(qMRMLSegmentEditorWidget);

  // Make sure previous observations are cleared before setting up the new ones
  this->removeSliceObservations();

  // Set up interactor observations
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();

  // Slice views
  foreach (QString sliceViewName, layoutManager->sliceViewNames())
  {
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    qMRMLSliceView* sliceView = sliceWidget->sliceView();
    vtkRenderWindowInteractor* interactor = sliceView->interactorStyle()->GetInteractor();

    // Create command for slice view
    SegmentEditorInteractionEventInfo* eventInfo = new SegmentEditorInteractionEventInfo();
    eventInfo->EditorWidget = this;
    eventInfo->ViewWidget = sliceWidget;
    vtkCallbackCommand* interactionCallbackCommand = vtkCallbackCommand::New();
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(eventInfo) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processEvents );

    // Connect interactor events
    QList<QVariant> tags;
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MouseMoveEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::EnterEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::LeaveEvent, interactionCallbackCommand, 1.0));
    // Save tags in slice widget
    sliceWidget->setProperty(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier(), QVariant(tags));
    
    // Node observations
    vtkMRMLSliceNode* sliceNode = sliceWidget->sliceLogic()->GetSliceNode();
    unsigned long tag = sliceNode->AddObserver(vtkCommand::ModifiedEvent, interactionCallbackCommand, 1.0);
    QString tagStr = QString::number((qulonglong)tag);
    sliceNode->SetAttribute(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier(), tagStr.toLatin1().constData());

    // Save command and event info
    d->InteractionCallbackCommands << interactionCallbackCommand;
    d->InteractionCallbackEventInfos << eventInfo;
  }

  // 3D views
  for (int threeDViewId=0; threeDViewId<layoutManager->threeDViewCount(); ++threeDViewId)
  {
    qMRMLThreeDWidget* threeDWidget = layoutManager->threeDWidget(threeDViewId);
    qMRMLThreeDView* threeDView = threeDWidget->threeDView();
    vtkRenderWindowInteractor* interactor = threeDView->interactor();

    // Create command for view
    SegmentEditorInteractionEventInfo* eventInfo = new SegmentEditorInteractionEventInfo();
    eventInfo->EditorWidget = this;
    eventInfo->ViewWidget = threeDWidget;
    vtkCallbackCommand* interactionCallbackCommand = vtkCallbackCommand::New();
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(eventInfo) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processEvents );

    // Connect interactor events
    QList<QVariant> tags;
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MouseMoveEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::EnterEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::LeaveEvent, interactionCallbackCommand, 1.0));
    // Save tags in view widget
    threeDWidget->setProperty(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier(), QVariant(tags));

    // Node observations
    vtkMRMLViewNode* viewNode = threeDWidget->mrmlViewNode();
    unsigned long tag = viewNode->AddObserver(vtkCommand::ModifiedEvent, interactionCallbackCommand, 1.0);
    QString tagStr = QString::number((qulonglong)tag);
    viewNode->SetAttribute(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier(), tagStr.toLatin1().constData());
  
    // Save command and event info
    d->InteractionCallbackCommands << interactionCallbackCommand;
    d->InteractionCallbackEventInfos << eventInfo;
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::removeSliceObservations()
{
  Q_D(qMRMLSegmentEditorWidget);

  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();

  // Slice views
  foreach (QString sliceViewName, layoutManager->sliceViewNames())
  {
    // Interactors
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    qMRMLSliceView* sliceView = sliceWidget->sliceView();
    vtkRenderWindowInteractor* interactor = sliceView->interactorStyle()->GetInteractor();
    QList<QVariant> tags = sliceWidget->property(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier()).toList();
    foreach (QVariant tagVariant, tags)
    {
      unsigned long tag = tagVariant.toULongLong();
      interactor->RemoveObserver(tag);
    }

    // Nodes
    vtkMRMLSliceNode* sliceNode = sliceWidget->sliceLogic()->GetSliceNode();
    QString tagStr(sliceNode->GetAttribute(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier()));
    unsigned long tag = tagStr.toULongLong();
    sliceNode->RemoveObserver(tag);
  }

  // 3D views
  for (int threeDViewId=0; threeDViewId<layoutManager->threeDViewCount(); ++threeDViewId)
  {
    qMRMLThreeDWidget* threeDWidget = layoutManager->threeDWidget(threeDViewId);
    qMRMLThreeDView* threeDView = threeDWidget->threeDView();
    vtkRenderWindowInteractor* interactor = threeDView->interactor();
    QList<QVariant> tags = threeDWidget->property(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier()).toList();
    foreach (QVariant tagVariant, tags)
    {
      unsigned long tag = tagVariant.toULongLong();
      interactor->RemoveObserver(tag);
    }

    // Nodes
    vtkMRMLViewNode* viewNode = threeDWidget->mrmlViewNode();
    QString tagStr(viewNode->GetAttribute(qSlicerSegmentEditorAbstractEffect::observerTagIdentifier()));
    unsigned long tag = tagStr.toULongLong();
    viewNode->RemoveObserver(tag);
  }

  // Clear observation commands
  foreach (vtkCallbackCommand* command, d->InteractionCallbackCommands)
  {
    command->Delete();
  }
  d->InteractionCallbackCommands.clear();
  foreach (SegmentEditorInteractionEventInfo* eventInfo, d->InteractionCallbackEventInfos)
  {
    delete eventInfo;
  }
  d->InteractionCallbackEventInfos.clear();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onEffectButtonClicked(QAbstractButton* button)
{
  Q_D(qMRMLSegmentEditorWidget);

  // Get effect that was just clicked
  qSlicerSegmentEditorAbstractEffect* clickedEffect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
    button->property("Effect").value<QObject*>() );

  // If the selected effect was clicked again, then de-select
  if (d->ActiveEffect == clickedEffect)
  {
    d->EffectButtonGroup.setExclusive(false);
    button->blockSignals(true);

    button->setChecked(false);
    this->setActiveEffect(NULL);

    button->blockSignals(false);
    d->EffectButtonGroup.setExclusive(true);
  }
  else
  {
     this->setActiveEffect(clickedEffect);
  }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::processEvents(vtkObject* caller,
                                        unsigned long eid,
                                        void* clientData,
                                        void* vtkNotUsed(callData))
{
  // Get and parse client data
  SegmentEditorInteractionEventInfo* eventInfo = reinterpret_cast<SegmentEditorInteractionEventInfo*>(clientData);
  qMRMLSegmentEditorWidget* editorWidget = eventInfo->EditorWidget;
  qMRMLWidget* viewWidget = eventInfo->ViewWidget;
  if (!editorWidget || !viewWidget)
  {
    qCritical() << "qMRMLSegmentEditorWidget::processInteractionEvents: Invalid event data!";
    return;
  }

  // Get active effect
  qSlicerSegmentEditorAbstractEffect* activeEffect = editorWidget->activeEffect();
  if (!activeEffect)
  {
    return;
  }

  // Call processing function of active effect. Handle both interactor and view node events
  vtkRenderWindowInteractor* callerInteractor = vtkRenderWindowInteractor::SafeDownCast(caller);
  vtkMRMLAbstractViewNode* callerViewNode = vtkMRMLAbstractViewNode::SafeDownCast(caller);
  if (callerInteractor)
  {
    activeEffect->processInteractionEvents(callerInteractor, eid, viewWidget);
  }
  else if (callerViewNode)
  {
    activeEffect->processViewNodeEvents(callerViewNode, eid, viewWidget);
  }
  else
  {
    qCritical() << "qMRMLSegmentEditorWidget::processInteractionEvents: Unsupported caller object!";
  }
}
