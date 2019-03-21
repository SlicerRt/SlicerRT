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

// Beams includes
#include "qMRMLBeamsTableView.h"
#include "ui_qMRMLBeamsTableView.h"

#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// VTK includes
#include <vtkWeakPointer.h>

// Qt includes
#include <QDebug>
#include <QKeyEvent>
#include <QStringList>
#include <QPushButton>

// SlicerQt includes
#include "qSlicerApplication.h"

#define ID_PROPERTY "ID"

//-----------------------------------------------------------------------------
class qMRMLBeamsTableViewPrivate: public Ui_qMRMLBeamsTableView
{
  Q_DECLARE_PUBLIC(qMRMLBeamsTableView);

protected:
  qMRMLBeamsTableView* const q_ptr;
public:
  qMRMLBeamsTableViewPrivate(qMRMLBeamsTableView& object);
  void init();

  /// Sets table message and takes care of the visibility of the label
  void setMessage(const QString& message);

  /// Return the column index for a given string, -1 if not a valid header
  int columnIndex(QString label);

  /// Find name item of row corresponding to a beam node ID
  QTableWidgetItem* findItemByBeamNodeID(QString beamNodeID);

public:
  /// RT plan MRML node containing shown beams
  vtkWeakPointer<vtkMRMLRTPlanNode> PlanNode;

private:
  QStringList ColumnLabels;
};

//-----------------------------------------------------------------------------
qMRMLBeamsTableViewPrivate::qMRMLBeamsTableViewPrivate(qMRMLBeamsTableView& object)
  : q_ptr(&object)
{
  this->PlanNode = nullptr;
}

//-----------------------------------------------------------------------------
void qMRMLBeamsTableViewPrivate::init()
{
  Q_Q(qMRMLBeamsTableView);
  this->setupUi(q);

  this->setMessage(QString());

  // Set table header properties
  this->ColumnLabels << "Number" << "Name" << "Gantry" << "Weight" << "Edit" << "Clone";
  this->BeamsTable->setHorizontalHeaderLabels(
    QStringList() << "#" << "Name" << "Gantry" << "Weight" << "" );
  this->BeamsTable->setColumnCount(this->ColumnLabels.size());

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  this->BeamsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
  this->BeamsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
  this->BeamsTable->horizontalHeader()->setStretchLastSection(1);

  // Select rows
  this->BeamsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Make connections
  QObject::connect(this->BeamsTable, SIGNAL(itemChanged(QTableWidgetItem*)),
                   q, SLOT(onBeamTableItemChanged(QTableWidgetItem*)));
  QObject::connect(this->BeamsTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                   q, SIGNAL(selectionChanged(QItemSelection,QItemSelection)));

  this->BeamsTable->installEventFilter(q);
}

//-----------------------------------------------------------------------------
int qMRMLBeamsTableViewPrivate::columnIndex(QString label)
{
  if (!this->ColumnLabels.contains(label))
    {
    qCritical() << Q_FUNC_INFO << ": Invalid column label!";
    return -1;
    }
  return this->ColumnLabels.indexOf(label);
}

//-----------------------------------------------------------------------------
void qMRMLBeamsTableViewPrivate::setMessage(const QString& message)
{
  this->BeamsTableMessageLabel->setVisible(!message.isEmpty());
  this->BeamsTableMessageLabel->setText(message);
}

