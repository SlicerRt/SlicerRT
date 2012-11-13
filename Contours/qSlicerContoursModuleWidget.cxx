/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

// SlicerQt includes
#include "qSlicerContoursModuleWidget.h"
#include "ui_qSlicerContoursModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// SlicerRt includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
class qSlicerContoursModuleWidgetPrivate: public Ui_qSlicerContoursModule
{
  Q_DECLARE_PUBLIC(qSlicerContoursModuleWidget);
protected:
  qSlicerContoursModuleWidget* const q_ptr;
public:
  qSlicerContoursModuleWidgetPrivate(qSlicerContoursModuleWidget& object);
  ~qSlicerContoursModuleWidgetPrivate();
public:
  /// List of currently selected contour nodes. Contains the selected
  /// contour node or the children of the selected contour hierarchy node
  std::vector<vtkMRMLContourNode*> SelectedContourNodes;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerContoursModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidgetPrivate::qSlicerContoursModuleWidgetPrivate(qSlicerContoursModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
  this->SelectedContourNodes.clear();
}

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidgetPrivate::~qSlicerContoursModuleWidgetPrivate()
{
  this->SelectedContourNodes.clear();
}

//-----------------------------------------------------------------------------
// qSlicerContoursModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidget::qSlicerContoursModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerContoursModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidget::~qSlicerContoursModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::onEnter()
{
  if (this->mrmlScene() == 0)
  {
    return;
  }

  Q_D(qSlicerContoursModuleWidget);

  d->ModuleWindowInitialized = true;

  this->contourNodeChanged( d->MRMLNodeComboBox_Contour->currentNode() );
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::setup()
{
  Q_D(qSlicerContoursModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(contourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->comboBox_ActiveRepresentation, SIGNAL(currentIndexChanged(int)), this, SLOT(activeRepresentationComboboxSelectionChanged(int)) );
  connect( d->pushButton_ApplyChangeRepresentation, SIGNAL(clicked()), this, SLOT(applyChangeRepresentationClicked()) );
  connect( d->SliderWidget_OversamplingFactor, SIGNAL(valueChanged(double)), this, SLOT(oversamplingFactorChanged(double)) );
  connect( d->SliderWidget_TargetReductionFactorPercent, SIGNAL(valueChanged(double)), this, SLOT(targetReductionFactorPercentChanged(double)) );

  d->label_NoReferenceWarning->setVisible(false);
  d->label_NewConversionWarning->setVisible(false);
  d->label_NoRibbonWarning->setVisible(false);
  d->label_ActiveSelected->setVisible(false);
  d->label_LabelmapsNotReconvertedWarning->setVisible(false);
}


//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType qSlicerContoursModuleWidget::getRepresentationTypeOfSelectedContours()
{
  Q_D(qSlicerContoursModuleWidget);

  bool sameRepresentationTypes = true;
  vtkMRMLContourNode::ContourRepresentationType representationType = vtkMRMLContourNode::None;

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (representationType == vtkMRMLContourNode::None)
    {
      representationType = (*it)->GetActiveRepresentationType();
    }
    else if ((*it)->GetActiveRepresentationType() == vtkMRMLContourNode::None) // Sanity check
    {
      std::cerr << "Warning: Invalid representation type (None) found for the contour node '" << (*it)->GetName() << "'!" << std::endl;
    }
    else if (representationType != (*it)->GetActiveRepresentationType())
    {
      sameRepresentationTypes = false;
    }
  }

  if (sameRepresentationTypes)
  {
    return representationType;
  }
  else
  {
    return vtkMRMLContourNode::None;
  }
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getReferenceVolumeNodeIdOfSelectedContours(QString &referenceVolumeNodeId)
{
  Q_D(qSlicerContoursModuleWidget);

  referenceVolumeNodeId.clear();
  bool sameReferenceVolumeNodeId = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (referenceVolumeNodeId.isEmpty())
    {
      referenceVolumeNodeId = QString( (*it)->GetRasterizationReferenceVolumeNodeId() );
    }
    else if (referenceVolumeNodeId.compare( (*it)->GetRasterizationReferenceVolumeNodeId() ))
    {
      sameReferenceVolumeNodeId = false;
      referenceVolumeNodeId.clear();
    }
  }

  return sameReferenceVolumeNodeId;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getOversamplingFactorOfSelectedContours(double &oversamplingFactor)
{
  Q_D(qSlicerContoursModuleWidget);

  oversamplingFactor = 0.0;
  bool sameOversamplingFactor = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (oversamplingFactor == 0.0)
    {
      oversamplingFactor = (*it)->GetRasterizationOversamplingFactor();
    }
    else if (oversamplingFactor != (*it)->GetRasterizationOversamplingFactor())
    {
      sameOversamplingFactor = false;
      oversamplingFactor = -1.0;
    }
  }

  return sameOversamplingFactor;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getTargetReductionFactorOfSelectedContours(double &targetReductionFactor)
{
  Q_D(qSlicerContoursModuleWidget);

  targetReductionFactor = 0.0;
  bool sameTargetReductionFactor = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (targetReductionFactor == 0.0)
    {
      targetReductionFactor = (*it)->GetDecimationTargetReductionFactor();
    }
    else if (targetReductionFactor != (*it)->GetDecimationTargetReductionFactor())
    {
      sameTargetReductionFactor = false;
      targetReductionFactor = -1.0;
    }
  }

  return sameTargetReductionFactor;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::selectedContoursContainRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType, bool allMustContain/*=true*/)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (allMustContain && !(*it)->RepresentationExists(representationType))
    {
      // At least one misses the requested representation
      return false;
    }
    else if (!allMustContain && (*it)->RepresentationExists(representationType))
    {
      // At least one has the requested representation
      return true;
    }
  }

