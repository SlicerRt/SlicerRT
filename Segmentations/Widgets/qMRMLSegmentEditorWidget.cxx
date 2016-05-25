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
#include <vtkAlgorithmOutput.h>
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkDataArray.h>
#include <vtkGeneralTransform.h>
#include <vtkImageThreshold.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

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
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLViewNode.h>

// Qt includes
#include <QDebug>
#include <QPushButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <QVBoxLayout>

// CTK includes
#include <ctkFlowLayout.h>

static const int BINARY_LABELMAP_SCALAR_TYPE = VTK_UNSIGNED_CHAR;
static const unsigned char BINARY_LABELMAP_VOXEL_FULL = 255;
static const unsigned char BINARY_LABELMAP_VOXEL_EMPTY = 0;

//---------------------------------------------------------------------------
class vtkSegmentEditorEventCallbackCommand : public vtkCallbackCommand
{
public:
  static vtkSegmentEditorEventCallbackCommand *New()
    {
    return new vtkSegmentEditorEventCallbackCommand;
    }
  /// Segment editor widget observing the event
  QWeakPointer<qMRMLSegmentEditorWidget> EditorWidget;
  /// Slice widget or 3D widget
  QWeakPointer<qMRMLWidget> ViewWidget;
};

//-----------------------------------------------------------------------------
struct SegmentEditorEventObservation
{
  vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> CallbackCommand;
  vtkWeakPointer<vtkObject> ObservedObject;
  QVector<int> ObservationTags;
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

  /// Select first segment in table view
  void selectFirstSegment();

  /// Show selected segment in 2D views as fill only, all the others as outline only
  /// for per-segment effects, otherwise show all segments as fill only
  void showSelectedSegment();

  /// Enable or disable effects and their options based on input selection
  void updateEffectsEnabled();

  /// Set cursor for effect. If effect is NULL then the cursor is reset to default.
  void setEffectCursor(qSlicerSegmentEditorAbstractEffect* effect);

  /// Updates edited labelmap based on reference geometry. Updates reference geometry if it is empty.
  bool updateEditedLabelmap();

  /// Updates selected segment labelmap in a geometry aligned with editedLabelmap.
  bool updateSelectedSegmentLabelmap();

  /// Updates a resampled master volume in a geometry aligned with editedLabelmap.
  bool updateAlignedMasterVolume();

  /// Updates mask labelmap aligned with editedLabelmap.
  void updateMaskLabelmap();

  std::string getReferenceImageGeometryFromSegmentation(vtkSegmentation* segmentation);

public:
  /// Segment editor parameter set node containing all selections and working images
  vtkWeakPointer<vtkMRMLSegmentEditorNode> ParameterSetNode;

  vtkWeakPointer<vtkMRMLSegmentationNode> SegmentationNode;

  vtkWeakPointer<vtkMRMLScalarVolumeNode> MasterVolumeNode;

  /// Default effect ordering
  QStringList DefaultEffectOrder;

  /// List of registered effect instances
  QList<qSlicerSegmentEditorAbstractEffect*> RegisteredEffects;

  /// Active effect
  qSlicerSegmentEditorAbstractEffect* ActiveEffect;

  /// Structure containing necessary objects for each slice and 3D view handling interactions
  QVector<SegmentEditorEventObservation> EventObservations;

  /// Button group for the effects
  QButtonGroup EffectButtonGroup;

  /// These volumes are owned by this widget and a pointer is given to each effect
  /// so that they can access and modify it
  vtkOrientedImageData* AlignedMasterVolume;
  vtkOrientedImageData* EditedLabelmap;
  vtkOrientedImageData* SelectedSegmentLabelmap;
  vtkOrientedImageData* MaskLabelmap;

  /// Input data that is used for computing AlignedMasterVolume.
  /// It is stored so that it can be determined that the master volume has to be updated
  vtkMRMLVolumeNode* AlignedMasterVolumeUpdateMasterVolumeNode;
  vtkMRMLTransformNode* AlignedMasterVolumeUpdateMasterVolumeNodeTransform;
  vtkMRMLTransformNode* AlignedMasterVolumeUpdateSegmentationNodeTransform;

