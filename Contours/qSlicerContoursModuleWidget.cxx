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

// SlicerQt includes
#include "qSlicerContoursModuleWidget.h"
#include "ui_qSlicerContoursModule.h"
#include <qSlicerApplication.h>

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// Contours includes
#include "vtkConvertContourRepresentations.h"
#include "vtkMRMLContourModelDisplayNode.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

// Qt includes
#include <QProgressDialog>
#include <QMainWindow>
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Contours
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
  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::onEnter: Invalid scene!";
    return;
  }

  Q_D(qSlicerContoursModuleWidget);

  d->ModuleWindowInitialized = true;

  this->contourNodeChanged( d->MRMLNodeComboBox_Contour->currentNode() );

  this->updateWidgetsInCreateContourFromRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::setup()
{
  this->testInit();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::referenceVolumeNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
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
void qSlicerContoursModuleWidget::sourceRepresentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);
  UNUSED_VARIABLE(node);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::sourceRepresentationNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }

  this->updateWidgetsInCreateContourFromRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::targetContourSetNodeChanged(vtkMRMLNode* node)
{
  Q_UNUSED(node);
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::targetContourSetNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }

  this->updateWidgetsInCreateContourFromRepresentationGroup();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::targetContourNameChanged(const QString& value)
{
  Q_UNUSED(value);
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::sourceRepresentationNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }

  this->updateWidgetsInCreateContourFromRepresentationGroup();
}

//-----------------------------------------------------------------------------
double qSlicerContoursModuleWidget::getOversamplingFactor()
{
  Q_D(qSlicerContoursModuleWidget);

  return pow(2.0, d->horizontalSlider_OversamplingFactor->value());
}

