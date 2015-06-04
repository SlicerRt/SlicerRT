#include "qMRMLDoubleSpinBoxDelegate.h"

// Qt includes
#include <QDoubleSpinBox>
#include <QEvent>
#include <QApplication>

//-----------------------------------------------------------------------------
qMRMLDoubleSpinBoxDelegate::qMRMLDoubleSpinBoxDelegate(QObject *parent)
  : QStyledItemDelegate(parent) { }

//-----------------------------------------------------------------------------
QWidget* qMRMLDoubleSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
  QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
  editor->setMinimum(0.0);
  editor->setMaximum(1.0);
  editor->setSingleStep(0.1);
  //TODO: Emitting commitData actually closes the editor which we do not want
  //      Interestingly the same code works in qMRMLItemDelegate where the editor is not closed
  //connect(editor, SIGNAL(valueChanged(double)), this, SLOT(commitSenderData()));
  return editor;
}

//-----------------------------------------------------------------------------
void qMRMLDoubleSpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  double value = index.model()->data(index, Qt::EditRole).toDouble();
  QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
  spinBox->setValue(value);
}

//-----------------------------------------------------------------------------
void qMRMLDoubleSpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
  spinBox->interpretText();
  double value = spinBox->value();
  model->setData(index, value, Qt::EditRole);
}

//-----------------------------------------------------------------------------
void qMRMLDoubleSpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}

//------------------------------------------------------------------------------
void qMRMLDoubleSpinBoxDelegate::commitSenderData()
{
  QWidget* editor = qobject_cast<QWidget*>(this->sender());
  emit commitData(editor);
}
