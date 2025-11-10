/*==============================================================================

  Copyright (c) German Cancer Research Center (DKFZ),
  Heidelberg, Germany. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Lina Bucher, German Cancer
  Research Center (DKFZ) and Institute for Biomedical Engineering (IBT),
  Karlsruhe Institute of Technology (KIT).

==============================================================================*/

#ifndef __qMRMLObjectivesTableWidget_h
#define __qMRMLObjectivesTableWidget_h

// ExternalBeamPlanning includes
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

  /// Get segmentation MRML node
  Q_INVOKABLE vtkMRMLSegmentationNode* segmentationNode();

public slots:
  /// Set plan MRML node
  Q_INVOKABLE void setPlanNode(vtkMRMLNode* node);

  /// Set segmentation MRML node
  Q_INVOKABLE void setSegmentationNode(vtkMRMLSegmentationNode* node);

  bool eventFilter(QObject* target, QEvent* event);

  /// Called when plan node reference is modified in an observed plan node
  void onPlanNodeReferenceModified();

  /// Called when objective is added in an observed plan node
  void onObjectiveAdded();

  /// Called when objective is removed in an observed plan node
  void onObjectiveRemoved();

  /// Called when segment is modified in an observed segmentation node
  void onSegmentModified(vtkObject* caller, void* callData, unsigned long eventId, void* clientData);

  /// Called when segment is added in an observed segmentation node
  void onSegmentAdded(vtkObject* caller, void* callData, unsigned long eventId, void* clientData);

  /// Called when segment is removed in an observed segmentation node
  void onSegmentRemoved(vtkObject* caller, void* callData, unsigned long eventId, void* clientData);

  /// Called when selected objective is changed or newly added to the table
  void onObjectiveChanged(int row);

  /// Called when selected segment is changed in the table and updates segment name in objectiveNode
  void onSegmentChanged(int row);

  /// Called when overlap priority is changed in the table, updates value in objectiveNode's attributes
  /// and calls updateOverlapPriorityForSegment
  void onOverlapPriorityChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode);

  /// Called when penalty is changed in table and updates value in objectiveNode's attributes
  void onPenaltyChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode);

  /// Called when parameter is changed in table and updates value in objectiveNode's attributes
  void onParameterChanged(std::string name, std::string value, vtkMRMLRTObjectiveNode* objectiveNode);

  /// Finds overlap priority of the same segment already present in the table and otherwise uses
  /// default value (0:target, 50:non-target)
  int findOverlapPriorityValueOfSegment(int row);

  /// Updates the overlap priority for all rows with the given segment ID
  void updateOverlapPriorityForSegment(const QString& segmentID, int newValue);

  /// Updates the objectiveNodes in the plan optimizer according to the current objectives in the table
  void setObjectivesInPlanOptimizer();

  /// Removes row from table and its objective from currentObjectiveNodes and scene
  void removeRowFromRowIndex(int row);

  /// Deletes objectives table
  void deleteObjectivesTable();

protected:
  // Keeps track of current objectives in table
  std::vector<vtkMRMLRTObjectiveNode*> currentObjectiveNodes;

protected slots:
  /// Update objectives table
  void updateObjectivesTable();


protected:
  QScopedPointer<qMRMLObjectivesTableWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLObjectivesTableWidget);
  Q_DISABLE_COPY(qMRMLObjectivesTableWidget);
};

#endif
