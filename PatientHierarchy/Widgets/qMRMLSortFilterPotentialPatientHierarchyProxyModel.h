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

#ifndef __qMRMLSortFilterPotentialPatientHierarchyProxyModel_h
#define __qMRMLSortFilterPotentialPatientHierarchyProxyModel_h

// SlicerRt includes
#include "qSlicerPatientHierarchyModuleWidgetsExport.h"

// MRMLWidgets includes
#include "qMRMLSortFilterProxyModel.h"

class qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate;

/// \ingroup Slicer_QtModules_PatientHierarchy
class Q_SLICER_MODULE_PATIENTHIERARCHY_WIDGETS_EXPORT qMRMLSortFilterPotentialPatientHierarchyProxyModel
  : public qMRMLSortFilterProxyModel
{
  Q_OBJECT
public:
  typedef qMRMLSortFilterProxyModel Superclass;
  qMRMLSortFilterPotentialPatientHierarchyProxyModel(QObject *parent=0);
  virtual ~qMRMLSortFilterPotentialPatientHierarchyProxyModel();

protected:
  /// Filters nodes to decide which to display in the view
  virtual AcceptType filterAcceptsNode(vtkMRMLNode* node)const;
  
protected:
  QScopedPointer<qMRMLSortFilterPotentialPatientHierarchyProxyModelPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSortFilterPotentialPatientHierarchyProxyModel);
  Q_DISABLE_COPY(qMRMLSortFilterPotentialPatientHierarchyProxyModel);
};

#endif
