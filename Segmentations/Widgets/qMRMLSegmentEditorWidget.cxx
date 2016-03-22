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
#include "vtkMRMLSegmentEditorNode.h"
#include "vtkSegmentation.h"
#include "vtkSegment.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Segment editor effects includes
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorAbstractLabelEffect.h"
#include "qSlicerSegmentEditorEffectFactory.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorObserver.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkAlgorithmOutput.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

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

// CTK includes
#include <ctkFlowLayout.h>

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
  void initializeEffects();
  /// Initialize an effect. Called from \sa initializeEffects
  void initializeEffect(qSlicerSegmentEditorAbstractEffect* effect);

  /// Simple mechanism to let the effects know that edited labelmap has changed
  void notifyEffectsOfEditedLabelmapChange();
  /// Simple mechanism to let the effects know that master volume has changed
  void notifyEffectsOfMasterVolumeNodeChange();
  /// Simple mechanism to let the effects know that layout has changed
  void notifyEffectsOfLayoutChange();

  /// Create edited labelmap from selected segment, using the bounds of the master volume
  /// \return Success flag
  bool createEditedLabelmapFromSelectedSegment();

  /// Select first segment in table view
  void selectFirstSegment();

  /// Show selected segment in 2D views as fill only, all the others as outline only
  /// for per-segment effects, otherwise show all segments as fill only
  void showSelectedSegment();

  /// Enable or disable effects and their options based on input selection
  void updateEffectsEnabled();

  /// Set cursor for effect. If effect is NULL then the cursor is reset to default.
  void setEffectCursor(qSlicerSegmentEditorAbstractEffect* effect);

