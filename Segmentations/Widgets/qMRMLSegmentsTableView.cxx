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
#include "qMRMLSegmentsTableView.h"
#include "ui_qMRMLSegmentsTableView.h"

#include "qMRMLDoubleSpinBoxDelegate.h"

#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkSegmentation.h"
#include "vtkSegment.h"

// MRML includes
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLModelNode.h>

// Qt includes
#include <QStringList>
#include <QDebug>

// qMRML includes
#include "qMRMLItemDelegate.h"

//-----------------------------------------------------------------------------
class qMRMLSegmentsTableViewPrivate: public Ui_qMRMLSegmentsTableView
{
  Q_DECLARE_PUBLIC(qMRMLSegmentsTableView);

protected:
  qMRMLSegmentsTableView* const q_ptr;
public:
  qMRMLSegmentsTableViewPrivate(qMRMLSegmentsTableView& object);
  void init();

  /// Sets table message and takes care of the visibility of the label
  void setMessage(const QString& message);

  /// Return the column index for a given string, -1 if not a valid header
  int columnIndex(QString label);

  /// Find name item of row corresponding to a segment ID
  QTableWidgetItem* findItemBySegmentID(QString segmentID);

public:
  /// Segmentation MRML node containing shown segments
  vtkMRMLSegmentationNode* SegmentationNode;

  /// Model or labelmap volume MRML node containing a representation (for import/export)
  vtkMRMLDisplayableNode* RepresentationNode;

  /// Mode of segment table. See modes \sa SegmentTableMode
  qMRMLSegmentsTableView::SegmentTableMode Mode;

private:
  QStringList ColumnLabels;
};

