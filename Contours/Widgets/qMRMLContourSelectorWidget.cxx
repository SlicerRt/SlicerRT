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

// qMRML includes
#include "qMRMLContourSelectorWidget.h"
#include "ui_qMRMLContourSelectorWidget.h"
#include "qMRMLNodeComboBox.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkSlicerContoursModuleLogic.h"

// Qt includes
#include <QDebug>

//------------------------------------------------------------------------------
class qMRMLContourSelectorWidgetPrivate: public Ui_qMRMLContourSelectorWidget
{
  Q_DECLARE_PUBLIC(qMRMLContourSelectorWidget);
protected:
  qMRMLContourSelectorWidget* const q_ptr;
public:
  qMRMLContourSelectorWidgetPrivate(qMRMLContourSelectorWidget& object);
  void init();

  /// Representation type required from the selected contour
  vtkMRMLContourNode::ContourRepresentationType RequiredRepresentation;

  /// Flag determining whether contour hierarchies are accepted or only individual contours
  bool AcceptContourHierarchies;

  /// List of currently selected contour nodes. Contains the selected
  /// contour node or the children of the selected contour hierarchy node
  std::vector<vtkMRMLContourNode*> SelectedContourNodes;

  /// Forced reference volume node by ID.
  /// This means that there will be no search for a forced (DICOM-based) referenced volume, this will be used instead
  QString ForcedReferenceVolumeNodeID;

  /// List of all contour selector widgets that are grouped under this widget instance as slave widgets.
  /// The instances in this list should have their \sa MasterContourSelectorWidget member set.
  /// Having items in this list makes this instance the master
  QList<qMRMLContourSelectorWidget*> SlaveContourSelectorWidgets;

  /// Pointer to the master contour selector widget if this instance is grouped under another instance
  /// NULL if not grouped
  qMRMLContourSelectorWidget* MasterContourSelectorWidget;

  /// Flag determining if the current selection is valid. Takes the occasional slave instances into account
  /// Updated automatically in the /sa updateWidgetState method
  bool IsSelectionValid;
};

