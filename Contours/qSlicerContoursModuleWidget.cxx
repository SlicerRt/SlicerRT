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

// Qt includes

// SlicerQt includes
#include "qSlicerContoursModuleWidget.h"
#include "ui_qSlicerContoursModule.h"

// SlicerRtCommon includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"
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
  connect( d->doubleSpinBox_DownsamplingFactor, SIGNAL(valueChanged(double)), this, SLOT(downsamplingFactorChanged(double)) );
  connect( d->SliderWidget_TargetReductionFactor, SIGNAL(valueChanged(double)), this, SLOT(targetReductionFactorChanged(double)) );

  d->label_Warning->setVisible(false);
  d->label_ActiveSelected->setVisible(false);
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
bool qSlicerContoursModuleWidget::isReferenceVolumeNeeded()
{
  Q_D(qSlicerContoursModuleWidget);

  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = this->getRepresentationTypeOfSelectedContours();
  if (!d->SelectedContourNodes.size() || (int)representationTypeInSelectedNodes == d->comboBox_ActiveRepresentation->currentIndex())
  {
    // If the user did not change the representation
    return false;
  }
  else if (representationTypeInSelectedNodes != vtkMRMLContourNode::IndexedLabelmap
    && d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::IndexedLabelmap)
  {
    for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
    {
      if ((*it)->GetIndexedLabelmapVolumeNodeId() == NULL)
      {
        return true;
      }
    }
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
    return;
  }

  d->comboBox_ActiveRepresentation->setEnabled(true);
  d->SelectedContourNodes.clear();

  if (node->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
    if (contourNode)
    {
      d->SelectedContourNodes.push_back(contourNode);
      d->comboBox_ActiveRepresentation->setCurrentIndex((int)contourNode->GetActiveRepresentationType());

      d->doubleSpinBox_DownsamplingFactor->blockSignals(true);
      d->doubleSpinBox_DownsamplingFactor->setValue(contourNode->GetRasterizationDownsamplingFactor());
      d->doubleSpinBox_DownsamplingFactor->blockSignals(false);
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

    // Select the representation type shared by all the children contour nodes
    vtkMRMLContourNode::ContourRepresentationType representationType = this->getRepresentationTypeOfSelectedContours();
    d->comboBox_ActiveRepresentation->blockSignals(true); // Ensure the state si set only once
    if (representationType != vtkMRMLContourNode::None)
    {
      d->comboBox_ActiveRepresentation->setCurrentIndex((int)representationType);

      // Make sure the state is set even if the representation combobox selection has not actually changed
      this->onActiveRepresentationComboboxSelectionChanged((int)representationType);
    }
    else
    {
      d->comboBox_ActiveRepresentation->setCurrentIndex(-1); // Void selection
      this->onActiveRepresentationComboboxSelectionChanged(-1);
    }
    d->comboBox_ActiveRepresentation->blockSignals(false);

    // Get the downsampling factor of the selected contour nodes
    double downsamplingFactor = 0.0;
    bool sameDownsamplingFactor = true;
    for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
    {
      if (downsamplingFactor == 0.0)
      {
        downsamplingFactor = (*it)->GetRasterizationDownsamplingFactor();
      }
      else if (downsamplingFactor != (*it)->GetRasterizationDownsamplingFactor())
      {
        sameDownsamplingFactor = false;
      }
    }
    // Set the downsampling factor on the GUI and also set the first found downsampling factor to all the nodes if they are not the same
    if (sameDownsamplingFactor)
    {
      d->doubleSpinBox_DownsamplingFactor->blockSignals(true);
    }
    d->doubleSpinBox_DownsamplingFactor->setValue(downsamplingFactor);
    d->doubleSpinBox_DownsamplingFactor->blockSignals(false);
  }
  else
  {
    std::cerr << "Error: Invalid node type for ContourNode!" << std::endl;
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::activeRepresentationComboboxSelectionChanged(int index)
{
  this->onActiveRepresentationComboboxSelectionChanged(index);
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::onActiveRepresentationComboboxSelectionChanged(int index)
{
  Q_D(qSlicerContoursModuleWidget);

  d->label_ActiveSelected->setVisible(false);
  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(NULL);
  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);
  d->doubleSpinBox_DownsamplingFactor->setEnabled(false);
  d->label_DownsamplingFactor->setEnabled(false);
  d->pushButton_ApplyChangeRepresentation->setEnabled(false);
  d->label_TargetReductionFactor->setEnabled(false);
  d->SliderWidget_TargetReductionFactor->setEnabled(false);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = this->getRepresentationTypeOfSelectedContours();
  if (!d->SelectedContourNodes.size() || (int)representationTypeInSelectedNodes == index)
  {
    // If the user did not change the representation
    d->label_ReferenceVolume->setEnabled(false);
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);
    if (index != (int)vtkMRMLContourNode::None)
    {
      d->label_ActiveSelected->setVisible(true);
    }
  }
  else if (representationTypeInSelectedNodes != vtkMRMLContourNode::IndexedLabelmap
    && index == (int)vtkMRMLContourNode::IndexedLabelmap)
  {
    // If the active representation of the selected nodes is not labelmap, but the user changed it to labelmap
    bool referenceVolumeNeeded = this->isReferenceVolumeNeeded();
    d->label_Warning->setVisible(referenceVolumeNeeded);
    d->doubleSpinBox_DownsamplingFactor->setEnabled(referenceVolumeNeeded);
    d->label_DownsamplingFactor->setEnabled(referenceVolumeNeeded);
    d->MRMLNodeComboBox_ReferenceVolume->setEnabled(referenceVolumeNeeded);
    d->label_ReferenceVolume->setEnabled(referenceVolumeNeeded);
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
  }
  else if (representationTypeInSelectedNodes == vtkMRMLContourNode::IndexedLabelmap
    && index == (int)vtkMRMLContourNode::ClosedSurfaceModel)
  {
    d->label_TargetReductionFactor->setEnabled(true);
    d->SliderWidget_TargetReductionFactor->setEnabled(true);
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
  }
  else if (index == (int)vtkMRMLContourNode::BitfieldLabelmap)
  {
    // Bitfield labelmap is not supported yet
    d->pushButton_ApplyChangeRepresentation->setEnabled(false);
  }
  else
  {
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
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

  d->label_Warning->setVisible(false);

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (volumeNode)
  {
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
    for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
    {
      (*it)->SetAndObserveRasterizationReferenceVolumeNodeId(volumeNode->GetID());
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::downsamplingFactorChanged(double value)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    (*it)->SetRasterizationDownsamplingFactor(value);
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::targetReductionFactorChanged(double value)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    (*it)->SetDecimationTargetReductionFactor(value);
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

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    if (d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::RibbonModel)
    {
      (*it)->SetActiveRepresentationByType(vtkMRMLContourNode::RibbonModel);
    }
    else if (d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::IndexedLabelmap)
    {
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
    else if (d->comboBox_ActiveRepresentation->currentIndex() == (int)vtkMRMLContourNode::BitfieldLabelmap)
    {
      vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = this->getRepresentationTypeOfSelectedContours();
      d->comboBox_ActiveRepresentation->setCurrentIndex((int)representationTypeInSelectedNodes);
      std::cerr << "Bitfield labelmap representation is not yet supported!" << std::endl;
    }
  }

  // We're done converting, disable controls
  d->MRMLNodeComboBox_ReferenceVolume->setCurrentNode(NULL);
  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);
  d->doubleSpinBox_DownsamplingFactor->setEnabled(false);
  d->label_DownsamplingFactor->setEnabled(false);
  d->pushButton_ApplyChangeRepresentation->setEnabled(false);

  QApplication::restoreOverrideCursor();
}
