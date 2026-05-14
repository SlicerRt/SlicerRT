#include "qSlicerMinMaxEUDConstraint.h"

#include <cmath>

qSlicerMinMaxEUDConstraint::qSlicerMinMaxEUDConstraint(QObject* parent)
  : qSlicerAbstractConstraint(parent)
{
  this->m_Name = "EUD";
  this->initializeParameters();
}

qSlicerMinMaxEUDConstraint::~qSlicerMinMaxEUDConstraint() = default;

void qSlicerMinMaxEUDConstraint::initializeParameters()
{
  this->constraintParameters["exponent"] = 5.0;
  this->constraintParameters["eudMin"] = 0.0;
  this->constraintParameters["eudMax"] = 30.0;
}

int qSlicerMinMaxEUDConstraint::numConstraints(int /*numVoxels*/) const
{
  return 1;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxEUDConstraint::computeDoseConstraintFunction(const DoseType& dose)
{
  double k = this->constraintParameters["exponent"].toDouble();
  int n = static_cast<int>(dose.size());
  double powerSum = dose.array().pow(k).sum();
  double eud = std::pow(powerSum / n, 1.0 / k);

  DoseType result(1);
  result[0] = eud;
  return result;
}

Eigen::MatrixXd
qSlicerMinMaxEUDConstraint::computeDoseConstraintJacobian(const DoseType& dose)
{
  double k = this->constraintParameters["exponent"].toDouble();
  int n = static_cast<int>(dose.size());

  double powerSum = dose.array().pow(k).sum();
  // d(EUD)/d(dose_i) = n^(-1/k) * powerSum^((1-k)/k) * dose_i^(k-1)
  double scale = std::pow(1.0 / n, 1.0 / k) * std::pow(powerSum, (1.0 - k) / k);
  Eigen::MatrixXd jac(1, n);
  jac.row(0) = scale * dose.array().pow(k - 1.0).matrix().transpose();
  return jac;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxEUDConstraint::upperBounds(int /*numVoxels*/)
{
  DoseType ub(1);
  ub[0] = this->constraintParameters["eudMax"].toDouble();
  return ub;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxEUDConstraint::lowerBounds(int /*numVoxels*/)
{
  DoseType lb(1);
  lb[0] = this->constraintParameters["eudMin"].toDouble();
  return lb;
}
