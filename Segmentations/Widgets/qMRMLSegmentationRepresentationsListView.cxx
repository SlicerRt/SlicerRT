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

// Segmentations includes
#include "qMRMLSegmentationRepresentationsListView.h"
#include "ui_qMRMLSegmentationRepresentationsListView.h"

#include "qMRMLSegmentationConversionParametersWidget.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkSegmentation.h"

// Qt includes
#include <QDebug>
#include <QVariant>
#include <QPushButton>
#include <QDialog>
#include <QMessageBox>

// --------------------------------------------------------------------------
class qMRMLSegmentationRepresentationsListViewPrivate: public Ui_qMRMLSegmentationRepresentationsListView
{
  Q_DECLARE_PUBLIC(qMRMLSegmentationRepresentationsListView);
protected:
  qMRMLSegmentationRepresentationsListView* const q_ptr;
public:
  qMRMLSegmentationRepresentationsListViewPrivate(qMRMLSegmentationRepresentationsListView& object);
  void init();

  /// Sets table message and takes care of the visibility of the label
  void setMessage(const QString& message);

  /// Return the column index for a given string, -1 if not a valid header
  int columnIndex(QString label);

public:
  /// Segmentation MRML node containing shown segments
  vtkMRMLSegmentationNode* SegmentationNode;

private:
  QStringList ColumnLabels;
};

// --------------------------------------------------------------------------
qMRMLSegmentationRepresentationsListViewPrivate::qMRMLSegmentationRepresentationsListViewPrivate(qMRMLSegmentationRepresentationsListView& object)
  : q_ptr(&object)
{
  this->SegmentationNode = NULL;
}

// --------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListViewPrivate::init()
{
  Q_Q(qMRMLSegmentationRepresentationsListView);
  this->setupUi(q);

  this->setMessage(QString());

  // Set up initial look of node representations list
  this->RepresentationsList->setSelectionMode(QAbstractItemView::NoSelection);
  this->RepresentationsList->setStyleSheet( "QListWidget::item { border-bottom: 1px solid lightGray; }" );
}

//-----------------------------------------------------------------------------
int qMRMLSegmentationRepresentationsListViewPrivate::columnIndex(QString label)
{
  return this->ColumnLabels.indexOf(label);
}

// --------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListViewPrivate::setMessage(const QString& message)
{
  this->RepresentationsListMessageLabel->setVisible(!message.isEmpty());
  this->RepresentationsListMessageLabel->setText(message);
}


// --------------------------------------------------------------------------
// qMRMLSegmentationRepresentationsListView methods

// --------------------------------------------------------------------------
qMRMLSegmentationRepresentationsListView::qMRMLSegmentationRepresentationsListView(QWidget* _parent)
  : QWidget(_parent)
  , d_ptr(new qMRMLSegmentationRepresentationsListViewPrivate(*this))
{
  Q_D(qMRMLSegmentationRepresentationsListView);
  d->init();
  this->populateRepresentationsList();
}

// --------------------------------------------------------------------------
qMRMLSegmentationRepresentationsListView::~qMRMLSegmentationRepresentationsListView()
{
}

