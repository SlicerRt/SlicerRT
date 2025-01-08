/*==============================================================================

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

// Objectives includes
#include "qMRMLObjectivesTableWidget.h"
#include "ui_qMRMLObjectivesTableWidget.h"

// Optimization Engine includes
#include "qSlicerPlanOptimizerPluginHandler.h"
#include "qSlicerAbstractPlanOptimizer.h"
#include "qSlicerPlanOptimizerLogic.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"

// MRML includes
#include <vtkMRMLObjectiveNode.h>
#include <vtkMRMLSegmentationNode.h>

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QDebug>
#include <QKeyEvent>
#include <QStringList>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QLineEdit>

// SlicerQt includes
#include "qSlicerApplication.h"

#include <QMetaType>
#include <vtkMRMLObjectiveNode.h>

// Register vtkSmartPointer<vtkMRMLObjectiveNode> with Qt's meta-object system
Q_DECLARE_METATYPE(vtkSmartPointer<vtkMRMLObjectiveNode>)
Q_DECLARE_METATYPE(vtkMRMLObjectiveNode*)

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
   this->ColumnLabels << "Number" << "ObjectiveName" << "Segments";
   this->ObjectivesTable->setHorizontalHeaderLabels(
     QStringList() << "#" << "Objective" << "Segments");
   this->ObjectivesTable->setColumnCount(this->ColumnLabels.size());

 #if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
   this->ObjectivesTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
 #else
   this->ObjectivesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
 #endif
   this->ObjectivesTable->horizontalHeader()->setStretchLastSection(1);

   // Select rows
   this->ObjectivesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

   //// Make connections
   //QObject::connect(this->ObjectivesTable, SIGNAL(itemChanged(QTableWidgetItem*)),
   //                 q, SLOT(onObjectiveTableItemChanged(QTableWidgetItem*)));
   //QObject::connect(this->ObjectivesTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
   //                 q, SIGNAL(selectionChanged(QItemSelection,QItemSelection)));

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
  // this->updateBeamTable();
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

    if (planNode)
    {
        //// Connect beam added and removed events
        //qvtkReconnect(d->PlanNode, planNode, vtkMRMLRTPlanNode::BeamAdded, this, SLOT(onBeamAdded(vtkObject*, void*)));
        //qvtkReconnect(d->PlanNode, planNode, vtkMRMLRTPlanNode::BeamRemoved, this, SLOT(onBeamRemoved(vtkObject*, void*)));

        //// Connect modified events of contained beam nodes to update table
        //std::vector<vtkMRMLRTBeamNode*> beams;
        //planNode->GetBeams(beams);
        //for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
        //{
        //    vtkMRMLRTBeamNode* beamNode = (*beamIt);
        //    qvtkConnect(beamNode, vtkCommand::ModifiedEvent, this, SLOT(updateBeamTable()));
        //}
    }

    d->PlanNode = planNode;
    this->updateObjectivesTable();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLObjectivesTableWidget::planNode()
{
    Q_D(qMRMLObjectivesTableWidget);

    return d->PlanNode;
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
    std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> availableObjectives = selectedEngine->getAvailableObjectives();
    if (availableObjectives.empty())
    {
        qCritical() << Q_FUNC_INFO << ": No objectives available for the selected optimizer";
        return;
    }

    // Objectives (Combo Box)
    QComboBox* objectivesDropdown = new QComboBox();
    QStringList objectives;
    for (auto objective : availableObjectives)
    {
        QVariant var;
        var.setValue(static_cast<void*>(objective.GetPointer()));
        objectivesDropdown->addItem(objective->GetName(), var);
    }
    d->ObjectivesTable->setCellWidget(row, d->columnIndex("ObjectiveName"), objectivesDropdown);


    // Segmentations (List Widget with multi-select)
    QListWidget* segmentationsListWidget = new QListWidget();
    segmentationsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

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
            QListWidgetItem* item = new QListWidgetItem(segment->GetName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            segmentationsListWidget->addItem(item);
        }
    }
    d->ObjectivesTable->setCellWidget(row, d->columnIndex("Segments"), segmentationsListWidget);

	// Adjust row height based on number of segments
	int numVisibleSegments = segmentationsListWidget->count();
	if (numVisibleSegments > 3)
	{
		numVisibleSegments = 3;
	}
	int rowHeight = segmentationsListWidget->sizeHintForRow(0) * numVisibleSegments;
    d->ObjectivesTable->setRowHeight(row, rowHeight);

    connect(segmentationsListWidget, &QListWidget::itemChanged, this, [this, row](QListWidgetItem* item) {
        this->onSegmentationItemChanged(item, row);
    });
}

void qMRMLObjectivesTableWidget::onSegmentationItemChanged(QListWidgetItem* item, int row)
{
    Q_D(qMRMLObjectivesTableWidget);
    Q_UNUSED(item);

    QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("ObjectiveName")));
    vtkMRMLObjectiveNode* objectiveNode = static_cast<vtkMRMLObjectiveNode*>(objectivesDropdown->currentData().value<void*>());

    if (!objectiveNode)
    {
        qCritical() << Q_FUNC_INFO << ": Failed to retrieve objective node";
        return;
    }

    if (item->checkState() == Qt::Checked)
    {
        // The checkbox was checked
        objectiveNode->AddSegmentation(item->text().toStdString());
    }
    else
    {
        // The checkbox was unchecked
        // check if segment selected in another row with the same objective
        if (!isSegmentSelectedElswhere(item->text(), objectiveNode, row))
        {
            objectiveNode->RemoveSegmentation(item->text().toStdString());
        }
        else
        {
            qWarning() << Q_FUNC_INFO << ": Segment" << item->text() << "is still selected in another row for the same objective";
        }
    }
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::checkSegmentationsForObjectives()
{
    Q_D(qMRMLObjectivesTableWidget);

    vtkMRMLRTPlanNode* planNode = d->PlanNode;
    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
    std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> availableObjectives = selectedEngine->getAvailableObjectives();

    for (auto objective : availableObjectives)
    {
        qDebug() << "Objective:" << QString::fromStdString(objective->GetName()) << "has segmentations:" << objective->GetSegmentations();
    }
}

    

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onObjectiveRemoved()
{
    Q_D(qMRMLObjectivesTableWidget);

    // Get the selected row(s)
    QList<QTableWidgetItem*> selectedItems = d->ObjectivesTable->selectedItems();
    QList<int> selectedRows;
    for (QTableWidgetItem* item : selectedItems) {
        int row = item->row();

        if (!selectedRows.contains(row)) {
            selectedRows.append(row);
        }
    }

	// Remove the selected rows and the selected segments from the objectives
    for (int row : selectedRows) {
        QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("ObjectiveName")));
        vtkMRMLObjectiveNode* objectiveNode = static_cast<vtkMRMLObjectiveNode*>(objectivesDropdown->currentData().value<void*>());
        if (!objectiveNode)
        {
            qCritical() << Q_FUNC_INFO << ": Failed to retrieve objective node";
            continue;
        }
        QListWidget* segmentationsListWidget = qobject_cast<QListWidget*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));
        if (!segmentationsListWidget)
        {
            qCritical() << Q_FUNC_INFO << ": Failed to retrieve segmentations list widget";
            continue;
        }
        for (int i = 0; i < segmentationsListWidget->count(); ++i)
        {
            QListWidgetItem* segmentItem = segmentationsListWidget->item(i);
            if (segmentItem->checkState() == Qt::Checked)
            {
				// if segment is not selected in another row, remove from objective
                if (!isSegmentSelectedElswhere(segmentItem->text(), objectiveNode, row))
                {
                    objectiveNode->RemoveSegmentation(segmentItem->text().toStdString());
                }
            }
        }

        // remove the row
        d->ObjectivesTable->removeRow(row);
    }
}

//-----------------------------------------------------------------------------
bool qMRMLObjectivesTableWidget::isSegmentSelectedElswhere(const QString& segementName, vtkMRMLObjectiveNode* objectiveNode, int currentRow)
{
	Q_D(qMRMLObjectivesTableWidget);

	for (int otherRow = 0; otherRow < d->ObjectivesTable->rowCount(); ++otherRow)
	{
		// only itereate through other rows
		if (otherRow == currentRow)
		{
			continue;
		}
		// select rows with the same objective
		QComboBox* otherObjectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(otherRow, d->columnIndex("ObjectiveName")));
		vtkMRMLObjectiveNode* otherObjectiveNode = static_cast<vtkMRMLObjectiveNode*>(otherObjectivesDropdown->currentData().value<void*>());
		if (otherObjectiveNode == objectiveNode)
		{
			// check if the segment is selected in the other row
			QListWidget* otherSegmentationsListWidget = qobject_cast<QListWidget*>(d->ObjectivesTable->cellWidget(otherRow, d->columnIndex("Segments")));
			for (int j = 0; j < otherSegmentationsListWidget->count(); ++j)
			{
				QListWidgetItem* otherSegmentItem = otherSegmentationsListWidget->item(j);
				if (otherSegmentItem->text() == segementName && otherSegmentItem->checkState() == Qt::Checked)
				{
					return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::updateObjectivesTable()
{
	//ToDo: load objectives from plan node
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::deleteObjectivesTable()
{
	Q_D(qMRMLObjectivesTableWidget);

	// Remove all rows
	while (d->ObjectivesTable->rowCount() > 0)
	{
		d->ObjectivesTable->removeRow(0);
	}
}