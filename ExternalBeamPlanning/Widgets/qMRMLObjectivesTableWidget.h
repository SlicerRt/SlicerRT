/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

#ifndef __qMRMLObjectivesTableWidget_h
#define __qMRMLObjectivesTableWidget_h

// External Beam Planning includes
#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// MRMLWidgets includes
#include "qMRMLWidget.h"
#include <qlistwidget.h>

// CTK includes
//#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// MRML includes
#include <vtkMRMLObjectiveNode.h>

class qMRMLObjectivesTableWidgetPrivate;
class vtkMRMLNode;
//class QTableWidget;
class QTableWidgetItem;
class QItemSelection;

class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qMRMLObjectivesTableWidget : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qMRMLWidget Superclass;
  /// Constructor
  explicit qMRMLObjectivesTableWidget(QWidget* parent = nullptr);
  /// Destructor
  ~qMRMLObjectivesTableWidget() override;

  /// Get plan MRML node
  Q_INVOKABLE vtkMRMLNode* planNode();


public slots:
  /// Set plan MRML node
  Q_INVOKABLE void setPlanNode(vtkMRMLNode* node);

  /// Called when objective is added in an observed plan node
  void onObjectiveAdded();

  void onSegmentationItemChanged(QListWidgetItem* item, int row);

  void checkSegmentationsForObjectives();

  /// Called when objective is removed in an observed plan node
  void onObjectiveRemoved();

  bool isSegmentSelectedElswhere(const QString& segementName, vtkMRMLObjectiveNode* objectiveNode, int currentRow);

//signals:
//
//
protected slots:
  /// Update beam table according to the plan node
  void updateObjectivesTable();


protected:
   QScopedPointer<qMRMLObjectivesTableWidgetPrivate> d_ptr;

private:
   Q_DECLARE_PRIVATE(qMRMLObjectivesTableWidget);
   Q_DISABLE_COPY(qMRMLObjectivesTableWidget);
};

#endif
