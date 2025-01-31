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
    connect(objectivesDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        this->updateObjectives();
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
    connect(segmentationsDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        this->updateObjectives();
    });

	this->updateObjectives();
    }
 
void qMRMLObjectivesTableWidget::updateObjectives()
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

    // iterate through all rows and add segments to objectiveNodes
    for (int row = 0; row < d->ObjectivesTable->rowCount(); ++row)
    {
        // get selected objective & segment
        QComboBox* objectivesDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("ObjectiveName")));
        QComboBox* segmentationsDropdown = qobject_cast<QComboBox*>(d->ObjectivesTable->cellWidget(row, d->columnIndex("Segments")));

		qSlicerAbstractPlanOptimizer::ObjectiveStruct objective = objectivesDropdown->currentData().value<qSlicerAbstractPlanOptimizer::ObjectiveStruct>();

        // create new objective node and save in optimizer
        vtkMRMLRTObjectiveNode* objectiveNode = vtkMRMLRTObjectiveNode::New();
		objectiveNode->SetName(objective.name.c_str());
        objectiveNode->SetSegmentation(segmentationsDropdown->currentText().toStdString().c_str());


        // Create a widget to hold multiple QLineEdit boxes for parameters
        QWidget* parameterWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(parameterWidget);
        layout->setContentsMargins(0, 0, 0, 0);

		// Add a QLineEdit box for each parameter and add to objectiveNode attributes
        std::map<std::string, std::string> parameters = objective.parameters;
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

        // Add the parameter widget to the table
        int columnIndex = d->columnIndex("Parameters");
        if (columnIndex == -1)
        {
            // Add a new column for the parameters if it doesn't exist
            columnIndex = d->ObjectivesTable->columnCount();
            d->ObjectivesTable->insertColumn(columnIndex);
            d->ObjectivesTable->setHorizontalHeaderItem(columnIndex, new QTableWidgetItem("Parameters"));
            d->ColumnLabels << "Parameters";
        }
        d->ObjectivesTable->setCellWidget(row, columnIndex, parameterWidget);

        // save objective in optimizer
        selectedEngine->saveObjectiveInOptimizer(objectiveNode);

        // Add the objective node to the slicer scene
        qSlicerApplication::application()->mrmlScene()->AddNode(objectiveNode);
    }
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
        d->ObjectivesTable->removeRow(row);
    }
    this->updateObjectives();
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

    // delete all objectives from optimizer
    vtkMRMLRTPlanNode* planNode = d->PlanNode;
    if (!planNode)
    {
        qCritical() << Q_FUNC_INFO << ": Invalid plan node";
        return;
    }
    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
    selectedEngine->removeAllObjectives();

	// Remove all rows
	while (d->ObjectivesTable->rowCount() > 0)
	{
		d->ObjectivesTable->removeRow(0);
	}
}