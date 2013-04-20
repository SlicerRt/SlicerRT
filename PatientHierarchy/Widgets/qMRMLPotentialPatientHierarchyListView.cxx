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

// QT includes
#include <QSortFilterProxyModel>


// qMRML includes
#include "qMRMLPotentialPatientHierarchyListView.h"
#include "qMRMLScenePotentialPatientHierarchyModel.h"
#include "qMRMLSortFilterPotentialPatientHierarchyProxyModel.h"

//------------------------------------------------------------------------------
class qMRMLPotentialPatientHierarchyListViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLPotentialPatientHierarchyListView);
protected:
  qMRMLPotentialPatientHierarchyListView* const q_ptr;
public:
  qMRMLPotentialPatientHierarchyListViewPrivate(qMRMLPotentialPatientHierarchyListView& object);
  void init();

  qMRMLSortFilterPotentialPatientHierarchyProxyModel* SortFilterModel;
};

//------------------------------------------------------------------------------
qMRMLPotentialPatientHierarchyListViewPrivate::qMRMLPotentialPatientHierarchyListViewPrivate(qMRMLPotentialPatientHierarchyListView& object)
  : q_ptr(&object)
{
  this->SortFilterModel = NULL;
}

//------------------------------------------------------------------------------
void qMRMLPotentialPatientHierarchyListViewPrivate::init()
{
  Q_Q(qMRMLPotentialPatientHierarchyListView);
  
  qMRMLScenePotentialPatientHierarchyModel* sceneModel = new qMRMLScenePotentialPatientHierarchyModel(q);

  this->SortFilterModel = new qMRMLSortFilterPotentialPatientHierarchyProxyModel(q);
  this->SortFilterModel->setSourceModel(sceneModel);

  q->QListView::setModel(this->SortFilterModel);
}

//------------------------------------------------------------------------------
qMRMLPotentialPatientHierarchyListView::qMRMLPotentialPatientHierarchyListView(QWidget *_parent)
  : QListView(_parent)
  , d_ptr(new qMRMLPotentialPatientHierarchyListViewPrivate(*this))
{
  Q_D(qMRMLPotentialPatientHierarchyListView);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLPotentialPatientHierarchyListView::~qMRMLPotentialPatientHierarchyListView()
{
}

//------------------------------------------------------------------------------
void qMRMLPotentialPatientHierarchyListView::setMRMLScene(vtkMRMLScene* scene)
{
  QSortFilterProxyModel* sortModel = qobject_cast<QSortFilterProxyModel*>(this->model());
  qMRMLSceneModel* mrmlModel = qobject_cast<qMRMLSceneModel*>(sortModel->sourceModel());
  Q_ASSERT(mrmlModel);

  mrmlModel->setMRMLScene(scene);
  if (scene)
  {
    this->setRootIndex(sortModel->index(0, 0));
    sortModel->invalidate();
  }
}

//------------------------------------------------------------------------------
vtkMRMLScene* qMRMLPotentialPatientHierarchyListView::mrmlScene()const
{
  QSortFilterProxyModel* sortModel = qobject_cast<QSortFilterProxyModel*>(this->model());
  Q_ASSERT(qobject_cast<const qMRMLSceneModel*>(sortModel->sourceModel()));
  return qobject_cast<const qMRMLSceneModel*>(sortModel->sourceModel())->mrmlScene();
}

//--------------------------------------------------------------------------
qMRMLSortFilterProxyModel* qMRMLPotentialPatientHierarchyListView::sortFilterProxyModel()const
{
  Q_D(const qMRMLPotentialPatientHierarchyListView);
  Q_ASSERT(d->SortFilterModel);
  return d->SortFilterModel;
}
