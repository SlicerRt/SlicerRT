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

//------------------------------------------------------------------------------
class qMRMLContourSelectorWidgetPrivate: public Ui_qMRMLContourSelectorWidget
{
  Q_DECLARE_PUBLIC(qMRMLContourSelectorWidget);
protected:
  qMRMLContourSelectorWidget* const q_ptr;
public:
  qMRMLContourSelectorWidgetPrivate(qMRMLContourSelectorWidget& object);
  void init();

  vtkMRMLContourNode::ContourRepresentationType RequiredRepresentation;
};

//------------------------------------------------------------------------------
qMRMLContourSelectorWidgetPrivate::qMRMLContourSelectorWidgetPrivate(qMRMLContourSelectorWidget& object)
  : q_ptr(&object)
{
  this->RequiredRepresentation = vtkMRMLContourNode::None;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidgetPrivate::init()
{
  Q_Q(qMRMLContourSelectorWidget);
  this->setupUi(q);

  // Show only displayable hierarchies that are contour hierarchies
  this->MRMLNodeComboBox_Contour->addAttribute( QString("vtkMRMLDisplayableHierarchyNode"), QString(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) );

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
}

//------------------------------------------------------------------------------
vtkMRMLContourNode::ContourRepresentationType qMRMLContourSelectorWidget::requiredRepresentation()
{
  Q_D(qMRMLContourSelectorWidget);
  return d->RequiredRepresentation;
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::contourNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLContourSelectorWidget);
}

//------------------------------------------------------------------------------
void qMRMLContourSelectorWidget::referenceVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLContourSelectorWidget);
}
