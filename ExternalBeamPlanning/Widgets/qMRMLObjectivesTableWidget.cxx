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

// ExternalBeamPlanning includes
#include "qMRMLObjectivesTableWidget.h"
#include "ui_qMRMLObjectivesTableWidget.h"
#include "qSlicerPlanOptimizerPluginHandler.h"
#include "qSlicerAbstractPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"

// MRML includes
#include <vtkMRMLRTObjectiveNode.h>
#include <vtkMRMLSegmentationNode.h>

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QDebug>
#include <QKeyEvent>
#include <QStringList>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>

// Register vtkSmartPointer<vtkMRMLRTObjectiveNode> with Qt's meta-object system
Q_DECLARE_METATYPE(vtkSmartPointer<vtkMRMLRTObjectiveNode>)
Q_DECLARE_METATYPE(vtkMRMLRTObjectiveNode*)

inline QDebug operator<<(QDebug debug, const std::string& str)
{
  debug.nospace() << QString::fromStdString(str);
  return debug.space();
}

//-----------------------------------------------------------------------------
class qMRMLObjectivesTableWidgetPrivate: public Ui_qMRMLObjectivesTableWidget
{
  Q_DECLARE_PUBLIC(qMRMLObjectivesTableWidget);

protected:
  qMRMLObjectivesTableWidget* const q_ptr;
public:
  qMRMLObjectivesTableWidgetPrivate(qMRMLObjectivesTableWidget& object);
  void init();

  /// Sets table message and takes care of the visibility of the label
  void setMessage(const QString& message);
  
  /// Return the column index for a given string, -1 if not a valid header
  int columnIndex(QString label);

public:
  /// RT plan MRML node
  vtkWeakPointer<vtkMRMLRTPlanNode> PlanNode;
  /// Segmentation MRML node
  vtkWeakPointer<vtkMRMLSegmentationNode> SegmentationNode;

private:
  QStringList ColumnLabels;
};



//-----------------------------------------------------------------------------
qMRMLObjectivesTableWidgetPrivate::qMRMLObjectivesTableWidgetPrivate(qMRMLObjectivesTableWidget& object)
  : q_ptr(&object)
{
  this->PlanNode = nullptr;
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidgetPrivate::init()
{
  Q_Q(qMRMLObjectivesTableWidget);
  this->setupUi(q);

  this->setMessage(QString());

  // Set table header properties
  this->ColumnLabels << "Number" << "ObjectiveName" << "Segments" << "OverlapPriority" << "Penalty" << "Parameters";
  this->ObjectivesTable->setHorizontalHeaderLabels(
    QStringList() << "#" << "Objective" << "Segments" << "OP" << "p" << "Parameters");
  this->ObjectivesTable->setColumnCount(this->ColumnLabels.size());

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  this->ObjectivesTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
  this->ObjectivesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
  this->ObjectivesTable->horizontalHeader()->setStretchLastSection(1);

  // Select rows
  this->ObjectivesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->ObjectivesTable->installEventFilter(q);
}


//-----------------------------------------------------------------------------
int qMRMLObjectivesTableWidgetPrivate::columnIndex(QString label)
{
  if (!this->ColumnLabels.contains(label))
  {
    qCritical() << Q_FUNC_INFO << ": Invalid column label!";
    return -1;
  }
  return this->ColumnLabels.indexOf(label);
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidgetPrivate::setMessage(const QString& message)
{
  this->ObjectivesTableMessageLabel->setVisible(!message.isEmpty());
  this->ObjectivesTableMessageLabel->setText(message);
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qMRMLObjectivesTableWidget methods

//-----------------------------------------------------------------------------
qMRMLObjectivesTableWidget::qMRMLObjectivesTableWidget(QWidget* _parent) //constructor
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLObjectivesTableWidgetPrivate(*this))
{
  Q_D(qMRMLObjectivesTableWidget);
  d->init();
  this->updateObjectivesTable();
}

//-----------------------------------------------------------------------------
qMRMLObjectivesTableWidget::~qMRMLObjectivesTableWidget() = default;


//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::setPlanNode(vtkMRMLNode* node)
{
  Q_D(qMRMLObjectivesTableWidget);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(node);

  // Connect plan modified events to population of the table
  qvtkReconnect(d->PlanNode, planNode, vtkCommand::ModifiedEvent, this, SLOT(updateObjectivesTable()));
  qvtkReconnect(d->PlanNode, planNode, vtkMRMLNode::ReferenceAddedEvent, this, SLOT(onPlanNodeReferenceModified()));
  qvtkReconnect(d->PlanNode, planNode, vtkMRMLNode::ReferenceRemovedEvent, this, SLOT(onPlanNodeReferenceModified()));
  qvtkReconnect(d->PlanNode, planNode, vtkMRMLNode::ReferenceModifiedEvent, this, SLOT(onPlanNodeReferenceModified()));

  // Update the segmentation node observer
  setSegmentationNode(d->PlanNode ? d->PlanNode->GetSegmentationNode() : nullptr);

  // Clear objective table if plan node changed
  if (d->PlanNode != planNode)
  {
    if (d->PlanNode)
    {
      this->deleteObjectivesTable();
    }
    d->PlanNode = planNode;
  }

  this->updateObjectivesTable();
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::setSegmentationNode(vtkMRMLSegmentationNode* node)
{
  Q_D(qMRMLObjectivesTableWidget);

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node);

  if (segmentationNode == d->SegmentationNode)
  {
    return;
  }

  // Connect segment modified event to update of the table
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentAdded,
                this, SLOT(onSegmentAdded(vtkObject*, void*, unsigned long, void*)));
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentRemoved,
                this, SLOT(onSegmentRemoved(vtkObject*, void*, unsigned long, void*)));
  qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentModified,
                this, SLOT(onSegmentModified(vtkObject*, void*, unsigned long, void*)));

  d->SegmentationNode = node;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLObjectivesTableWidget::planNode()
{
  Q_D(qMRMLObjectivesTableWidget);

  return d->PlanNode;
}

