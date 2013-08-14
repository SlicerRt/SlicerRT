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
#include <qSlicerApplication.h>

// SlicerRt includes
#include "SlicerRtCommon.h"

// Contours includes
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkConvertContourRepresentations.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLDisplayableHierarchyNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QProgressDialog>
#include <QMainWindow>

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
  vtkSlicerContoursModuleLogic* logic() const;
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
vtkSlicerContoursModuleLogic*
qSlicerContoursModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerContoursModuleWidget);
  return vtkSlicerContoursModuleLogic::SafeDownCast(q->logic());
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

  // Filter out hierarchy nodes that are not contour hierarchy nodes
  d->MRMLNodeComboBox_Contour->addAttribute( QString("vtkMRMLDisplayableHierarchyNode"), QString(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );

  // Make connections
  connect( d->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(contourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );

  connect( d->comboBox_ChangeActiveRepresentation, SIGNAL(currentIndexChanged(int)), this, SLOT(activeRepresentationComboboxSelectionChanged(int)) );
  connect( d->pushButton_ApplyChangeRepresentation, SIGNAL(clicked()), this, SLOT(applyChangeRepresentationClicked()) );
  connect( d->horizontalSlider_OversamplingFactor, SIGNAL(valueChanged(int)), this, SLOT(oversamplingFactorChanged(int)) );
  connect( d->SliderWidget_TargetReductionFactorPercent, SIGNAL(valueChanged(double)), this, SLOT(targetReductionFactorPercentChanged(double)) );

  d->label_NoReferenceWarning->setVisible(false);
  d->label_NewConversion->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_ActiveSelected->setVisible(false);
  d->label_CreatedFromLabelmap->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  if (!this->mrmlScene() || !d->ModuleWindowInitialized)
  {
    return;
  }
  if (!node)
  {
    d->label_NoReferenceWarning->setVisible(true);
    return;
  }

  d->label_NoReferenceWarning->setVisible(false);

  this->updateWidgetsInChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::oversamplingFactorChanged(int value)
{
  Q_D(qSlicerContoursModuleWidget);
  UNUSED_VARIABLE(value);

  d->lineEdit_OversamplingFactor->setText( QString::number(this->getOversamplingFactor()) );

  this->updateWidgetsInChangeActiveRepresentationGroup();
}
//-----------------------------------------------------------------------------

double qSlicerContoursModuleWidget::getOversamplingFactor()
{
  Q_D(qSlicerContoursModuleWidget);

  return pow(2.0, d->horizontalSlider_OversamplingFactor->value());
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::targetReductionFactorPercentChanged(double value)
{
  UNUSED_VARIABLE(value);

  this->updateWidgetsInChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveSelectedContoursBeenCreatedFromLabelmap()
{
  Q_D(qSlicerContoursModuleWidget);

  bool allCreatedFromLabelmap = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if ( allCreatedFromLabelmap && !(*it)->HasBeenCreatedFromIndexedLabelmap() )
    {
      allCreatedFromLabelmap = false;
      break;
    }
  }

  return allCreatedFromLabelmap;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getReferenceVolumeNodeIdOfSelectedContours(QString &referenceVolumeNodeId)
{
  Q_D(qSlicerContoursModuleWidget);

  referenceVolumeNodeId.clear();
  bool sameReferenceVolumeNodeId = true;
  bool allCreatedFromLabelmap = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if ( allCreatedFromLabelmap && !(*it)->HasBeenCreatedFromIndexedLabelmap() )
    {
      allCreatedFromLabelmap = false;
    }
    else
    {
      continue;
    }

    if (referenceVolumeNodeId.isEmpty())
    {
      referenceVolumeNodeId = QString( (*it)->GetRasterizationReferenceVolumeNodeId() );
    }
    else if (referenceVolumeNodeId.compare( (*it)->GetRasterizationReferenceVolumeNodeId() ))
    {
      sameReferenceVolumeNodeId = false;
      referenceVolumeNodeId.clear();
      break;
    }
  }

  return allCreatedFromLabelmap || sameReferenceVolumeNodeId;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::getOversamplingFactorOfSelectedContours(double &oversamplingFactor)
{
  Q_D(qSlicerContoursModuleWidget);

  if (d->SelectedContourNodes.size() == 0)
  {
    oversamplingFactor = -1.0;
    return true;
  }

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
void qSlicerContoursModuleWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->SelectedContourNodes.clear();

  if (!this->mrmlScene() || !node || !d->ModuleWindowInitialized)
  {
    d->comboBox_ChangeActiveRepresentation->setEnabled(false);
    d->label_ActiveRepresentation->setText(tr("[No node is selected]"));
    d->label_ActiveRepresentation->setToolTip(tr(""));
    return;
  }

  // Get contour nodes from selection
  vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(node, d->SelectedContourNodes);

  // Update UI from selected contours nodes list
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->label_NewConversion->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);
  d->label_CreatedFromLabelmap->setVisible(false);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  vtkMRMLNode* selectedNode = d->MRMLNodeComboBox_Contour->currentNode();
  if (!this->mrmlScene() || !selectedNode || !d->ModuleWindowInitialized)
  {
    d->comboBox_ChangeActiveRepresentation->setEnabled(false);
    d->label_ActiveRepresentation->setText(tr("[No node is selected]"));
    d->label_ActiveRepresentation->setToolTip(tr(""));
    return;
  }

  d->comboBox_ChangeActiveRepresentation->setEnabled(true);

  // Select the representation type shared by all the children contour nodes
  if (d->SelectedContourNodes.size() == 0)
  {
    d->label_ActiveRepresentation->setText(tr("No contours in selection"));
    d->label_ActiveRepresentation->setToolTip(tr("The selected hierarchy node contains no contours in the structure set"));

    d->CTKCollapsibleButton_ChangeActiveRepresentation->setEnabled(false);
  }
  else
  {
    vtkMRMLContourNode::ContourRepresentationType representationType = vtkSlicerContoursModuleLogic::GetRepresentationTypeOfContours(d->SelectedContourNodes);
    if (representationType != vtkMRMLContourNode::None)
    {
      d->label_ActiveRepresentation->setText(d->comboBox_ChangeActiveRepresentation->itemText((int)representationType));
      d->label_ActiveRepresentation->setToolTip(tr(""));
    }
    else
    {
      d->label_ActiveRepresentation->setText(tr("Various"));
      d->label_ActiveRepresentation->setToolTip(tr("The selected hierarchy node contains contours with different active representation types"));
    }

    d->CTKCollapsibleButton_ChangeActiveRepresentation->setEnabled(true);
  }

  // Look for referenced volume for contours and set it as default if found
  vtkMRMLScalarVolumeNode* referencedVolume = vtkSlicerContoursModuleLogic::GetReferencedVolumeForContours(d->SelectedContourNodes);
  if (referencedVolume)
  {
    for (std::vector<vtkMRMLContourNode*>::iterator contourIt = d->SelectedContourNodes.begin(); contourIt != d->SelectedContourNodes.end(); ++contourIt)
    {
      (*contourIt)->SetAndObserveRasterizationReferenceVolumeNodeId(referencedVolume->GetID());
      (*contourIt)->SetRasterizationOversamplingFactor(1.0);
    }
  }

  // Get reference volume node ID for the selected contour nodes
  QString referenceVolumeNodeId;
  bool sameReferenceVolumeNodeId = this->getReferenceVolumeNodeIdOfSelectedContours(referenceVolumeNodeId);

  // Set reference volume on the GUI
  if (!referenceVolumeNodeId.isEmpty())
  {
    if (sameReferenceVolumeNodeId)
    {
      d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
    }
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(referenceVolumeNodeId);
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
  }
  // If all selected contours have the same reference, then leave it as empty
  else
  {
    if (this->haveSelectedContoursBeenCreatedFromLabelmap())
    {
      d->label_CreatedFromLabelmap->setVisible(true);
    }
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(true);
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(NULL);
    d->MRMLNodeComboBox_ReferenceVolume->blockSignals(false);
  }

  // Get the oversampling factor of the selected contour nodes
  double oversamplingFactor = 0.0;
  bool sameOversamplingFactor = this->getOversamplingFactorOfSelectedContours(oversamplingFactor);

  // Set the oversampling factor on the GUI
  if (oversamplingFactor != -1.0)
  {
    if (sameOversamplingFactor)
    {
      d->horizontalSlider_OversamplingFactor->blockSignals(true);
    }
    d->horizontalSlider_OversamplingFactor->setValue( (int)(log(oversamplingFactor)/log(2.0)) );
    d->horizontalSlider_OversamplingFactor->blockSignals(false);
  }
  d->lineEdit_OversamplingFactor->setText( QString::number(this->getOversamplingFactor()) );

  // Get target reduction factor for the selected contour nodes
  double targetReductionFactor;
  bool sameTargetReductionFactor = this->getTargetReductionFactorOfSelectedContours(targetReductionFactor);

  // Set the oversampling factor on the GUI ([1] applies here as well)
  if (targetReductionFactor != -1.0)
  {
    if (sameTargetReductionFactor)
    {
      d->SliderWidget_TargetReductionFactorPercent->blockSignals(true);
    }
    d->SliderWidget_TargetReductionFactorPercent->setValue(targetReductionFactor);
    d->SliderWidget_TargetReductionFactorPercent->blockSignals(false);
  }

  // Update apply button state, warning labels, etc.
  this->updateWidgetsInChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::activeRepresentationComboboxSelectionChanged(int index)
{
  UNUSED_VARIABLE(index);

  this->updateWidgetsInChangeActiveRepresentationGroup();
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType qSlicerContoursModuleWidget::getTargetRepresentationType()
{
  Q_D(qSlicerContoursModuleWidget);

  return (vtkMRMLContourNode::ContourRepresentationType)d->comboBox_ChangeActiveRepresentation->currentIndex();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::showConversionParameterControlsForTargetRepresentation(vtkMRMLContourNode::ContourRepresentationType targetRepresentationType)
{
  Q_D(qSlicerContoursModuleWidget);

  d->MRMLNodeComboBox_ReferenceVolume->setVisible(true);
  d->label_ReferenceVolume->setVisible(true);
  d->horizontalSlider_OversamplingFactor->setVisible(true);
  d->lineEdit_OversamplingFactor->setVisible(true);
  d->label_OversamplingFactor->setVisible(true);
  d->label_TargetReductionFactor->setVisible(true);
  d->SliderWidget_TargetReductionFactorPercent->setVisible(true);

  if ((targetRepresentationType != (int)vtkMRMLContourNode::IndexedLabelmap
    && targetRepresentationType != (int)vtkMRMLContourNode::ClosedSurfaceModel)
    || !this->mrmlScene() )
  {
    d->MRMLNodeComboBox_ReferenceVolume->setVisible(false);
    d->label_ReferenceVolume->setVisible(false);
    d->horizontalSlider_OversamplingFactor->setVisible(false);
    d->lineEdit_OversamplingFactor->setVisible(false);
    d->label_OversamplingFactor->setVisible(false);
  }
  if ( targetRepresentationType != (int)vtkMRMLContourNode::ClosedSurfaceModel
    || !this->mrmlScene() )
  {
    d->label_TargetReductionFactor->setVisible(false);
    d->SliderWidget_TargetReductionFactorPercent->setVisible(false);
  }
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChangedForIndexedLabelmap(vtkMRMLContourNode* contourNode)
{
  Q_D(qSlicerContoursModuleWidget);

  // Reference volume has changed if the contour has been created from labelmap (the reference volume is the same as
  // the indexed labelmap representation, and the reference combobox has empty selection, so the reference node is NULL)
  // and the reference node is not NULL, or if not created from labelmap, but the selection does not match the current reference
  bool referenceVolumeNodeChanged = ( ( contourNode->HasBeenCreatedFromIndexedLabelmap()
                                     && d->MRMLNodeComboBox_ReferenceVolume->currentNode() != NULL )
                                   || ( !contourNode->HasBeenCreatedFromIndexedLabelmap()
                                     && d->MRMLNodeComboBox_ReferenceVolume->currentNodeID().compare(contourNode->GetRasterizationReferenceVolumeNodeId()) ) );
  bool oversamplingFactorChanged = ( contourNode->HasBeenCreatedFromIndexedLabelmap() ? false
                                   : ( fabs(this->getOversamplingFactor() - contourNode->GetRasterizationOversamplingFactor()) > EPSILON ) );

  return contourNode->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap)
    && (referenceVolumeNodeChanged || oversamplingFactorChanged);
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChangedForClosedSurfaceModel(vtkMRMLContourNode* contourNode)
{
  Q_D(qSlicerContoursModuleWidget);

  bool targetReductionFactorChanged = ( fabs(d->SliderWidget_TargetReductionFactorPercent->value() - contourNode->GetDecimationTargetReductionFactor()) > EPSILON );

  return contourNode->RepresentationExists(vtkMRMLContourNode::ClosedSurfaceModel) && targetReductionFactorChanged;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChanged(vtkMRMLContourNode* contourNode)
{
  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType = this->getTargetRepresentationType();

  return ( ( ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap || targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel )
            && haveConversionParametersChangedForIndexedLabelmap(contourNode) )
          || ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
            && haveConversionParametersChangedForClosedSurfaceModel(contourNode) ) );
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChangedInAnySelectedContour()
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (this->haveConversionParametersChanged(*it))
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isSuitableSourceAvailableForConversion(vtkMRMLContourNode* contourNode)
{
  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType = this->getTargetRepresentationType();

  if ( targetRepresentationType == vtkMRMLContourNode::RibbonModel
    && !contourNode->RepresentationExists(vtkMRMLContourNode::RibbonModel) )
  {
    return false;
  }
  else if ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
    && !contourNode->RepresentationExists(vtkMRMLContourNode::RibbonModel)
    && !contourNode->RepresentationExists(vtkMRMLContourNode::ClosedSurfaceModel) )
  {
    return false;
  }
  else if ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
    && !contourNode->RepresentationExists(vtkMRMLContourNode::RibbonModel)
    && ( !contourNode->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap)
      || this->haveConversionParametersChangedForIndexedLabelmap(contourNode) ) )
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::isSuitableSourceAvailableForConversionForAllSelectedContours()
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (!this->isSuitableSourceAvailableForConversion(*it))
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetsInChangeActiveRepresentationGroup()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->label_NewConversion->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);

  // Get representation type for the selected contour nodes and the target type
  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = vtkSlicerContoursModuleLogic::GetRepresentationTypeOfContours(d->SelectedContourNodes);
  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType = this->getTargetRepresentationType();

  // If target representation type matches all active representations
  if (targetRepresentationType == representationTypeInSelectedNodes)
  {
    d->label_ActiveSelected->setVisible(true);
    if (this->haveConversionParametersChangedInAnySelectedContour())
    {
      if (this->isSuitableSourceAvailableForConversionForAllSelectedContours())
      {
        this->showConversionParameterControlsForTargetRepresentation(targetRepresentationType);
        d->label_NewConversion->setVisible(true);
        d->pushButton_ApplyChangeRepresentation->setEnabled(true);
      }
      else
      {
        this->showConversionParameterControlsForTargetRepresentation(vtkMRMLContourNode::None);
        d->label_NoSourceWarning->setVisible(true);
        d->pushButton_ApplyChangeRepresentation->setEnabled(false);
      }
    }
    else
    {
      this->showConversionParameterControlsForTargetRepresentation(targetRepresentationType);
      d->pushButton_ApplyChangeRepresentation->setEnabled(false);
    }

    return;
  }

  // If any selected contour lacks a suitable source representation for the actual conversion, then show warning and hide all conversion parameters
  if (!this->isSuitableSourceAvailableForConversionForAllSelectedContours())
  {
    this->showConversionParameterControlsForTargetRepresentation(representationTypeInSelectedNodes);
    d->label_NoSourceWarning->setVisible(true);
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);

    return;
  }

  // Show conversion parameters for selected target representation
  this->showConversionParameterControlsForTargetRepresentation(targetRepresentationType);

  // If there is no reference volume selected but should be, then show warning and disable the apply button
  if (!vtkSlicerContoursModuleLogic::IsReferenceVolumeValidForAllContours(d->SelectedContourNodes, this->getTargetRepresentationType()))
  {
    d->label_NoReferenceWarning->setVisible(true);
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);

    return;
  }

  // If every condition is fine, then enable Apply button
  d->pushButton_ApplyChangeRepresentation->setEnabled(true);

  // Show new conversion message if needed
  if (this->haveConversionParametersChangedInAnySelectedContour())
  {
    d->label_NewConversion->setVisible(true);
  }
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

  // TODO: Workaround for update issues
  this->mrmlScene()->StartState(vtkMRMLScene::BatchProcessState);

  QProgressDialog *convertProgress = new QProgressDialog(qSlicerApplication::application()->mainWindow());
  convertProgress->setModal(true);
  convertProgress->setMinimumDuration(150);
  convertProgress->setLabelText("Converting contours to target representation...");
  convertProgress->show();
  QApplication::processEvents();
  unsigned int numberOfContours = d->SelectedContourNodes.size();
  unsigned int currentContour = 0;

  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType = this->getTargetRepresentationType();

  // Apply change representation and occurrent conversion on each selected contour
  // We assume that representation change (and conversion if necessary) is possible for each selected contour,
  // as the Apply button should only be enabled if it is ensured (\sa updateWidgetsFromChangeActiveRepresentationGroup)
  for (std::vector<vtkMRMLContourNode*>::iterator currentContourIt = d->SelectedContourNodes.begin(); currentContourIt != d->SelectedContourNodes.end(); ++currentContourIt)
  {
    // Convert if necessary
    if ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
      || targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel )
    {
      if ( !(*currentContourIt)->RepresentationExists(vtkMRMLContourNode::IndexedLabelmap)
        || this->haveConversionParametersChangedForIndexedLabelmap(*currentContourIt) )
      {
        this->convertToIndexedLabelmap(*currentContourIt);
      }
    }
    if (targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel)
    {
      if ( !(*currentContourIt)->RepresentationExists(vtkMRMLContourNode::ClosedSurfaceModel)
        || this->haveConversionParametersChangedForClosedSurfaceModel(*currentContourIt) )
      {
        this->convertToClosedSurfaceModel(*currentContourIt);
      }
    }

    // Set target representation to node after the occurrent conversions
    (*currentContourIt)->SetActiveRepresentationByType(targetRepresentationType);

    // Set progress
    ++currentContour;
    convertProgress->setValue(currentContour/(double)numberOfContours * 100.0);
  }

  d->label_ActiveRepresentation->setText(d->comboBox_ChangeActiveRepresentation->currentText());
  d->label_ActiveRepresentation->setToolTip(tr(""));

  this->updateWidgetsInChangeActiveRepresentationGroup();

  delete convertProgress;
  this->mrmlScene()->EndState(vtkMRMLScene::BatchProcessState);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::convertToIndexedLabelmap(vtkMRMLContourNode* contourNode)
{
  Q_D(qSlicerContoursModuleWidget);

  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
  double oversamplingFactor = this->getOversamplingFactor();

  // Set indexed labelmap conversion parameters
  contourNode->SetAndObserveRasterizationReferenceVolumeNodeId(referenceVolumeNode ? referenceVolumeNode->GetID() : NULL);
  contourNode->SetRasterizationOversamplingFactor(oversamplingFactor);

  // Delete occurrent existing representation and re-convert
  vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
  converter->SetContourNode(contourNode);
  converter->ReconvertRepresentation(vtkMRMLContourNode::IndexedLabelmap);

  vtkMRMLScalarVolumeNode* indexedLabelmapNode = contourNode->GetIndexedLabelmapVolumeNode();
  if (!indexedLabelmapNode)
  {
    vtkErrorWithObjectMacro(contourNode, "convertToIndexedLabelmap: Failed to get indexed labelmap representation for contour node '" << contourNode->GetName() << "' !");
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::convertToClosedSurfaceModel(vtkMRMLContourNode* contourNode)
{
  Q_D(qSlicerContoursModuleWidget);

  double targetReductionFactor = d->SliderWidget_TargetReductionFactorPercent->value();

  // Set closed surface model conversion parameters
  contourNode->SetDecimationTargetReductionFactor(targetReductionFactor);

  // Delete occurrent existing representation and re-convert
  vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
  converter->SetContourNode(contourNode);
  converter->ReconvertRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);

  vtkMRMLModelNode* closedSurfaceModelNode = contourNode->GetClosedSurfaceModelNode();
  if (!closedSurfaceModelNode)
  {
    vtkErrorWithObjectMacro(contourNode, "convertToClosedSurfaceModel:: Failed to get closed surface model representation from contour node '" << contourNode->GetName() << "' !");
    return false;
  }

  return true;
}