  int MaskModeComboBoxFixedItemsCount;
};

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::qMRMLSegmentEditorWidgetPrivate(qMRMLSegmentEditorWidget& object)
  : q_ptr(&object)
  , ActiveEffect(NULL)
  , ParameterSetNode(NULL)
  , AlignedMasterVolume(NULL)
  , SelectedSegmentLabelmap(NULL)
  , EditedLabelmap(NULL)
  , MaskLabelmap(NULL)
  , MaskModeComboBoxFixedItemsCount(0)
  , AlignedMasterVolumeUpdateMasterVolumeNode(NULL)
  , AlignedMasterVolumeUpdateMasterVolumeNodeTransform(NULL)
  , AlignedMasterVolumeUpdateSegmentationNodeTransform(NULL)
{
  this->AlignedMasterVolume = vtkOrientedImageData::New();
  this->EditedLabelmap = vtkOrientedImageData::New();
  this->MaskLabelmap = vtkOrientedImageData::New();
  this->SelectedSegmentLabelmap = vtkOrientedImageData::New();
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::~qMRMLSegmentEditorWidgetPrivate()
{
  Q_Q(qMRMLSegmentEditorWidget);
  q->removeViewObservations();

  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
  {
    delete effect;
  }
  this->RegisteredEffects.clear();
  if (this->AlignedMasterVolume)
  {
    this->AlignedMasterVolume->Delete();
    this->AlignedMasterVolume = NULL;
  }
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
  if (this->SelectedSegmentLabelmap)
  {
    this->SelectedSegmentLabelmap->Delete();
    this->SelectedSegmentLabelmap = NULL;
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::init()
{
  Q_Q(qMRMLSegmentEditorWidget);
  this->setupUi(q);

  this->MaskModeComboBox->addItem(QObject::tr("Everywhere"), vtkMRMLSegmentEditorNode::PaintAllowedEverywhere);
  this->MaskModeComboBox->addItem(QObject::tr("Inside all segments"), vtkMRMLSegmentEditorNode::PaintAllowedInsideAllSegments);
  this->MaskModeComboBox->addItem(QObject::tr("Inside all visible segments"), vtkMRMLSegmentEditorNode::PaintAllowedInsideVisibleSegments);
  this->MaskModeComboBox->addItem(QObject::tr("Outside all segments"), vtkMRMLSegmentEditorNode::PaintAllowedOutsideAllSegments);
  this->MaskModeComboBox->addItem(QObject::tr("Outside all visible segments"), vtkMRMLSegmentEditorNode::PaintAllowedOutsideVisibleSegments);
  this->MaskModeComboBox->insertSeparator(this->MaskModeComboBox->count());
  this->MaskModeComboBoxFixedItemsCount = this->MaskModeComboBox->count();

  this->OverwriteModeComboBox->addItem(QObject::tr("All segments"), vtkMRMLSegmentEditorNode::OverwriteAllSegments);
  this->OverwriteModeComboBox->addItem(QObject::tr("Visible segments"), vtkMRMLSegmentEditorNode::OverwriteVisibleSegments);
  this->OverwriteModeComboBox->addItem(QObject::tr("None"), vtkMRMLSegmentEditorNode::OverwriteNone);

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

  QObject::connect( this->MaskModeComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(onMaskModeChanged(int)));
  QObject::connect( this->MasterVolumeIntensityMaskCheckBox, SIGNAL(toggled(bool)), q, SLOT(onMasterVolumeIntensityMaskChecked(bool)));
  QObject::connect( this->MasterVolumeIntensityMaskRangeWidget, SIGNAL(valuesChanged(double,double)), q, SLOT(onMasterVolumeIntensityMaskRangeChanged(double,double)));
  QObject::connect( this->OverwriteModeComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(onOverwriteModeChanged(int)));

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
  effect->setCallbackSlots(q,
    SLOT(applyChangesToSelectedSegment()),
    SLOT(setActiveEffectByName(QString)),
    SLOT(updateVolume(void*)) );

  effect->setVolumes(this->AlignedMasterVolume, this->EditedLabelmap, this->MaskLabelmap, this->SelectedSegmentLabelmap);

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
bool qMRMLSegmentEditorWidgetPrivate::updateEditedLabelmap()
{
  Q_Q(qMRMLSegmentEditorWidget);
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkSegmentation* segmentation = segmentationNode ? segmentationNode->GetSegmentation() : NULL;

  if (!segmentationNode || !segmentation || !this->EditedLabelmap)
  {
    return false;
  }

  std::string editedLabelmapReferenceImageGeometryBaseline = vtkSegmentationConverter::SerializeImageGeometry(this->EditedLabelmap);

  // Determine reference geometry
  std::string referenceImageGeometry;
  referenceImageGeometry = this->getReferenceImageGeometryFromSegmentation(segmentation);
  if (referenceImageGeometry.empty())
  {
    // If no reference image geometry could be determined then use the master volume's geometry

    vtkMRMLScalarVolumeNode* masterVolumeNode = this->ParameterSetNode->GetMasterVolumeNode();
    if (!masterVolumeNode)
    {
      // cannot determine reference geometry
      return false;
    }
    // Transform the master volume to the segmentation node's coordinate system
    // TODO: this is inefficient, as it actually creates a resampled data set, while we would only need the geometry
    //vtkSmartPointer<vtkOrientedImageData> masterVolume = vtkSmartPointer<vtkOrientedImageData>::Take(
    //  vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(masterVolumeNode, segmentationNode->GetParentTransformNode()));

    //referenceImageGeometry = vtkSegmentationConverter::SerializeImageGeometry(masterVolume);

    // Update the referenceImageGeometry parameter so that next time 
    segmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(masterVolumeNode);
    //segmentation->SetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName(), referenceImageGeometry);
    referenceImageGeometry = this->getReferenceImageGeometryFromSegmentation(segmentation);

  }

  // Set reference geometry to labelmap (origin, spacing, directions, extents) and allocate scalars
  vtkNew<vtkMatrix4x4> referenceGeometryMatrix;
  int referenceExtent[6] = {0,-1,0,-1,0,-1};
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceGeometryMatrix.GetPointer(), referenceExtent);
  bool labelmapVoxelDataReallocated = false;
  int* labelmapExtent = this->EditedLabelmap->GetExtent();
  if (referenceExtent[1]-referenceExtent[0] != labelmapExtent[1]-labelmapExtent[0]
    || referenceExtent[3]-referenceExtent[2] != labelmapExtent[3]-labelmapExtent[2]
    || referenceExtent[5]-referenceExtent[4] != labelmapExtent[5]-labelmapExtent[4])
    {
      labelmapVoxelDataReallocated = true;
    }  
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, this->EditedLabelmap, BINARY_LABELMAP_SCALAR_TYPE, 1);

  // If scalars were reallocate then make sure the new memory area is initialized with zeros
  if (labelmapVoxelDataReallocated)
  {
    vtkOrientedImageDataResample::FillImage(this->EditedLabelmap, BINARY_LABELMAP_VOXEL_EMPTY);
  }

  if (editedLabelmapReferenceImageGeometryBaseline != referenceImageGeometry)
  {
    this->notifyEffectsOfEditedLabelmapChange();
  }
  
  return true;
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::updateSelectedSegmentLabelmap()
{
  Q_Q(qMRMLSegmentEditorWidget);  
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* masterVolumeNode = this->ParameterSetNode->GetMasterVolumeNode();
  updateEditedLabelmap();
  if (!segmentationNode || !masterVolumeNode || !this->EditedLabelmap || !this->AlignedMasterVolume)
  {
    return false;
  }
  const char* selectedSegmentID = this->ParameterSetNode->GetSelectedSegmentID();
  if (!selectedSegmentID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment selection";
    return false;
  }

  vtkSegment* selectedSegment = segmentationNode->GetSegmentation()->GetSegment(selectedSegmentID);
  vtkOrientedImageData* segmentLabelmap = vtkOrientedImageData::SafeDownCast(
    selectedSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()));
  if (!segmentLabelmap)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to get binary labelmap representation in segmentation " << segmentationNode->GetName();
    return false;
  }

  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(segmentLabelmap, this->EditedLabelmap, this->SelectedSegmentLabelmap,
    /*linearInterpolation=*/false, /*padImage=*/false);

  return true;
}


//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::updateAlignedMasterVolume()
{
  Q_Q(qMRMLSegmentEditorWidget);
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* masterVolumeNode = this->ParameterSetNode->GetMasterVolumeNode();
  updateEditedLabelmap();
  if (!segmentationNode || !masterVolumeNode || !this->EditedLabelmap || !this->AlignedMasterVolume)
  {
    return false;
  }
  
  // If master volume node and transform nodes did not change then we don't need to update the aligned
  // master volume.
  if (vtkOrientedImageDataResample::DoGeometriesMatch(this->EditedLabelmap, this->AlignedMasterVolume)
    && vtkOrientedImageDataResample::DoExtentsMatch(this->EditedLabelmap, this->AlignedMasterVolume)
    && this->AlignedMasterVolumeUpdateMasterVolumeNode == masterVolumeNode
    && this->AlignedMasterVolumeUpdateMasterVolumeNodeTransform == masterVolumeNode->GetParentTransformNode()
    && this->AlignedMasterVolumeUpdateSegmentationNodeTransform == segmentationNode->GetParentTransformNode() )
  {
    // Extents and nodes are matching, check if they have not been modified since the aligned master
    // volume generation.
    bool updateAlignedMasterVolumeRequired = false;
    if (masterVolumeNode->GetMTime() > this->AlignedMasterVolume->GetMTime())
    {
      updateAlignedMasterVolumeRequired = true;
    }
    else if (masterVolumeNode->GetParentTransformNode() && masterVolumeNode->GetParentTransformNode()->GetMTime() > this->AlignedMasterVolume->GetMTime())
    {
      updateAlignedMasterVolumeRequired = true;
    }
    else if (segmentationNode->GetParentTransformNode() && segmentationNode->GetParentTransformNode()->GetMTime() > this->AlignedMasterVolume->GetMTime())
    {
      updateAlignedMasterVolumeRequired = true;
    }
    if (!updateAlignedMasterVolumeRequired)
    {
      return true;
    }
  }

  // Get a read-only version of masterVolume as a vtkOrientedImageData
  vtkNew<vtkOrientedImageData> masterVolume;
  masterVolume->vtkImageData::ShallowCopy(masterVolumeNode->GetImageData());
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  masterVolumeNode->GetIJKToRASMatrix(ijkToRasMatrix);
  masterVolume->SetGeometryFromImageToWorldMatrix(ijkToRasMatrix);
  
  vtkNew<vtkGeneralTransform> masterVolumeToSegmentationTransform;
  vtkMRMLTransformNode::GetTransformBetweenNodes(masterVolumeNode->GetParentTransformNode(), segmentationNode->GetParentTransformNode(), masterVolumeToSegmentationTransform.GetPointer());
  
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(masterVolume.GetPointer(), this->EditedLabelmap, this->AlignedMasterVolume,
    /*linearInterpolation=*/true, /*padImage=*/false, masterVolumeToSegmentationTransform.GetPointer());

  this->AlignedMasterVolumeUpdateMasterVolumeNode = masterVolumeNode;
  this->AlignedMasterVolumeUpdateMasterVolumeNodeTransform = masterVolumeNode->GetParentTransformNode();
  this->AlignedMasterVolumeUpdateSegmentationNodeTransform = segmentationNode->GetParentTransformNode();

  return true;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::updateMaskLabelmap()
{
  Q_Q(qMRMLSegmentEditorWidget);

  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  vtkOrientedImageData* maskImage = this->MaskLabelmap;
  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if (!this->EditedLabelmap || !segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment selection!";
    return;
  }
  if (!this->ParameterSetNode->GetSelectedSegmentID())
  {
    return;
  }

  std::vector<std::string> allSegmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDs(allSegmentIDs);

  std::vector<std::string> visibleSegmentIDs;
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (displayNode)
  {
    for (std::vector<std::string>::iterator segmentIDIt = allSegmentIDs.begin(); segmentIDIt != allSegmentIDs.end(); ++segmentIDIt)
    {
      if (displayNode->GetSegmentVisibility(*segmentIDIt))
      {
        visibleSegmentIDs.push_back(*segmentIDIt);
      }
    }
  }

  std::string editedSegmentID(this->ParameterSetNode->GetSelectedSegmentID());
  
  std::vector<std::string> maskSegmentIDs;
  bool paintInsideSegments = false;
  switch (this->ParameterSetNode->GetMaskMode())
  {
  case vtkMRMLSegmentEditorNode::PaintAllowedEverywhere:
    paintInsideSegments = false;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedInsideAllSegments:
    paintInsideSegments = true;
    maskSegmentIDs = allSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedInsideVisibleSegments:
    paintInsideSegments = true;
    maskSegmentIDs = visibleSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedOutsideAllSegments:
    paintInsideSegments = false;
    maskSegmentIDs = allSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedOutsideVisibleSegments:
    paintInsideSegments = false;
    maskSegmentIDs = visibleSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment:
    paintInsideSegments = true;
    if (this->ParameterSetNode->GetMaskSegmentID())
    {
      maskSegmentIDs.push_back(this->ParameterSetNode->GetMaskSegmentID());
    }
    else
    {
      qWarning() << Q_FUNC_INFO << ": PaintAllowedInsideSingleSegment selected but no mask segment is specified";
    }
    break;
  default:
    qWarning() << Q_FUNC_INFO << ": unknown mask mode";
  }

  // Always allow paint inside edited segment
  if (paintInsideSegments)
  {
    // include edited segment in "inside" mask
    if (std::find(maskSegmentIDs.begin(), maskSegmentIDs.end(), editedSegmentID) == maskSegmentIDs.end())
    {
      // add it if it's not in the segment list already
      maskSegmentIDs.push_back(editedSegmentID);
    }
  }
  else
  {
    // exclude edited segment from "outside" mask
    maskSegmentIDs.erase(std::remove(maskSegmentIDs.begin(), maskSegmentIDs.end(), editedSegmentID), maskSegmentIDs.end());
  }

  // Update mask if edited labelmap is valid, and the edited labelmap was modified after the last time the mask was updated
  int editedLabelmapExtent[6] = { 0, -1, 0, -1, 0, -1 };
  this->EditedLabelmap->GetExtent(editedLabelmapExtent);
  if (this->EditedLabelmap->GetMTime() > maskImage->GetMTime()
    && editedLabelmapExtent[0] < editedLabelmapExtent[1] && editedLabelmapExtent[2] < editedLabelmapExtent[3] && editedLabelmapExtent[4] < editedLabelmapExtent[5])
  {
    maskImage->SetExtent(editedLabelmapExtent);
    maskImage->AllocateScalars(VTK_SHORT, 1); // Change scalar type from unsigned int back to short for merged labelmap generation

    vtkSmartPointer<vtkMatrix4x4> mergedImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    segmentationNode->GenerateMergedLabelmap(maskImage, mergedImageToWorldMatrix, this->EditedLabelmap, maskSegmentIDs);

    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetInputData(maskImage);
    threshold->SetInValue(paintInsideSegments ? 0 : 255);
    threshold->SetOutValue(paintInsideSegments ? 255 : 0);
    threshold->ReplaceInOn();
    threshold->ThresholdByUpper(1);
    threshold->SetOutputScalarType(VTK_UNSIGNED_CHAR);
    threshold->Update();

    maskImage->DeepCopy(threshold->GetOutput());
    maskImage->SetGeometryFromImageToWorldMatrix(mergedImageToWorldMatrix);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::selectFirstSegment()
{
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
  // Here we could change the selected segment's display.
  // It seems that it is not necessary for now, but keep the method
  // as a placeholder for a while.
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::updateEffectsEnabled()
{
  if (!this->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  int wasModified = d->ParameterSetNode->StartModify();

  updateWidgetFromSegmentationNode();
  updateWidgetFromMasterVolumeNode();

  // Disable adding new segments until master volume is set (or reference geometry is specified for the segmentation).
  // This forces the user to select a master volume before start addding segments.
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  vtkSegmentation* segmentation = segmentationNode ? segmentationNode->GetSegmentation() : NULL;
  bool enableAddSegments = (segmentationNode!=NULL) && ((d->MasterVolumeNode != NULL) || (!d->getReferenceImageGeometryFromSegmentation(segmentation).empty()));
  d->AddSegmentButton->setEnabled(enableAddSegments);

  // Segments list section

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

  // Masking section

  bool wasBlocked = d->MasterVolumeIntensityMaskCheckBox->blockSignals(true);
  d->MasterVolumeIntensityMaskCheckBox->setChecked(d->ParameterSetNode->GetMasterVolumeIntensityMask());
  d->MasterVolumeIntensityMaskCheckBox->blockSignals(wasBlocked);

  // Initialize mask range if it has never set and intensity masking es enabled
  if (d->ParameterSetNode->GetMasterVolumeIntensityMask()
    && d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[0] == d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[1])
  {
    // threshold was uninitialized, set some default
    double range[2] = { 0.0 };
    d->MasterVolumeNode->GetImageData()->GetScalarRange(range);
    d->ParameterSetNode->SetMasterVolumeIntensityMaskRange(range[0] + 0.25*(range[1] - range[0]), range[0] + 0.75*(range[1] - range[0]));
  }

  wasBlocked = d->MasterVolumeIntensityMaskRangeWidget->blockSignals(true);
  d->MasterVolumeIntensityMaskRangeWidget->setVisible(d->ParameterSetNode->GetMasterVolumeIntensityMask());
  d->MasterVolumeIntensityMaskRangeWidget->setMinimumValue(d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[0]);
  d->MasterVolumeIntensityMaskRangeWidget->setMaximumValue(d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[1]);
  d->MasterVolumeIntensityMaskRangeWidget->blockSignals(wasBlocked);

  wasBlocked = d->OverwriteModeComboBox->blockSignals(true);
  int overwriteModeIndex = -1;
  if (d->ParameterSetNode->GetOverwriteMode() == vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment)
  {
    // segment item
    overwriteModeIndex = d->OverwriteModeComboBox->findData(d->ParameterSetNode->GetMaskSegmentID());
  }
  else
  {
    // fixed item, identified by overwrite mode id
    overwriteModeIndex = d->OverwriteModeComboBox->findData(d->ParameterSetNode->GetOverwriteMode());
  }
  d->OverwriteModeComboBox->setCurrentIndex(overwriteModeIndex);
  d->OverwriteModeComboBox->blockSignals(wasBlocked);

  // Effects section
 
  updateWidgetFromEffect();

  d->ParameterSetNode->EndModify(wasModified);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateWidgetFromSegmentationNode()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  // Save segmentation node selection
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (segmentationNode == d->SegmentationNode)
  {
    return;
  }

  // Connect events needed to update closed surface button
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::RepresentationCreated, this, SLOT(onSegmentAddedRemoved()));
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::RepresentationRemoved, this, SLOT(onSegmentAddedRemoved()));
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentAdded, this, SLOT(onSegmentAddedRemoved()));
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentRemoved, this, SLOT(onSegmentAddedRemoved()));
  d->SegmentationNode = segmentationNode;

  bool wasBlocked = d->MRMLNodeComboBox_Segmentation->blockSignals(true);
  d->MRMLNodeComboBox_Segmentation->setCurrentNode(d->SegmentationNode);
  d->MRMLNodeComboBox_Segmentation->blockSignals(wasBlocked);

  // Update closed surface button with new segmentation
  this->onSegmentAddedRemoved();

  d->EffectsGroupBox->setEnabled(d->SegmentationNode != NULL);
  d->MaskingGroupBox->setEnabled(d->SegmentationNode != NULL);
  d->EffectsOptionsFrame->setEnabled(d->SegmentationNode != NULL);
  d->MRMLNodeComboBox_MasterVolume->setEnabled(segmentationNode != NULL);
  d->CreateSurfaceButton->setEnabled(segmentationNode != NULL);

  // The below functions only apply to valid segmentation node selection
  if (!segmentationNode)
  {
    return;
  }

  // If a geometry reference volume was defined for this segmentation then select it as master volumeSelect master volume node
  vtkMRMLNode* referenceVolumeNode = segmentationNode->GetNodeReference(
    vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str());
  // Make sure the master volume selection is performed fully before proceeding
  d->MRMLNodeComboBox_MasterVolume->setCurrentNode(referenceVolumeNode);

  // Make sure there is a display node and get it
  segmentationNode->CreateDefaultDisplayNodes();
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());

  // Remember whether closed surface is present so that it can be re-converted later if necessary
  bool closedSurfacePresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
  // Show closed surface in 3D if present
  if (closedSurfacePresent)
  {
    displayNode->SetPreferredDisplayRepresentationName3D(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
  }

  // Representation related operations
  if (segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
  {
    // Make sure binary labelmap representation exists
    if (!segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      QString message = QString("Failed to create binary labelmap representation in segmentation %1 for editing!\nPlease see Segmentations module for details.").
        arg(segmentationNode->GetName());
      QMessageBox::critical(NULL, tr("Failed to create binary labelmap for editing"), message);
      qCritical() << Q_FUNC_INFO << ": " << message;
      return;
    }

    // Editing is only possible if binary labelmap is the master representation
    if (strcmp(segmentationNode->GetSegmentation()->GetMasterRepresentationName(),
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
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
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

        // All other representations are invalidated when changing to binary labelmap.
        // Re-creating closed surface if it was present before, so that changes can be seen.
        if (closedSurfacePresent)
        {
          segmentationNode->GetSegmentation()->CreateRepresentation(
            vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
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
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    }

    // Select first segment to enable all effects (including per-segment ones)
    d->selectFirstSegment();
  }
  else
  {
    // If segmentation contains no segments, then set binary labelmap as master by default
    segmentationNode->GetSegmentation()->SetMasterRepresentationName(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
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
std::string qMRMLSegmentEditorWidgetPrivate::getReferenceImageGeometryFromSegmentation(vtkSegmentation* segmentation)
{
  if (!segmentation)
  {
    return "";
  }

  // If "reference image geometry" conversion parameter is set then use that
  std::string referenceImageGeometry = segmentation->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName());
  if (!referenceImageGeometry.empty())
  {
    // TODO: Use oversampling (if it's 'A' then ignore and changed to 1)
    return referenceImageGeometry;
  }
  if (segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    // If no reference image geometry is specified but there are labels already then determine geometry from that
    referenceImageGeometry = segmentation->DetermineCommonLabelmapGeometry();
    return referenceImageGeometry;
  }
  return "";
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateWidgetFromMasterVolumeNode()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    return;
  }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
  {
    return;
  }
  vtkMRMLScalarVolumeNode* masterVolumeNode = d->ParameterSetNode->GetMasterVolumeNode();
  if (masterVolumeNode == d->MasterVolumeNode)
  {
    // no change
    return;
  }

  qvtkReconnect(d->MasterVolumeNode, masterVolumeNode, vtkMRMLVolumeNode::ImageDataModifiedEvent, this, SLOT(onMasterVolumeImageDataModified()));
  d->MasterVolumeNode = masterVolumeNode;

  bool wasBlocked = d->MRMLNodeComboBox_MasterVolume->blockSignals(true);
  d->MRMLNodeComboBox_MasterVolume->setCurrentNode(d->MasterVolumeNode);
  d->MRMLNodeComboBox_MasterVolume->blockSignals(wasBlocked);

  this->onMasterVolumeImageDataModified();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeImageDataModified()
{
  Q_D(qMRMLSegmentEditorWidget);

  // Update intensity range slider widget
  if (d->MasterVolumeNode != NULL && d->MasterVolumeNode->GetImageData() != NULL)
  {
    double range[2] = { 0.0, 0.0 };
    d->MasterVolumeNode->GetImageData()->GetScalarRange(range);
    d->MasterVolumeIntensityMaskRangeWidget->setMinimum(range[0]);
    d->MasterVolumeIntensityMaskRangeWidget->setMaximum(range[1]);
    d->MasterVolumeIntensityMaskRangeWidget->setEnabled(true);
  }
  else
  {
    d->MasterVolumeIntensityMaskRangeWidget->setEnabled(false);
  }
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

  if (!d->ParameterSetNode)
  {
    if (effect != NULL)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    }
    return;
  }

  d->ParameterSetNode->SetActiveEffectName(effect ? effect->name().toLatin1() : "");
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateWidgetFromEffect()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  // Disable editing if no master volume node is set:
  // master volume determines the extent of editing, so even though the segmentation is valid
  // without a master volume, editing is not possible until it is selected.
  d->updateEffectsEnabled();

  const char* activeEffectName = d->ParameterSetNode->GetActiveEffectName();
  qSlicerSegmentEditorAbstractEffect* activeEffect = this->effectByName(activeEffectName); // newly selected effect
  if (activeEffect == d->ActiveEffect)
  {
    return;
  }
  
  // Deactivate previously selected effect
  if (d->ActiveEffect)
  {
    d->ActiveEffect->deactivate();
  }

  if (activeEffect)
  {
    // Create observations between view interactors and the editor widget.
    // The captured events are propagated to the active effect if any.
    this->setupViewObservations();

    // TODO: we should check if edited labelmap and aligned master volume is up-to-date,
    // switch here instead of qMRMLSegmentEditorWidget::onSegmentationNodeChanged, as
    // a segmentation node may be selected by the user accidentally.

    // Activate newly selected effect
    d->updateEditedLabelmap(); // always pre-allocate editedLabelmap image (it may be modified by the effect)
    activeEffect->activate();
    activeEffect->updateGUIFromMRML();
    d->ActiveEffectLabel->setText(activeEffect->name());
    d->HelpLabel->setToolTip(activeEffect->helpText());

    // Check button that belongs to the effect in case this call did not come from the GUI
    QList<QAbstractButton*> effectButtons = d->EffectButtonGroup.buttons();
    foreach (QAbstractButton* effectButton, effectButtons)
    {
      if (!effectButton->objectName().compare(activeEffect->name()))
      {
        effectButton->blockSignals(true);
        effectButton->setChecked(true);
        effectButton->blockSignals(false);
        break;
      }
    }


    // If selected effect is not per-segment, then clear segment selection
    // and prevent selection until a per-segment one is selected
    if (!activeEffect->perSegment())
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

    this->removeViewObservations();
  }

  // Set cursor for active effect
  d->setEffectCursor(activeEffect);

  // Set active effect
  d->ActiveEffect = activeEffect;

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

  // Connect modified event on ParameterSetNode to updating the widget
  qvtkReconnect(d->ParameterSetNode, newSegmentEditorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set parameter set node
  d->ParameterSetNode = newSegmentEditorNode;

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
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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

    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  setActiveEffect(NULL); // deactivate current effect when we switch to a different segmentation
  d->ParameterSetNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
  //this->updateEditedVolumes();
  //this->updateMaskLabelmap();
  //d->notifyEffectsOfEditedLabelmapChange();

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

    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
    int wasModified = d->ParameterSetNode->StartModify();
    d->ParameterSetNode->SetAndObserveMasterVolumeNode(volumeNode);
    d->ParameterSetNode->EndModify(wasModified);

    if (volumeNode)
    {
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
    }

    // Notify effects about change
    d->notifyEffectsOfMasterVolumeNodeChange();
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
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
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
void qMRMLSegmentEditorWidget::onSegmentAddedRemoved()
{
  Q_D(qMRMLSegmentEditorWidget);

  vtkMRMLSegmentationNode* segmentationNode = NULL;
  if (d->ParameterSetNode)
  {
    segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
  }

  // Update state of CreateSurfaceButton
  if (segmentationNode)
  {
    // Enable button if there is at least one segment in the segmentation
    d->CreateSurfaceButton->setEnabled(segmentationNode->GetSegmentation()->GetNumberOfSegments());

    // Change button state based on whether it contains closed surface representation
    bool closedSurfacePresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
    d->CreateSurfaceButton->blockSignals(true);
    d->CreateSurfaceButton->setChecked(closedSurfacePresent);
    d->CreateSurfaceButton->blockSignals(false);
  }
  else
  {
    d->CreateSurfaceButton->setEnabled(false);
  }

  // Update mask mode combo box with current segment names

  bool wasBlocked = d->MaskModeComboBox->blockSignals(true);
  
  // save selection (if it's a non-fixed item)
  QString selectedSegmentId;
  if (d->MaskModeComboBox->currentIndex() >= d->MaskModeComboBoxFixedItemsCount)
  {
    selectedSegmentId = d->MaskModeComboBox->itemData(d->MaskModeComboBox->currentIndex()).toString();
  }

  // Remove segment names, keep only fixed items
  while (d->MaskModeComboBox->count() > d->MaskModeComboBoxFixedItemsCount)
  {
    d->MaskModeComboBox->removeItem(d->MaskModeComboBox->count()-1);
  }

  if (segmentationNode)
  {
    //bool labelmapPresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
    //  vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );

    vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
    vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      QString segmentName = segmentIt->second->GetName();
      d->MaskModeComboBox->addItem(tr("Inside ") + segmentName, QString::fromLocal8Bit(segmentIt->first.c_str()));
    }

    // restore selection
    if (!selectedSegmentId.isEmpty())
    {
      int maskModeIndex = d->MaskModeComboBox->findData(selectedSegmentId);
      d->MaskModeComboBox->setCurrentIndex(maskModeIndex);
    }
  }
  d->MaskModeComboBox->blockSignals(wasBlocked);

  if (segmentationNode && d->MaskModeComboBox->currentIndex()<0)
  {
    // probably the currently selected mask segment was deleted,
    // switch to the first masking option (no mask).
    d->MaskModeComboBox->setCurrentIndex(0);
  }

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
    // Create command for slice view
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    qMRMLSliceView* sliceView = sliceWidget->sliceView();
    vtkNew<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand;
    interactionCallbackCommand->EditorWidget = this;
    interactionCallbackCommand->ViewWidget = sliceWidget;
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(interactionCallbackCommand.GetPointer()) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processEvents );

    // Connect interactor events
    vtkRenderWindowInteractor* interactor = sliceView->interactorStyle()->GetInteractor();
    SegmentEditorEventObservation interactorObservation;
    interactorObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    interactorObservation.ObservedObject = interactor;
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseMoveEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::EnterEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeaveEvent, interactorObservation.CallbackCommand, 1.0);
    d->EventObservations << interactorObservation;
    
    // Slice node observations
    vtkMRMLSliceNode* sliceNode = sliceWidget->sliceLogic()->GetSliceNode();
    SegmentEditorEventObservation sliceNodeObservation;
    sliceNodeObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    sliceNodeObservation.ObservedObject = sliceNode;
    sliceNodeObservation.ObservationTags << sliceNode->AddObserver(vtkCommand::ModifiedEvent, sliceNodeObservation.CallbackCommand, 1.0);
    d->EventObservations << sliceNodeObservation;
  }

  // 3D views
  for (int threeDViewId=0; threeDViewId<layoutManager->threeDViewCount(); ++threeDViewId)
  {
    // Create command for 3D view
    qMRMLThreeDWidget* threeDWidget = layoutManager->threeDWidget(threeDViewId);
    qMRMLThreeDView* threeDView = threeDWidget->threeDView();
    vtkNew<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand;
    interactionCallbackCommand->EditorWidget = this;
    interactionCallbackCommand->ViewWidget = threeDWidget;
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(interactionCallbackCommand.GetPointer()) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processEvents );

    // Connect interactor events
    vtkRenderWindowInteractor* interactor = threeDView->interactor();
    SegmentEditorEventObservation interactorObservation;
    interactorObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    interactorObservation.ObservedObject = interactor;
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseMoveEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::EnterEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeaveEvent, interactorObservation.CallbackCommand, 1.0);
    d->EventObservations << interactorObservation;

    // 3D view node observations
    vtkMRMLViewNode* viewNode = threeDWidget->mrmlViewNode();
    SegmentEditorEventObservation viewNodeObservation;
    viewNodeObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    viewNodeObservation.ObservedObject = viewNode;
    viewNodeObservation.ObservationTags << viewNode->AddObserver(vtkCommand::ModifiedEvent, viewNodeObservation.CallbackCommand, 1.0);
    d->EventObservations << viewNodeObservation;
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::removeViewObservations()
{
  Q_D(qMRMLSegmentEditorWidget);
  foreach (SegmentEditorEventObservation eventObservation, d->EventObservations)
  {
    if (eventObservation.ObservedObject)
    {
      foreach (int observationTag, eventObservation.ObservationTags)
      {
        eventObservation.ObservedObject->RemoveObserver(observationTag);
      }
    }
  }
  d->EventObservations.clear();
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::applyChangesToSelectedSegment()
{
  Q_D(qMRMLSegmentEditorWidget);
  
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setActiveEffectByName(QString effectName)
{
  qSlicerSegmentEditorAbstractEffect* effect = this->effectByName(effectName);
  this->setActiveEffect(effect);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateVolume(void* volumeToUpdate)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (volumeToUpdate == d->AlignedMasterVolume)
  {
    d->updateAlignedMasterVolume();
  }
  else if (volumeToUpdate == d->EditedLabelmap)
  {
    d->updateEditedLabelmap();
  }
  else if (volumeToUpdate == d->MaskLabelmap)
  {
    d->updateMaskLabelmap();
  }
  else if (volumeToUpdate == d->SelectedSegmentLabelmap)
  {
    d->updateSelectedSegmentLabelmap();
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Failed to udate volume";
  }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::processEvents(vtkObject* caller,
                                        unsigned long eid,
                                        void* clientData,
                                        void* vtkNotUsed(callData))
{
  // Get and parse client data
  vtkSegmentEditorEventCallbackCommand* callbackCommand = reinterpret_cast<vtkSegmentEditorEventCallbackCommand*>(clientData);
  qMRMLSegmentEditorWidget* self = callbackCommand->EditorWidget.data();
  qMRMLWidget* viewWidget = callbackCommand->ViewWidget.data();
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
    bool abortEvent = activeEffect->processInteractionEvents(callerInteractor, eid, viewWidget);
    if (abortEvent)
    {
      /// Set the AbortFlag on the vtkCommand associated with the event.
      /// It causes other observers of the interactor not to receive the events.
      callbackCommand->SetAbortFlag(1);
    }
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

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeIntensityMaskChecked(bool checked)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }
  d->ParameterSetNode->SetMasterVolumeIntensityMask(checked);
  /*
  this->ThresholdRangeWidget->blockSignals(true);
  this->ThresholdRangeWidget->setVisible(checked);
  this->ThresholdRangeWidget->blockSignals(false);
  */
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeIntensityMaskRangeChanged(double min, double max)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }
  d->ParameterSetNode->SetMasterVolumeIntensityMaskRange(min, max);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMaskModeChanged(int index)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }

  QString selectedSegmentId;
  if (index >= d->MaskModeComboBoxFixedItemsCount)
  {
    // specific index is selected
    d->ParameterSetNode->SetMaskSegmentID(d->MaskModeComboBox->itemData(index).toString().toLatin1());
    d->ParameterSetNode->SetMaskMode(vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment);
  }
  else
  {
    // inside/outside all/visible segments
    d->ParameterSetNode->SetMaskMode(d->MaskModeComboBox->itemData(index).toInt());
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onOverwriteModeChanged(int index)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
  }
  d->ParameterSetNode->SetOverwriteMode(d->OverwriteModeComboBox->itemData(index).toInt());
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::segmentationNodeSelectorVisible() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_Segmentation->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNodeSelectorVisible(bool visible)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MRMLNodeComboBox_Segmentation->setVisible(visible);
  d->label_Segmentation->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::masterVolumeNodeSelectorVisible() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MRMLNodeComboBox_MasterVolume->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMasterVolumeNodeSelectorVisible(bool visible)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MRMLNodeComboBox_MasterVolume->setVisible(visible);
  d->label_MasterVolume->setVisible(visible);
}