public:
  /// Segment editor parameter set node containing all selections and working images
  vtkWeakPointer<vtkMRMLSegmentEditorNode> ParameterSetNode;

  /// Default effect ordering
  QStringList DefaultEffectOrder;

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
  , ActiveEffect(NULL)
  , ParameterSetNode(NULL)
{
  this->InteractionCallbackCommands.clear();
  this->InteractionCallbackEventInfos.clear();
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::~qMRMLSegmentEditorWidgetPrivate()
{
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
    q, SLOT(onSegmentSelectionChanged(QItemSelection,QItemSelection)) );
  QObject::connect( this->AddSegmentButton, SIGNAL(clicked()), q, SLOT(onAddSegment()) );
  QObject::connect( this->RemoveSegmentButton, SIGNAL(clicked()), q, SLOT(onRemoveSegment()) );
  QObject::connect( this->CreateSurfaceButton, SIGNAL(toggled(bool)), q, SLOT(onCreateSurfaceToggled(bool)) );

  // Widget properties
  this->SegmentsTableView->setMode(qMRMLSegmentsTableView::EditorMode);
  this->AddSegmentButton->setEnabled(false);
  this->RemoveSegmentButton->setEnabled(false);
  this->CreateSurfaceButton->setEnabled(false);
  this->EffectsGroupBox->setEnabled(false);
  this->OptionsGroupBox->setEnabled(false);

  this->EffectButtonGroup.setExclusive(true);
  QObject::connect(&this->EffectButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), q, SLOT(onEffectButtonClicked(QAbstractButton*) ) );

  // Create layout for effect options
  QVBoxLayout* layout = new QVBoxLayout(this->EffectsOptionsFrame);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // Define default effect order
  this->DefaultEffectOrder << "Paint" << "Draw" << "Erase" << "Wand" << "LevelTracing"
    << "Rectangle" << "Dilate" << "Erode" << "GrowCut" << "Threshold"; //TODO: Add island effects, etc.

  // Instantiate and expose effects
  this->initializeEffects();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::initializeEffects()
{
  Q_Q(qMRMLSegmentEditorWidget);

  // Deactivate possible previous buttons
  QList<QAbstractButton*> effectButtons = this->EffectButtonGroup.buttons();
  foreach (QAbstractButton* button, effectButtons)
  {
    this->EffectButtonGroup.removeButton(button);
    //button->setVisible(false);
    button->deleteLater();
  }

  // Create local copy of factory effects, so that
  // - Effects can have different parameters
  // - Segment editors can have different active effects
  qSlicerSegmentEditorEffectFactory::instance()->copyEffects(this->RegisteredEffects);

  // Setup layout
  ctkFlowLayout* effectsGroupLayout = new ctkFlowLayout();
  effectsGroupLayout->setContentsMargins(4,4,4,4);
  effectsGroupLayout->setSpacing(4);
  this->EffectsGroupBox->setLayout(effectsGroupLayout);

  // Initialize effects specified in default ordering
  QList<qSlicerSegmentEditorAbstractEffect*> registeredEffectsCopy = this->RegisteredEffects;
  foreach (QString effectName, this->DefaultEffectOrder)
  {
    qSlicerSegmentEditorAbstractEffect* effect = q->effectByName(effectName);
    if (effect)
    {
      this->initializeEffect(effect);
      registeredEffectsCopy.removeOne(effect);
    }
  }
  // Initialize the rest of the effects
  foreach (qSlicerSegmentEditorAbstractEffect* effect, registeredEffectsCopy)
  {
    this->initializeEffect(effect);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::initializeEffect(qSlicerSegmentEditorAbstractEffect* effect)
{
  Q_Q(qMRMLSegmentEditorWidget);
  if (!effect)
  {
    return;
  }

  // Create button for activating effect
  QPushButton* effectButton = new QPushButton(this->EffectsGroupBox);
  effectButton->setObjectName(effect->name());
  effectButton->setCheckable(true);
  effectButton->setIcon(effect->icon());
  effectButton->setToolTip(effect->name());
  effectButton->setMaximumWidth(31);
  effectButton->setProperty("Effect", QVariant::fromValue<QObject*>(effect));

  this->EffectButtonGroup.addButton(effectButton);
  this->EffectsGroupBox->layout()->addWidget(effectButton);

  // Connect effect apply signal to commit changes to selected segment
  effect->connectApply(q, SLOT(applyChangesToSelectedSegment()));
  // Connect effect signal that allows initiating effect selection
  effect->connectSelectEffect(q, SLOT(setActiveEffectByName(QString)));

  // Add effect options frame to the options widget and hide them
  effect->setupOptionsFrame();
  QFrame* effectOptionsFrame = effect->optionsFrame();
  effectOptionsFrame->setVisible(false);
  this->EffectsOptionsFrame->layout()->addWidget(effectOptionsFrame);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::notifyEffectsOfEditedLabelmapChange()
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    effect->editedLabelmapChanged();
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::notifyEffectsOfMasterVolumeNodeChange()
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    effect->masterVolumeNodeChanged();
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::notifyEffectsOfLayoutChange()
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    effect->layoutChanged();
  }
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::createEditedLabelmapFromSelectedSegment()
{
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* masterVolumeNode = this->ParameterSetNode->GetMasterVolumeNode();
  const char* selectedSegmentID = this->ParameterSetNode->GetSelectedSegmentID();
  vtkOrientedImageData* editedLabelmap = this->ParameterSetNode->GetEditedLabelmap();

  // Clear edited labelmap
  editedLabelmap->Initialize();

  if (!segmentationNode || !masterVolumeNode || !editedLabelmap || !selectedSegmentID)
  {
    return false;
  }

  // Get binary labelmap representation of selected segment
  vtkSegment* selectedSegment = segmentationNode->GetSegmentation()->GetSegment(selectedSegmentID);
  vtkOrientedImageData* segmentLabelmap = vtkOrientedImageData::SafeDownCast(
    selectedSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
  if (!segmentLabelmap)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get binary labelmap representation in segmentation " << segmentationNode->GetName();
    return false;
  }

  // If segment is empty, then set up binary labelmap to be valid and editable
  if (segmentLabelmap->IsEmpty())
  {
    // Disable modified event to prevent execution of operations caused by it
    segmentationNode->GetSegmentation()->SetMasterRepresentationModifiedEnabled(false);

    // Create one voxel large labelmap and set values to zero
    int extent[6] = {0,0,0,0,0,0};
    segmentLabelmap->SetExtent(extent);
    segmentLabelmap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

    void* imageDataVoxelsPointer = segmentLabelmap->GetScalarPointerForExtent(extent);
    if (!imageDataVoxelsPointer)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to allocate memory for one-voxel image!";
      return false;
    }
    memset(imageDataVoxelsPointer, 0, segmentLabelmap->GetScalarSize());

    // Re-enable master representation modified event
    segmentationNode->GetSegmentation()->SetMasterRepresentationModifiedEnabled(true);
  }

  // Convert master volume to a temporary oriented image data
  vtkSmartPointer<vtkOrientedImageData> masterVolumeOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(masterVolumeNode) );

  // Pad segment binary labelmap to contain master volume to enable editing on its bounds
  if (!vtkOrientedImageDataResample::PadImageToContainImage(
    segmentLabelmap, masterVolumeOrientedImageData, editedLabelmap) )
  {
    qCritical() << Q_FUNC_INFO << ": Failed to pad binary labelmap of selected segment "
      << selectedSegmentID << " in segmentation " << segmentationNode->GetName() << " to contain master volume " << masterVolumeNode->GetName();
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::selectFirstSegment()
{
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if ( segmentationNode
    && segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0 )
  {
    std::vector<std::string> segmentIDs;
    segmentationNode->GetSegmentation()->GetSegmentIDs(segmentIDs);

    QStringList firstSegmentID;
    firstSegmentID << QString(segmentIDs[0].c_str());
    this->SegmentsTableView->setSelectedSegmentIDs(firstSegmentID);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::showSelectedSegment()
{
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentation display node!";
    return;
  }

  std::string selectedSegmentId(this->ParameterSetNode->GetSelectedSegmentID() ? this->ParameterSetNode->GetSelectedSegmentID() : "");

  // Show fill for selected segment, outline of all other segments
  // if per-segment effect is selected, otherwise show all as fill
  int wasModified = displayNode->StartModify();
  vtkSegmentation::SegmentMap segmentMap = segmentationNode->GetSegmentation()->GetSegments();
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
  {
    std::string currentSegmentId(segmentIt->first);
    if ( (!currentSegmentId.empty() && !currentSegmentId.compare(selectedSegmentId))
      || (this->ActiveEffect && !this->ActiveEffect->perSegment()) )
    {
      displayNode->SetSegmentVisibility2DFill(currentSegmentId, true);
      displayNode->SetSegmentVisibility2DOutline(currentSegmentId, false);
    }
    else
    {
      displayNode->SetSegmentVisibility2DFill(currentSegmentId, false);
      displayNode->SetSegmentVisibility2DOutline(currentSegmentId, true);
    }
  }
  displayNode->EndModify(wasModified);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::updateEffectsEnabled()
{
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }
  
  // Disable effect selection and options altogether if no master volume is selected
  bool masterVolumeSelected = this->ParameterSetNode->GetMasterVolumeNode();
  this->EffectsGroupBox->setEnabled(masterVolumeSelected);
  this->OptionsGroupBox->setEnabled(masterVolumeSelected);

  // Enable only non-per-segment effects if no segment is selected, otherwise enable all effects
  QString selectedSegmentID(this->ParameterSetNode->GetSelectedSegmentID());
  bool segmentSelected = !selectedSegmentID.isEmpty();
  QList<QAbstractButton*> effectButtons = this->EffectButtonGroup.buttons();
  foreach (QAbstractButton* effectButton, effectButtons)
  {
    qSlicerSegmentEditorAbstractEffect* effect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
      effectButton->property("Effect").value<QObject*>() );
    if (!effect)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to get effect from effect button " << effectButton->objectName();
      continue;
    }

    effectButton->setEnabled(segmentSelected || !effect->perSegment());
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::setEffectCursor(qSlicerSegmentEditorAbstractEffect* effect)
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  foreach(QString sliceViewName, layoutManager->sliceViewNames())
  {
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    if (effect)
    {
      sliceWidget->setCursor(effect->createCursor(sliceWidget));
    }
    else
    {
      sliceWidget->unsetCursor();
    }
  }
  /*
  TODO: Activate this when implementing effects in 3D views.
  It is not enabled now because:
  1. Cursor is not changedin the main view just in the header (it is a bug that should be fixed in the threeDWidget)
  2. Cursor should probably only changed for effects that work in the 3D view. Effects should state if they operate in
     slice and/or 3D view.

  for (int threeDViewId = 0; threeDViewId < layoutManager->threeDViewCount(); ++threeDViewId)
  {
    qMRMLThreeDWidget* threeDWidget = layoutManager->threeDWidget(threeDViewId);
    if (effect)
    {
      threeDWidget->setCursor(effect->createCursor(threeDWidget));
    }
    else
    {
      threeDWidget->unsetCursor();
    }
  }
  */
}

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
void qMRMLSegmentEditorWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  // Restore selections
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  d->MRMLNodeComboBox_Segmentation->blockSignals(true);
  d->MRMLNodeComboBox_Segmentation->setCurrentNode(segmentationNode);
  d->MRMLNodeComboBox_Segmentation->blockSignals(false);
  // Make sure handler method is called
  // (even if the same node is selected, hence the blockSignal calls above to prevent double call)
  this->onSegmentationNodeChanged(segmentationNode);

  vtkMRMLScalarVolumeNode* masterVolumeNode = d->ParameterSetNode->GetMasterVolumeNode();
  d->MRMLNodeComboBox_MasterVolume->setCurrentNode(masterVolumeNode);

  const char* selectedSegmentID = d->ParameterSetNode->GetSelectedSegmentID();
  if (selectedSegmentID)
  {
    QStringList segmentID;
    segmentID << QString(selectedSegmentID);
    d->SegmentsTableView->setSelectedSegmentIDs(segmentID);
  }
  else
  {
    d->SegmentsTableView->clearSelection();
  }

  const char* activeEffectName = d->ParameterSetNode->GetActiveEffectName();
  qSlicerSegmentEditorAbstractEffect* activeEffect = this->effectByName(activeEffectName);
  this->setActiveEffect(activeEffect);

  // Create observations between view interactors and the editor widget.
  // The captured events are propagated to the active effect if any.
  this->setupViewObservations();
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

  if (effect)
  {
    // Activate newly selected effect
    effect->activate();
    effect->updateGUIFromMRML();
    d->ActiveEffectLabel->setText(effect->name());
    d->HelpLabel->setToolTip(effect->helpText());

    // Check button that belongs to the effect in case this call did not come from the GUI
    QList<QAbstractButton*> effectButtons = d->EffectButtonGroup.buttons();
    foreach (QAbstractButton* effectButton, effectButtons)
    {
      if (!effectButton->objectName().compare(effect->name()))
      {
        effectButton->blockSignals(true);
        effectButton->setChecked(true);
        effectButton->blockSignals(false);
        break;
      }
    }


    // If selected effect is not per-segment, then clear segment selection
    // and prevent selection until a per-segment one is selected
    if (!effect->perSegment())
    {
      // Clear selection
      d->SegmentsTableView->clearSelection();
    }
    else if (d->ActiveEffect && !d->ActiveEffect->perSegment())
    {
      // Select first segment if previous effect was not per-segment
      d->selectFirstSegment();
    }
  }
  else
  {
    d->ActiveEffectLabel->setText("None");
    d->HelpLabel->setToolTip("No effect is selected");

    // Uncheck button that belongs to the effect in case this call did not come from the GUI
    QAbstractButton* effectButton = d->EffectButtonGroup.checkedButton();
    if (effectButton)
    {
      d->EffectButtonGroup.setExclusive(false);
      effectButton->blockSignals(true);
      effectButton->setChecked(false);
      effectButton->blockSignals(false);
      d->EffectButtonGroup.setExclusive(true);
    }
  }

  // Set cursor for active effect
  d->setEffectCursor(effect);

  // Set active effect
  d->ActiveEffect = effect;

  // Make sure the selected segment is properly shown based on selections
  d->showSelectedSegment();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (newScene == this->mrmlScene())
  {
    return;
  }

  Superclass::setMRMLScene(newScene);

  // Make connections that depend on the Slicer application
  QObject::connect( qSlicerApplication::application()->layoutManager(), SIGNAL(layoutChanged(int)), this, SLOT(onLayoutChanged(int)) );

  // Update UI
  this->updateWidgetFromMRML();
}

//------------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* qMRMLSegmentEditorWidget::mrmlSegmentEditorNode()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->ParameterSetNode;
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMRMLSegmentEditorNode(vtkMRMLSegmentEditorNode* newSegmentEditorNode)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (d->ParameterSetNode == newSegmentEditorNode)
  {
    return;
  }

  // Set parameter set node
  d->ParameterSetNode = newSegmentEditorNode;

  // Connect modified event on ParameterSetNode to updating the widget
  qvtkReconnect(d->ParameterSetNode, newSegmentEditorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  if (!d->ParameterSetNode)
  {
    return;
  }

  // Set parameter set node to all effects
  foreach(qSlicerSegmentEditorAbstractEffect* effect, d->RegisteredEffects)
  {
    effect->setParameterSetNode(d->ParameterSetNode);
    effect->setMRMLDefaults();

    // Connect parameter modified event to update effect options widget
    qvtkReconnect(d->ParameterSetNode, vtkMRMLSegmentEditorNode::EffectParameterModified, effect, SLOT(updateGUIFromMRML()) );
  }

  // Update UI
  this->updateWidgetFromMRML();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MRMLNodeComboBox_Segmentation->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentEditorWidget::segmentationNode()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_Segmentation->currentNode();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNodeID(const QString& nodeID)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MRMLNodeComboBox_Segmentation->setCurrentNodeID(nodeID);
}

//------------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::segmentationNodeID()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_Segmentation->currentNodeID();
}