//------------------------------------------------------------------------------
qMRMLContourSelectorWidgetPrivate::qMRMLContourSelectorWidgetPrivate(qMRMLContourSelectorWidget& object)
  : q_ptr(&object)
{
  this->RequiredRepresentation = vtkMRMLContourNode::None;
  this->AcceptContourHierarchies = false;
  this->ForcedReferenceVolumeNodeID.clear();
  this->SlaveContourSelectorWidgets.clear();
  this->MasterContourSelectorWidget = NULL;
  this->IsSelectionValid = false;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidgetPrivate::init()
{
  Q_Q(qMRMLContourSelectorWidget);
  this->setupUi(q);

  q->setAcceptContourHierarchies(this->AcceptContourHierarchies);

  QObject::connect( this->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(contourNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SIGNAL(currentNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SIGNAL(currentReferenceVolumeNodeChanged(vtkMRMLNode*)) );

  // Disable contour combobox until required representation type is not explicitly set
  this->MRMLNodeComboBox_Contour->setEnabled(false);

  // Hide reference related widgets and all message labels by default
  this->frame_ReferenceVolumeSelection->setVisible(false);
  this->label_ValidRequiredRepresentation->setVisible(false);
  this->label_ClosedSurfaceModelConversion->setVisible(false);
  this->label_NoContoursInSelection->setVisible(false);
  this->label_ReConversion->setVisible(false);
  this->label_NoSource->setVisible(false);
}

//------------------------------------------------------------------------------
qMRMLContourSelectorWidget::qMRMLContourSelectorWidget(QWidget *_parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLContourSelectorWidgetPrivate(*this))
{
  Q_D(qMRMLContourSelectorWidget);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLContourSelectorWidget::~qMRMLContourSelectorWidget()
{
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::updateWidgetState()
{
  Q_D(qMRMLContourSelectorWidget);

  bool isSelectionValid = false;

  // If this is the only widget (no grouping), then validate it as master
  if (d->SlaveContourSelectorWidgets.empty() && !d->MasterContourSelectorWidget)
  {
    // Update this widget
    isSelectionValid = this->validateSelection(d->SelectedContourNodes, false);
  }
  // If widgets are grouped and this is the master, then validate them separately as slaves (to show critical messages locally)
  // and then validate the unified selection as master (to show selection widgets and other messages)
  else if (!d->MasterContourSelectorWidget)
  {
    // Validate this instance's (the master's) selection and display messages that show invalid selection
    isSelectionValid = this->validateSelection(d->SelectedContourNodes, true);

    // Gather all selected contours from the widget group
    std::vector<vtkMRMLContourNode*> selectedContoursInGroup;
    for (std::vector<vtkMRMLContourNode*>::iterator contourIt = d->SelectedContourNodes.begin(); contourIt != d->SelectedContourNodes.end(); ++contourIt)
    {
      selectedContoursInGroup.push_back(*contourIt);
    }

    for (int slaveIndex = 0; slaveIndex < d->SlaveContourSelectorWidgets.size(); ++slaveIndex)
    {
      qMRMLContourSelectorWidget* currentSlave = d->SlaveContourSelectorWidgets.at(slaveIndex);
      std::vector<vtkMRMLContourNode*> selectedContoursInCurrentSlave = currentSlave->selectedContourNodes();

      // Add slave's selection to the unified selected contour list
      for (std::vector<vtkMRMLContourNode*>::iterator contourIt = selectedContoursInCurrentSlave.begin(); contourIt != selectedContoursInCurrentSlave.end(); ++contourIt)
      {
        selectedContoursInGroup.push_back(*contourIt);
      }

      // Validate slave's selection and display messages locally that show invalid selection
      isSelectionValid &= currentSlave->validateSelection(selectedContoursInCurrentSlave, true);
    }

    // If selection is valid in each widget in the group then display controls and other messages for only this instance (the master)
    if (isSelectionValid)
    {
      this->validateSelection(selectedContoursInGroup, false);
    }
  }

  // If validity changed, then set it and emit a signal about it
  if (isSelectionValid != d->IsSelectionValid)
  {
    d->IsSelectionValid = isSelectionValid;
    emit selectionValidityChanged();
  }
}

//------------------------------------------------------------------------------
bool qMRMLContourSelectorWidget::validateSelection(std::vector<vtkMRMLContourNode*>& contours, bool slave)
{
  Q_D(qMRMLContourSelectorWidget);

  d->frame_ReferenceVolumeSelection->setVisible(false);
  d->label_ValidRequiredRepresentation->setVisible(false);
  d->label_ClosedSurfaceModelConversion->setVisible(false);
  d->label_NoContoursInSelection->setVisible(false);
  d->label_ReConversion->setVisible(false);
  d->label_NoSource->setVisible(false);

  bool validSelection = true;

  // If required representation is indexed labelmap or closed surface model, then set reference
  // Forced reference will be set if available (and the contour has not been created from labelmap), otherwise DICOM reference will be used if available
  // When setting a reference, the indexed labelmap is invalidated, and this fact influences the validation process below.
  if ( (d->RequiredRepresentation == vtkMRMLContourNode::IndexedLabelmap || d->RequiredRepresentation == vtkMRMLContourNode::ClosedSurfaceModel) )
  {
    this->setReferenceInSelection(contours);
  }

  // If there are no contours in selection
  if (contours.size() == 0)
  {
    // Only display no contours warning if there is a (contour hierarchy) node selected
    if (d->MRMLNodeComboBox_Contour->currentNode())
    {
      d->label_NoContoursInSelection->setVisible(true);
    }
    validSelection = false;
  }
  // If selected contours all contain the required representation
  else if (vtkSlicerContoursModuleLogic::ContoursContainRepresentation(contours, d->RequiredRepresentation))
  {
    if (!slave) // Only display this message for the master
    {
      // There is a valid required representation in every selected contour
      d->label_ValidRequiredRepresentation->setVisible(true);
    }
  }
  else if (d->RequiredRepresentation == vtkMRMLContourNode::RibbonModel)
  {
    // There is no conversion implemented to ribbon model yet
    d->label_NoSource->setVisible(true);
    validSelection = false;
  }
  // If selection does not contain the required representation (which is indexed labelmap or closed surface model)
  //   and the selected contours also do not contain indexed labelmap representations
  else if (!vtkSlicerContoursModuleLogic::ContoursContainRepresentation(contours, vtkMRMLContourNode::IndexedLabelmap))
  {
    // If there are no possible sources for conversion to indexed labelmap
    // Note: The case where some contours contain only ribbon, while others contain only closed surface is not handled.
    //   It is a quite exotic case, and the user can solve this case using the Contours module
    if ( !vtkSlicerContoursModuleLogic::ContoursContainRepresentation(contours, vtkMRMLContourNode::RibbonModel)
      && !vtkSlicerContoursModuleLogic::ContoursContainRepresentation(contours, vtkMRMLContourNode::ClosedSurfaceModel) )
    {
      d->label_NoSource->setVisible(true);
      validSelection = false;
    }
    else if (!slave) // Only display this message for the master
    {
      // Show reference selector for the master if every condition is given for conversion to indexed labelmap
      d->frame_ReferenceVolumeSelection->setVisible(true);

      // If there is at least one contour in the selection that contains indexed labelmap already, and then show re-conversion message
      // TODO: Decide if we need such a warning. It is quite hard to determine whether an actual re-conversion will take place,
      //   and it is not a crucial information (as there are model sources, the labelmap can be re-converted using any reference any time)
      //if (vtkSlicerContoursModuleLogic::ContoursContainRepresentation(contours, vtkMRMLContourNode::IndexedLabelmap, false))
      //{
      //  d->label_ReConversion->setVisible(true);
      //}
      // Show closed surface conversion message if required representation is closed surface
      if (d->RequiredRepresentation == vtkMRMLContourNode::ClosedSurfaceModel)
      {
        d->label_ClosedSurfaceModelConversion->setVisible(true);
      }
    }
  }
  // If selection contains indexed labelmap and the required representation is closed surface model, but it's not present in selection
  else if (d->RequiredRepresentation == vtkMRMLContourNode::ClosedSurfaceModel)
  {
    if (!slave)
    {
      d->label_ClosedSurfaceModelConversion->setVisible(true);
    }
  }
  else
  {
    // This control branch should never be hit
    qCritical() << "qMRMLContourSelectorWidget::validateSelection: Unexpected state! This is probably a bug, please save the scene and report this";
    return false;
  }

  return validSelection;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setReferenceInSelection(std::vector<vtkMRMLContourNode*>& contours)
{
  Q_D(qMRMLContourSelectorWidget);

  if (contours.empty())
  {
    return;
  }

  // If there is no forced reference set
  // Note: The forced reference of this instance is used, because this is called only from the master (moreover the forced reference is the same throughout the group)
  if (d->ForcedReferenceVolumeNodeID.isEmpty())
  {
    // Look for referenced volume for contours in DICOM and set it as forced if found
    vtkMRMLScalarVolumeNode* referencedVolume = vtkSlicerContoursModuleLogic::GetReferencedVolumeByDicomForContours(contours);
    if (referencedVolume)
    {
      // Set referenced volume and turn off oversampling in selected contours
      d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
      d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(referencedVolume->GetID());
      d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
      emit currentReferenceVolumeNodeChanged(referencedVolume);

      d->label_OversamplingFactorValue->setText("1");

      for (std::vector<vtkMRMLContourNode*>::iterator contourIt = contours.begin(); contourIt != contours.end(); ++contourIt)
      {
        // This call invalidates (deletes) the occasionally existing indexed labelmap representation if the new reference differs from the original one
        (*contourIt)->SetAndObserveRasterizationReferenceVolumeNodeId(referencedVolume->GetID());
        (*contourIt)->SetRasterizationOversamplingFactor(1.0);
      }
    }
  }
  // If there is a forced reference set
  else
  {
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(d->ForcedReferenceVolumeNodeID);
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
    vtkMRMLScalarVolumeNode* forcedReferenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->mrmlScene()->GetNodeByID(d->ForcedReferenceVolumeNodeID.toLatin1().constData()));
    emit currentReferenceVolumeNodeChanged(forcedReferenceVolumeNode);

    d->label_OversamplingFactorValue->setText("2");

    for (std::vector<vtkMRMLContourNode*>::iterator contourIt = contours.begin(); contourIt != contours.end(); ++contourIt)
    {
      if (!(*contourIt)->HasBeenCreatedFromIndexedLabelmap())
      {
        // This call invalidates (deletes) the occasionally existing indexed labelmap representation if the new reference differs from the original one
        (*contourIt)->SetAndObserveRasterizationReferenceVolumeNodeId(d->ForcedReferenceVolumeNodeID.toLatin1().constData());
        (*contourIt)->SetRasterizationOversamplingFactor(2.0);
      }
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setRequiredRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType)
{
  Q_D(qMRMLContourSelectorWidget);
  d->RequiredRepresentation = representationType;

  if (d->RequiredRepresentation != vtkMRMLContourNode::None)
  {
    // Enable contour combobox now that required representation type is explicitly set
    d->MRMLNodeComboBox_Contour->setEnabled(true);
  }

  if (!d->MasterContourSelectorWidget)
  {
    // Update widget if this is the master of a group or if this is the only widget
    this->updateWidgetState();
  }
  else
  {
    // Call master's update function if this is a slave
    d->MasterContourSelectorWidget->updateWidgetState();
  }
}

//------------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType qMRMLContourSelectorWidget::requiredRepresentation()
{
  Q_D(qMRMLContourSelectorWidget);
  return d->RequiredRepresentation;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setAcceptContourHierarchies(bool acceptContourHierarchies)
{
  Q_D(qMRMLContourSelectorWidget);
  d->AcceptContourHierarchies = acceptContourHierarchies;

  QStringList contourNodeTypes;
  contourNodeTypes << "vtkMRMLContourNode";
  if (d->AcceptContourHierarchies)
  {
    contourNodeTypes << "vtkMRMLDisplayableHierarchyNode";
  }
  d->MRMLNodeComboBox_Contour->setNodeTypes(contourNodeTypes);

  if (d->AcceptContourHierarchies)
  {
    // Show only displayable hierarchies that are contour hierarchies
    d->MRMLNodeComboBox_Contour->addAttribute( QString("vtkMRMLDisplayableHierarchyNode"),
      QString(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );
  }

  // Propagate setting to slaves if this is a master instance
  if (!d->SlaveContourSelectorWidgets.empty())
  {
    // Set the same forced reference in all slaves
    for (int slaveIndex = 0; slaveIndex < d->SlaveContourSelectorWidgets.size(); ++slaveIndex)
    {
      d->SlaveContourSelectorWidgets.at(slaveIndex)->setAcceptContourHierarchies(acceptContourHierarchies);
    }
  }
}

//------------------------------------------------------------------------------
bool qMRMLContourSelectorWidget::acceptContourHierarchies()
{
  Q_D(qMRMLContourSelectorWidget);
  return d->AcceptContourHierarchies;
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLContourSelectorWidget::currentNode()
{
  Q_D(qMRMLContourSelectorWidget);
  return d->MRMLNodeComboBox_Contour->currentNode();
}

//------------------------------------------------------------------------------
QString qMRMLContourSelectorWidget::currentNodeID()
{
  Q_D(qMRMLContourSelectorWidget);
  return d->MRMLNodeComboBox_Contour->currentNodeID();
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setCurrentNodeID(const QString& nodeID)
{
  Q_D(qMRMLContourSelectorWidget);
  d->MRMLNodeComboBox_Contour->setCurrentNodeID(nodeID);
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLContourSelectorWidget);

  // Get contour nodes from selection
  vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(node, d->SelectedContourNodes);

  if (!d->MasterContourSelectorWidget)
  {
    // Update widget if this is the master of a group or if this is the only widget
    this->updateWidgetState();
  }
  else
  {
    // Call master's update function if this is a slave
    d->MasterContourSelectorWidget->updateWidgetState();
  }
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLContourSelectorWidget);

  if (!node)
  {
    return;
  }
  if (!node->IsA("vtkMRMLScalarVolumeNode"))
  {
    qCritical() << "qMRMLContourSelectorWidget::referenceVolumeNodeChanged: Input node is not a scalar volume!";
    return;
  }

  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = d->SelectedContourNodes.begin(); contourIt != d->SelectedContourNodes.end(); ++contourIt)
  {
    (*contourIt)->SetAndObserveRasterizationReferenceVolumeNodeId(node->GetID());
    (*contourIt)->SetRasterizationOversamplingFactor(2.0);
  }

  d->label_OversamplingFactorValue->setText("2");
}

//------------------------------------------------------------------------------
vtkMRMLContourNode* qMRMLContourSelectorWidget::selectedContourNode()
{
  Q_D(qMRMLContourSelectorWidget);

  if (d->AcceptContourHierarchies)
  {
    qWarning() << "qMRMLContourSelectorWidget::selectedContourNode: Cannot request one selected contour node if hierarchies are accepted (see setAcceptContourHierarchies method)";
    return NULL;
  }

  if (d->SelectedContourNodes.size() != 1)
  {
    qCritical() << "qMRMLContourSelectorWidget::selectedContourNode: Invalid number of selected contour nodes: " << d->SelectedContourNodes.size();
    return NULL;
  }

  return *(d->SelectedContourNodes.begin());
}

//------------------------------------------------------------------------------
std::vector<vtkMRMLContourNode*> qMRMLContourSelectorWidget::selectedContourNodes()
{
  Q_D(qMRMLContourSelectorWidget);

  return d->SelectedContourNodes;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setForcedReferenceVolumeNodeID(const QString& nodeID)
{
  Q_D(qMRMLContourSelectorWidget);

  d->ForcedReferenceVolumeNodeID = nodeID;

  // If this is a master instance
  if (!d->SlaveContourSelectorWidgets.empty())
  {
    // Set the same forced reference in all slaves
    for (int slaveIndex = 0; slaveIndex < d->SlaveContourSelectorWidgets.size(); ++slaveIndex)
    {
      d->SlaveContourSelectorWidgets.at(slaveIndex)->setForcedReferenceVolumeNodeID(nodeID);
    }

    this->updateWidgetState();
  }
}

//------------------------------------------------------------------------------
QString qMRMLContourSelectorWidget::forcedReferenceVolumeNodeID()
{
  Q_D(qMRMLContourSelectorWidget);

  return d->ForcedReferenceVolumeNodeID;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setCurrentReferenceVolumeNodeID(const QString& nodeID)
{
  Q_D(qMRMLContourSelectorWidget);

  if (!d->frame_ReferenceVolumeSelection->isVisible())
  {
    qWarning() << "qMRMLContourSelectorWidget::setCurrentReferenceVolumeNodeID: Unable to set currently selected reference volume because the reference selector is not active";
    return;
  }

  // If this is the master of a group or if this is the only widget
  if (!d->MasterContourSelectorWidget)
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(nodeID);
  }
  else
  {
    // Set the reference volume to the master widget
    d->MasterContourSelectorWidget->setCurrentReferenceVolumeNodeID(nodeID);
  }
}

//------------------------------------------------------------------------------
QString qMRMLContourSelectorWidget::currentReferenceVolumeNodeID()
{
  Q_D(qMRMLContourSelectorWidget);

  if (!d->frame_ReferenceVolumeSelection->isVisible())
  {
    return QString();
  }

  // If this is the master of a group or if this is the only widget
  if (!d->MasterContourSelectorWidget)
  {
    return d->MRMLNodeComboBox_ReferenceVolume->currentNodeID();
  }
  else
  {
    // Return the selected reference volume of the master widget
    return d->MasterContourSelectorWidget->currentReferenceVolumeNodeID();
  }
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::addSlaveContourSelectorWidget(qMRMLContourSelectorWidget* newSlaveInstance)
{
  Q_D(qMRMLContourSelectorWidget);

  d->SlaveContourSelectorWidgets.push_back(newSlaveInstance);

  newSlaveInstance->setMasterContourSelectorWidget(this);
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setMasterContourSelectorWidget(qMRMLContourSelectorWidget* masterInstance)
{
  Q_D(qMRMLContourSelectorWidget);

  d->MasterContourSelectorWidget = masterInstance;

  if (masterInstance)
  {
    this->setAcceptContourHierarchies(masterInstance->acceptContourHierarchies());
    d->RequiredRepresentation = masterInstance->requiredRepresentation();
    d->ForcedReferenceVolumeNodeID = masterInstance->forcedReferenceVolumeNodeID();
  }
}

//------------------------------------------------------------------------------
bool qMRMLContourSelectorWidget::isSelectionValid()
{
  Q_D(qMRMLContourSelectorWidget);

  return d->IsSelectionValid;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLContourSelectorWidget);

  if (newScene == this->mrmlScene())
  {
    return;
  }

  Superclass::setMRMLScene(newScene);

  d->ForcedReferenceVolumeNodeID.clear();
  d->IsSelectionValid = false;

  d->MRMLNodeComboBox_Contour->setCurrentNodeID("");
  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID("");
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::ungroup()
{
  Q_D(qMRMLContourSelectorWidget);

  // If this is the master of a group or if this is the only widget
  if (!d->MasterContourSelectorWidget)
  {
    // Set the same forced reference in all slaves
    for (int slaveIndex = 0; slaveIndex < d->SlaveContourSelectorWidgets.size(); ++slaveIndex)
    {
      d->SlaveContourSelectorWidgets.at(slaveIndex)->setMasterContourSelectorWidget(NULL);
    }

    d->SlaveContourSelectorWidgets.clear();
  }
  else
  {
    d->MasterContourSelectorWidget->ungroup();
  }
}