//-----------------------------------------------------------------------------
int qSlicerContoursModuleWidget::getOversamplingFactorSliderValueFromOversamplingFactor(double oversamplingFactor)
{
  return (int)(log(oversamplingFactor)/log(2.0));
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

  if (d->SelectedContourNodes.empty())
  {
    return false;
  }

  referenceVolumeNodeId.clear();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = (*d->SelectedContourNodes.begin())->GetRasterizationReferenceVolumeNode();
  if (referenceVolumeNode != NULL)
  {
    referenceVolumeNodeId = referenceVolumeNode->GetID();
  }
  bool sameReferenceVolumeNodeId = true;
  bool allCreatedFromLabelmap = true;
  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    vtkMRMLScalarVolumeNode* thisContourReferenceVolumeNode = (*it)->GetRasterizationReferenceVolumeNode();
    if ( allCreatedFromLabelmap && !(*it)->HasBeenCreatedFromIndexedLabelmap() )
    {
      allCreatedFromLabelmap = false;
    }
    else
    {
      continue;
    }

    if (referenceVolumeNode != thisContourReferenceVolumeNode)
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

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::contourNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }
  if (!node)
  {
    d->comboBox_ChangeActiveRepresentation->setEnabled(false);
    d->label_RepresentationsForContour->setText(tr("[No node is selected]"));
    d->label_RepresentationsForContour->setToolTip(tr(""));
    return;
  }

  // Get contour nodes from selection
  vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(node, d->SelectedContourNodes);

  // Update UI from selected contours nodes list
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::showContourFromRepresentationPanel(QString contourSetNodeID)
{
  Q_D(qSlicerContoursModuleWidget);

  d->CTKCollapsibleButton_ConvertRepresentation->setCollapsed(false);
  d->MRMLNodeComboBox_TargetContourSet->setCurrentNodeID(QString(contourSetNodeID.toLatin1().constData()));
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_NewConversion->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);
  d->label_CreatedFromLabelmap->setVisible(false);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::updateWidgetFromMRML: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }
  if (this->mrmlScene()->IsClosing())
  {
    return; // Skip UI update if the scene is closing
  }
  vtkMRMLNode* selectedNode = d->MRMLNodeComboBox_Contour->currentNode();
  if (!selectedNode)
  {
    d->comboBox_ChangeActiveRepresentation->setEnabled(false);
    d->label_RepresentationsForContour->setText(tr("[No node is selected]"));
    d->label_RepresentationsForContour->setToolTip(tr(""));
    return;
  }

  d->comboBox_ChangeActiveRepresentation->setEnabled(true);

  // Select the representation type shared by all the children contour nodes
  if (d->SelectedContourNodes.size() == 0)
  {
    d->label_RepresentationsForContour->setText(tr("No contours in selection"));
    d->label_RepresentationsForContour->setToolTip(tr("The selected hierarchy node contains no contours in the structure set"));

    d->CTKCollapsibleButton_ChangeActiveRepresentation->setEnabled(false);
  }
  else
  {
    if (d->SelectedContourNodes.size() == 1)
    {
      std::string representationsForContourNode = buildRepresentationsString(d->SelectedContourNodes[0]);
      d->label_RepresentationsForContour->setText(QString(representationsForContourNode.c_str()));
      d->label_RepresentationsForContour->setToolTip(tr("The available representations for this contour node"));
    }
    else
    {
      vtkMRMLContourNode::ContourRepresentationType representationType = vtkSlicerContoursModuleLogic::GetRepresentationTypeOfContours(d->SelectedContourNodes);
      if (representationType != vtkMRMLContourNode::None)
      {
        d->label_RepresentationsForContour->setText(d->comboBox_ChangeActiveRepresentation->itemText((int)representationType));
        d->label_RepresentationsForContour->setToolTip(tr(""));
      }
      else
      {
        d->label_RepresentationsForContour->setText(tr("Various"));
        d->label_RepresentationsForContour->setToolTip(tr("The selected hierarchy node contains contours with different representation types"));
      }
    }

    d->CTKCollapsibleButton_ChangeActiveRepresentation->setEnabled(true);
  }

  // Look for referenced volume by DICOM for contours and set it as default if found
  vtkMRMLScalarVolumeNode* referencedVolume = vtkSlicerContoursModuleLogic::GetReferencedVolumeByDicomForContours(d->SelectedContourNodes);
  if (referencedVolume && !this->haveSelectedContoursBeenCreatedFromLabelmap())
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(referencedVolume);
    
    d->horizontalSlider_OversamplingFactor->blockSignals(true);
    d->horizontalSlider_OversamplingFactor->setValue( this->getOversamplingFactorSliderValueFromOversamplingFactor(1.0) );
    d->horizontalSlider_OversamplingFactor->blockSignals(false);
    d->lineEdit_OversamplingFactor->setText( QString::number(this->getOversamplingFactor()) );
  }
  // If no referenced volume was found by DICOM, then set the reference and oversampling factor specified in the nodes
  else
  {
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
      d->horizontalSlider_OversamplingFactor->setValue( this->getOversamplingFactorSliderValueFromOversamplingFactor(oversamplingFactor) );
      d->horizontalSlider_OversamplingFactor->blockSignals(false);
    }
    d->lineEdit_OversamplingFactor->setText( QString::number(this->getOversamplingFactor()) );
  }

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
  this->updateWidgetsInCreateContourFromRepresentationGroup();
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

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::contourNodeChanged: Invalid scene!";
    return;
  }
  if (!d->ModuleWindowInitialized)
  {
    return;
  }
  
  if ( (targetRepresentationType != (int)vtkMRMLContourNode::IndexedLabelmap
    && targetRepresentationType != (int)vtkMRMLContourNode::ClosedSurfaceModel) )
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

  // Reference volume has changed if the reference volume doesn't match the stored reference volume
  bool referenceVolumeNodeChanged = ( d->MRMLNodeComboBox_ReferenceVolume->currentNode() != contourNode->GetRasterizationReferenceVolumeNode() );
  bool oversamplingFactorChanged = ( contourNode->HasBeenCreatedFromIndexedLabelmap() ? false
                                   : ( fabs(this->getOversamplingFactor() - contourNode->GetRasterizationOversamplingFactor()) > EPSILON ) );

  return contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap)
    && (referenceVolumeNodeChanged || oversamplingFactorChanged);
}

