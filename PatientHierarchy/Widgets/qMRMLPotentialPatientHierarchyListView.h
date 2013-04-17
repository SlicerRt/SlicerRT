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

#ifndef __qMRMLPotentialPatientHierarchyListView_h
#define __qMRMLPotentialPatientHierarchyListView_h

// Qt includes
#include <QListView>

// PatientHierarchy includes
#include "qSlicerPatientHierarchyModuleWidgetsExport.h"

class qMRMLPotentialPatientHierarchyListViewPrivate;
class vtkMRMLScene;
class qMRMLSortFilterProxyModel;

/// \ingroup Slicer_QtModules_PatientHierarchy
class Q_SLICER_MODULE_PATIENTHIERARCHY_WIDGETS_EXPORT qMRMLPotentialPatientHierarchyListView : public QListView
{
  Q_OBJECT
public:
  qMRMLPotentialPatientHierarchyListView(QWidget *parent=0);
  virtual ~qMRMLPotentialPatientHierarchyListView();

  vtkMRMLScene* mrmlScene()const;

  qMRMLSortFilterProxyModel* sortFilterProxyModel()const;

public slots:
  void setMRMLScene(vtkMRMLScene* scene);

protected:
  QScopedPointer<qMRMLPotentialPatientHierarchyListViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLPotentialPatientHierarchyListView);
  Q_DISABLE_COPY(qMRMLPotentialPatientHierarchyListView);
};

#endif