//-----------------------------------------------------------------------------
qMRMLSegmentsTableViewPrivate::qMRMLSegmentsTableViewPrivate(qMRMLSegmentsTableView& object)
  : q_ptr(&object)
{
  this->SegmentationNode = NULL;
  this->RepresentationNode = NULL;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableViewPrivate::init()
{
  Q_Q(qMRMLSegmentsTableView);
  this->setupUi(q);

  this->setMessage(QString());

  // Set table header properties
  this->ColumnLabels << "Visible3D" << "Visible2DFill" << "Visible2DOutline" << "Color" << "Opacity" << "Name";
  this->SegmentsTable->setColumnCount(this->ColumnLabels.size());

  this->SegmentsTable->horizontalHeaderItem(
    this->columnIndex("Visible3D"))->setIcon(QIcon(":/Icons/Small/SlicerModels.png") );
  this->SegmentsTable->horizontalHeaderItem(
    this->columnIndex("Visible2DFill"))->setIcon(QIcon(":/Icons/SlicesLabelNoOutline.png") );
  this->SegmentsTable->horizontalHeaderItem(
    this->columnIndex("Visible2DOutline"))->setIcon(QIcon(":/Icons/SlicesLabelOutline.png") );

  this->SegmentsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  this->SegmentsTable->horizontalHeader()->setStretchLastSection(1);

  // Select rows
  this->SegmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Make connections
  QObject::connect(this->SegmentsTable, SIGNAL(itemChanged(QTableWidgetItem*)),
                   q, SLOT(onSegmentTableItemChanged(QTableWidgetItem*)));
  QObject::connect(this->SegmentsTable, SIGNAL(itemClicked(QTableWidgetItem*)),
                   q, SLOT(onSegmentTableItemClicked(QTableWidgetItem*)));
  QObject::connect(this->SegmentsTable->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                   q, SIGNAL(selectionChanged(QItemSelection,QItemSelection)));

  // Set item delegate to handle color and opacity changes
  qMRMLItemDelegate* itemDelegate = new qMRMLItemDelegate(this->SegmentsTable);
  this->SegmentsTable->setItemDelegateForColumn(this->columnIndex("Color"), itemDelegate);
  //this->SegmentsTable->setItemDelegateForColumn(this->columnIndex("Opacity"), itemDelegate);
  this->SegmentsTable->setItemDelegateForColumn(this->columnIndex("Opacity"), new qMRMLDoubleSpinBoxDelegate(this->SegmentsTable));
}

//-----------------------------------------------------------------------------
int qMRMLSegmentsTableViewPrivate::columnIndex(QString label)
{
  if (!this->ColumnLabels.contains(label))
  {
    qCritical() << "qMRMLSegmentsTableViewPrivate::columnIndex: Invalid column label!";
    return -1;
  }
  return this->ColumnLabels.indexOf(label);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableViewPrivate::setMessage(const QString& message)
{
  this->SegmentsTableMessageLabel->setVisible(!message.isEmpty());
  this->SegmentsTableMessageLabel->setText(message);
}

//-----------------------------------------------------------------------------
QTableWidgetItem* qMRMLSegmentsTableViewPrivate::findItemBySegmentID(QString segmentID)
{
  Q_Q(qMRMLSegmentsTableView);
  for (int row=0; row<this->SegmentsTable->rowCount(); ++row)
  {
    QTableWidgetItem* item = this->SegmentsTable->item(row, this->columnIndex("Name"));
    if (!item->data(q->IDRole).toString().compare(segmentID))
    {
      return item;
    }
  }

  return NULL;
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// qMRMLSegmentsTableView methods

//-----------------------------------------------------------------------------
qMRMLSegmentsTableView::qMRMLSegmentsTableView(QWidget* _parent)
  : QWidget(_parent)
  , d_ptr(new qMRMLSegmentsTableViewPrivate(*this))
{
  Q_D(qMRMLSegmentsTableView);
  d->init();
  this->setMode(VisibilityOptionsMode);
  this->populateSegmentTable();
}

//-----------------------------------------------------------------------------
qMRMLSegmentsTableView::~qMRMLSegmentsTableView()
{
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::setSegmentationNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentsTableView);

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node);

  // Clear representation node if segmentation node is valid
  if (segmentationNode)
  {
    this->setRepresentationNode(NULL);
  }

  // Connect display modified event to population of the table
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                 this, SLOT( updateWidgetFromMRML() ) );

  // Connect segment added/removed and display modified events to population of the table
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentAdded,
                 this, SLOT( populateSegmentTable() ) );
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentRemoved,
                 this, SLOT( populateSegmentTable() ) );
  qvtkReconnect( d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentModified,
                 this, SLOT( updateWidgetFromMRML() ) );

  d->SegmentationNode = segmentationNode;
  this->populateSegmentTable();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::setRepresentationNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentsTableView);

  vtkMRMLLabelMapVolumeNode* labelmapNode = vtkMRMLLabelMapVolumeNode::SafeDownCast(node);
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);

  // Clear segmentation node if representation node is valid
  if (labelmapNode || modelNode)
  {
    this->setSegmentationNode(NULL);
  }

  d->RepresentationNode = (labelmapNode ? (vtkMRMLDisplayableNode*)labelmapNode : (vtkMRMLDisplayableNode*)modelNode);
  this->populateSegmentTable();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentsTableView::segmentationNode()
{
  Q_D(qMRMLSegmentsTableView);

  return d->SegmentationNode;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentsTableView::representationNode()
{
  Q_D(qMRMLSegmentsTableView);

  return d->RepresentationNode;
}

//-----------------------------------------------------------------------------
int qMRMLSegmentsTableView::mode()const
{
  Q_D(const qMRMLSegmentsTableView);

  return (int)d->Mode;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::setMode(SegmentTableMode mode)
{
  Q_D(qMRMLSegmentsTableView);

  if (mode == VisibilityOptionsMode)
    {
    d->SegmentsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible3D"), false);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DFill"), false);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DOutline"), false);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Color"), false);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Opacity"), false);
    }
  else if (mode == SimpleListMode)
    {
    d->SegmentsTable->horizontalHeader()->setVisible(false);
    d->SegmentsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible3D"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DFill"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DOutline"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Color"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Opacity"), true);
    }
  else if (mode == RepresentationMode)
    {
    d->SegmentsTable->horizontalHeader()->setVisible(false);
    d->SegmentsTable->setSelectionMode(QAbstractItemView::NoSelection);

    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible3D"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DFill"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DOutline"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Color"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Opacity"), true);
    }
  else if (mode == EditorMode)
    {
    d->SegmentsTable->horizontalHeader()->setVisible(true);
    d->SegmentsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible3D"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DFill"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Visible2DOutline"), true);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Color"), false);
    d->SegmentsTable->setColumnHidden(d->columnIndex("Opacity"), true);
    }
  else
    {
    qWarning() << "qMRMLSegmentsTableView::setMode: Invalid mode";
    }

  d->Mode = mode;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::setMode(int mode)
{
  this->setMode((SegmentTableMode)mode);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::populateSegmentTable()
{
  Q_D(qMRMLSegmentsTableView);

  d->setMessage(QString());

  // Block signals so that onSegmentTableItemChanged function is not called when populating
  d->SegmentsTable->blockSignals(true);

  d->SegmentsTable->clearContents();

  // Show node name and type if representation node
  if (d->RepresentationNode)
  {
    // Force simple list mode
    if (d->Mode != RepresentationMode)
    {
      qWarning() << "qMRMLSegmentsTableView::populateSegmentTable: Representation node is selected, but mode is not representation mode! Setting to representation mode.";
      this->setMode(RepresentationMode);
    }

    d->SegmentsTable->setRowCount(1);
    d->SegmentsTable->setRowHeight(0, 52);

    QString name = QString("%1\n(%2 node)").arg(d->RepresentationNode->GetName()).arg(d->RepresentationNode->GetNodeTagName());
    QTableWidgetItem* representationItem = new QTableWidgetItem(name);
    representationItem->setToolTip(name);
    representationItem->setFlags(representationItem->flags() & ~Qt::ItemIsEditable);
    QFont boldFont;
    boldFont.setWeight(QFont::Bold);
    representationItem->setFont(boldFont);
    d->SegmentsTable->setItem(0, d->columnIndex("Name"), representationItem);

    d->SegmentsTable->blockSignals(false);
    return;
  }

  // If not representation, then segmentation must be selected. Check validity
  if (!d->SegmentationNode)
    {
    d->setMessage(tr("No node is selected"));
    d->SegmentsTable->setRowCount(0);
    d->SegmentsTable->blockSignals(false);
    return;
    }
  else if (d->SegmentationNode->GetSegmentation()->GetNumberOfSegments() == 0)
    {
    d->setMessage(tr("Empty segmentation"));
    d->SegmentsTable->setRowCount(0);
    d->SegmentsTable->blockSignals(false);
    return;
    }

  // Get segmentation display node
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    d->SegmentationNode->GetDisplayNode() );
  if (!displayNode)
  {
    qCritical() << "qMRMLSegmentsTableView::populateSegmentTable: No display node for segmentation!";
    return;
  }

  vtkSegmentation::SegmentMap segmentMap = d->SegmentationNode->GetSegmentation()->GetSegments();
  d->SegmentsTable->setRowCount(segmentMap.size());
  int row = 0;
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++row)
  {
    QString segmentId(segmentIt->first.c_str());

    // Row height is smaller than default (which is 30)
    d->SegmentsTable->setRowHeight(row, 20);

    // Segment name
    QString name(segmentIt->second->GetName());
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);
    nameItem->setData(IDRole, segmentId);
    d->SegmentsTable->setItem(row, d->columnIndex("Name"), nameItem);

    // Get segment display properties
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    if (!displayNode->GetSegmentDisplayProperties(segmentIt->first, properties))
      {
      continue;
      }

    // Visibility 3D
    QTableWidgetItem* visibility3DItem = new QTableWidgetItem();
    visibility3DItem->setData(VisibilityRole, QVariant(properties.Visible3D));
    visibility3DItem->setData(IDRole, segmentId);
    visibility3DItem->setData(Qt::CheckStateRole, QVariant());
    visibility3DItem->setFlags(visibility3DItem->flags() & ~Qt::ItemIsUserCheckable);
    // Disable editing so that a double click won't bring up an entry box
    visibility3DItem->setFlags(visibility3DItem->flags() & ~Qt::ItemIsEditable);
    if (properties.Visible3D)
      {
      visibility3DItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerVisible.png"));
      }
    else
      {
      visibility3DItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerInvisible.png"));
      }
    d->SegmentsTable->setItem(row, d->columnIndex("Visible3D"), visibility3DItem);

    // Visibility 2D fill
    QTableWidgetItem* visibility2DFillItem = new QTableWidgetItem();
    visibility2DFillItem->setData(VisibilityRole, QVariant(properties.Visible2DFill));
    visibility2DFillItem->setData(IDRole, segmentId);
    visibility2DFillItem->setData(Qt::CheckStateRole, QVariant());
    visibility2DFillItem->setFlags(visibility2DFillItem->flags() & ~Qt::ItemIsUserCheckable);
    // Disable editing so that a double click won't bring up an entry box
    visibility2DFillItem->setFlags(visibility2DFillItem->flags() & ~Qt::ItemIsEditable);
    if (properties.Visible2DFill)
      {
      visibility2DFillItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerVisible.png"));
      }
    else
      {
      visibility2DFillItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerInvisible.png"));
      }
    d->SegmentsTable->setItem(row, d->columnIndex("Visible2DFill"), visibility2DFillItem);

    // Visibility 2D outline
    QTableWidgetItem* visibility2DOutlineItem = new QTableWidgetItem();
    visibility2DOutlineItem->setData(VisibilityRole, QVariant(properties.Visible2DOutline));
    visibility2DOutlineItem->setData(IDRole, segmentId);
    visibility2DOutlineItem->setData(Qt::CheckStateRole, QVariant());
    visibility2DOutlineItem->setFlags(visibility2DOutlineItem->flags() & ~Qt::ItemIsUserCheckable);
    // Disable editing so that a double click won't bring up an entry box
    visibility2DOutlineItem->setFlags(visibility2DOutlineItem->flags() & ~Qt::ItemIsEditable);
    if (properties.Visible2DOutline)
      {
      visibility2DOutlineItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerVisible.png"));
      }
    else
      {
      visibility2DOutlineItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerInvisible.png"));
      }
    d->SegmentsTable->setItem(row, d->columnIndex("Visible2DOutline"), visibility2DOutlineItem);

    // Color
    QTableWidgetItem* colorItem = new QTableWidgetItem();
    QColor color = QColor::fromRgbF(properties.Color[0], properties.Color[1], properties.Color[2]);
    colorItem->setData(Qt::DecorationRole, color);
    colorItem->setData(IDRole, segmentId);
    colorItem->setToolTip("Color");
    d->SegmentsTable->setItem(row, d->columnIndex("Color"), colorItem);

    // Opacity (show only 3D opacity; if the user changes it then it applies to all types of opacity)
    QTableWidgetItem* opacityItem = new QTableWidgetItem();
    //QString displayedOpacity = QString::number(properties.Opacity3D, 'f', 2);
    //opacityItem->setData(Qt::EditRole, displayedOpacity); // for qMRMLItemDelegate
    opacityItem->setData(Qt::EditRole, properties.Opacity3D); // for qMRMLDoubleSpinBoxDelegate
    opacityItem->setData(IDRole, segmentId);
    opacityItem->setToolTip("Opacity");
    d->SegmentsTable->setItem(row, d->columnIndex("Opacity"), opacityItem);
  }

  // Unblock signals
  d->SegmentsTable->blockSignals(false);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::updateWidgetFromMRML()
{
  Q_D(qMRMLSegmentsTableView);

  if (d->Mode == RepresentationMode)
  {
    qCritical() << "qMRMLSegmentsTableView::updateWidgetFromMRML: This function must not be called in representation mode!";
    return;
  }
  if ( !d->SegmentationNode
    || d->SegmentsTable->rowCount() != d->SegmentationNode->GetSegmentation()->GetNumberOfSegments() )
  {
    this->populateSegmentTable();
    return;
  }
  // Get segmentation display node
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    d->SegmentationNode->GetDisplayNode() );
  if (!displayNode)
  {
    qCritical() << "qMRMLSegmentsTableView::updateWidgetFromMRML: No display node for segmentation!";
    return;
  }

  // Find items for each segment and update each field
  vtkSegmentation::SegmentMap segmentMap = d->SegmentationNode->GetSegmentation()->GetSegments();
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
  {
    QTableWidgetItem* nameItem = d->findItemBySegmentID(segmentIt->first.c_str());
    if (!nameItem)
    {
      qCritical() << "qMRMLSegmentsTableView::updateWidgetFromMRML: Cannot find table item correspondig to segment ID '" << segmentIt->first.c_str() << " in segmentation node " << d->SegmentationNode->GetName();
      continue;
    }
    int row = nameItem->row();

    // Name
    nameItem->setText(segmentIt->second->GetName());

    // Get segment display properties
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    if (!displayNode->GetSegmentDisplayProperties(segmentIt->first, properties))
      {
      qWarning() << "qMRMLSegmentsTableView::updateWidgetFromMRML: Unable to find display properties for segment ID '" << segmentIt->first.c_str() << " in segmentation node " << d->SegmentationNode->GetName();
      continue;
      }

    // Visibility 3D
    QTableWidgetItem* visibility3DItem = d->SegmentsTable->item(row, d->columnIndex("Visible3D"));
    if (visibility3DItem)
    {
      visibility3DItem->setData(VisibilityRole, QVariant(properties.Visible3D));
      if (properties.Visible3D)
        {
        visibility3DItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerVisible.png"));
        }
      else
        {
        visibility3DItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerInvisible.png"));
        }
    }

    // Visibility 2D fill
    QTableWidgetItem* visibility2DFillItem = d->SegmentsTable->item(row, d->columnIndex("Visible2DFill"));
    if (visibility2DFillItem)
    {
      visibility2DFillItem->setData(VisibilityRole, QVariant(properties.Visible2DFill));
      if (properties.Visible2DFill)
        {
        visibility2DFillItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerVisible.png"));
        }
      else
        {
        visibility2DFillItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerInvisible.png"));
        }
    }

    // Visibility 2D outline
    QTableWidgetItem* visibility2DOutlineItem = d->SegmentsTable->item(row, d->columnIndex("Visible2DOutline"));
    if (visibility2DOutlineItem)
    {
      visibility2DOutlineItem->setData(VisibilityRole, QVariant(properties.Visible2DOutline));
      if (properties.Visible2DOutline)
        {
        visibility2DOutlineItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerVisible.png"));
        }
      else
        {
        visibility2DOutlineItem->setData(Qt::DecorationRole, QPixmap(":/Icons/Small/SlicerInvisible.png"));
        }
    }

    // Color
    QTableWidgetItem* colorItem = d->SegmentsTable->item(row, d->columnIndex("Color"));
    if (colorItem)
    {
      QColor color = QColor::fromRgbF(properties.Color[0], properties.Color[1], properties.Color[2]);
      colorItem->setData(Qt::DecorationRole, color);
    }

    // Opacity (show only 3D opacity; if the user changes it then it applies to all types of opacity)
    QTableWidgetItem* opacityItem =  d->SegmentsTable->item(row, d->columnIndex("Opacity"));
    if (opacityItem)
    {
      //QString displayedOpacity = QString::number(properties.PolyDataOpacity, 'f', 2);
      //opacityItem->setData(Qt::EditRole, displayedOpacity); // for qMRMLItemDelegate
      opacityItem->setData(Qt::EditRole, properties.Opacity3D); // for qMRMLDoubleSpinBoxDelegate
    }
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::onSegmentTableItemChanged(QTableWidgetItem* changedItem)
{
  Q_D(qMRMLSegmentsTableView);

  d->setMessage(QString());

  if (!changedItem || !d->SegmentationNode)
    {
    return;
    }

  // All items contain the segment ID, get that
  QString segmentId = changedItem->data(IDRole).toString();

  // If segment name has been changed
  if (changedItem->column() == d->columnIndex("Name"))
    {
    QString nameText(changedItem->text());
    vtkSegment* segment = d->SegmentationNode->GetSegmentation()->GetSegment(segmentId.toLatin1().constData());
    if (!segment)
      {
      qCritical() << "qMRMLSegmentsTableView::onSegmentTableItemChanged: Segment with ID '" << segmentId << "' not found in segmentation node " << d->SegmentationNode->GetName();
      return;
      }
    segment->SetName(nameText.toLatin1().constData());
    }
  // If visualization has been changed
  else
    {
    // For all other columns we need the display node
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
      d->SegmentationNode->GetDisplayNode() );
    if (!displayNode)
      {
      qCritical() << "qMRMLSegmentsTableView::onSegmentTableItemChanged: No display node for segmentation!";
      return;
      }
    // Get display properties
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    if (!displayNode->GetSegmentDisplayProperties(segmentId.toLatin1().constData(), properties))
      {
      return;
      }

    bool valueChanged = false;

    // Visibility changed
    if (changedItem->column() == d->columnIndex("Visible3D"))
      {
      int visible = changedItem->data(VisibilityRole).toInt();
      if (properties.Visible3D != (bool)visible)
        {
        properties.Visible3D = (bool)visible;
        valueChanged = true;
        }
      }
    // Visibility changed
    else if (changedItem->column() == d->columnIndex("Visible2DFill"))
      {
      int visible = changedItem->data(VisibilityRole).toInt();
      if (properties.Visible2DFill != (bool)visible)
        {
        properties.Visible2DFill = (bool)visible;
        valueChanged = true;
        }
      }
    // Visibility changed
    else if (changedItem->column() == d->columnIndex("Visible2DOutline"))
      {
      int visible = changedItem->data(VisibilityRole).toInt();
      if (properties.Visible2DOutline != (bool)visible)
        {
        properties.Visible2DOutline = (bool)visible;
        valueChanged = true;
        }
      }
    // Color changed
    else if (changedItem->column() == d->columnIndex("Color"))
      {
      QColor color = changedItem->data(Qt::DecorationRole).value<QColor>();
      QColor oldColor = QColor::fromRgbF(properties.Color[0], properties.Color[1], properties.Color[2]);
      if (oldColor != color)
        {
        properties.Color[0] = color.redF();
        properties.Color[1] = color.greenF();
        properties.Color[2] = color.blueF();
        valueChanged = true;
        }
      }
    // Opacity changed
    else if (changedItem->column() == d->columnIndex("Opacity"))
      {
      QString opacity = changedItem->data(Qt::EditRole).toString();
      QString currentOpacity = QString::number( properties.Opacity3D, 'f', 2);
      if (opacity != currentOpacity)
        {
        // Set to all kinds of opacities as they are combined on the UI
        properties.Opacity3D = opacity.toDouble();
        properties.Opacity2DFill = opacity.toDouble();
        properties.Opacity2DOutline = opacity.toDouble();
        valueChanged = true;
        }
      }
    // Set changed properties to segmentation display node if a value has actually changed
    if (valueChanged)
      {
      displayNode->SetSegmentDisplayProperties(segmentId.toLatin1().constData(), properties);
      }
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::onSegmentTableItemClicked(QTableWidgetItem* item)
{
  Q_D(qMRMLSegmentsTableView);

  if (!item)
    {
    return;
    }

  int column = item->column();
  if ( column == d->columnIndex(QString("Visible3D"))
    || column == d->columnIndex(QString("Visible2DFill"))
    || column == d->columnIndex(QString("Visible2DOutline")) )
    {
    // Toggle the visibility role, the icon update is triggered by this change
    if (item->data(VisibilityRole) == QVariant(false))
      {
      item->setData(VisibilityRole, QVariant(true));
      }
    else
      {
      item->setData(VisibilityRole, QVariant(false));
      }
    }
}

//-----------------------------------------------------------------------------
int qMRMLSegmentsTableView::segmentCount() const
{
  Q_D(const qMRMLSegmentsTableView);

  return d->SegmentsTable->rowCount();
}

//-----------------------------------------------------------------------------
QStringList qMRMLSegmentsTableView::selectedSegmentIDs()
{
  Q_D(qMRMLSegmentsTableView);

  QList<QTableWidgetItem*> selectedItems = d->SegmentsTable->selectedItems();
  QStringList selectedSegmentIds;
  QSet<int> rows;
  foreach (QTableWidgetItem* item, selectedItems)
  {
    int row = item->row();
    if (!rows.contains(row))
    {
      rows.insert(row);
      selectedSegmentIds << item->data(IDRole).toString();
    }
  }

  return selectedSegmentIds;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::setSelectedSegmentIDs(QStringList segmentIDs)
{
  Q_D(qMRMLSegmentsTableView);

  // Deselect selected items first
  this->clearSelection();

  // Find item by segment ID
  foreach (QString segmentID, segmentIDs)
  {
    QTableWidgetItem* segmentItem = d->findItemBySegmentID(segmentID);
    if (!segmentItem)
    {
      qCritical() << "qMRMLSegmentsTableView::setSelectedSegmentIDs: Cannot find table item correspondig to segment ID '" << segmentID << " in segmentation node " << d->SegmentationNode->GetName();
      continue;
    }

    // Select item for segment
    d->SegmentsTable->setItemSelected(segmentItem, true);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentsTableView::clearSelection()
{
  Q_D(qMRMLSegmentsTableView);

  // Deselect selected items first
  QList<QTableWidgetItem*> selectedItems = d->SegmentsTable->selectedItems();
  foreach (QTableWidgetItem* item, selectedItems)
  {
    d->SegmentsTable->setItemSelected(item, false);
  }
}