//-----------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::haveConversionParametersChangedForClosedSurfaceModel(vtkMRMLContourNode* contourNode)
{
  Q_D(qSlicerContoursModuleWidget);

  bool targetReductionFactorChanged = ( fabs(d->SliderWidget_TargetReductionFactorPercent->value() - contourNode->GetDecimationTargetReductionFactor()) > EPSILON );

  return contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel) && targetReductionFactorChanged;
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
    && !contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) )
  {
    return false;
  }
  else if ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
    && !contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel)
    && !contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel) )
  {
    return false;
  }
  else if ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
    && !contourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel)
    && ( !contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap)
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

  if (d->SelectedContourNodes.size() == 0)
  {
    return false;
  }

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
bool qSlicerContoursModuleWidget::isReferenceVolumeValidForAllContours()
{
  Q_D(qSlicerContoursModuleWidget);

  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType = this->getTargetRepresentationType();

  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = d->SelectedContourNodes.begin(); contourIt != d->SelectedContourNodes.end(); ++contourIt)
  {
    // If (target is indexed labelmap OR an intermediate labelmap is needed for closed surface conversion BUT missing)
    // AND (the selected reference node is empty AND was not created from labelmap), then reference volume selection is invalid
    if ( ( targetRepresentationType == vtkMRMLContourNode::IndexedLabelmap
      || ( targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel
      && !(*contourIt)->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) ) )
      && ( !d->MRMLNodeComboBox_ReferenceVolume->currentNode()
        && !(*contourIt)->HasBeenCreatedFromIndexedLabelmap() ) )
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

  d->label_NewConversion->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_NoReferenceWarning->setVisible(false);

  // Get representation type for the selected contour nodes and the target type
  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = vtkSlicerContoursModuleLogic::GetRepresentationTypeOfContours(d->SelectedContourNodes);
  vtkMRMLContourNode::ContourRepresentationType targetRepresentationType = this->getTargetRepresentationType();

  // If target representation type matches all active representations
  if (targetRepresentationType == representationTypeInSelectedNodes)
  {
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
  if (!this->isReferenceVolumeValidForAllContours())
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
void qSlicerContoursModuleWidget::updateWidgetsInCreateContourFromRepresentationGroup()
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_NoInputWarning->setVisible(false);
  d->label_ContourCreatedFromLabelmap->setVisible(false);
  d->label_ContourCreatedFromModel->setVisible(false);
  d->label_ContourSetContainsContourName->setVisible(false);

  bool hasSourceNode(true);
  vtkMRMLDisplayableNode* sourceNode = vtkMRMLDisplayableNode::SafeDownCast(d->MRMLNodeComboBox_ConvertRepresentationSource->currentNode());
  if (sourceNode == NULL)
  {
    hasSourceNode = false;
  }

  bool isNameValid(true);
  bool hasContourSet(true);
  vtkMRMLSubjectHierarchyNode* contourSet = vtkMRMLSubjectHierarchyNode::SafeDownCast(d->MRMLNodeComboBox_TargetContourSet->currentNode());
  if (contourSet != NULL)
  {
    // Check if target contour name exists in requested structure set
    QString targetNameQ = d->lineEdit_TargetContourName->text();
    std::string targetNameStd(targetNameQ.toStdString());

    if (!targetNameStd.empty())
    {
      std::vector<vtkMRMLContourNode*> nodes;
      this->GetContoursFromContourSet(contourSet, nodes);
      for( std::vector<vtkMRMLContourNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
      {
        if (STRCASECMP( (*it)->GetName(), targetNameStd.c_str() ) == 0)
        {
          isNameValid = false;
          d->label_ContourSetContainsContourName->setVisible(true);
          break;
        }
      }
    }
  }
  else
  {
    hasContourSet = false;
  }

  if (!hasSourceNode || !hasContourSet)
  {
    d->label_ContourSetContainsContourName->setVisible(false);
    d->label_NoInputWarning->setVisible(true);
  }
  
  bool enabled = hasSourceNode && isNameValid && hasContourSet;
  d->pushButton_CreateContourFromRepresentation->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::applyChangeRepresentationClicked()
{
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::applyChangeRepresentationClicked: Invalid scene!";
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // TODO: Workaround for update issues
  this->mrmlScene()->StartState(vtkMRMLScene::BatchProcessState);

  // Initialize progress bar
  unsigned int numberOfContours = d->SelectedContourNodes.size();
  unsigned int currentContour = 0;
  QProgressDialog *convertProgress = new QProgressDialog(qSlicerApplication::application()->mainWindow());
  convertProgress->setModal(true);
  convertProgress->setMinimumDuration(150);
  std::stringstream ss;
  ss << "Converting contour" << (numberOfContours > 1 ? "s" : "") << " to target representation...";
  convertProgress->setLabelText(ss.str().c_str());
  convertProgress->show();
  QApplication::processEvents();

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
      if ( !(*currentContourIt)->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap)
        || this->haveConversionParametersChangedForIndexedLabelmap(*currentContourIt) )
      {
        this->convertToIndexedLabelmap(*currentContourIt);
      }
    }
    if (targetRepresentationType == vtkMRMLContourNode::ClosedSurfaceModel)
    {
      if ( !(*currentContourIt)->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel)
        || this->haveConversionParametersChangedForClosedSurfaceModel(*currentContourIt) )
      {
        this->convertToClosedSurfaceModel(*currentContourIt);
      }
    }

    // Set progress
    ++currentContour;
    convertProgress->setValue(currentContour/(double)numberOfContours * 100.0);
  }

  // Update the label with the new representations added
  std::string representationsForContourNode = buildRepresentationsString(d->SelectedContourNodes[0]);
  d->label_RepresentationsForContour->setText(QString(representationsForContourNode.c_str()));
  d->label_RepresentationsForContour->setToolTip(tr("The available representations for this contour node"));

  this->updateWidgetsInChangeActiveRepresentationGroup();

  delete convertProgress;
  this->mrmlScene()->EndState(vtkMRMLScene::BatchProcessState);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::onCreateContourFromRepresentationClicked()
{
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    qCritical() << "qSlicerContoursModuleWidget::applyChangeRepresentationClicked: Invalid scene!";
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  // TODO: Workaround for update issues
  this->mrmlScene()->StartState(vtkMRMLScene::BatchProcessState);

  vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(d->MRMLNodeComboBox_ConvertRepresentationSource->currentNode());
  std::string targetName = std::string(d->lineEdit_TargetContourName->text().toLatin1());

  vtkMRMLContourNode* newContourNode = vtkSlicerContoursModuleLogic::CreateContourFromRepresentation(displayableNode, targetName.empty() ? NULL : targetName.c_str() );

  if (newContourNode == NULL)
  {
    qCritical() << "Creation of new contour node failed.";
  }
  else
  {
    // Make all node connections
    if (!qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("ContourSets")->addNodeToSubjectHierarchy( newContourNode, vtkMRMLSubjectHierarchyNode::SafeDownCast(d->MRMLNodeComboBox_TargetContourSet->currentNode()) ))
    {
      qCritical() << "Unable to connect new contour node <" << newContourNode->GetName() << "> to subject heirarchy.";
    }
    else
    {
      // Reset the name so warnings don't show up
      d->lineEdit_TargetContourName->setText(QString(""));

      // Reset the source node as it is now a representation
      d->MRMLNodeComboBox_ConvertRepresentationSource->setCurrentNodeIndex(-1);

      // Don't show a warning message after a successful operation, it's confusing
      d->label_NoInputWarning->setVisible(false);

      // Do show a nice success message!
      if (vtkMRMLModelNode::SafeDownCast(displayableNode) != NULL)
      {
        d->label_ContourCreatedFromModel->setVisible(true);
      }
      else if (vtkMRMLScalarVolumeNode::SafeDownCast(displayableNode) != NULL)
      {
        d->label_ContourCreatedFromLabelmap->setVisible(true);
      }
      else
      {
        qCritical() << "Unknown representation source type. What did you just do!?";
      }
    }
  }

  // Remove the input node so that only the contour remains
  this->mrmlScene()->RemoveNode(displayableNode);

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

  // convert representation to desired type
  vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
  converter->SetContourNode(contourNode);
  converter->ConvertToRepresentation(vtkMRMLContourNode::IndexedLabelmap);

  if (!contourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
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
  converter->ConvertToRepresentation(vtkMRMLContourNode::ClosedSurfaceModel);

  if (!contourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel))
  {
    vtkErrorWithObjectMacro(contourNode, "convertToClosedSurfaceModel:: Failed to get closed surface model representation from contour node '" << contourNode->GetName() << "' !");
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::testInit()
{
  Q_D(qSlicerContoursModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Filter out hierarchy nodes that are not contour hierarchy nodes
  d->MRMLNodeComboBox_Contour->addAttribute( QString("vtkMRMLSubjectHierarchyNode"), QString(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );

  // Filter out hierarchy nodes that are not contour hierarchy nodes
  d->MRMLNodeComboBox_TargetContourSet->addAttribute( QString("vtkMRMLSubjectHierarchyNode"), QString(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );

  // Filter out non-labelmap nodes
  d->MRMLNodeComboBox_ConvertRepresentationSource->addAttribute( QString("vtkMRMLScalarVolumeNode"), QString(SlicerRtCommon::VOLUME_LABELMAP_IDENTIFIER_ATTRIBUTE_NAME), QVariant(1) );

  // Connect to the external show signal
  qSlicerSubjectHierarchyAbstractPlugin* contourSetsPlugin = qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName(QString("ContourSets"));
  connect( contourSetsPlugin, SIGNAL(createContourFromRepresentationClicked(QString)), this, SLOT(showContourFromRepresentationPanel(QString)) );

  // MRML inputs
  connect( d->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(contourNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_ConvertRepresentationSource, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(sourceRepresentationNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_TargetContourSet, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(targetContourSetNodeChanged(vtkMRMLNode*)) );

  // Input widgets
  connect( d->comboBox_ChangeActiveRepresentation, SIGNAL(currentIndexChanged(int)), this, SLOT(activeRepresentationComboboxSelectionChanged(int)) );
  connect( d->horizontalSlider_OversamplingFactor, SIGNAL(valueChanged(int)), this, SLOT(oversamplingFactorChanged(int)) );
  connect( d->SliderWidget_TargetReductionFactorPercent, SIGNAL(valueChanged(double)), this, SLOT(targetReductionFactorPercentChanged(double)) );
  connect( d->lineEdit_TargetContourName, SIGNAL(textChanged(const QString&)), this, SLOT(targetContourNameChanged(const QString&)) );

  // Buttons
  connect( d->pushButton_ApplyChangeRepresentation, SIGNAL(clicked()), this, SLOT(applyChangeRepresentationClicked()) );
  connect( d->pushButton_CreateContourFromRepresentation, SIGNAL(clicked()), this, SLOT(onCreateContourFromRepresentationClicked()) );
  connect( d->pushButton_ExtractLabelmap, SIGNAL(clicked()), this, SLOT(extractLabelmapClicked()) );

  d->label_NoReferenceWarning->setVisible(false);
  d->label_NewConversion->setVisible(false);
  d->label_NoSourceWarning->setVisible(false);
  d->label_CreatedFromLabelmap->setVisible(false);

  d->label_NoInputWarning->setVisible(false);
  d->label_ContourCreatedFromLabelmap->setVisible(false);
  d->label_ContourCreatedFromModel->setVisible(false);
  d->label_ContourSetContainsContourName->setVisible(false);
}

//-----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* qSlicerContoursModuleWidget::testGetCurrentReferenceVolumeNode()
{
  Q_D(qSlicerContoursModuleWidget);

  return vtkMRMLScalarVolumeNode::SafeDownCast(d->MRMLNodeComboBox_ReferenceVolume->currentNode());
}

//-----------------------------------------------------------------------------
vtkMRMLContourNode* qSlicerContoursModuleWidget::testGetCurrentContourNode()
{
  Q_D(qSlicerContoursModuleWidget);

  return vtkMRMLContourNode::SafeDownCast(d->MRMLNodeComboBox_Contour->currentNode());
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::testSetReferenceVolumeNode( vtkMRMLScalarVolumeNode* node )
{
  Q_D(qSlicerContoursModuleWidget);

  if (node != NULL)
  {
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(QString(node->GetID()));
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::testSetTargetRepresentationType( vtkMRMLContourNode::ContourRepresentationType targetRepresentationType )
{
  Q_D(qSlicerContoursModuleWidget);

  d->comboBox_ChangeActiveRepresentation->setCurrentIndex((int)targetRepresentationType);

  this->activeRepresentationComboboxSelectionChanged((int)targetRepresentationType);
}

//-----------------------------------------------------------------------------
Ui_qSlicerContoursModule* qSlicerContoursModuleWidget::testGetDPointer()
{
  Q_D(qSlicerContoursModuleWidget);

  return d;
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::testSetContourNode( vtkMRMLContourNode* node )
{
  Q_D(qSlicerContoursModuleWidget);

  if (node != NULL)
  {
    d->MRMLNodeComboBox_Contour->setCurrentNodeID(node->GetID());
  }
}

//---------------------------------------------------------------------------
bool qSlicerContoursModuleWidget::GetContoursFromContourSet( vtkMRMLSubjectHierarchyNode* contourSetNode, std::vector< vtkMRMLContourNode* >& outputContourList )
{
  if (contourSetNode == NULL)
  {
    qCritical() << "Invalid structure set node sent to qSlicerContoursModuleWidget::GetContoursFromContourSet";
    return false;
  }
  outputContourList.clear();

  // Grab all contour nodes, get SH node for that contour node, check to see if parent is structure set node
  vtkCollection* contourNodes = this->mrmlScene()->GetNodesByClass("vtkMRMLContourNode");
  for (int i = 0; i < contourNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLContourNode* node = vtkMRMLContourNode::SafeDownCast(contourNodes->GetItemAsObject(i));
    if (node == NULL)
    {
      continue;
    }
    vtkMRMLSubjectHierarchyNode* hierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(node);
    if (hierarchyNode == NULL)
    {
      continue;
    }
    
    if (vtkMRMLSubjectHierarchyNode::GetChildWithName( contourSetNode, hierarchyNode->GetNameWithoutPostfix().c_str() ) != NULL)
    {
      outputContourList.push_back(node);
    }
  }
  contourNodes->Delete();

  return true;
}

//---------------------------------------------------------------------------
std::string qSlicerContoursModuleWidget::buildRepresentationsString( vtkMRMLContourNode* node )
{
  std::vector<std::string> representations;
  std::stringstream ss;
  if (node->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
  {
    representations.push_back("Labelmap");
  }
  if (node->HasRepresentation(vtkMRMLContourNode::RibbonModel))
  {
    representations.push_back("Ribbon model");
  }
  if (node->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel))
  {
    representations.push_back("Closed surface");
  }
  // Create a more descriptive string of the current representations
  for (unsigned int i = 0; i < representations.size(); ++i)
  {
    ss << representations[i];
    if (i != representations.size()-1)
    {
      ss << ", ";
    }
  }
  return ss.str();
}

//---------------------------------------------------------------------------
void qSlicerContoursModuleWidget::extractLabelmapClicked()
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    d->logic()->ExtractLabelmapFromContour(*it);
  }
}
