#include "qSlicerAbstractConstraint.h"

qSlicerAbstractConstraint::qSlicerAbstractConstraint(QObject* parent)
  : QObject(parent)
{}

qSlicerAbstractConstraint::~qSlicerAbstractConstraint() = default;

QString qSlicerAbstractConstraint::name() const
{
  return m_Name;
}

void qSlicerAbstractConstraint::setName(QString name)
{
  m_Name = name;
}

QMap<QString, QVariant> qSlicerAbstractConstraint::getConstraintParameters() const
{
  return constraintParameters;
}

void qSlicerAbstractConstraint::setConstraintParameters(QMap<QString, QVariant> parameters)
{
  constraintParameters = parameters;
}