  if (allMustContain)
  {
    // All contours have the requested representation
    return true;
  }
  else
  {
    // None of the contours have the requested representation
    return false;
  }
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isConversionNeeded(vtkMRMLContourNode* contourNode, vtkMRMLContourNode::ContourRepresentationType representationToConvertTo)
{
  Q_D(qSlicerContoursModuleWidget);

  bool referenceVolumeNodeChanged = ( d->MRMLNodeComboBox_ReferenceVolume->currentNodeId().compare(
                                      QString(contourNode->GetRasterizationReferenceVolumeNodeId()) ) );
  bool oversamplingFactorChanged = ( fabs(d->SliderWidget_OversamplingFactor->value()
                                   - contourNode->GetRasterizationOversamplingFactor()) > EPSILON );
  bool targetReductionFactorChanged = ( fabs(d->SliderWidget_TargetReductionFactorPercent->value()
                                      - contourNode->GetDecimationTargetReductionFactor()) > EPSILON );

  if ( representationToConvertTo == (int)vtkMRMLContourNode::RibbonModel )
  {
    // Not implemented yet
    return false;
  }
  else if ( representationToConvertTo == (int)vtkMRMLContourNode::IndexedLabelmap )
  {
    return ( !contourNode->GetIndexedLabelmapVolumeNodeId()
           || referenceVolumeNodeChanged || oversamplingFactorChanged );
  }
  else if ( representationToConvertTo == (int)vtkMRMLContourNode::ClosedSurfaceModel )
  {
    return ( !contourNode->GetIndexedLabelmapVolumeNodeId() 
          || !contourNode->GetClosedSurfaceModelNodeId()
          || referenceVolumeNodeChanged || oversamplingFactorChanged || targetReductionFactorChanged );
  }

  return false;
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    d->comboBox_ActiveRepresentation->setEnabled(false);
    d->label_ActiveRepresentation->setText(tr("[No node is selected]"));
    d->label_ActiveRepresentation->setToolTip(tr(""));
    return;
  }

  d->SelectedContourNodes.clear();

