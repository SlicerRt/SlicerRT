#include "qSlicerMinMaxMeanDoseConstraint.h"

qSlicerMinMaxMeanDoseConstraint::qSlicerMinMaxMeanDoseConstraint(QObject* parent)
  : qSlicerAbstractConstraint(parent)
{
  this->m_Name = "Mean Dose";
  this->initializeParameters();
}

qSlicerMinMaxMeanDoseConstraint::~qSlicerMinMaxMeanDoseConstraint() = default;

void qSlicerMinMaxMeanDoseConstraint::initializeParameters()
{
  this->constraintParameters["minMeanDose"] = 0.0;
  this->constraintParameters["maxMeanDose"] = 30.0;
}

int qSlicerMinMaxMeanDoseConstraint::numConstraints(int /*numVoxels*/) const
{
  return 1;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxMeanDoseConstraint::computeDoseConstraintFunction(const DoseType& dose)
{
  DoseType result(1);
  result[0] = dose.mean();
  return result;
}

Eigen::MatrixXd
qSlicerMinMaxMeanDoseConstraint::computeDoseConstraintJacobian(const DoseType& dose)
{
  int n = static_cast<int>(dose.size());
  return Eigen::MatrixXd::Constant(1, n, 1.0 / n);
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxMeanDoseConstraint::upperBounds(int /*numVoxels*/)
{
  DoseType ub(1);
  ub[0] = this->constraintParameters["maxMeanDose"].toDouble();
  return ub;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxMeanDoseConstraint::lowerBounds(int /*numVoxels*/)
{
  DoseType lb(1);
  lb[0] = this->constraintParameters["minMeanDose"].toDouble();
  return lb;
}
