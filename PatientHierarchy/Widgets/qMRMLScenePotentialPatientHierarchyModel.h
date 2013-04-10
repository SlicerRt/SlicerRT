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

#ifndef __qMRMLScenePotentialPatientHierarchyModel_h
#define __qMRMLScenePotentialPatientHierarchyModel_h

// SlicerRt includes
#include "qSlicerPatientHierarchyModuleWidgetsExport.h"

// MRMLWidgets includes
#include "qMRMLSceneModel.h"

class qMRMLScenePotentialPatientHierarchyModelPrivate;

/// \ingroup Slicer_QtModules_PatientHierarchy
class Q_SLICER_MODULE_PATIENTHIERARCHY_WIDGETS_EXPORT qMRMLScenePotentialPatientHierarchyModel : public qMRMLSceneModel
{
  Q_OBJECT

public:
  typedef qMRMLSceneModel Superclass;
  qMRMLScenePotentialPatientHierarchyModel(QObject *parent=0);
  virtual ~qMRMLScenePotentialPatientHierarchyModel();

  /// Function returning the supported MIME types
  virtual QStringList mimeTypes()const;

  /// Function encoding the dragged item to MIME data
  virtual QMimeData* mimeData(const QModelIndexList &indexes)const;

protected:
  /// Overridden function to handle tree view item display from node data
  virtual void updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column);

  /// Overridden function to handle node update from tree view item
  virtual void updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item);

private:
  Q_DECLARE_PRIVATE(qMRMLScenePotentialPatientHierarchyModel);
  Q_DISABLE_COPY(qMRMLScenePotentialPatientHierarchyModel);
};

#endif