//-----------------------------------------------------------------------------
QTableWidgetItem* qMRMLBeamsTableViewPrivate::findItemByBeamNodeID(QString beamNodeID)
{
  Q_Q(qMRMLBeamsTableView);
  for (int row=0; row<this->BeamsTable->rowCount(); ++row)
    {
    QTableWidgetItem* item = this->BeamsTable->item(row, this->columnIndex("Name"));
    if (!item)
      {
      return nullptr;
      }
    if (!item->data(q->IDRole).toString().compare(beamNodeID))
      {
      return item;
      }
    }

  return nullptr;
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qMRMLBeamsTableView methods

//-----------------------------------------------------------------------------
qMRMLBeamsTableView::qMRMLBeamsTableView(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLBeamsTableViewPrivate(*this))
{
  Q_D(qMRMLBeamsTableView);
  d->init();
  this->updateBeamTable();
}

//-----------------------------------------------------------------------------
qMRMLBeamsTableView::~qMRMLBeamsTableView()
{
}

//-----------------------------------------------------------------------------
void qMRMLBeamsTableView::setPlanNode(vtkMRMLNode* node)
{
  Q_D(qMRMLBeamsTableView);

  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(node);

  // Connect plan modified events to population of the table
  qvtkReconnect( d->PlanNode, planNode, vtkCommand::ModifiedEvent, this, SLOT( updateBeamTable() ) );

  if (planNode)
  {
    // Connect beam added and removed events
    qvtkReconnect( d->PlanNode, planNode, vtkMRMLRTPlanNode::BeamAdded, this, SLOT( onBeamAdded(vtkObject*,void*) ) );
    qvtkReconnect( d->PlanNode, planNode, vtkMRMLRTPlanNode::BeamRemoved, this, SLOT( onBeamRemoved(vtkObject*,void*) ) );

    // Connect modified events of contained beam nodes to update table
    std::vector<vtkMRMLRTBeamNode*> beams;
    planNode->GetBeams(beams);
    for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
    {
      vtkMRMLRTBeamNode* beamNode = (*beamIt);
      qvtkConnect( beamNode, vtkCommand::ModifiedEvent, this, SLOT( updateBeamTable() ) );
    }
  }

  d->PlanNode = planNode;
  this->updateBeamTable();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLBeamsTableView::planNode()
{
  Q_D(qMRMLBeamsTableView);

  return d->PlanNode;
}

//-----------------------------------------------------------------------------
void qMRMLBeamsTableView::updateBeamTable()
{
  Q_D(qMRMLBeamsTableView);

  d->setMessage(QString());

  // Block signals so that onBeamTableItemChanged function is not called when populating
  d->BeamsTable->blockSignals(true);

  d->BeamsTable->clearContents();

  // Check selection validity
  if (!d->PlanNode)
  {
    d->setMessage(tr("No node is selected"));
    d->BeamsTable->setRowCount(0);
    d->BeamsTable->blockSignals(false);
    return;
  }
  else if (d->PlanNode->GetNumberOfBeams() == 0)
  {
    d->setMessage(tr("Empty plan"));
    d->BeamsTable->setRowCount(0);
    d->BeamsTable->blockSignals(false);
    return;
  }

  // Get beams
  std::vector<vtkMRMLRTBeamNode*> beams;
  d->PlanNode->GetBeams(beams);
  d->BeamsTable->setRowCount(beams.size());
  int row = 0;
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt, ++row)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);

    // Row height is smaller than default (which is 30)
    //d->BeamsTable->setRowHeight(row, 20);

    // Beam number
    QString beamNumber = QString::number(beamNode->GetBeamNumber());
    QTableWidgetItem* numberItem = new QTableWidgetItem(beamNumber);
    numberItem->setData(IDRole, beamNode->GetID());
    d->BeamsTable->setItem(row, d->columnIndex("Number"), numberItem);

    // Beam name
    QString name(beamNode->GetName());
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);
    nameItem->setData(IDRole, beamNode->GetID());
    d->BeamsTable->setItem(row, d->columnIndex("Name"), nameItem);

    // Gantry angle
    QString gantryAngle = QString::number(beamNode->GetGantryAngle());
    QTableWidgetItem* gantryAngleItem = new QTableWidgetItem(gantryAngle);
    gantryAngleItem->setData(IDRole, beamNode->GetID());
    d->BeamsTable->setItem(row, d->columnIndex("Gantry"), gantryAngleItem);

    // Beam weight
    QString beamWeight = QString::number(beamNode->GetBeamWeight());
    QTableWidgetItem* beamWeightItem = new QTableWidgetItem(beamWeight);
    beamWeightItem->setData(IDRole, beamNode->GetID());
    d->BeamsTable->setItem(row, d->columnIndex("Weight"), beamWeightItem);

    // Edit button
    QPushButton* editButton = new QPushButton("Edit");
    editButton->setMaximumWidth(52);
    editButton->setToolTip("Show beam details in Beams module");
    editButton->setProperty(ID_PROPERTY, beamNode->GetID());
    connect(editButton, SIGNAL(clicked()), this, SLOT(onEditButtonClicked()));
    d->BeamsTable->setCellWidget(row, d->columnIndex("Edit"), editButton);

    // Clone button
    QPushButton* cloneButton = new QPushButton("Clone");
    cloneButton->setMaximumWidth(52);
    cloneButton->setToolTip("Create a copy of this beam");
    cloneButton->setProperty(ID_PROPERTY, beamNode->GetID());
    connect(cloneButton, SIGNAL(clicked()), this, SLOT(onCloneButtonClicked()));
    d->BeamsTable->setCellWidget(row, d->columnIndex("Clone"), cloneButton);
}

  // Unblock signals
  d->BeamsTable->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qMRMLBeamsTableView::onBeamTableItemChanged(QTableWidgetItem* changedItem)
{
  Q_D(qMRMLBeamsTableView);

  d->setMessage(QString());

  if (!changedItem || !d->PlanNode || !d->PlanNode->GetScene())
  {
    return;
  }

  // All items contain the beam node ID, get that
  QString beamNodeID = changedItem->data(IDRole).toString();
  // Get beam node from scene
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(
    d->PlanNode->GetScene()->GetNodeByID(beamNodeID.toLatin1().constData()) );
  if (!beamNode)
  {
    qCritical() << Q_FUNC_INFO << ": Beam node with ID '" << beamNodeID << "' not found!";
    return;
  }

  // If beam number has been changed
  bool ok = true;
  if (changedItem->column() == d->columnIndex("Number"))
  {
    int beamNumber = changedItem->text().toInt(&ok);
    if (ok)
    {
      beamNode->SetBeamNumber(beamNumber);
    }
    else
    {
      d->setMessage(tr("Beam number must be an integer number"));
    }
  }
  // If beam name has been changed
  else if (changedItem->column() == d->columnIndex("Name"))
  {
    QString nameText(changedItem->text());
    beamNode->SetName(nameText.toLatin1().constData());
  }
  // If gantry angle has been changed
  else if (changedItem->column() == d->columnIndex("Gantry"))
  {
    double gantryAngle = changedItem->text().toDouble(&ok);
    if (ok && gantryAngle >= 0.0 && gantryAngle < 360.0)
    {
      beamNode->SetGantryAngle(gantryAngle);
    }
    else
    {
      d->setMessage(tr("Gantry angle must be a number"));
    }
  }
  // If beam weight has been changed
  else if (changedItem->column() == d->columnIndex("Weight"))
  {
    double beamWeight = changedItem->text().toDouble(&ok);
    if (ok)
    {
      beamNode->SetBeamWeight(beamWeight);
    }
    else
    {
      d->setMessage(tr("Beam weight must be a number"));
    }
  }
}

