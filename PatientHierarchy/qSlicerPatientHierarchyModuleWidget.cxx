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
#include "qSlicerPatientHierarchyModuleWidget.h"
#include "ui_qSlicerPatientHierarchyModule.h"

// SlicerRtCommon includes
#include "SlicerRtCommon.h"

// SlicerRt includes
#include "qMRMLScenePatientHierarchyModel.h"
#include "qMRMLSortFilterPatientHierarchyProxyModel.h"

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_PatientHierarchy
class qSlicerPatientHierarchyModuleWidgetPrivate: public Ui_qSlicerPatientHierarchyModule
{
  Q_DECLARE_PUBLIC(qSlicerPatientHierarchyModuleWidget);
protected:
  qSlicerPatientHierarchyModuleWidget* const q_ptr;
public:
  qSlicerPatientHierarchyModuleWidgetPrivate(qSlicerPatientHierarchyModuleWidget& object);
  ~qSlicerPatientHierarchyModuleWidgetPrivate();
public:
  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  /// Scene model
  qMRMLScenePatientHierarchyModel* SceneModel;
};

//-----------------------------------------------------------------------------
// qSlicerPatientHierarchyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPatientHierarchyModuleWidgetPrivate::qSlicerPatientHierarchyModuleWidgetPrivate(qSlicerPatientHierarchyModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , SceneModel(NULL)
{
}

//-----------------------------------------------------------------------------
qSlicerPatientHierarchyModuleWidgetPrivate::~qSlicerPatientHierarchyModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPatientHierarchyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPatientHierarchyModuleWidget::qSlicerPatientHierarchyModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerPatientHierarchyModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerPatientHierarchyModuleWidget::~qSlicerPatientHierarchyModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    return;
  }

  Q_D(qSlicerPatientHierarchyModuleWidget);

  d->ModuleWindowInitialized = true;

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::setup()
{
  Q_D(qSlicerPatientHierarchyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect( d->DisplayMRMLIDsCheckBox, SIGNAL(toggled(bool)), this, SLOT(setMRMLIDsVisible(bool)) );

  // Set up tree view
  //connect( d->MRMLTreeView, SIGNAL(editNodeRequested(vtkMRMLNode*)), qSlicerApplication::application(), SLOT(openNodeModule(vtkMRMLNode*)));

  // Make connections for the attribute table widget
  connect( d->MRMLTreeView, SIGNAL(currentNodeChanged(vtkMRMLNode*)), d->MRMLNodeAttributeTableWidget, SLOT(setMRMLNode(vtkMRMLNode*)) );

  this->setSceneModel();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPatientHierarchyModuleWidget);

  this->setSceneModel();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::setSceneModel()
{
  if (!this->mrmlScene())
  {
    return;
  }

  Q_D(qSlicerPatientHierarchyModuleWidget);

  // Set scene model
  d->SceneModel = new qMRMLScenePatientHierarchyModel(this);
  qMRMLSortFilterProxyModel* filterModel = new qMRMLSortFilterPatientHierarchyProxyModel(this);
  d->MRMLTreeView->setSceneModel(d->SceneModel, tr("PatientHierarchy"));
  d->MRMLTreeView->setSortFilterProxyModel(filterModel);

  d->MRMLTreeView->header()->setStretchLastSection(false);
  d->MRMLTreeView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  d->MRMLTreeView->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  d->MRMLTreeView->header()->setResizeMode(2, QHeaderView::Stretch);

  this->setMRMLIDsVisible(d->DisplayMRMLIDsCheckBox->isChecked());

  d->MRMLTreeView->sortFilterProxyModel()->invalidate();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::setMRMLIDsVisible(bool visible)
{
  Q_D(qSlicerPatientHierarchyModuleWidget);

  d->MRMLTreeView->setColumnHidden(d->SceneModel->IDColumn(), !visible);
  const int columnCount = d->MRMLTreeView->header()->count();
  for(int i = 0; i < columnCount; ++i)
  {
    d->MRMLTreeView->resizeColumnToContents(i);
  }
  d->DisplayMRMLIDsCheckBox->setChecked(visible);
}