//-----------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListView::setSegmentationNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentationRepresentationsListView);

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node);

  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::MasterRepresentationModified,
                 this, SLOT( populateRepresentationsList() ) );
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::RepresentationModified,
                 this, SLOT( populateRepresentationsList() ) );
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentModified,
                 this, SLOT( populateRepresentationsList() ) );
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentAdded,
                 this, SLOT( populateRepresentationsList() ) );
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentRemoved,
                 this, SLOT( populateRepresentationsList() ) );

  d->SegmentationNode = segmentationNode;
  this->populateRepresentationsList();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentationRepresentationsListView::segmentationNode()
{
  Q_D(qMRMLSegmentationRepresentationsListView);

  return d->SegmentationNode;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListView::populateRepresentationsList()
{
  Q_D(qMRMLSegmentationRepresentationsListView);

  d->setMessage(QString());

  // Block signals so that onMasterRepresentationChanged function is not called when populating
  d->RepresentationsList->blockSignals(true);

  d->RepresentationsList->clear();

  if (!d->SegmentationNode)
    {
    d->setMessage(tr("No node is selected"));
    d->RepresentationsList->blockSignals(false);
    return;
    }

  // Get available representation names
  std::set<std::string> representationNames;
  vtkSegmentation* segmentation = d->SegmentationNode->GetSegmentation();
  segmentation->GetAvailableRepresentationNames(representationNames);

  int row = 0;
  for (std::set<std::string>::iterator reprIt=representationNames.begin(); reprIt!=representationNames.end(); ++reprIt, ++row)
    {
    QString name(reprIt->c_str());

    QWidget* representationWidget = new QWidget(d->RepresentationsList);
    QHBoxLayout* representationLayout = new QHBoxLayout(representationWidget);
    representationLayout->setContentsMargins(4, 2, 0, 2);
    representationLayout->setSpacing(6);

    QListWidgetItem* representationItem = new QListWidgetItem();
    representationItem->setFlags(representationItem->flags() & ~Qt::ItemIsEditable);
    representationItem->setSizeHint(QSize(-1,26));

    // Representation name
    QLabel* nameLabel = new QLabel(name, representationWidget);
    QFont nameFont;
    nameFont.setWeight(QFont::Bold);
    nameLabel->setFont(nameFont);
    nameLabel->setMinimumWidth(128);
    representationLayout->addWidget(nameLabel);

    // Determine whether current representation is master or is present
    bool master = !name.compare(segmentation->GetMasterRepresentationName());
    bool present = segmentation->ContainsRepresentation(reprIt->c_str());

    // Status
    if (master)
      {
      representationItem->setIcon(QIcon(":/Icons/Master.png"));
      representationItem->setToolTip(tr("This is the master representation.\n  1. This representation is saved on disk\n  2. If this representation is modified, the others are cleared"));
      }
    else if (present)
      {
      representationItem->setIcon(QIcon(":/Icons/Present.png"));
      representationItem->setToolTip(tr("This representation is present"));
      }
    else
      {
      QPixmap emptyPixmap(16, 16);
      emptyPixmap.fill(Qt::transparent);
      QIcon emptyIcon(emptyPixmap);
      representationItem->setIcon(emptyIcon);
      representationItem->setToolTip(tr("This representation is not present"));
      }

    // Action
    if (!master)
      {
      if (present)
        {
        QPushButton* updateButton = new QPushButton(representationWidget);
        updateButton->setText("Update");
        updateButton->setProperty("RepresentationName", QVariant(name));
        updateButton->setMaximumWidth(72);
        updateButton->setMaximumHeight(22);
        updateButton->setMinimumHeight(22);
        QObject::connect(updateButton, SIGNAL(clicked()),
                         this, SLOT(createRepresentationAdvanced()));

        QPushButton* makeMasterButton = new QPushButton(representationWidget);
        makeMasterButton->setText("Make master");
        makeMasterButton->setProperty("RepresentationName", QVariant(name));
        makeMasterButton->setMaximumWidth(72);
        makeMasterButton->setMaximumHeight(22);
        makeMasterButton->setMinimumHeight(22);
        QObject::connect(makeMasterButton, SIGNAL(clicked()),
                         this, SLOT(makeMaster()));

        representationLayout->addWidget(updateButton);
        representationLayout->addWidget(makeMasterButton);
        }
      else
        {
        QLabel* actionLabel = new QLabel();
        actionLabel->setText("Create: ");

        QPushButton* defaultButton = new QPushButton(representationWidget);
        defaultButton->setText("Default");
        defaultButton->setProperty("RepresentationName", QVariant(name));
        defaultButton->setMaximumWidth(72);
        defaultButton->setMaximumHeight(22);
        defaultButton->setMinimumHeight(22);
        QObject::connect(defaultButton, SIGNAL(clicked()),
                         this, SLOT(createRepresentationDefault()));

        QPushButton* advancedButton = new QPushButton(representationWidget);
        advancedButton->setText("Advanced");
        advancedButton->setProperty("RepresentationName", QVariant(name));
        advancedButton->setMaximumWidth(72);
        advancedButton->setMaximumHeight(22);
        advancedButton->setMinimumHeight(22);
        QObject::connect(advancedButton, SIGNAL(clicked()),
                         this, SLOT(createRepresentationAdvanced()));

        representationLayout->addWidget(actionLabel);
        representationLayout->addWidget(defaultButton);
        representationLayout->addWidget(advancedButton);
        }
      }

    representationLayout->addStretch();
    d->RepresentationsList->addItem(representationItem);
    d->RepresentationsList->setItemWidget(representationItem, representationWidget);
    }

  // Unblock signals
  d->RepresentationsList->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListView::createRepresentationDefault()
{
  Q_D(qMRMLSegmentationRepresentationsListView);

  // Get representation name
  QPushButton* actionButton = qobject_cast<QPushButton*>(this->sender());
  QString representationName = actionButton->property("RepresentationName").toString();

  // Perform conversion using cheapest path and default conversion parameters
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  if (!d->SegmentationNode->GetSegmentation()->CreateRepresentation(representationName.toLatin1().constData()))
  {
    QString message = QString("Failed to convert %1 to %2!\n\nProbably there is no valid conversion path between the master representation and %2").arg(d->SegmentationNode->GetName()).arg(representationName);
    QMessageBox::warning(NULL, tr("Conversion failed"), message);
  }
  QApplication::restoreOverrideCursor();

  this->populateRepresentationsList();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListView::createRepresentationAdvanced()
{
  Q_D(qMRMLSegmentationRepresentationsListView);

  // Get representation name
  QPushButton* actionButton = qobject_cast<QPushButton*>(this->sender());
  QString representationName = actionButton->property("RepresentationName").toString();

  // Create dialog to show the parameters widget in a popup window
  QDialog* parametersDialog = new QDialog(NULL, Qt::Dialog);
  parametersDialog->setWindowTitle("Advanced segmentation conversion");
  QVBoxLayout* layout = new QVBoxLayout(parametersDialog);
  parametersDialog->resize(QSize(640,360));
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  // Create conversion parameters widget to allow user editing conversion details
  qMRMLSegmentationConversionParametersWidget* parametersWidget = new qMRMLSegmentationConversionParametersWidget(parametersDialog);
  parametersWidget->setSegmentationNode(d->SegmentationNode);
  parametersWidget->setTargetRepresentationName(representationName);
  layout->addWidget(parametersWidget);

  // Connect conversion done event to dialog close
  QObject::connect(parametersWidget, SIGNAL(conversionDone()),
                   parametersDialog, SLOT(accept()));

  // Show dialog
  parametersDialog->exec();

  // Delete dialog when done
  delete parametersDialog;

  this->populateRepresentationsList();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentationRepresentationsListView::makeMaster()
{
  Q_D(qMRMLSegmentationRepresentationsListView);

  // Get representation name
  QPushButton* button = qobject_cast<QPushButton*>(this->sender());
  QString representationName = button->property("RepresentationName").toString();

  // Warn user about the consequences of changing master representation
  QMessageBox::StandardButton answer =
    QMessageBox::question(NULL, tr("Really change master representation?"),
    tr("Changing master representation will make the 'gold standard' representation the selected one, and will result in deletion of all the other representations.\n"
    "This may mean losing important data that cannot be created again from the new master representation.\n\n"
    "(Reminder: Master representation is the data type which is saved to disk, and which is used as input when creating other representations)\n\n"
    "Do you wish to proceed with changing master representation?"),
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (answer == QMessageBox::Yes)
  {
    d->SegmentationNode->GetSegmentation()->SetMasterRepresentationName(representationName.toLatin1().constData());

    this->populateRepresentationsList();
  }
}