//-----------------------------------------------------------------------------
vtkMRMLSegmentationNode* qMRMLObjectivesTableWidget::segmentationNode()
{
  Q_D(qMRMLObjectivesTableWidget);

  return d->SegmentationNode;
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::updateObjectivesTable()
{
  Q_D(qMRMLObjectivesTableWidget);

  d->setMessage(QString());

  // Block Signals so that onPlanReferenceModified is not called when populating
  d->ObjectivesTable->blockSignals(true);
  this->deleteObjectivesTable();

  // Check selection validity
  if (!d->PlanNode)
  {
    d->setMessage("No plan node selected.");
    d->ObjectivesTable->setRowCount(0);
    d->ObjectivesTable->blockSignals(false);
    return;
  }

  qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(d->PlanNode->GetPlanOptimizerName());
  if (!selectedEngine)
  {
    d->setMessage("No valid plan optimizer selected in the plan node.");
    d->ObjectivesTable->setRowCount(0);
    d->ObjectivesTable->blockSignals(false);
    return;
  }

  // Get saved objectives from optimizer and repopulate table
  std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectiveNodes = selectedEngine->getSavedObjectiveNodes();
  for (int row = 0; row < objectiveNodes.size(); ++row)
  {
    this->onObjectiveAdded();
    vtkSmartPointer<vtkMRMLRTObjectiveNode> objectiveNode = objectiveNodes[row];

    // Objective Name
    QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("ObjectiveName")));
    objectivesDropdown->setCurrentText(objectiveNode->GetName());
    
    // Segment
    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));
    vtkMRMLSegmentationNode* segmentationNode = d->PlanNode->GetSegmentationNode();
    if (!segmentationNode)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid segmentation node";
      continue;
    }
    vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
    vtkSegment* segment = segmentation->GetSegment(objectiveNode->GetSegmentID());
    if (!segment)
    {
      qCritical() << Q_FUNC_INFO << ": Segment with ID" << objectiveNode->GetSegmentID() << "not found in segmentation.";
      continue;
    }
    segmentationsDropdown->setCurrentText(segment->GetName());

    // Overlap Priority
    QSpinBox* overlapPrioritySpinBox = qobject_cast<QSpinBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("OverlapPriority")));
    const char* overlapPriorityAttr = objectiveNode->GetAttribute("overlapPriority");
    if (!overlapPriorityAttr)
    {
      qCritical() << Q_FUNC_INFO << ": No overlap priority attribute found in objective node, using default value.";
    }
    unsigned int overlapPriority = overlapPriorityAttr ? atoi(overlapPriorityAttr) : 0;
    if (overlapPriority)
    {
      overlapPrioritySpinBox->setValue(overlapPriority);
    }

    // Penalty
    QSpinBox* penaltySpinBox = qobject_cast<QSpinBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Penalty")));
    const char* penaltyAttr = objectiveNode->GetAttribute("penalty");
    if (!penaltyAttr)
    {
      qCritical() << Q_FUNC_INFO << ": No penalty attribute found in objective node, using default value.";
    }
    unsigned int penalty = penaltyAttr ? atoi(penaltyAttr) : 0;
    if (penalty)
    {
      penaltySpinBox->setValue(penalty);
    }

    // Parameters
    QWidget* parameterWidget = d->ObjectivesTable->cellWidget(row, d->columnIndex("Parameters"));
    if (parameterWidget)
    {
      QBoxLayout* layout = qobject_cast<QBoxLayout*>(parameterWidget->layout());
      if (layout)
      {
        // Iterate over all line edits in the layout (labels are at even indices, line edits at odd indices)
        for (int i = 0; i < layout->count(); i+=2)
        {
          QLabel* parameterLabel = qobject_cast<QLabel*>(layout->itemAt(i)->widget());
          QLineEdit* parameterLineEdit = qobject_cast<QLineEdit*>(layout->itemAt(i+1)->widget());
          if (!parameterLabel || !parameterLineEdit)
          {
            qCritical() << Q_FUNC_INFO << ": No label or line edit found for parameter " << i;
          }
          else
          {
            // Get parameter name from label and find corresponding attribute in objective node
            std::string parameterName = parameterLabel->text().toStdString();
            const char* parameterValue = objectiveNode->GetAttribute(parameterName.c_str());
            if (!parameterValue)
            {
              qCritical() << Q_FUNC_INFO << ": No attribute found in objective node for parameter " << parameterName.c_str();
              continue;
            }
            parameterLineEdit->setText(QString::fromStdString(parameterValue));
          }
        }
      }
    }
  }

  // Unblock signals
  d->ObjectivesTable->blockSignals(false);
}

