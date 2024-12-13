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
   this->ColumnLabels << "Number" << "ObjectiveName" << "Parameter";
   this->ObjectivesTable->setHorizontalHeaderLabels(
     QStringList() << "#" << "Objective" << "Parameter");
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

//------------------------------------------------------------------------------
//void qMRMLObjectivesTableWidget::onObjectiveAdded()
//{
//    Q_D(qMRMLObjectivesTableWidget);
//    // Assuming this->ObjectivesTable is your QTableWidget
//    int row = d->ObjectivesTable->rowCount();
//    d->ObjectivesTable->insertRow(row);
//
//    // TODO: adjust numbers when row removed
//    // Create index for new row
//    QTableWidgetItem* numberItem = new QTableWidgetItem(QString::number(row + 1));
//    d->ObjectivesTable->setItem(row, d->columnIndex("Number"), numberItem);
//
//
//    // call available objectives
//    vtkMRMLRTPlanNode* planNode = d->PlanNode;
//    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
//    selectedEngine->setAvailableObjectives();
//    std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> availableObjectives = selectedEngine->getAvailableObjectives();
//
//    // Objectives (dropdown menu)
//    QComboBox* objectivesDropdown = new QComboBox();
//    QStringList objectives;
//    for (auto objective : availableObjectives)
//    {
//        objectives.push_back(objective->GetName());
//    }
//    objectivesDropdown->addItems(objectives);
//    d->ObjectivesTable->setCellWidget(row, 1, objectivesDropdown);


    //// Edit button
    //QPushButton* editButton = new QPushButton("Edit");
    //editButton->setMaximumWidth(52);
    //connect(editButton, SIGNAL(clicked()), this, SLOT(onEditButtonClicked()));
    //d->ObjectivesTable->setCellWidget(row, 2, editButton);

    
void qMRMLObjectivesTableWidget::onObjectiveAdded()
{
    Q_D(qMRMLObjectivesTableWidget);
    int row = d->ObjectivesTable->rowCount();
    d->ObjectivesTable->insertRow(row);

    QTableWidgetItem* numberItem = new QTableWidgetItem(QString::number(row + 1));
    d->ObjectivesTable->setItem(row, d->columnIndex("Number"), numberItem);

    vtkMRMLRTPlanNode* planNode = d->PlanNode;
    qSlicerAbstractPlanOptimizer* selectedEngine = qSlicerPlanOptimizerPluginHandler::instance()->PlanOptimizerByName(planNode->GetPlanOptimizerName());
    selectedEngine->setAvailableObjectives();
    std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> availableObjectives = selectedEngine->getAvailableObjectives();

    QComboBox* objectivesDropdown = new QComboBox();
    QStringList objectives;
    for (auto objective : availableObjectives)
    {
        objectives.push_back(objective->GetName());
    }
    objectivesDropdown->addItems(objectives);
    d->ObjectivesTable->setCellWidget(row, 1, objectivesDropdown);


	// Segmentations (list widget)
    QListWidget* segmentationsListWidget = new QListWidget();
    segmentationsListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    vtkMRMLSegmentationNode* segmentationNode = planNode->GetSegmentationNode();
    vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
    std::vector<std::string> segmentIDs;
    segmentationNode->GetSegmentation()->GetSegmentIDs(segmentIDs);

    for (const std::string& segmentID : segmentIDs)
    {
        vtkSegment* segment = segmentation->GetSegment(segmentID);
        if (segment)
        {
            QListWidgetItem* item = new QListWidgetItem(segment->GetName());
            segmentationsListWidget->addItem(item);
        }
    }

    d->ObjectivesTable->setCellWidget(row, 2, segmentationsListWidget);

    //int rowNumber = row;
    //connect(objectivesDropdown, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, row](int index){
    //this->adjustRowLayout(index, row);
    //});
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
    d->ObjectivesTable->removeRow(row);
    }
  }
}


void qMRMLObjectivesTableWidget::adjustRowLayout(int index, int row)
{
  Q_D(qMRMLObjectivesTableWidget);
  // Clear existing widgets/cells in the row (if any)
  for (int col = 2; col < d->ObjectivesTable->columnCount(); col++) {
    QTableWidgetItem* item = d->ObjectivesTable->item(row, col);
    if (item) {
      delete item;
    }
  }

  // Adjust the number of parameters based on the selected objective
  switch (index) {
    case 0: {// Objective 1 selected
      // Create the parameter item
      QTableWidgetItem* parameterItem0 = new QTableWidgetItem("Parameter 1");
      d->ObjectivesTable->setItem(row, d->columnIndex("Parameter"), parameterItem0);
      break;
    }
    case 1: { // Objective 2 selected
      // Create the parameter items
      QTableWidgetItem* parameterItem10 = new QTableWidgetItem("Parameter 1");
      QTableWidgetItem* parameterItem11 = new QTableWidgetItem("Parameter 2");
      d->ObjectivesTable->setItem(row, d->columnIndex("Parameter"), parameterItem10);
      d->ObjectivesTable->setItem(row, d->columnIndex("Parameter") + 1, parameterItem11);
      break;
    }
    case 2: { // Objective 3 selected
      // Create the parameter items
      QTableWidgetItem* parameterItem20 = new QTableWidgetItem("Parameter 1");
      QTableWidgetItem* parameterItem21 = new QTableWidgetItem("Parameter 2");
      QTableWidgetItem* parameterItem22 = new QTableWidgetItem("Parameter 3");
      d->ObjectivesTable->setItem(row, d->columnIndex("Parameter"), parameterItem20);
      d->ObjectivesTable->setItem(row, d->columnIndex("Parameter") + 1, parameterItem21);
      d->ObjectivesTable->setItem(row, d->columnIndex("Parameter") + 2, parameterItem22);
      break;
    }
    default:
      break;
  }
}

void qMRMLObjectivesTableWidget::updateObjectivesTable()
{
}

//------------------------------------------------------------------------------
void qMRMLObjectivesTableWidget::onEditButtonClicked()
{
    Q_D(qMRMLObjectivesTableWidget);
    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    //if (!senderButton || !d->PlanNode || !d->PlanNode->GetScene())
    //{
    //    return;
    //}

    // Open the module or dialog for adjusting the parameters
    // Example: Open a dialog
    QDialog* dialog = new QDialog();
    dialog->setWindowTitle("Adjust Objective Parameters");
    // Add widgets and layout for adjusting the parameters
    // ...
    dialog->exec();
    delete dialog;
}