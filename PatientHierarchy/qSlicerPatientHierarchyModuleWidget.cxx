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
};

//-----------------------------------------------------------------------------
// qSlicerPatientHierarchyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPatientHierarchyModuleWidgetPrivate::qSlicerPatientHierarchyModuleWidgetPrivate(qSlicerPatientHierarchyModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
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
  connect( d->ShowHiddenCheckBox, SIGNAL(toggled(bool)), d->MRMLTreeView->sortFilterProxyModel(), SLOT(setShowHidden(bool)) );

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
  qMRMLSceneModel* sceneModel = new qMRMLScenePatientHierarchyModel(this);
  qMRMLSortFilterProxyModel* filterModel = new qMRMLSortFilterPatientHierarchyProxyModel(this);
  d->MRMLTreeView->setSceneModel(sceneModel, tr("PatientHierarchy"));
  d->MRMLTreeView->setSortFilterProxyModel(filterModel);

  d->MRMLTreeView->sceneModel()->setIDColumn(1);
  d->MRMLTreeView->sceneModel()->setHorizontalHeaderLabels(
    QStringList() << "Nodes" << "IDs");

  d->MRMLTreeView->header()->setStretchLastSection(false);
  d->MRMLTreeView->header()->setResizeMode(0, QHeaderView::Stretch);
  d->MRMLTreeView->header()->setResizeMode(1, QHeaderView::ResizeToContents);

  this->setMRMLIDsVisible(d->DisplayMRMLIDsCheckBox->isChecked());

  connect(d->ShowHiddenCheckBox, SIGNAL(toggled(bool)),
    d->MRMLTreeView->sortFilterProxyModel(), SLOT(setShowHidden(bool)));

  d->MRMLTreeView->sortFilterProxyModel()->invalidate();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::setMRMLIDsVisible(bool visible)
{
  Q_D(qSlicerPatientHierarchyModuleWidget);

  d->MRMLTreeView->setColumnHidden(1, !visible);
  const int columnCount = d->MRMLTreeView->header()->count();
  for(int i = 0; i < columnCount; ++i)
  {
    d->MRMLTreeView->resizeColumnToContents(i);
  }
  d->DisplayMRMLIDsCheckBox->setChecked(visible);
}
