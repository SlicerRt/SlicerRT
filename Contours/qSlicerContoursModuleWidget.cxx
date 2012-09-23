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
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerContoursModuleWidget);

  bool enabled = true;
  if (!this->mrmlScene() || !node)
  {
    d->comboBox_ActiveRepresentation->setEnabled(false);
    return;
  }

  d->comboBox_ActiveRepresentation->setEnabled(true);

  d->SelectedContourNodes.clear();
  vtkMRMLNode* contourNode = this->mrmlScene()->GetNodeByID(
    d->MRMLNodeComboBox_Contour->currentNodeId().toLatin1() );

  if (contourNode->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(contourNode);
    if (contourNode)
    {
      d->SelectedContourNodes.push_back(contourNode);
    }
  }
  else if (contourNode->IsA("vtkMRMLContourHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLContourHierarchyNode::SafeDownCast(contourNode)->GetChildrenContourNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      std::cerr << "Warning: Selected contour hierarchy node has no children contour nodes!";
      return;
    }

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
    std::cerr << "Error: Invalid node type for ContourNode!";
    return;
  }

  activeRepresentationComboboxSelectionChanged(d->comboBox_ActiveRepresentation->currentIndex());
}

//-----------------------------------------------------------------------------
void qSlicerContoursModuleWidget::activeRepresentationComboboxSelectionChanged(int index)
{
  Q_D(qSlicerContoursModuleWidget);

  if (!this->mrmlScene())
  {
    return;
  }

  if (d->SelectedContourNodes.size() > 1)
  {
    d->pushButton_ApplyChangeRepresentation->setEnabled(true);
  }
  else
  {
    vtkMRMLContourNode::ContourRepresentationType currentIndex
      = (vtkMRMLContourNode::ContourRepresentationType)d->comboBox_ActiveRepresentation->currentIndex();
    if (currentIndex == vtkMRMLContourNode::RibbonModel)
    {
    }
    else if (currentIndex == vtkMRMLContourNode::IndexedLabelmap)
    {
    }
    else if (currentIndex == vtkMRMLContourNode::ClosedSurfaceModel)
    {
    }
    else if (currentIndex == vtkMRMLContourNode::BitfieldLabelmap)
    {
      // Not implemented yet
      d->pushButton_ApplyChangeRepresentation->setEnabled(false);
    }
    else
    {
      std::cerr << "Error: Invalid representation type text!";
    }
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
