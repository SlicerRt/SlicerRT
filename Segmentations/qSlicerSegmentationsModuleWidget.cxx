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

// Segmentations includes
#include "qSlicerSegmentationsModuleWidget.h"
#include "ui_qSlicerSegmentationsModule.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

#include "qMRMLSegmentsTableView.h"
#include "qMRMLSegmentationRepresentationsListView.h"

// SlicerQt includes
#include <qSlicerApplication.h>

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLLabelMapVolumeNode.h"
#include "vtkMRMLModelNode.h"

// VTK includes
#include <vtkSmartPointer.h>

// Qt includes
#include <QDebug>
#include <QMessageBox>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Segmentations
class qSlicerSegmentationsModuleWidgetPrivate: public Ui_qSlicerSegmentationsModule
{
  Q_DECLARE_PUBLIC(qSlicerSegmentationsModuleWidget);
protected:
  qSlicerSegmentationsModuleWidget* const q_ptr;
public:
  qSlicerSegmentationsModuleWidgetPrivate(qSlicerSegmentationsModuleWidget& object);
  ~qSlicerSegmentationsModuleWidgetPrivate();
  vtkSlicerSegmentationsModuleLogic* logic() const;
public:
  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerSegmentationsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentationsModuleWidgetPrivate::qSlicerSegmentationsModuleWidgetPrivate(qSlicerSegmentationsModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentationsModuleWidgetPrivate::~qSlicerSegmentationsModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerSegmentationsModuleLogic*
qSlicerSegmentationsModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerSegmentationsModuleWidget);
  return vtkSlicerSegmentationsModuleLogic::SafeDownCast(q->logic());
} 

