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
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "qMRMLScenePatientHierarchyModel.h"
#include "qMRMLSortFilterPatientHierarchyProxyModel.h"

// VTK includes

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

  // Set up tree view
  d->PatientHierarchyTreeView->setLogic( vtkSlicerPatientHierarchyModuleLogic::SafeDownCast(this->logic()) );
  connect( d->PatientHierarchyTreeView, SIGNAL(currentNodeChanged(vtkMRMLNode*)), d->MRMLNodeAttributeTableWidget, SLOT(setMRMLNode(vtkMRMLNode*)) );

  this->setMRMLIDsVisible(d->DisplayMRMLIDsCheckBox->isChecked());
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPatientHierarchyModuleWidget);

  d->PatientHierarchyTreeView->sortFilterProxyModel()->invalidate();
}

//-----------------------------------------------------------------------------
void qSlicerPatientHierarchyModuleWidget::setMRMLIDsVisible(bool visible)
{
  Q_D(qSlicerPatientHierarchyModuleWidget);

  d->PatientHierarchyTreeView->setColumnHidden(d->PatientHierarchyTreeView->sceneModel()->idColumn(), !visible);
  const int columnCount = d->PatientHierarchyTreeView->header()->count();
  for(int i = 0; i < columnCount; ++i)
  {
    d->PatientHierarchyTreeView->resizeColumnToContents(i);
  }
  d->DisplayMRMLIDsCheckBox->setChecked(visible);
}
