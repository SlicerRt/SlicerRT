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
#include "qMRMLNodeCombobox.h"

// SlicerRt includes
#include "SlicerRtCommon.h"
#include "vtkSlicerContoursModuleLogic.h"

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
};

//------------------------------------------------------------------------------
qMRMLContourSelectorWidgetPrivate::qMRMLContourSelectorWidgetPrivate(qMRMLContourSelectorWidget& object)
  : q_ptr(&object)
{
  this->RequiredRepresentation = vtkMRMLContourNode::None;
  this->AcceptContourHierarchies = false;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidgetPrivate::init()
{
  Q_Q(qMRMLContourSelectorWidget);
  this->setupUi(q);

  q->setAcceptContourHierarchies(this->AcceptContourHierarchies);

  QObject::connect( this->MRMLNodeComboBox_Contour, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(contourNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MRMLNodeComboBox_ReferenceVolume, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(referenceVolumeNodeChanged(vtkMRMLNode*)) );

  // Disable contour combobox until representation type is not explicitly set
  this->MRMLNodeComboBox_Contour->setEnabled(false);

  // Hide reference related widgets by default
  this->MRMLNodeComboBox_ReferenceVolume->setVisible(false);
  this->label_Reference->setVisible(false);
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
void qMRMLContourSelectorWidget::setRequiredRepresentation(vtkMRMLContourNode::ContourRepresentationType representationType)
{
  Q_D(qMRMLContourSelectorWidget);
  d->RequiredRepresentation = representationType;

  // If the required representation is labelmap or surface, and there is no labelmap representation
  // in the selected contour yet, then show the reference volume selector widget and select the default reference
  if ( d->RequiredRepresentation == vtkMRMLContourNode::IndexedLabelmap)
  {
    d->MRMLNodeComboBox_ReferenceVolume->setVisible(true);
    d->label_Reference->setVisible(true);

    //TODO: select the default reference
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
}

//------------------------------------------------------------------------------
bool qMRMLContourSelectorWidget::acceptContourHierarchies()
{
  Q_D(qMRMLContourSelectorWidget);
  return d->AcceptContourHierarchies;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLContourSelectorWidget);

  // Get contour nodes from selection
  vtkSlicerContoursModuleLogic::GetContourNodesFromSelectedNode(node, d->SelectedContourNodes);

  // Set reference volume
  vtkMRMLScalarVolumeNode* referencedVolume = vtkSlicerContoursModuleLogic::GetReferencedVolumeForContours(d->SelectedContourNodes);
  if (referencedVolume)
  {
    // Set referenced volume and turn off oversampling in selected contours
    d->MRMLNodeComboBox_ReferenceVolume->setCurrentNodeID(referencedVolume->GetID());

    for (std::vector<vtkMRMLContourNode*>::iterator contourIt = d->SelectedContourNodes.begin(); contourIt != d->SelectedContourNodes.end(); ++contourIt)
    {
      (*contourIt)->SetRasterizationReferenceVolumeNodeId(referencedVolume->GetID());
      (*contourIt)->SetRasterizationOversamplingFactor(1.0);
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLContourSelectorWidget);

  if (!node->IsA("vtkMRMLScalarVolumeNode"))
  {
    error
  }

  for (std::vector<vtkMRMLContourNode*>::iterator contourIt = d->SelectedContourNodes.begin(); contourIt != d->SelectedContourNodes.end(); ++contourIt)
  {
    (*contourIt)->SetRasterizationReferenceVolumeNodeId(node->GetID());
  }
}

//------------------------------------------------------------------------------
vtkMRMLContourNode* qMRMLContourSelectorWidget::selectedContourNode()
{
  Q_D(qMRMLContourSelectorWidget);
  return NULL; //TODO
}

