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

#ifndef __qMRMLPatientHierarchyTreeView_h
#define __qMRMLPatientHierarchyTreeView_h

// qMRML includes
#include "qMRMLTreeView.h"

// PatientHierarchy includes
#include "qSlicerPatientHierarchyModuleWidgetsExport.h"

class qMRMLPatientHierarchyTreeViewPrivate;
class qMRMLScenePatientHierarchyModel;
class vtkSlicerPatientHierarchyModuleLogic;

/// \ingroup Slicer_QtModules_PatientHierarchy
class Q_SLICER_MODULE_PATIENTHIERARCHY_WIDGETS_EXPORT qMRMLPatientHierarchyTreeView : public qMRMLTreeView
{
  Q_OBJECT

public:
  typedef qMRMLTreeView Superclass;
  qMRMLPatientHierarchyTreeView(QWidget *parent=0);
  virtual ~qMRMLPatientHierarchyTreeView();

  /// Register the logic
  void setLogic(vtkSlicerPatientHierarchyModuleLogic* logic);

protected:
  /// Toggle visibility
  virtual void toggleVisibility(const QModelIndex& index);

protected:
  QScopedPointer<qMRMLPatientHierarchyTreeViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLPatientHierarchyTreeView);
  Q_DISABLE_COPY(qMRMLPatientHierarchyTreeView);
};

#endif