  if (node->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
    if (contourNode)
    {
      d->SelectedContourNodes.push_back(contourNode);
    }
  }
  else if (node->IsA("vtkMRMLContourHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLContourHierarchyNode::SafeDownCast(node)->GetChildrenContourNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      std::cerr << "Warning: Selected contour hierarchy node has no children contour nodes!" << std::endl;
      return;
    }

    // Collect contour nodes in the hierarchy and determine whether their active representation types are the same
    for (int i=0; i<childContourNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(childContourNodes->GetItemAsObject(i));
      if (contourNode)
      {
        d->SelectedContourNodes.push_back(contourNode);
      }
    }
  }
  else
  {
    std::cerr << "Error: Invalid node type for ContourNode!" << std::endl;
    return;
  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->label_NewConversionWarning->setVisible(false);
  d->label_NoRibbonWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);
  d->label_LabelmapsNotReconvertedWarning->setVisible(false);

  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(NULL);
  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);
  d->SliderWidget_OversamplingFactor->setEnabled(false);
  d->label_OversamplingFactor->setEnabled(false);

  d->label_TargetReductionFactor->setEnabled(false);
  d->SliderWidget_TargetReductionFactorPercent->setEnabled(false);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  vtkMRMLNode* selectedNode = d->MRMLNodeComboBox_Contour->currentNode();
  if (!this->mrmlScene() || !selectedNode || !d->ModuleWindowInitialized)
  {
    d->comboBox_ActiveRepresentation->setEnabled(false);
    d->label_ActiveRepresentation->setText(tr("[No node is selected]"));
    d->label_ActiveRepresentation->setToolTip(tr(""));
    return;
  }

  d->comboBox_ActiveRepresentation->setEnabled(true);

  // Select the representation type shared by all the children contour nodes
  vtkMRMLContourNode::ContourRepresentationType representationType = this->getRepresentationTypeOfSelectedContours();
  d->comboBox_ActiveRepresentation->blockSignals(true); // Ensure the state is set only once
  if (representationType != vtkMRMLContourNode::None)
  {
    d->comboBox_ActiveRepresentation->setCurrentIndex((int)representationType);

    d->label_ActiveRepresentation->setText(d->comboBox_ActiveRepresentation->itemText((int)representationType));
    d->label_ActiveRepresentation->setToolTip(tr(""));
  }
  else
  {
    d->comboBox_ActiveRepresentation->setCurrentIndex(-1); // Void selection

    d->label_ActiveRepresentation->setText(tr("Various"));
    d->label_ActiveRepresentation->setToolTip(tr("The selected hierarchy node contains contours with different active representation types"));
  }
  d->comboBox_ActiveRepresentation->blockSignals(false);

  // Get reference volume node ID for the selected contour nodes
  QString referenceVolumeNodeId;
  bool sameReferenceVolumeNodeId = this->getReferenceVolumeNodeIdOfSelectedContours(referenceVolumeNodeId);

  // Set the oversampling factor on the GUI and also set the first found oversampling factor to all the nodes if they are not the same
  if (sameReferenceVolumeNodeId)
  {
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
  }
  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(referenceVolumeNodeId);
  d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);

  // Get the oversampling factor of the selected contour nodes
  double oversamplingFactor = 0.0;
  bool sameOversamplingFactor = this->getOversamplingFactorOfSelectedContours(oversamplingFactor);

  // Set the oversampling factor on the GUI and also set the first found oversampling factor to all the nodes if they are not the same
  if (sameOversamplingFactor)
  {
    d->SliderWidget_OversamplingFactor->blockSignals(true);
  }
  d->SliderWidget_OversamplingFactor->setValue(oversamplingFactor);
  d->SliderWidget_OversamplingFactor->blockSignals(false);

  // Get target reduction factor for the selected contour nodes
  double targetReductionFactor;
  bool sameTargetReductionFactor = this->getTargetReductionFactorOfSelectedContours(targetReductionFactor);

  // Set the oversampling factor on the GUI and also set the first found oversampling factor to all the nodes if they are not the same
  if (sameTargetReductionFactor)
  {
    d->SliderWidget_TargetReductionFactorPercent->blockSignals(true);
  }
  d->SliderWidget_TargetReductionFactorPercent->setValue(targetReductionFactor);
  d->SliderWidget_TargetReductionFactorPercent->blockSignals(false);

  // Update apply button state, warning labels, etc.
  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::activeRepresentationComboboxSelectionChanged(int index)
{
  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetsFromChangeActiveRepresentationGroup()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->label_NewConversionWarning->setVisible(false);
  d->label_NoRibbonWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);
  d->label_LabelmapsNotReconvertedWarning->setVisible(false);

  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);
  d->label_ReferenceVolume->setEnabled(false);
  d->SliderWidget_OversamplingFactor->setEnabled(false);
  d->label_OversamplingFactor->setEnabled(false);

  d->label_TargetReductionFactor->setEnabled(false);
  d->SliderWidget_TargetReductionFactorPercent->setEnabled(false);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  if (!this->mrmlScene())
  {
    return;
  }

  int convertToRepresentationType = d->comboBox_ActiveRepresentation->currentIndex();

  // Get reference volume node ID for the selected contour nodes
  QString referenceVolumeNodeId;
  bool sameReferenceVolumeNodeId = this->getReferenceVolumeNodeIdOfSelectedContours(referenceVolumeNodeId);
  bool referenceVolumeNodeChanged = ( !sameReferenceVolumeNodeId
    || d->MRMLNodeComboBox_ReferenceVolume->currentNodeId().compare(referenceVolumeNodeId) );

  // Get the oversampling factor of the selected contour nodes
  double oversamplingFactor = 0.0;
  bool sameOversamplingFactor = this->getOversamplingFactorOfSelectedContours(oversamplingFactor);
  bool oversamplingFactorChanged = ( !sameOversamplingFactor
    || fabs(d->SliderWidget_OversamplingFactor->value() - oversamplingFactor) > EPSILON );

  // Get target reduction factor for the selected contour nodes
  double targetReductionFactor;
  bool sameTargetReductionFactor = this->getTargetReductionFactorOfSelectedContours(targetReductionFactor);
  bool targetReductionFactorChanged = ( !sameTargetReductionFactor
    || fabs(d->SliderWidget_TargetReductionFactorPercent->value() - targetReductionFactor) > EPSILON );

  // Get representation type for the selected contour nodes
  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = this->getRepresentationTypeOfSelectedContours();

  // If current type is selected
  bool activeSelected = ((int)representationTypeInSelectedNodes == convertToRepresentationType);
  if ( activeSelected && convertToRepresentationType != (int)vtkMRMLContourNode::None )
  {
    d->label_ActiveSelected->setVisible(true);
  }

  // No contour nodes are selected
  if (!d->SelectedContourNodes.size())
  {
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);
  }
  // Converting to ribbon
  else if ( convertToRepresentationType == (int)vtkMRMLContourNode::RibbonModel )
  {
    bool ribbonModelPresent = this->selectedContoursContainRepresentation(vtkMRMLContourNode::RibbonModel);

    d->pushButton_ApplyChangeRepresentation->setEnabled(ribbonModelPresent);
    d->label_NoRibbonWarning->setEnabled(!ribbonModelPresent);
  }
  // Converting to indexed labelmap
  else if ( convertToRepresentationType == (int)vtkMRMLContourNode::IndexedLabelmap )
  {
    d->MRMLNodeComboBox_ReferenceVolume->setEnabled(true);
    d->label_ReferenceVolume->setEnabled(true);
    d->SliderWidget_OversamplingFactor->setEnabled(true);
    d->label_OversamplingFactor->setEnabled(true);

    d->pushButton_ApplyChangeRepresentation->setEnabled(
      !activeSelected || referenceVolumeNodeChanged || oversamplingFactorChanged );

    bool conversionNeeded = true;

    // If every selected contour contains indexed labelmap
    if ( this->selectedContoursContainRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
    {
      bool parametersChanged = referenceVolumeNodeChanged || oversamplingFactorChanged;

      d->label_NewConversionWarning->setVisible(parametersChanged);
      conversionNeeded = parametersChanged;
    }
    // If at least one contour has indexed labelmap representation
    else if ( this->selectedContoursContainRepresentation(vtkMRMLContourNode::IndexedLabelmap, false) )
    {
      d->label_NewConversionWarning->setVisible(true);
    }

    // If reference volume is not selected
    if (!d->MRMLNodeComboBox_ReferenceVolume->currentNode())
    {
      d->label_NoReferenceWarning->setVisible(conversionNeeded);
      d->pushButton_ApplyChangeRepresentation->setEnabled(!conversionNeeded);
    }
  }
  // Converting to closed surface model
  else if ( convertToRepresentationType == (int)vtkMRMLContourNode::ClosedSurfaceModel )
  {
    d->label_TargetReductionFactor->setEnabled(true);
    d->SliderWidget_TargetReductionFactorPercent->setEnabled(true);

    d->pushButton_ApplyChangeRepresentation->setEnabled(
      !activeSelected || targetReductionFactorChanged );

    // If at least one contour misses indexed labelmap representation
    if (!this->selectedContoursContainRepresentation(vtkMRMLContourNode::IndexedLabelmap))
    {
      d->MRMLNodeComboBox_ReferenceVolume->setEnabled(true);
      d->label_ReferenceVolume->setEnabled(true);
      d->SliderWidget_OversamplingFactor->setEnabled(true);
      d->label_OversamplingFactor->setEnabled(true);

      if (!d->MRMLNodeComboBox_ReferenceVolume->currentNode())
      {
        d->label_NoReferenceWarning->setVisible(true);
        d->pushButton_ApplyChangeRepresentation->setEnabled(false);
      }

      // If there is at least one indexed labelmap present in the selected contours
      if (this->selectedContoursContainRepresentation(vtkMRMLContourNode::IndexedLabelmap, false))
      {
        d->label_LabelmapsNotReconvertedWarning->setVisible(true);
      }
    }

    // If every selected contour contains closed surface model
    if (this->selectedContoursContainRepresentation(vtkMRMLContourNode::ClosedSurfaceModel))
    {
      d->pushButton_ApplyChangeRepresentation->setEnabled(targetReductionFactorChanged);
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  if (!this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    return;
  }

  d->label_NoReferenceWarning->setVisible(false);

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::oversamplingFactorChanged(double value)
{
  Q_D(qSlicerContoursModuleWidget);

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::targetReductionFactorPercentChanged(double value)
{
  Q_D(qSlicerContoursModuleWidget);

  this->updateWidgetsFromChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::applyChangeRepresentationClicked()
{
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
  double oversamplingFactor = d->SliderWidget_OversamplingFactor->value();
  double targetReductionFactor = d->SliderWidget_TargetReductionFactorPercent->value();

  // Apply change representation on all selected contours
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    bool conversionNeeded = this->isConversionNeeded((*it), (vtkMRMLContourNode::ContourRepresentationType)d->comboBox_ActiveRepresentation->currentIndex());

    // Set conversion parameters
    if (conversionNeeded)
    {
      if (volumeNode)
      {
        (*it)->SetAndObserveRasterizationReferenceVolumeNodeId(volumeNode->GetID());
      }
      (*it)->SetRasterizationOversamplingFactor(oversamplingFactor);
      (*it)->SetDecimationTargetReductionFactor(targetReductionFactor / 100.0);
    }

    // Do conversion if necessary
    if (d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::RibbonModel)
    {
      (*it)->SetActiveRepresentationByType(vtkMRMLContourNode::RibbonModel);
    }
    else if (d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::IndexedLabelmap)
    {
      if (conversionNeeded)
      {
        // Delete original representation and re-convert
        (*it)->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);
      }

      vtkMRMLScalarVolumeNode* indexedLabelmapNode = (*it)->GetIndexedLabelmapVolumeNode();
      if (!indexedLabelmapNode)
      {
        std::cerr << "Failed to get '" << (std::string)d->comboBox_ActiveRepresentation->currentText().toLatin1()
          << "' representation from contour node '" << (*it)->GetName() << "' !" << std::endl;
      }
      else
      {
        (*it)->SetActiveRepresentationByNode(indexedLabelmapNode);
      }
    }
    else if (d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::ClosedSurfaceModel)
    {
      if (conversionNeeded)
      {
        // Delete original representation and re-convert
        (*it)->ReconvertRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);
      }

      vtkMRMLModelNode* closedSurfaceModelNode = (*it)->GetClosedSurfaceModelNode();
      if (!closedSurfaceModelNode)
      {
        std::cerr << "Failed to get '" << (std::string)d->comboBox_ActiveRepresentation->currentText().toLatin1()
          << "' representation from contour node '" << (*it)->GetName() << "' !" << std::endl;
      }
      else
      {
        (*it)->SetActiveRepresentationByNode((vtkMRMLDisplayableNode*)closedSurfaceModelNode);
      }
    }
  }

  d->label_ActiveRepresentation->setText(d->comboBox_ActiveRepresentation->currentText());
  d->label_ActiveRepresentation->setToolTip(tr(""));

  this->updateWidgetsFromChangeActiveRepresentationGroup();

  QApplication::restoreOverrideCursor();
}
