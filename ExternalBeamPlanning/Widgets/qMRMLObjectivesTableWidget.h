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

// CTK includes
#include <ctkVTKObject.h>

// MRML includes
#include <vtkMRMLRTObjectiveNode.h>

class qMRMLObjectivesTableWidgetPrivate;
class vtkMRMLNode;

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

  bool eventFilter(QObject* target, QEvent* event);

  /// Called when objective is added in an observed plan node
  void onObjectiveAdded();

  /// Called when objective is removed in an observed plan node
  void onObjectiveRemoved();

  void removeRowFromRowIndex(int row);

  void onObjectiveChanged(int row);

  void onSegmentChanged(int row);

  void onOverlapPriorityChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode);

  void onPenaltyChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode);

  void onParameterChanged(std::string name, std::string value, vtkMRMLRTObjectiveNode* objectiveNode);

  int findOverlapPriorityValueOfSegment(int row);

  void updateOverlapPriorityForSegment(const QString& segmentName, int newValue);

  void setObjectivesInPlanOptimizer();

  /// delete objectives Table and removes all objectives from plan optimizer
  void deleteObjectivesTable();

protected:
  std::vector<vtkMRMLRTObjectiveNode*> currentObjectiveNodes;

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
