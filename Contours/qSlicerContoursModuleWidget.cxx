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
};

//-----------------------------------------------------------------------------
// qSlicerContoursModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerContoursModuleWidgetPrivate::qSlicerContoursModuleWidgetPrivate(qSlicerContoursModuleWidget& object)
  : q_ptr(&object)
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
void qSlicerContoursModuleWidget::setup()
{
  Q_D(qSlicerContoursModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connections
  connect( d->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(contourNodeChanged(vtkMRMLNode*)) );

  connect( d->comboBox_ActiveRepresentation, SIGNAL(currentIndexChanged(int)), this, SLOT(activeRepresentationComboboxSelectionChanged(int)) );
  connect( d->pushButton_ApplyChangeRepresentation, SIGNAL(clicked()), this, SLOT(applyChangeRepresentationClicked()) );
  connect( d->doubleSpinBox_DownsamplingFactor, SIGNAL(valueChanged(double)), this, SLOT(downsamplingFactorChanged(double)) );
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);
  d->label_ReferenceVolume->setEnabled(false);
  d->doubleSpinBox_DownsamplingFactor->setEnabled(false);
  d->label_DownsamplingFactor->setEnabled(false);

  bool enabled = true;
  if (!this->mrmlScene() || !node)
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
    }
  }
  else if (node->IsA("vtkMRMLContourHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLContourHierarchyNode::SafeDownCast(node)->GetChildrenContourNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      std::cerr << "Warning: Selected contour hierarchy node has no children contour nodes!";
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
    vtkMRMLContourNode::ContourRepresentationType representationType = this->GetRepresentationTypeOfSelectedContours();
    if (representationType != vtkMRMLContourNode::None)
    {
      d->comboBox_ActiveRepresentation->setCurrentIndex((int)representationType);
    }
    else
    {
      d->comboBox_ActiveRepresentation->setCurrentIndex(-1); // Void selection
    }
  }
  else
  {
    std::cerr << "Error: Invalid node type for ContourNode!";
    return;
  }
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::activeRepresentationComboboxSelectionChanged(int index)
{
  Q_D(qSlicerContoursModuleWidget);

  d->pushButton_ApplyChangeRepresentation->setEnabled(false);
  d->MRMLNodeComboBox_ReferenceVolume->setEnabled(false);
  d->doubleSpinBox_DownsamplingFactor->setEnabled(false);
  d->label_DownsamplingFactor->setEnabled(false);

  if (!this->mrmlScene())
  {
    return;
  }

  vtkMRMLContourNode::ContourRepresentationType representationTypeInSelectedNodes = this->GetRepresentationTypeOfSelectedContours();
  if (!d->SelectedContourNodes.size() || (int)representationTypeInSelectedNodes == index)
  {
    // If the user did not change the representation
    d->label_ReferenceVolume->setEnabled(false);
  }
  else if (representationTypeInSelectedNodes != vtkMRMLContourNode::IndexedLabelmap
    && index == (int)vtkMRMLContourNode::IndexedLabelmap)
  {
    // If the active representation of the selected nodes is not labelmap, but the user changed it to labelmap
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
    d->MRMLNodeComboBox_ReferenceVolume->setEnabled(true);
    d->doubleSpinBox_DownsamplingFactor->setEnabled(true);
    d->label_DownsamplingFactor->setEnabled(true);
    d->label_ReferenceVolume->setEnabled(true);
  }
  else if (index == (int)vtkMRMLContourNode::BitfieldLabelmap)
  {
    // Bitfield labelmap is not supported yet
    d->label_ReferenceVolume->setEnabled(false);
  }
  else
  {
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
  }
}


//-----------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType qSlicerContoursModuleWidget::GetRepresentationTypeOfSelectedContours()
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
      std::cerr << "Warning: Invalid representation type (None) found for the contour node '" << (*it)->GetName() << "'!";    
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
void qSlicerContoursModuleWidget::downsamplingFactorChanged(double value)
{
  Q_D(qSlicerContoursModuleWidget);

  for (std::vector<vtkMRMLContourNode*>::iterator it = d->SelectedContourNodes.begin(); it != d->SelectedContourNodes.end(); ++it)
  {
    (*it)->SetRasterizationDownsamplingFactor(value);
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

  QApplication::restoreOverrideCursor();
}