//-----------------------------------------------------------------------------
// qSlicerSegmentationsModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSegmentationsModuleWidget::qSlicerSegmentationsModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSegmentationsModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentationsModuleWidget::~qSlicerSegmentationsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentationsModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerSegmentationsModuleWidget);

  d->ModuleWindowInitialized = true;

  this->onSegmentationNodeChanged( d->MRMLNodeComboBox_Segmentation->currentNode() );
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::setup()
{
  this->init();
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentationDisplayNode* qSlicerSegmentationsModuleWidget::segmentationDisplayNode()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationNode* segmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (!segmentationNode)
  {
    return NULL;
  }

  return vtkMRMLSegmentationDisplayNode::SafeDownCast( segmentationNode->GetDisplayNode() );
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  // Update display group from segmentation display node
  this->updateWidgetFromDisplayNode();

  // Update copy/move/import/export buttons from selection
  this->updateCopyMoveButtonStates();

  // Update segment handler button states based on segment selection
  this->onSegmentSelectionChanged(QItemSelection(),QItemSelection());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::updateWidgetFromDisplayNode()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  // Update display property widgets
  d->checkBox_Visible->setChecked( displayNode->GetVisibility() );
  d->SliderWidget_Opacity->setValue( displayNode->GetOpacity() );
  d->checkBox_SliceIntersectionVisible->setChecked( displayNode->GetSliceIntersectionVisibility() );
  d->spinBox_SliceIntersectionThickness->setValue( displayNode->GetSliceIntersectionThickness() );
  d->comboBox_Representation->setCurrentIndex( d->comboBox_Representation->findText(
    displayNode->GetPolyDataDisplayRepresentationName() ) );

  // Set display node to display widgets
  d->DisplayNodeViewComboBox->setMRMLDisplayNode(displayNode);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::updateCopyMoveButtonStates()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  // Disable copy/move buttons then enable later based on selection
  d->toolButton_MoveFromCurrentSegmentation->setEnabled(false);
  d->toolButton_CopyFromCurrentSegmentation->setEnabled(false);
  d->toolButton_CopyToCurrentSegmentation->setEnabled(false);
  d->toolButton_MoveToCurrentSegmentation->setEnabled(false);

  // Set button states that copy/move to current segmentation
  vtkMRMLSegmentationNode* otherSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
    d->SegmentsTableView_Other->segmentationNode() );
  vtkMRMLNode* otherRepresentationNode = d->SegmentsTableView_Other->representationNode();
  if (otherSegmentationNode)
  {
    // All options are possible if other node is segmentation
    d->toolButton_CopyToCurrentSegmentation->setEnabled(true);
    d->toolButton_MoveToCurrentSegmentation->setEnabled(true);
  }
  else if (otherRepresentationNode)
  {
    // Move from other node is disabled if other node is representation
    d->toolButton_CopyToCurrentSegmentation->setEnabled(true);
  }

  // Set button states that copy/move from current segmentation
  vtkMRMLSegmentationNode* currentSegmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (currentSegmentationNode && currentSegmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
  {
    d->toolButton_MoveFromCurrentSegmentation->setEnabled(true);
    d->toolButton_CopyFromCurrentSegmentation->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::populateRepresentationsCheckboxes()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  // Prevent selecting incrementally added representations thus changing MRML properties
  d->comboBox_Representation->blockSignals(true);

  // Populate model representations combobox
  d->comboBox_Representation->clear();
  std::vector<std::string> modelRepresentationNames;
  displayNode->GetPolyDataRepresentationNames(modelRepresentationNames);
  for (std::vector<std::string>::iterator reprIt = modelRepresentationNames.begin();
    reprIt != modelRepresentationNames.end(); ++reprIt)
  {
    d->comboBox_Representation->addItem(reprIt->c_str());
  }

  // Set selection from MRML
  d->comboBox_Representation->setCurrentIndex( d->comboBox_Representation->findText(
    displayNode->GetPolyDataDisplayRepresentationName() ) );

  // Unblock signals
  d->comboBox_Representation->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::init()
{
  Q_D(qSlicerSegmentationsModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Ensure that four representations fit in the table by default
  d->RepresentationsListView->setMinimumHeight(108);

  // Set icons to tool buttons
  d->toolButton_AddLabelmap->setIcon(QIcon(":/Icons/AddLabelmap.png"));
  d->toolButton_AddModel->setIcon(QIcon(":/Icons/Small/SlicerAddModel.png"));

  // Make connections
  connect(d->MRMLNodeComboBox_Segmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(onSegmentationNodeChanged(vtkMRMLNode*)) );
  connect(d->MRMLNodeComboBox_Segmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    d->SegmentsTableView, SLOT(setSegmentationNode(vtkMRMLNode*)) );
  connect(d->MRMLNodeComboBox_Segmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    d->SegmentsTableView_Current, SLOT(setSegmentationNode(vtkMRMLNode*)) );
  connect(d->MRMLNodeComboBox_Segmentation, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    d->RepresentationsListView, SLOT(setSegmentationNode(vtkMRMLNode*)) );

  connect(d->SegmentsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
    this, SLOT(onSegmentSelectionChanged(QItemSelection,QItemSelection)));
  connect(d->pushButton_AddSegment, SIGNAL(clicked()),
    this, SLOT(onAddSegment()) );
  connect(d->pushButton_EditSelected, SIGNAL(clicked()),
    this, SLOT(onEditSelectedSegment()) );
  connect(d->pushButton_DeleteSelected, SIGNAL(clicked()),
    this, SLOT(onDeleteSelectedSegments()) );

  connect(d->MRMLNodeComboBox_OtherSegmentationOrRepresentationNode, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setOtherSegmentationOrRepresentationNode(vtkMRMLNode*)) );

  connect(d->checkBox_Visible, SIGNAL(stateChanged(int)),
    this, SLOT(onVisibilityChanged(int)) );
  connect(d->SliderWidget_Opacity, SIGNAL(valueChanged(double)),
    this, SLOT(onOpacityChanged(double)) );
  connect(d->checkBox_SliceIntersectionVisible, SIGNAL(stateChanged(int)),
    this, SLOT(onSliceIntersectionVisibilityChanged(int)) );
  connect(d->spinBox_SliceIntersectionThickness, SIGNAL(valueChanged(int)),
    this, SLOT(onSliceIntersectionThicknessChanged(int)) );
  connect(d->comboBox_Representation, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onModelRepresentationChanged(int)) );

  connect(d->toolButton_AddLabelmap, SIGNAL(clicked()),
    this, SLOT(onAddLabelmap()) );
  connect(d->toolButton_AddModel, SIGNAL(clicked()),
    this, SLOT(onAddModel()) );

  connect(d->toolButton_MoveFromCurrentSegmentation, SIGNAL(clicked()),
    this, SLOT(onMoveFromCurrentSegmentation()) );
  connect(d->toolButton_CopyFromCurrentSegmentation, SIGNAL(clicked()),
    this, SLOT(onCopyFromCurrentSegmentation()) );
  connect(d->toolButton_CopyToCurrentSegmentation, SIGNAL(clicked()),
    this, SLOT(onCopyToCurrentSegmentation()) );
  connect(d->toolButton_MoveToCurrentSegmentation, SIGNAL(clicked()),
    this, SLOT(onMoveToCurrentSegmentation()) );

  // Show only segment names in copy/view segment list and make it non-editable
  d->SegmentsTableView_Current->setMode(qMRMLSegmentsTableView::SimpleListMode);
  d->SegmentsTableView_Other->setMode(qMRMLSegmentsTableView::SimpleListMode);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentationsModuleWidget::segmentationNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }

  // Disconnect all nodes from functions connected in a QVTK way
  qvtkDisconnect( 0, vtkCommand::ModifiedEvent, this, SLOT( updateWidgetFromMRML() ) );
  qvtkDisconnect( 0, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, SLOT( updateWidgetFromDisplayNode() ) );
  qvtkDisconnect( 0, vtkSegmentation::MasterRepresentationModified, this, SLOT( updateWidgetFromMRML() ) );
  qvtkDisconnect( 0, vtkSegmentation::RepresentationModified, this, SLOT( populateRepresentationsCheckboxes() ) );

  vtkMRMLSegmentationNode* segmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(node);
  if (segmentationNode)
  {
    // Connect node modified events to update widgets in display group function
    qvtkConnect( segmentationNode, vtkCommand::ModifiedEvent, this, SLOT( updateWidgetFromMRML() ) );
    qvtkConnect( segmentationNode, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, SLOT( updateWidgetFromDisplayNode() ) );
    qvtkConnect( segmentationNode, vtkSegmentation::MasterRepresentationModified, this, SLOT( updateWidgetFromMRML() ) );

    // Connect current segmentation node to populate representation checkboxes function
    qvtkConnect( segmentationNode, vtkSegmentation::RepresentationModified, this, SLOT( populateRepresentationsCheckboxes() ) );

    // Populate model representations combobox
    this->populateRepresentationsCheckboxes();

    if (segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
    {

    }
  }

  // Hide the current node in the other segmentation combo box
  QStringList hiddenNodeIDs;
  if (segmentationNode)
  {
    hiddenNodeIDs << QString(segmentationNode->GetID());
  }
  d->MRMLNodeComboBox_OtherSegmentationOrRepresentationNode->sortFilterProxyModel()->setHiddenNodeIDs(hiddenNodeIDs);

  // Update UI from selected segmentation node
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onSegmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qSlicerSegmentationsModuleWidget);

  QStringList selectedSegmentIds = d->SegmentsTableView->selectedSegmentIDs();
  d->pushButton_EditSelected->setEnabled(selectedSegmentIds.count() == 1);
  d->pushButton_DeleteSelected->setEnabled(selectedSegmentIds.count() > 0);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onAddSegment()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationNode* currentSegmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (!currentSegmentationNode)
  {
    return;
  }

  // Create empty segment in current segmentation
  currentSegmentationNode->GetSegmentation()->AddEmptySegment();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onEditSelectedSegment()
{
  //TODO:
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onDeleteSelectedSegments()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationNode* currentSegmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (!currentSegmentationNode)
  {
    return;
  }

  QStringList selectedSegmentIds = d->SegmentsTableView->selectedSegmentIDs();
  foreach (QString segmentId, selectedSegmentIds)
  {
    currentSegmentationNode->GetSegmentation()->RemoveSegment(segmentId.toLatin1().constData());
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::setOtherSegmentationOrRepresentationNode(vtkMRMLNode* node)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerSegmentationsModuleWidget::setOtherSegmentationOrRepresentationNode: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }

  // Decide if segmentation or representation node
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node);
  vtkMRMLLabelMapVolumeNode* labelmapNode = vtkMRMLLabelMapVolumeNode::SafeDownCast(node);
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);

  if (segmentationNode)
  {
    d->SegmentsTableView_Other->setMode(qMRMLSegmentsTableView::SimpleListMode);
    d->SegmentsTableView_Other->setSegmentationNode(node);
  }
  else if (labelmapNode || modelNode)
  {
    d->SegmentsTableView_Other->setMode(qMRMLSegmentsTableView::RepresentationMode);
    d->SegmentsTableView_Other->setRepresentationNode(node);
  }
  else
  {
    d->SegmentsTableView_Other->setSegmentationNode(NULL);
    d->SegmentsTableView_Other->setRepresentationNode(NULL);
  }

  // Update widgets based on selection
  this->updateCopyMoveButtonStates();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onVisibilityChanged(int visible)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  displayNode->SetVisibility(visible > 0 ? 1 : 0);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onOpacityChanged(double opacity)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  displayNode->SetOpacity(opacity);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onSliceIntersectionVisibilityChanged(int visible)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  displayNode->SetSliceIntersectionVisibility(visible > 0 ? 1 : 0);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onSliceIntersectionThicknessChanged(int thickness)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  displayNode->SetSliceIntersectionThickness(thickness);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onModelRepresentationChanged(int index)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationDisplayNode* displayNode = this->segmentationDisplayNode();
  if (!displayNode)
  {
    return;
  }

  // Get representation name from index
  QString representationName = d->comboBox_Representation->itemText(index);

  displayNode->SetPolyDataDisplayRepresentationName(representationName.toLatin1().constData());
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onAddLabelmap()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLScene* scene = this->mrmlScene();
  if (!scene)
  {
    return;
  }

  vtkSmartPointer<vtkMRMLLabelMapVolumeNode> labelmapNode = vtkSmartPointer<vtkMRMLLabelMapVolumeNode>::New();
  scene->AddNode(labelmapNode);

  // Select new labelmap in the other representation combobox
  d->MRMLNodeComboBox_OtherSegmentationOrRepresentationNode->setCurrentNode(labelmapNode);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onAddModel()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLScene* scene = this->mrmlScene();
  if (!scene)
  {
    return;
  }

  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  scene->AddNode(modelNode);

  // Select new model in the other representation table
  d->MRMLNodeComboBox_OtherSegmentationOrRepresentationNode->setCurrentNode(modelNode);
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentationsModuleWidget::copySegmentBetweenSegmentations(
  vtkSegmentation* fromSegmentation, vtkSegmentation* toSegmentation,
  QString segmentId, bool removeFromSource/*=false*/ )
{
  if (!fromSegmentation || !toSegmentation || segmentId.isEmpty())
  {
    return false;
  }

  std::string segmentIdStd(segmentId.toLatin1().constData());

  // Get segment
  vtkSegment* segment = fromSegmentation->GetSegment(segmentIdStd);
  if (!segment)
  {
    qCritical() << "qSlicerSegmentationsModuleWidget::copySegmentBetweenSegmentations: Failed to get segment!";
    return false;
  }

  // If target segmentation is newly created thus have no master representation, make it match the source
  if (!toSegmentation->GetMasterRepresentationName())
  {
    toSegmentation->SetMasterRepresentationName(fromSegmentation->GetMasterRepresentationName());
  }

  // Check whether target is suitable to accept the segment.
  if (!toSegmentation->CanAcceptSegment(segment))
  {
    qCritical() << "qSlicerSegmentationsModuleWidget::copySegmentBetweenSegmentations: Segmentation cannot accept segment " << segment->GetName() << "!";

    // Pop up error message to the user explaining the problem
    vtkMRMLSegmentationNode* fromNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentation(this->mrmlScene(), fromSegmentation);
    vtkMRMLSegmentationNode* toNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentation(this->mrmlScene(), toSegmentation);
    if (!fromNode || !toNode) // Sanity check, should never happen
    {
      qCritical() << "qSlicerSegmentationsModuleWidget::copySegmentBetweenSegmentations: Unable to get parent nodes for segmentaiton objects!";
      return false;
    }

    QString message = QString("Cannot convert source master representation '%1' into target master '%2', thus unable to copy segment '%3' from segmentation '%4' to '%5'.\n\nWould you like to change the master representation of '%5' to '%1'?\n\nNote: This may result in unwanted data loss in %5.")
      .arg(fromSegmentation->GetMasterRepresentationName()).arg(toSegmentation->GetMasterRepresentationName()).arg(segmentId).arg(fromNode->GetName()).arg(toNode->GetName());
    QMessageBox::StandardButton answer =
      QMessageBox::question(NULL, tr("Failed to copy segment"), message,
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
      // Convert target segmentation to master representation of source segmentation
      bool successfulConversion = toSegmentation->CreateRepresentation(fromSegmentation->GetMasterRepresentationName());
      if (!successfulConversion)
      {
        QString message = QString("Failed to convert %1 to %2!").arg(toNode->GetName()).arg(fromSegmentation->GetMasterRepresentationName());
        QMessageBox::warning(NULL, tr("Conversion failed"), message);
        return false;
      }

      // Change master representation of target to that of source
      toSegmentation->SetMasterRepresentationName(fromSegmentation->GetMasterRepresentationName());

      // Retry copy of segment
      return this->copySegmentBetweenSegmentations(fromSegmentation, toSegmentation, segmentId, removeFromSource);
    }

    return false;
  }

  // Perform the actual copy/move operation
  return toSegmentation->CopySegmentFromSegmentation(fromSegmentation, segmentIdStd, removeFromSource);
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentationsModuleWidget::copyFromCurrentSegmentation(bool removeFromSource/*=false*/)
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationNode* currentSegmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (!currentSegmentationNode)
  {
    return false;
  }

  vtkMRMLSegmentationNode* otherSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
    d->SegmentsTableView_Other->segmentationNode() );
  vtkMRMLNode* otherRepresentationNode = d->SegmentsTableView_Other->representationNode();

  // Get selected segment IDs
  QStringList selectedSegmentIds = d->SegmentsTableView_Current->selectedSegmentIDs();
  if (selectedSegmentIds.empty())
  {
    return false;
  }

  // If other node is segmentation
  if (otherSegmentationNode)
  {
    // Copy/move selected segments into other segmentation
    foreach (QString segmentId, selectedSegmentIds)
    {
      this->copySegmentBetweenSegmentations(currentSegmentationNode->GetSegmentation(),
        otherSegmentationNode->GetSegmentation(), segmentId, removeFromSource);
    }
  }
  // If other node is representation
  else if (otherRepresentationNode)
  {
    if (selectedSegmentIds.count() == 1)
    {
      // Export segment into representation node
      QString firstSegmentId = selectedSegmentIds.first();
      vtkSegment* segment = currentSegmentationNode->GetSegmentation()->GetSegment(firstSegmentId.toLatin1().constData());
      if (!vtkSlicerSegmentationsModuleLogic::ExportSegmentToRepresentationNode(segment, otherRepresentationNode))
      {
        QString message = QString("Failed to export segment %1 from segmentation %2 to representation node %3!\n\nMost probably the segment cannot be converted into representation corresponding to the selected representation node.").
          arg(firstSegmentId).arg(currentSegmentationNode->GetName()).arg(otherRepresentationNode->GetName());
        qCritical() << "qSlicerSegmentationsModuleWidget::copyFromCurrentSegmentation: " << message;
        QMessageBox::warning(NULL, tr("Failed to export segment"), message);
        return false;
      }
      else if (removeFromSource)
      {
        // Remove segment from current segmentation if export was successful
        currentSegmentationNode->GetSegmentation()->RemoveSegment(firstSegmentId.toLatin1().constData());
      }
    }
    // Multiple selection is only allowed for exporting to labelmap volume
    else
    {
      if (!otherRepresentationNode->IsA("vtkMRMLLabelMapVolumeNode"))
      {
        QMessageBox::warning(NULL, tr("Failed to export multiple segments"), tr("More than one segment can only be exported into a labelmap volume node"));
        return false;
      }

      // Export selected segments into a multi-label labelmap volume
      std::vector<std::string> segmentIDs;
      foreach (QString segmentId, selectedSegmentIds)
      {
        segmentIDs.push_back(segmentId.toLatin1().constData());
      }
      vtkMRMLLabelMapVolumeNode* otherLabelmapNode = vtkMRMLLabelMapVolumeNode::SafeDownCast(otherRepresentationNode);
      if (!vtkSlicerSegmentationsModuleLogic::ExportSegmentsToLabelmapNode(currentSegmentationNode, segmentIDs, otherLabelmapNode))
      {
        QString message = QString("Failed to export segments from segmentation %1 to representation node %2!\n\nMost probably the segment cannot be converted into representation corresponding to the selected representation node.").
          arg(currentSegmentationNode->GetName()).arg(otherLabelmapNode->GetName());
        qCritical() << "qSlicerSegmentationsModuleWidget::copyFromCurrentSegmentation: " << message;
        QMessageBox::warning(NULL, tr("Failed to export segment"), message);
        return false;
      }
      else if (removeFromSource)
      {
        // Remove segments from current segmentation if export was successful
        foreach (QString segmentId, selectedSegmentIds)
        {
          currentSegmentationNode->GetSegmentation()->RemoveSegment(segmentId.toLatin1().constData());
        }
      }
    }

    // Refresh other representation table
    d->SegmentsTableView_Other->setRepresentationNode(otherRepresentationNode);
  }

  return true;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onMoveFromCurrentSegmentation()
{
  this->copyFromCurrentSegmentation(true);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onCopyFromCurrentSegmentation()
{
  this->copyFromCurrentSegmentation();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onCopyToCurrentSegmentation()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationNode* currentSegmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (!currentSegmentationNode)
  {
    return;
  }

  vtkMRMLSegmentationNode* otherSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
    d->SegmentsTableView_Other->segmentationNode() );
  vtkMRMLNode* otherRepresentationNode = d->SegmentsTableView_Other->representationNode();

  // If other node is segmentation
  if (otherSegmentationNode)
  {
    // Copy other segments into current segmentation
    foreach (QString segmentId, d->SegmentsTableView_Other->selectedSegmentIDs())
    {
      this->copySegmentBetweenSegmentations(otherSegmentationNode->GetSegmentation(),
        currentSegmentationNode->GetSegmentation(), segmentId, false);
    }
  }
  // If other node is representation
  else if (otherRepresentationNode)
  {
    vtkMRMLLabelMapVolumeNode* labelmapNode = vtkMRMLLabelMapVolumeNode::SafeDownCast(otherRepresentationNode);
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(otherRepresentationNode);

    if (labelmapNode)
    {
      if (this->updateMasterRepresentationInSegmentation(currentSegmentationNode->GetSegmentation(), vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
      {
        if (!vtkSlicerSegmentationsModuleLogic::ImportLabelmapToSegmentationNode(labelmapNode, currentSegmentationNode))
        {
          QString message = QString("Failed to copy labels from labelmap volume node %1!").arg(labelmapNode->GetName());
          qCritical() << "qSlicerSegmentationsModuleWidget::onCopyToCurrentSegmentation: " << message;
          QMessageBox::warning(NULL, tr("Failed to import from labelmap volume"), message);
        }
      }
    }
    else if (modelNode)
    {
      vtkSmartPointer<vtkSegment> segment = vtkSmartPointer<vtkSegment>::Take(
        vtkSlicerSegmentationsModuleLogic::CreateSegmentFromModelNode(modelNode) );
      if (!segment.GetPointer())
      {
        QString message = QString("Failed to copy from model node %1!").arg(modelNode->GetName());
        qCritical() << "qSlicerSegmentationsModuleWidget::onCopyToCurrentSegmentation: " << message;
        QMessageBox::warning(NULL, tr("Failed to create segment from model"), message);
      }
      else
      {
        if (this->updateMasterRepresentationInSegmentation(currentSegmentationNode->GetSegmentation(), vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName()))
        {
          // Add segment to current segmentation
          currentSegmentationNode->GetSegmentation()->AddSegment(segment);
        }
      }
    }
    else
    {
      qCritical() << "qSlicerSegmentationsModuleWidget::onCopyToCurrentSegmentation: Reprsentation node needs to be either model or labelmap, but " << otherRepresentationNode->GetName() << " is " << otherRepresentationNode->GetNodeTagName();
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModuleWidget::onMoveToCurrentSegmentation()
{
  Q_D(qSlicerSegmentationsModuleWidget);

  vtkMRMLSegmentationNode* currentSegmentationNode =  vtkMRMLSegmentationNode::SafeDownCast(
    d->MRMLNodeComboBox_Segmentation->currentNode() );
  if (!currentSegmentationNode)
  {
    return;
  }

  vtkMRMLSegmentationNode* otherSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
    d->SegmentsTableView_Other->segmentationNode() );
  // If other node is segmentation
  if (otherSegmentationNode)
  {
    // Move other segment into current segmentation
    foreach (QString segmentId, d->SegmentsTableView_Other->selectedSegmentIDs())
    {
      this->copySegmentBetweenSegmentations(otherSegmentationNode->GetSegmentation(),
        currentSegmentationNode->GetSegmentation(), segmentId, true);
    }
  }
  // Only segment in segmentation can be moved into current segmentation
  else
  {
    qCritical() << "qSlicerSegmentationsModuleWidget::onMoveToCurrentSegmentation: Invalid operation!";
  }
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentationsModuleWidget::updateMasterRepresentationInSegmentation(vtkSegmentation* segmentation, QString representation)
{
  if (!segmentation || representation.isEmpty())
  {
    return false;
  }
  std::string newMasterRepresentation(representation.toLatin1().constData());

  // Set master representation to the added one if segmentation is empty or master representation is undefined
  if (segmentation->GetNumberOfSegments() == 0 || segmentation->GetMasterRepresentationName() == NULL)
  {
    segmentation->SetMasterRepresentationName(newMasterRepresentation.c_str());
    return true;
  }

  // No action is necessary if segmentation is non-empty and the master representation matches the contained one in segment
  if (!strcmp(segmentation->GetMasterRepresentationName(), newMasterRepresentation.c_str()))
  {
    return true;
  }

  // Get segmentation node for segmentation
  vtkMRMLScene* scene = this->mrmlScene();
  if (!scene)
  {
    return false;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentation(scene, segmentation);
  if (!segmentationNode)
  {
    return false;
  }

  // Ask the user if master was different but not empty
  QString message = QString("Segment is to be added in segmentation '%1' that contains a representation (%2) different than the master representation in the segmentation (%3). "
    "The master representation need to be changed so that the segment can be added. This might result in unwanted data loss.\n\n"
    "Do you wish to change the master representation to %2?").arg(segmentationNode->GetName()).arg(newMasterRepresentation.c_str()).arg(segmentation->GetMasterRepresentationName());
  QMessageBox::StandardButton answer =
    QMessageBox::question(NULL, tr("Master representation is needed to be changed to add segment"), message,
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (answer == QMessageBox::No)
  {
    return false;
  }

  // Make sure the new master representation exists before setting it
  if (!segmentation->CreateRepresentation(newMasterRepresentation.c_str()))
  {
    std::vector<std::string> containedRepresentationNamesInSegmentation;
    segmentation->GetContainedRepresentationNames(containedRepresentationNamesInSegmentation);
    if (containedRepresentationNamesInSegmentation.empty())
    {
      qCritical() << "qSlicerSegmentationsModuleWidget::updateMasterRepresentationInSegmentation: Master representation cannot be created in segmentation as it does not contain any representations!";
      return false;
    }

    std::string firstContainedRepresentation = (*containedRepresentationNamesInSegmentation.begin());
    qCritical() << "qSlicerSegmentationsModuleWidget::updateMasterRepresentationInSegmentation: Master representation cannot be created in segmentation! Setting master to the first representation found: " << firstContainedRepresentation.c_str();
    segmentation->SetMasterRepresentationName(newMasterRepresentation.c_str());
    return false;
  }

  // Set master representation to the added one if user agreed
  segmentation->SetMasterRepresentationName(newMasterRepresentation.c_str());
  return true;
}
