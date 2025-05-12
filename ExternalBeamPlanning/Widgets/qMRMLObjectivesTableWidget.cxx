#include "qMRMLObjectivesTableWidget.h"
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

#include "qSlicerAbstractObjective.h"

// Optimization Engine includes
#include "qSlicerPlanOptimizerPluginHandler.h"
#include "qSlicerAbstractPlanOptimizer.h"
#include "qSlicerPlanOptimizerLogic.h"

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
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QMap>

// SlicerQt includes
#include "qSlicerApplication.h"

#include <QMetaType>
#include <vtkMRMLRTObjectiveNode.h>

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
void qMRMLObjectivesTableWidget::updateObjectivesTable()
{
    Q_D(qMRMLObjectivesTableWidget);

    d->setMessage(QString());

    d->ObjectivesTable->update();
    d->ObjectivesTable->repaint();
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
    QStringList objectives;
    for (auto objective : availableObjectives)
    {
        QVariant var;
        var.setValue(objective);
		objectivesDropdown->addItem(objective.name.c_str(), var);
    }
    d->ObjectivesTable->setCellWidget(row, d->columnIndex("ObjectiveName"), objectivesDropdown);
    connect(objectivesDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row] {
        this->onObjectiveChanged(row);
    });

    // Segmentations (ComboBox)
    QComboBox* segmentationsDropdown = new QComboBox();
    QStringList segmentationsList;

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
            var.setValue(static_cast<void*>(segment));
            segmentationsDropdown->addItem(segment->GetName(), var);
        }
    }
    d->ObjectivesTable->setCellWidget(row, d->columnIndex("Segments"), segmentationsDropdown);
    connect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row] {
        this->onSegmentChanged(row);
    });

    this->onObjectiveChanged(row);

    this->updateObjectivesTable();
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

    // get selected objective & segment
    QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("ObjectiveName")));
    QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));

    qSlicerAbstractPlanOptimizer::ObjectiveStruct objectiveStruct = objectivesDropdown->currentData().value<qSlicerAbstractPlanOptimizer::ObjectiveStruct>();

	// create new objective node
    vtkMRMLRTObjectiveNode* objectiveNode = vtkMRMLRTObjectiveNode::New();
	objectiveNode->SetName(objectiveStruct.name.c_str());
    objectiveNode->SetSegmentation(segmentationsDropdown->currentText().toStdString().c_str());

	// create priority SpinBox
    QSpinBox* prioritySpinBox = new QSpinBox();
    prioritySpinBox->setValue(1);
    prioritySpinBox->setMinimum(0); // Set minimum value (optional)
    prioritySpinBox->setMaximum(10000); // Set maximum value (optional)
	objectiveNode->SetAttribute("Priority", std::to_string(prioritySpinBox->value()).c_str());
    connect(prioritySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, objectiveNode](int newValue) {
		this->onPriorityChanged(newValue, objectiveNode);
	});

    // Create a widget to hold multiple QLineEdit boxes for parameters
    QWidget* parameterWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(parameterWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    // Add a QLineEdit box for each parameter and add to objectiveNode attributes
    std::map<std::string, std::string> parameters = objectiveStruct.parameters;
    for (auto item = parameters.cbegin(); item != parameters.cend(); ++item)
    {
        // get paramters set in objectiveFunction
        std::string parameterName = item->first;
        std::string parameterValue = item->second;

        // add parameter as attribute to objectiveNode
        objectiveNode->SetAttribute(parameterName.c_str(), parameterValue.c_str());

        // create QLineEdit box for parameter
        QLabel* parameterLabel = new QLabel(parameterName.c_str());
        QLineEdit* parameterLineEdit = new QLineEdit();
        parameterLineEdit->setText(parameterValue.c_str());

        connect(parameterLineEdit, &QLineEdit::textChanged, this, [this, parameterName, objectiveNode](const QString& newValue) {
            this->onParameterChanged(parameterName, newValue.toStdString(), objectiveNode);
        });

        layout->addWidget(parameterLabel);
        layout->addWidget(parameterLineEdit);
    }
    parameterWidget->setLayout(layout);

    // Add priority widget and the parameter widget to the table
    int columnIndex = d->columnIndex("Priority");
    if (columnIndex == -1)
    {
		// Add new columns for priority & parameters if they don't exist
        columnIndex = d->ObjectivesTable->columnCount();
        d->ObjectivesTable->insertColumn(columnIndex);
		d->ObjectivesTable->setHorizontalHeaderItem(columnIndex, new QTableWidgetItem("Priority"));
		d->ColumnLabels << "Priority";
        d->ObjectivesTable->insertColumn(columnIndex+1);
        d->ObjectivesTable->setHorizontalHeaderItem(columnIndex+1, new QTableWidgetItem("Parameters"));
        d->ColumnLabels << "Parameters";
    }
	d->ObjectivesTable->setCellWidget(row, columnIndex, prioritySpinBox);
    d->ObjectivesTable->setCellWidget(row, columnIndex+1, parameterWidget);

    // get scene
	vtkMRMLScene* scene = d->PlanNode->GetScene();
	if (!scene)
	{
		qCritical() << Q_FUNC_INFO << ": Invalid scene";
	}

	if (this->currentObjectiveNodes.size() <= row || this->currentObjectiveNodes.empty())
	{
		// Add the new objective node
		this->currentObjectiveNodes.push_back(objectiveNode);
	}
	else
    {
		// update objective node in slicer scene
    	scene->RemoveNode(this->currentObjectiveNodes[row]);
		// replace objective node in currentObjectiveNodes
		this->currentObjectiveNodes[row]->Delete();
		this->currentObjectiveNodes[row] = objectiveNode;
	}


	// add objective node to slicer scene
	scene->AddNode(objectiveNode);

	// update objectives in optimizer
	this->setObjectivesInPlanOptimizer();
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onPriorityChanged(int newValue, vtkMRMLRTObjectiveNode* objectiveNode)
{
	Q_D(qMRMLObjectivesTableWidget);

	// update the priority value
	objectiveNode->SetAttribute("Priority", std::to_string(newValue).c_str());
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onParameterChanged(std::string name, std::string value, vtkMRMLRTObjectiveNode* objectiveNode)
{
    Q_D(qMRMLObjectivesTableWidget);

    // update the attribute value
    objectiveNode->RemoveAttribute(name.c_str());
    objectiveNode->SetAttribute(name.c_str(), value.c_str());
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onSegmentChanged(int row)
{
	Q_D(qMRMLObjectivesTableWidget);

    // get newly selected segmentation
	QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));

	// set segmentation in objectiveNode
	this->currentObjectiveNodes[row]->SetSegmentation(segmentationsDropdown->currentText().toStdString().c_str());

	// update objectives in optimizer
	this->setObjectivesInPlanOptimizer();
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

    // remove all objectiveNodes from optimizer
    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
    selectedEngine->removeAllObjectives();

	// iterate through objectives in currentObjectiveNodes and add to optimizer
	for (int i = 0; i < this->currentObjectiveNodes.size(); i++)
	{
		vtkMRMLRTObjectiveNode* objectiveNode = this->currentObjectiveNodes[i];
		selectedEngine->saveObjectiveInOptimizer(objectiveNode);
	}
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onObjectiveRemoved()
{
    Q_D(qMRMLObjectivesTableWidget);
    QList<QTableWidgetItem*> selectedItems = d->ObjectivesTable->selectedItems();
    QList<int> selectedRows;
    for (QTableWidgetItem* item : selectedItems) {
        int row = item->row();

        if (!selectedRows.contains(row)) {
            selectedRows.append(row);
        }
    }

    for (int row : selectedRows) {
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
            connect(objectivesDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i] {
                this->onObjectiveChanged(i);
            });
        }

        QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(i, d->columnIndex("Segments")));
        if (segmentationsDropdown)
        {
            disconnect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), nullptr, nullptr);
            connect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i] {
                this->onSegmentChanged(i);
            });
        }
    }

    // update objectives in optimizer
    this->setObjectivesInPlanOptimizer();
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::removeRowFromRowIndex(int row)
{
	Q_D(qMRMLObjectivesTableWidget);

	// get scene
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

    // remove row from table
    d->ObjectivesTable->removeRow(row);
}

//-----------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::deleteObjectivesTable()
{
	Q_D(qMRMLObjectivesTableWidget);

    // delete all objectives from optimizer
    vtkMRMLRTPlanNode* planNode = d->PlanNode;
    if (!planNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid plan node";
        return;
    }
    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
    selectedEngine->removeAllObjectives();

	// Remove all rows & delete all objective nodes
	while (d->ObjectivesTable->rowCount() > 0)
	{
		this->removeRowFromRowIndex(0);
	}
}