//------------------------------------------------------------------------------
bool qMRMLObjectivesTableWidget::eventFilter(QObject* target, QEvent* event)
{
  Q_D(qMRMLObjectivesTableWidget);
  if (target == d->ObjectivesTable)
  {
    // Prevent giving the focus to the previous/next widget if arrow keys are used
    // at the edge of the table (without this: if the current cell is in the top
    // row and user press the Up key, the focus goes from the table to the previous
    // widget in the tab order)
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
      QAbstractItemModel* model = d->ObjectivesTable->model();
      QModelIndex currentIndex = d->ObjectivesTable->currentIndex();

      if (model && (
        (keyEvent->key() == Qt::Key_Left && currentIndex.column() == 0)
        || (keyEvent->key() == Qt::Key_Up && currentIndex.row() == 0)
        || (keyEvent->key() == Qt::Key_Right && currentIndex.column() == model->columnCount() - 1)
        || (keyEvent->key() == Qt::Key_Down && currentIndex.row() == model->rowCount() - 1)))
      {
        return true;
      }
    }
  }
  return this->QWidget::eventFilter(target, event);
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onPlanNodeReferenceModified()
{
  Q_D(qMRMLObjectivesTableWidget);

  vtkMRMLRTPlanNode* planNode = d->PlanNode;

  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid plan node";
    return;
  }

  // Check if segmentation node is still valid
  vtkMRMLSegmentationNode* segmentationNode = planNode->GetSegmentationNode();

  if (segmentationNode != d->SegmentationNode)
  {
    // Clear objective table
    this->deleteObjectivesTable();

    // Clear objectives in optimizer
    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
    if (selectedEngine)
    {
      selectedEngine->removeAllObjectiveNodes();
    }

    // Update segmentation node observer
    this->setSegmentationNode(segmentationNode);
  }
}