//-----------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::currentSegmentID()const
{
  Q_D(const qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return QString();
  }

  const char* selectedSegmentID = d->ParameterSetNode->GetSelectedSegmentID();
  return QString(selectedSegmentID);
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMasterVolumeNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (node && !d->MRMLNodeComboBox_MasterVolume->isEnabled())
  {
    qCritical() << Q_FUNC_INFO << ": Cannot set master volume until segmentation is selected!";
    return;
  }
  d->MRMLNodeComboBox_MasterVolume->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentEditorWidget::masterVolumeNode()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_MasterVolume->currentNode();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMasterVolumeNodeID(const QString& nodeID)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->MRMLNodeComboBox_MasterVolume->isEnabled())
  {
    qCritical() << Q_FUNC_INFO << ": Cannot set master volume until segmentation is selected!";
    return;
  }
  d->MRMLNodeComboBox_MasterVolume->setCurrentNodeID(nodeID);
}

//------------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::masterVolumeNodeID()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_MasterVolume->currentNodeID();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    d->MRMLNodeComboBox_Segmentation->blockSignals(true);
    d->MRMLNodeComboBox_Segmentation->setCurrentNode(NULL);
    d->MRMLNodeComboBox_Segmentation->blockSignals(false);

    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  // Only enable master volume combobox and create surface action if segmentation selection is valid
  d->MRMLNodeComboBox_MasterVolume->setEnabled(node);
  d->CreateSurfaceButton->setEnabled(node);

  // Save segmentation node selection
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node);
  vtkMRMLSegmentationNode* currentSegmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (segmentationNode != currentSegmentationNode)
  {
    d->ParameterSetNode->DisableModifiedEventOn();
    d->ParameterSetNode->SetAndObserveSegmentationNode(segmentationNode);
    d->ParameterSetNode->DisableModifiedEventOff();

    // Connect events needed to update closed surface button
    qvtkReconnect( currentSegmentationNode, segmentationNode, vtkSegmentation::RepresentationCreated,
                   this, SLOT( updateCreateSurfaceButton() ) );
    qvtkReconnect( currentSegmentationNode, segmentationNode, vtkSegmentation::RepresentationRemoved,
                   this, SLOT( updateCreateSurfaceButton() ) );
    qvtkReconnect( currentSegmentationNode, segmentationNode, vtkSegmentation::SegmentAdded,
                   this, SLOT( updateCreateSurfaceButton() ) );
    qvtkReconnect( currentSegmentationNode, segmentationNode, vtkSegmentation::SegmentRemoved,
                   this, SLOT( updateCreateSurfaceButton() ) );
    // Update closed surface button with new segmentation
    this->updateCreateSurfaceButton();
  }

  // The below functions only apply to valid segmentation node selection
  if (!segmentationNode)
  {
    d->MRMLNodeComboBox_MasterVolume->setCurrentNode(NULL);
    return;
  }

  // Select master volume node in combobox if any
  vtkMRMLNode* referenceVolumeNode = segmentationNode->GetNodeReference(
    vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str() );
  // Make sure the master volume selection is performed fully before proceeding
  d->MRMLNodeComboBox_MasterVolume->blockSignals(true);
  d->MRMLNodeComboBox_MasterVolume->setCurrentNode(referenceVolumeNode);
  d->MRMLNodeComboBox_MasterVolume->blockSignals(false);
  this->onMasterVolumeNodeChanged(referenceVolumeNode);

  // Make sure there is a display node and get it
  segmentationNode->CreateDefaultDisplayNodes();
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    segmentationNode->GetDisplayNode());
  // Remember whether closed surface is present so that it can be re-converted later if necessary
  bool closedSurfacePresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
  // Show closed surface in 3D if present
  if (closedSurfacePresent)
  {
    displayNode->SetPreferredDisplayRepresentationName3D(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
  }

  // Representation related operations
  if (segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
  {
    // Make sure binary labelmap representation exists
    if (!segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) )
    {
      QString message = QString("Failed to create binary labelmap representation in segmentation %1 for editing!\nPlease see Segmentations module for details.").
        arg(segmentationNode->GetName());
      QMessageBox::critical(NULL, tr("Failed to create binary labelmap for editing"), message);
      qCritical() << Q_FUNC_INFO << ": " << message;
      return;
    }

    // Editing is only possible if binary labelmap is the master representation
    if ( strcmp(segmentationNode->GetSegmentation()->GetMasterRepresentationName(),
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) )
    {
      // If master is not binary labelmap, then ask the user if they wants to make it master
      QString message = QString("Editing is only possible if the master representation is binary labelmap, but currently the master representation is %1.\n\n"
        "Changing the master may mean losing important data that cannot be created again from the new master representation, "
        "such as nuance details in the model that is too fine to be represented in the labelmap grid.\n\n"
        "(Reminder: Master representation is the data type which is saved to disk, and which is used as input when creating other representations)\n\n").
        arg(segmentationNode->GetSegmentation()->GetMasterRepresentationName());
      QMessageBox::StandardButton answer =
        QMessageBox::question(NULL, tr("Change master representation to binary labelmap?"), message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (answer == QMessageBox::Yes)
      {
        segmentationNode->GetSegmentation()->SetMasterRepresentationName(
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );

        // All other representations are invalidated when changing to binary labelmap.
        // Re-creating closed surface if it was present before, so that changes can be seen.
        if (closedSurfacePresent)
        {
          segmentationNode->GetSegmentation()->CreateRepresentation(
            vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
        }
      }
      else
      {
        d->MRMLNodeComboBox_Segmentation->setCurrentNode(NULL);
        return;
      }
    }

    // Show binary labelmap in 2D
    if (displayNode)
    {
      displayNode->SetPreferredDisplayRepresentationName2D(
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );
    }

    // Select first segment to enable all effects (including per-segment ones)
    d->selectFirstSegment();
  }
  else
  {
    // If segmentation contains no segments, then set binary labelmap as master by default
    segmentationNode->GetSegmentation()->SetMasterRepresentationName(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );
  }

  // Set label layer to empty, because edit actor will be shown in the slice views during editing
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to get selection node to show segmentation node " << segmentationNode->GetName();
    return;
  }
  selectionNode->SetActiveLabelVolumeID(NULL);
  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  QStringList selectedSegmentIDs = d->SegmentsTableView->selectedSegmentIDs();
  d->RemoveSegmentButton->setEnabled(selectedSegmentIDs.count() > 0);
  if (selectedSegmentIDs.size() > 1)
  {
    qCritical() << Q_FUNC_INFO << ": One segment should be selected!";
    return;
  }

  // If selection did not change, then return
  QString currentSegmentID(d->ParameterSetNode->GetSelectedSegmentID());
  QString selectedSegmentID(selectedSegmentIDs.isEmpty() ? QString() : selectedSegmentIDs[0]);
  if (!currentSegmentID.compare(selectedSegmentID))
  {
    return;
  }

  // Set segment ID if changed
  d->ParameterSetNode->DisableModifiedEventOn();
  if (selectedSegmentIDs.isEmpty())
  {
    d->ParameterSetNode->SetSelectedSegmentID(NULL);
  }
  else
  {
    d->ParameterSetNode->SetSelectedSegmentID(selectedSegmentID.toLatin1().constData());

    // Also de-select current effect if it is not per-segment
    if (d->ActiveEffect && !d->ActiveEffect->perSegment())
    {
      this->setActiveEffect(NULL);
    }
  }
  d->ParameterSetNode->DisableModifiedEventOff();
  
  // Disable editing if no segment is selected
  d->updateEffectsEnabled();

  // Only enable remove button if a segment is selected
  d->RemoveSegmentButton->setEnabled(!selectedSegmentID.isEmpty());

  // Create edited labelmap from selected segment, using the bounds of the master volume
  d->createEditedLabelmapFromSelectedSegment();
  d->notifyEffectsOfEditedLabelmapChange();

  // Show selected segment as fill only, all the others as outline only for per-segment effects,
  // otherwise show all segments as fill only
  d->showSelectedSegment();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    d->MRMLNodeComboBox_MasterVolume->blockSignals(true);
    d->MRMLNodeComboBox_MasterVolume->setCurrentNode(NULL);
    d->MRMLNodeComboBox_MasterVolume->blockSignals(false);

    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  // Cannot set master volume if no segmentation node is selected
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }

  // Set master volume to parameter set node
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (d->ParameterSetNode->GetMasterVolumeNode() != volumeNode)
  {
    d->ParameterSetNode->DisableModifiedEventOn();
    d->ParameterSetNode->SetAndObserveMasterVolumeNode(volumeNode);
    d->ParameterSetNode->DisableModifiedEventOff();

    // Notify effects about change
    d->notifyEffectsOfMasterVolumeNodeChange();
  }

  // Disable editing if no master volume node is set:
  // master volume determines the extent of editing, so even though the segmentation is valid
  // without a master volume, editing is not possible until it is selected.
  d->updateEffectsEnabled();

  // Also disable adding new segments until master volume is set:
  // It defines the geometry of the labelmaps of the new segments
  d->AddSegmentButton->setEnabled(volumeNode);

  // Nothing else to do if no master volume is selected
  if (!volumeNode)
  {
    this->setActiveEffect(NULL);
    return;
  }

  // Set reference image connection and conversion parameter in selected segmentation
  if (segmentationNode->GetNodeReference(vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str()) != volumeNode)
  {
    segmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(volumeNode);
  }

  // Show reference volume in background layer of slice viewers
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to get selection node to show volume node " << volumeNode->GetName();
    return;
  }
  selectionNode->SetActiveVolumeID(volumeNode->GetID());
  selectionNode->SetSecondaryVolumeID(NULL); // Hide foreground volume
  qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();

  // Create edited labelmap from selected segment, using the bounds of the master volume
  if (d->createEditedLabelmapFromSelectedSegment())
  {
    d->notifyEffectsOfEditedLabelmapChange();
  }
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

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onAddSegment()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }

  // Create empty segment in current segmentation
  std::string addedSegmentID = segmentationNode->GetSegmentation()->AddEmptySegment();

  // Select the new segment
  if (!addedSegmentID.empty())
  {
    QStringList segmentIDList;
    segmentIDList << QString(addedSegmentID.c_str());
    d->SegmentsTableView->setSelectedSegmentIDs(segmentIDList);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onRemoveSegment()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  const char* selectedSegmentID = d->ParameterSetNode->GetSelectedSegmentID();
  if (!segmentationNode || !selectedSegmentID)
  {
    return;
  }

  // Remove segment
  segmentationNode->GetSegmentation()->RemoveSegment(selectedSegmentID);

  // Select first segment if there is at least one segment
  if (segmentationNode->GetSegmentation()->GetNumberOfSegments())
  {
    QStringList firstSegmentId;
    firstSegmentId << QString(segmentationNode->GetSegmentation()->GetSegments().begin()->first.c_str());
    d->SegmentsTableView->setSelectedSegmentIDs(firstSegmentId);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onCreateSurfaceToggled(bool on)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    return;
  }

  // If just have been checked, then create closed surface representation and show it
  if (on)
  {
    // Make sure closed surface representation exists
    if (segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() ))
    {
      // Set closed surface as displayed poly data representation
      displayNode->SetPreferredDisplayRepresentationName3D(
        vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
      // But keep binary labelmap for 2D
      displayNode->SetPreferredDisplayRepresentationName2D(
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );
    }
  }
  // If unchecked, then remove representation
  else
  {
    segmentationNode->GetSegmentation()->RemoveRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );

    // Trigger display update
    displayNode->Modified();
  }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateCreateSurfaceButton()
{
  Q_D(qMRMLSegmentEditorWidget);

  d->CreateSurfaceButton->setEnabled(false);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }

  // Enable button if there is at least one segment in the segmentation
  d->CreateSurfaceButton->setEnabled(segmentationNode->GetSegmentation()->GetNumberOfSegments());

  // Change button state based on whether it contains closed surface representation
  bool closedSurfacePresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
  d->CreateSurfaceButton->blockSignals(true);
  d->CreateSurfaceButton->setChecked(closedSurfacePresent);
  d->CreateSurfaceButton->blockSignals(false);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onLayoutChanged(int layoutIndex)
{
  Q_D(qMRMLSegmentEditorWidget);
  Q_UNUSED(layoutIndex);

  // Refresh view observations with the new layout
  this->setupViewObservations();

  // Set volume selection to all slice viewers in new layout
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (selectionNode && d->ParameterSetNode)
  {
    selectionNode->SetActiveVolumeID(d->ParameterSetNode->GetMasterVolumeNode() ? d->ParameterSetNode->GetMasterVolumeNode()->GetID() : NULL);
    selectionNode->SetSecondaryVolumeID(NULL);
    selectionNode->SetActiveLabelVolumeID(NULL);
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
  }

  // Let effects know about the updated layout
  d->notifyEffectsOfLayoutChange();
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

  return NULL;
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setupViewObservations()
{
  Q_D(qMRMLSegmentEditorWidget);

  // Make sure previous observations are cleared before setting up the new ones
  this->removeViewObservations();

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
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MouseMoveEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::KeyPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactionCallbackCommand, 1.0));
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
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::MouseMoveEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::KeyPressEvent, interactionCallbackCommand, 1.0));
    tags << QVariant((qulonglong)interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactionCallbackCommand, 1.0));
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
void qMRMLSegmentEditorWidget::removeViewObservations()
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

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::applyChangesToSelectedSegment()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  const char* selectedSegmentID = d->ParameterSetNode->GetSelectedSegmentID();
  if (!segmentationNode || !selectedSegmentID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment selection!";
    return;
  }

  vtkOrientedImageData* editedLabelmap = d->ParameterSetNode->GetEditedLabelmap();
  if (!editedLabelmap)
  {
    // If per-segment flag is off, then it is not an error (the effect itself has written it back to segmentation)
    if (d->ActiveEffect->perSegment())
    {
      qCritical() << Q_FUNC_INFO << ": Cannot apply edit operation because edited labelmap cannot be accessed!";
    }
    return;
  }

  // Label effects add the edited labelmap to the segment, because they provide the paint over
  // and paint threshold features, and the original segment data should not be lost.
  // The Erase label effect and all other (non-label) effects replace.
  bool append = false;
  vtkSmartPointer<vtkOrientedImageData> newSegmentLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
  if ( qobject_cast<qSlicerSegmentEditorAbstractLabelEffect*>(d->ActiveEffect)
    && d->ActiveEffect->name().compare("Erase") )
  {
    append = true;
  }

  // Copy the temporary padded edited labelmap to the segment.
  // Mask and threshold was already applied on edited labelmap at this point if requested.
  bool success = vtkSlicerSegmentationsModuleLogic::SetBinaryLabelmapToSegment(
    editedLabelmap, segmentationNode, selectedSegmentID, append );
  if (!success)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to set edited labelmap to selected segment!";
  }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setActiveEffectByName(QString effectName)
{
  qSlicerSegmentEditorAbstractEffect* effect = this->effectByName(effectName);
  this->setActiveEffect(effect);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::processEvents(vtkObject* caller,
                                        unsigned long eid,
                                        void* clientData,
                                        void* vtkNotUsed(callData))
{
  // Get and parse client data
  SegmentEditorInteractionEventInfo* eventInfo = reinterpret_cast<SegmentEditorInteractionEventInfo*>(clientData);
  qMRMLSegmentEditorWidget* self = eventInfo->EditorWidget;
  qMRMLWidget* viewWidget = eventInfo->ViewWidget;
  if (!self || !viewWidget)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid event data!";
    return;
  }
  // Do nothing if scene is closing
  if (!self->mrmlScene() || self->mrmlScene()->IsClosing())
  {
    return;
  }

  // Get active effect
  qSlicerSegmentEditorAbstractEffect* activeEffect = self->activeEffect();
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
    qCritical() << Q_FUNC_INFO << ": Unsupported caller object!";
  }
}
