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
#include "qSlicerSubjectHierarchyModuleWidget.h"
#include "ui_qSlicerSubjectHierarchyModule.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "qMRMLSceneSubjectHierarchyModel.h"
#include "qMRMLSortFilterSubjectHierarchyProxyModel.h"

// MRML includes
#include <vtkMRMLScene.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SubjectHierarchy
class qSlicerSubjectHierarchyModuleWidgetPrivate: public Ui_qSlicerSubjectHierarchyModule
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchyModuleWidget);
protected:
  qSlicerSubjectHierarchyModuleWidget* const q_ptr;
public:
  qSlicerSubjectHierarchyModuleWidgetPrivate(qSlicerSubjectHierarchyModuleWidget& object);
  ~qSlicerSubjectHierarchyModuleWidgetPrivate();
  vtkSlicerSubjectHierarchyModuleLogic* logic() const;
public:
  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModuleWidgetPrivate::qSlicerSubjectHierarchyModuleWidgetPrivate(qSlicerSubjectHierarchyModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModuleWidgetPrivate::~qSlicerSubjectHierarchyModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerSubjectHierarchyModuleLogic*
qSlicerSubjectHierarchyModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerSubjectHierarchyModuleWidget);
  return vtkSlicerSubjectHierarchyModuleLogic::SafeDownCast(q->logic());
} 


//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchyModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModuleWidget::qSlicerSubjectHierarchyModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSubjectHierarchyModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchyModuleWidget::~qSlicerSubjectHierarchyModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidget::enter()
{
  this->onEnter();
  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidget::exit()
{
  this->Superclass::exit();

  Q_D(qSlicerSubjectHierarchyModuleWidget);
  d->SubjectHierarchyTreeView->setMRMLScene(NULL);
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidget::onEnter()
{
  if (!this->mrmlScene())
  {
    return;
  }

  Q_D(qSlicerSubjectHierarchyModuleWidget);

  d->ModuleWindowInitialized = true;
  d->SubjectHierarchyTreeView->setMRMLScene(this->mrmlScene());

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidget::setup()
{
  Q_D(qSlicerSubjectHierarchyModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  // Make connection for the Show MRML IDs checkbox
  connect( d->DisplayMRMLIDsCheckBox, SIGNAL(toggled(bool)), this, SLOT(setMRMLIDsVisible(bool)) );

  // Set up tree view
  d->SubjectHierarchyTreeView->expandToDepth(4);
  connect( d->SubjectHierarchyTreeView, SIGNAL(currentNodeChanged(vtkMRMLNode*)), d->MRMLNodeAttributeTableWidget, SLOT(setMRMLNode(vtkMRMLNode*)) );

  // Connect subject hierarchy tree with potential nodes list
  connect( d->SubjectHierarchyTreeView->sceneModel(), SIGNAL(updateRelatedWidgets()), d->PotentialSubjectHierarchyListView->model(), SLOT(invalidate()) );

  this->setMRMLIDsVisible(d->DisplayMRMLIDsCheckBox->isChecked());
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerSubjectHierarchyModuleWidget);

  //d->SubjectHierarchyTreeView->sortFilterProxyModel()->invalidate();
  qMRMLSceneSubjectHierarchyModel* sceneModel = (qMRMLSceneSubjectHierarchyModel*)d->SubjectHierarchyTreeView->sceneModel();
  sceneModel->forceUpdateScene();
  d->SubjectHierarchyTreeView->expandToDepth(4);

  d->PotentialSubjectHierarchyListView->sortFilterProxyModel()->invalidate();
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchyModuleWidget::setMRMLIDsVisible(bool visible)
{
  Q_D(qSlicerSubjectHierarchyModuleWidget);

  d->SubjectHierarchyTreeView->setColumnHidden(d->SubjectHierarchyTreeView->sceneModel()->idColumn(), !visible);
  const int columnCount = d->SubjectHierarchyTreeView->header()->count();
  for(int i = 0; i < columnCount; ++i)
  {
    d->SubjectHierarchyTreeView->resizeColumnToContents(i);
  }
  d->DisplayMRMLIDsCheckBox->setChecked(visible);
}