//-----------------------------------------------------------------------------    
void qMRMLObjectivesTableWidget::onObjectiveAdded()
{
  Q_D(qMRMLObjectivesTableWidget);
  int row = d->ObjectivesTable->rowCount();
  d->ObjectivesTable->insertRow(row);

  QTableWidgetItem* numberItem = new QTableWidgetItem(QString::number(row + 1));
  d->ObjectivesTable->setItem(row, d->columnIndex("Number"), numberItem);

  vtkMRMLRTPlanNode* planNode = d->PlanNode;
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid plan node";
    return;
  }
  qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
  std::vector<qSlicerAbstractPlanOptimizer::ObjectiveStruct> availableObjectives = selectedEngine->getAvailableObjectives();
  if (availableObjectives.empty())
  {
    qCritical() << Q_FUNC_INFO << ": No objectives available for the selected optimizer";
    return;
  }

  // Objectives (Combo Box)
  QComboBox* objectivesDropdown = new QComboBox();
  for (auto objective : availableObjectives)
  {
    QVariant var;
    var.setValue(objective);
    objectivesDropdown->addItem(objective.name, var);
  }
  d->ObjectivesTable->setCellWidget(row, d->columnIndex("ObjectiveName"), objectivesDropdown);
  connect(objectivesDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row]
  {
    this->onObjectiveChanged(row);
  });

  // Segmentations (ComboBox)
  QComboBox* segmentationsDropdown = new QComboBox();

  vtkMRMLSegmentationNode* segmentationNode = planNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentation node";
    return;
  }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  std::vector<std::string> segmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDs(segmentIDs);

  for (const std::string& segmentID : segmentIDs)  
  {  
    vtkSegment* segment = segmentation->GetSegment(segmentID);  
    if (segment)
    {
      QVariant var;
      var.setValue(QString::fromStdString(segmentID));
      segmentationsDropdown->addItem(segment->GetName(), var);
    }
  }

  d->ObjectivesTable->setCellWidget(row, d->columnIndex("Segments"), segmentationsDropdown);
  connect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row]
  {
    this->onSegmentChanged(row);
  });

  this->onObjectiveChanged(row);
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onObjectiveRemoved()
{
  Q_D(qMRMLObjectivesTableWidget);
  QList<QTableWidgetItem*> selectedItems = d->ObjectivesTable->selectedItems();
  QList<int> selectedRows;
  for (QTableWidgetItem* item : selectedItems)
  {
    int row = item->row();
    if (!selectedRows.contains(row))
    {
      selectedRows.append(row);
    }
  }

  for (int row : selectedRows)
  {
    this->removeRowFromRowIndex(row);
  }

  // Update the row numbers in the table
  for (int i = 0; i < d->ObjectivesTable->rowCount(); ++i)
  {
    QTableWidgetItem* numberItem = d->ObjectivesTable->item(i, d->columnIndex("Number"));
    if (numberItem)
    {
      numberItem->setText(QString::number(i + 1));
    }

    // Reconnect signals for dropdowns to the correct row index
    QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(i, d->columnIndex("ObjectiveName")));
    if (objectivesDropdown)
    {
      disconnect(objectivesDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), nullptr, nullptr);
      connect(objectivesDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i]
      {
        this->onObjectiveChanged(i);
      });
    }

    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(i, d->columnIndex("Segments")));
    if (segmentationsDropdown)
    {
      disconnect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), nullptr, nullptr);
      connect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i]
      {
        this->onSegmentChanged(i);
      });
    }
  }

  // Update objectives in optimizer
  this->setObjectivesInPlanOptimizer();
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onSegmentModified(vtkObject* caller, void* callData, unsigned long eventId, void* clientData)
{
  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(caller);
  if (!segNode)
  {
    return;
  }

  const char* segmentID = reinterpret_cast<const char*>(callData);
  if (!segmentID)
  {
    return;
  }

  // Find the segment
  vtkSegment* segment = segNode->GetSegmentation()->GetSegment(segmentID);
  if (!segment)
  {
    return;
  }

  QString segmentIDStr = QString::fromStdString(segmentID);
  QString newSegmentName = QString::fromStdString(segment->GetName());

  // Update the dropdown if the modified segment is used in any of the objectives
  for (int row = 0; row < d_ptr->ObjectivesTable->rowCount(); ++row)
  {
    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d_ptr->ObjectivesTable->cellWidget(row, d_ptr->columnIndex("Segments")));

    // Find the index of the item with this segment ID
    int index = -1;
    for (int i = 0; i < segmentationsDropdown->count(); ++i)
    {
      if (segmentationsDropdown->itemData(i).toString() == segmentIDStr)
      {
        index = i;
        break;
      }
    }

    // Update the segment name in the dropdown
    if (index >= 0)
    {
      segmentationsDropdown->setItemText(index, newSegmentName);
      if (segmentationsDropdown->currentIndex() == index)
      {
        segmentationsDropdown->setCurrentText(newSegmentName);
      }

      // Set new segment name in the objectives saved in optimizer
      this->onSegmentChanged(row);
    }
  }
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onSegmentAdded(vtkObject* caller, void* callData, unsigned long eventId, void* clientData)
{
  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(caller);
  if (!segNode)
  {
    return;
  }

  const char* segmentID = reinterpret_cast<const char*>(callData);
  if (!segmentID)
  {
    return;
  }

  // Find the segment
  vtkSegment* segment = segNode->GetSegmentation()->GetSegment(segmentID);
  if (!segment)
  {
    return;
  }

  QString segmentIDStr = QString::fromStdString(segmentID);
  QString segmentName = QString::fromStdString(segment->GetName());

  // Add the new segment to the dropdown
  for (int row = 0; row < d_ptr->ObjectivesTable->rowCount(); ++row)
  {
    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d_ptr->ObjectivesTable->cellWidget(row, d_ptr->columnIndex("Segments")));

    // Check if the segment is already in the dropdown and add to dropdown if not
    int segmentIndex = segmentationsDropdown->findData(segmentIDStr);
    if (segmentIndex == -1)
    {
      QVariant var;
      var.setValue(segmentIDStr);
      segmentationsDropdown->addItem(segmentName, var);
    }
    else
    {
      continue;
    }
  }
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onSegmentRemoved(vtkObject* caller, void* callData, unsigned long eventId, void* clientData)
{
  vtkMRMLSegmentationNode* segNode = vtkMRMLSegmentationNode::SafeDownCast(caller);
  if (!segNode)
  {
    return;
  }

  const char* segmentID = reinterpret_cast<const char*>(callData);
  if (!segmentID)
  {
    return;
  }

  QString segmentIDStr = QString::fromStdString(segmentID);

  // Remove the new segment from the dropdown
  for (int row = 0; row < d_ptr->ObjectivesTable->rowCount(); ++row)
  {
    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d_ptr->ObjectivesTable->cellWidget(row, d_ptr->columnIndex("Segments")));

    // Check if the segment is in the dropdown and remove
    int segmentIndex = segmentationsDropdown->findData(segmentIDStr);
    if (segmentIndex != -1)
    {
      if (segmentationsDropdown->currentIndex() == segmentIndex)
      {
        segmentationsDropdown->blockSignals(true);
        segmentationsDropdown->setCurrentIndex(-1);
        segmentationsDropdown->blockSignals(false);

        // Set segment ID in objectiveNode to nullptr
        this->currentObjectiveNodes[row]->SetSegmentID(nullptr);
      }

      // Remove segment from dropdown
      segmentationsDropdown->removeItem(segmentationsDropdown->findData(segmentIDStr));
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onObjectiveChanged(int row)
{
  Q_D(qMRMLObjectivesTableWidget);

  if (row < 0 || row >= d->ObjectivesTable->rowCount())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid row index!";
    return;
  }
  vtkMRMLRTPlanNode* planNode = d->PlanNode;
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid plan node";
    return;
  }
  vtkMRMLScene* scene = planNode->GetScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
    return;
  }

  // Get selected objective & segment
  QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("ObjectiveName")));
  QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));

  QString segmentID = segmentationsDropdown->currentData().toString();
  if (segmentID.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Segment ID is empty!";
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = d->PlanNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentation node";
    return;
  }

  qSlicerAbstractPlanOptimizer::ObjectiveStruct objectiveStruct = objectivesDropdown->currentData().value<qSlicerAbstractPlanOptimizer::ObjectiveStruct>();

  // Create objectiveNode
  vtkMRMLRTObjectiveNode* objectiveNode = nullptr;
  if (row >= this->currentObjectiveNodes.size() || this->currentObjectiveNodes.empty())
  {
    // Create and add new objectiveNode
    objectiveNode = vtkMRMLRTObjectiveNode::New();
    scene->AddNode(objectiveNode);
    this->currentObjectiveNodes.push_back(objectiveNode);
  }
  else
  {
    // Reuse existing objectiveNode and delete old attributes
    objectiveNode = this->currentObjectiveNodes[row];
    std::vector<std::string> attrNames = objectiveNode->GetAttributeNames();
    for (const auto& name : attrNames)
    {
      objectiveNode->RemoveAttribute(name.c_str());
    }
  }
  objectiveNode->SetName(objectiveStruct.name.toStdString().c_str());
  objectiveNode->SetSegmentationAndSegmentID(segmentationNode, segmentID.toStdString().c_str());

  // Create overlap priority SpinBox
  int overlapPriorityValue = this->findOverlapPriorityValueOfSegment(row);
  QSpinBox* overlapPrioritySpinBox = new QSpinBox();
  overlapPrioritySpinBox->setValue(overlapPriorityValue);
  overlapPrioritySpinBox->setMinimum(0);
  overlapPrioritySpinBox->setMaximum(10000);
  objectiveNode->SetAttribute("overlapPriority", std::to_string(overlapPrioritySpinBox->value()).c_str());
  connect(overlapPrioritySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, objectiveNode](int newValue)
  {
    this->onOverlapPriorityChanged(newValue, objectiveNode);
  });

  // Create penalty SpinBox
  QSpinBox* penaltySpinBox = new QSpinBox();
  penaltySpinBox->setValue(10);
  penaltySpinBox->setMinimum(0);
  penaltySpinBox->setMaximum(10000);
  objectiveNode->SetAttribute("penalty", std::to_string(penaltySpinBox->value()).c_str());
  connect(penaltySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, objectiveNode](int newValue)
  {
    this->onPenaltyChanged(newValue, objectiveNode);
  });
  
  // Create a widget to hold multiple QLineEdit boxes for parameters
  QWidget* parameterWidget = new QWidget();
  QHBoxLayout* layout = new QHBoxLayout(parameterWidget);
  layout->setContentsMargins(0, 0, 0, 0);

  // Add a QLineEdit box for each parameter and add to objectiveNode attributes
  QMap<QString, QVariant> parameters = objectiveStruct.parameters;
  for (auto item = parameters.cbegin(); item != parameters.cend(); ++item)
  {
    // Get paramters set in objectiveStruct and add to objectiveNode attributes
    std::string parameterName = item.key().toStdString();
    std::string parameterValue = item.value().toString().toStdString();
    objectiveNode->SetAttribute(parameterName.c_str(), parameterValue.c_str());

    // Create QLineEdit box for parameter
    QLabel* parameterLabel = new QLabel(parameterName.c_str());
    QLineEdit* parameterLineEdit = new QLineEdit();
    parameterLineEdit->setText(parameterValue.c_str());

    connect(parameterLineEdit, &QLineEdit::textChanged, this, [this, parameterName, objectiveNode](const QString& newValue)
    {
      this->onParameterChanged(parameterName, newValue.toStdString(), objectiveNode);
    });

    layout->addWidget(parameterLabel);
    layout->addWidget(parameterLineEdit);
  }
  parameterWidget->setLayout(layout);

  // Add overlap priority, penalty & parameter widgets to the table
  d->ObjectivesTable->setCellWidget(row, d->columnIndex("OverlapPriority"), overlapPrioritySpinBox);
  d->ObjectivesTable->setCellWidget(row, d->columnIndex("Penalty"), penaltySpinBox);
  d->ObjectivesTable->setCellWidget(row, d->columnIndex("Parameters"), parameterWidget);

  // Update objectives in optimizer
  this->setObjectivesInPlanOptimizer();
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onSegmentChanged(int row)
{
  Q_D(qMRMLObjectivesTableWidget);

  // Get newly selected segmentation
  QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));
  QString segmentID = segmentationsDropdown->currentData().toString();
  if (segmentID.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment ID!";
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = d->PlanNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentation node";
    return;
  }
  vtkSegment* segment = segmentationNode->GetSegmentation()->GetSegment(segmentID.toStdString());
  if (!segment)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment!";
    return;
  }
  QString segmentName = segment->GetName();

  // Set segmentation in objectiveNode
  this->currentObjectiveNodes[row]->SetSegmentationAndSegmentID(segmentationNode, segmentID.toStdString().c_str());

  // Update objectives in optimizer
  this->setObjectivesInPlanOptimizer();

  // Find other rows with the same segment and update their overlap priority
  int newValue = this->findOverlapPriorityValueOfSegment(row);
  this->updateOverlapPriorityForSegment(segmentName, newValue);
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onOverlapPriorityChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode)
{
  Q_D(qMRMLObjectivesTableWidget);
  
  objectiveNode->SetAttribute("overlapPriority", std::to_string(newValue).c_str());

  vtkMRMLSegmentationNode* segmentationNode = objectiveNode->GetSegmentationNode();
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentation node";
    return;
  }
  vtkSegment* segment = segmentationNode->GetSegmentation()->GetSegment(objectiveNode->GetSegmentID());
  if (!segment)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid segment!";
    return;
  }
  QString segmentName = segment->GetName();

  this->updateOverlapPriorityForSegment(segmentName, newValue);
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onPenaltyChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode)
{
  Q_D(qMRMLObjectivesTableWidget);

  // Update the penalty value
  objectiveNode->SetAttribute("penalty", std::to_string(newValue).c_str());
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onParameterChanged(std::string name, std::string value, vtkMRMLRTObjectiveNode* objectiveNode)
{
  Q_D(qMRMLObjectivesTableWidget);
  
  // Update the attribute value
  objectiveNode->RemoveAttribute(name.c_str());
  objectiveNode->SetAttribute(name.c_str(), value.c_str());
}

//--------------------------------------------------------------------------------
int qMRMLObjectivesTableWidget::findOverlapPriorityValueOfSegment(int row)
{
  Q_D(qMRMLObjectivesTableWidget);
  QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));
  QString segmentName = segmentationsDropdown->currentText();

  // Get default value from segmentation
  vtkSegmentation* segmentation = d->PlanNode->GetSegmentationNode()->GetSegmentation();
  int newValue = segmentation->GetSegmentIndex(segmentation->GetSegmentIdBySegmentName(segmentName.toStdString()));

  // Get overlap priority of first segment with same name
  for (int i = 0; i < d->ObjectivesTable->rowCount(); ++i)
  {
    if (i == row)
    {
      continue;
    }
    else
    {
      QComboBox* currentSegmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(i, d->columnIndex("Segments")));
      if (currentSegmentationsDropdown && currentSegmentationsDropdown->currentText() == segmentName)
      {
        newValue = qobject_cast<QSpinBox*>(d->ObjectivesTable->cellWidget(i, d->columnIndex("OverlapPriority")))->value();
        break;
      }
    }
  }
  return newValue;
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::updateOverlapPriorityForSegment(const QString& segmentName, int newValue)
{
  Q_D(qMRMLObjectivesTableWidget);

  for (int row = 0; row < d->ObjectivesTable->rowCount(); ++row)
  {
    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));
    if (segmentationsDropdown && segmentationsDropdown->currentText() == segmentName)
    {
      QSpinBox* spinBox = qobject_cast<QSpinBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("OverlapPriority")));
      if (spinBox && spinBox->value() != newValue)
      {
        spinBox->blockSignals(true);
        spinBox->setValue(newValue);
        spinBox->blockSignals(false);
      }
      
      // Update the objective node's overlap priority attribute
      vtkMRMLRTObjectiveNode* objectiveNode = this->currentObjectiveNodes[row];
      if (objectiveNode)
      {
        objectiveNode->SetAttribute("overlapPriority", std::to_string(newValue).c_str());
      }
      else
      {
        qCritical() << Q_FUNC_INFO << ": Invalid objective node at row" << row;
      }
    }
  }
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::setObjectivesInPlanOptimizer()
{
  Q_D(qMRMLObjectivesTableWidget);
  vtkMRMLRTPlanNode* planNode = d->PlanNode;
  if (!planNode)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid plan node";
    return;
  }

  // Remove all objectiveNodes from optimizer
  qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
  selectedEngine->removeAllObjectiveNodes();

  // Iterate through objectives in currentObjectiveNodes and add to optimizer
  for (int i = 0; i < this->currentObjectiveNodes.size(); i++)
  {
    vtkMRMLRTObjectiveNode* objectiveNode = this->currentObjectiveNodes[i];
    selectedEngine->saveObjectiveNodeInOptimizer(objectiveNode);
  }
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::removeRowFromRowIndex(int row)
{
  Q_D(qMRMLObjectivesTableWidget);

  // Get scene
  vtkMRMLScene* scene = d->PlanNode->GetScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid scene";
  }

  // Remove the objective node from scene & currentObjectiveNodes
  if (row < this->currentObjectiveNodes.size())
  {
    scene->RemoveNode(this->currentObjectiveNodes[row]);
    this->currentObjectiveNodes[row]->Delete();
    this->currentObjectiveNodes.erase(this->currentObjectiveNodes.begin() + row);
  }

  // Remove row from table
  d->ObjectivesTable->removeRow(row);
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::deleteObjectivesTable()
{
  Q_D(qMRMLObjectivesTableWidget);

  // Remove all rows & delete all objective nodes
  while (d->ObjectivesTable->rowCount() > 0)
  {
    this->removeRowFromRowIndex(0);
  }
}