//-----------------------------------------------------------------------------
int qMRMLBeamsTableView::beamCount() const
{
  Q_D(const qMRMLBeamsTableView);

  return d->BeamsTable->rowCount();
}

//------------------------------------------------------------------------------
void qMRMLBeamsTableView::onEditButtonClicked()
{
  Q_D(qMRMLBeamsTableView);
  QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
  if (!senderButton || !d->PlanNode || !d->PlanNode->GetScene())
    {
    return;
    }

  // Get beam node from scene
  QString beamNodeID = senderButton->property(ID_PROPERTY).toString();
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(
    d->PlanNode->GetScene()->GetNodeByID(beamNodeID.toLatin1().constData()) );

  // Open Beams module and select beam
  qSlicerApplication::application()->openNodeModule(beamNode);
}

//------------------------------------------------------------------------------
void qMRMLBeamsTableView::onCloneButtonClicked()
{
  Q_D(qMRMLBeamsTableView);
  QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
  if (!senderButton || !d->PlanNode || !d->PlanNode->GetScene())
    {
    return;
    }

  // Get beam node from scene
  QString beamNodeID = senderButton->property(ID_PROPERTY).toString();
  vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(
    d->PlanNode->GetScene()->GetNodeByID(beamNodeID.toLatin1().constData()) );

  // Clone beam node in its parent plan
  beamNode->RequestCloning();
}

//-----------------------------------------------------------------------------
QStringList qMRMLBeamsTableView::selectedBeamNodeIDs()
{
  Q_D(qMRMLBeamsTableView);

  QList<QTableWidgetItem*> selectedItems = d->BeamsTable->selectedItems();
  QStringList selectedBeamNodeIds;
  QSet<int> rows;
  foreach (QTableWidgetItem* item, selectedItems)
    {
    int row = item->row();
    if (!rows.contains(row))
      {
      rows.insert(row);
      selectedBeamNodeIds << item->data(IDRole).toString();
      }
    }

  return selectedBeamNodeIds;
}

//------------------------------------------------------------------------------
bool qMRMLBeamsTableView::eventFilter(QObject* target, QEvent* event)
{
  Q_D(qMRMLBeamsTableView);
  if (target == d->BeamsTable)
  {
    // Prevent giving the focus to the previous/next widget if arrow keys are used
    // at the edge of the table (without this: if the current cell is in the top
    // row and user press the Up key, the focus goes from the table to the previous
    // widget in the tab order)
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
      QAbstractItemModel* model = d->BeamsTable->model();
      QModelIndex currentIndex = d->BeamsTable->currentIndex();

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

//------------------------------------------------------------------------------
void qMRMLBeamsTableView::onBeamAdded(vtkObject* caller, void* callData)
{
  Q_D(qMRMLBeamsTableView);
  Q_UNUSED(caller);

  char* beamNodeId = reinterpret_cast<char*>(callData);
  if (!beamNodeId)
  {
    return;
  }

  if (d->PlanNode)
  {
    vtkMRMLNode* beamNode = d->PlanNode->GetScene()->GetNodeByID(beamNodeId);
    qvtkConnect( beamNode, vtkCommand::ModifiedEvent, this, SLOT( updateBeamTable() ) );
  }
}

//------------------------------------------------------------------------------
void qMRMLBeamsTableView::onBeamRemoved(vtkObject* caller, void* callData)
{
  Q_D(qMRMLBeamsTableView);
  Q_UNUSED(caller);

  char* beamNodeId = reinterpret_cast<char*>(callData);
  if (!beamNodeId)
  {
    return;
  }

  if (d->PlanNode)
  {
    vtkMRMLNode* beamNode = d->PlanNode->GetScene()->GetNodeByID(beamNodeId);
    qvtkDisconnect( beamNode, vtkCommand::ModifiedEvent, this, SLOT( updateBeamTable() ) );
  }
}
