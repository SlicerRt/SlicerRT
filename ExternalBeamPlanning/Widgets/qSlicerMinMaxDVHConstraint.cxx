#include "qSlicerMinMaxDVHConstraint.h"

#include <algorithm>
#include <cmath>
#include <vector>

qSlicerMinMaxDVHConstraint::qSlicerMinMaxDVHConstraint(QObject* parent)
  : qSlicerAbstractConstraint(parent)
{
  this->m_Name = "DVH";
  this->initializeParameters();
}

qSlicerMinMaxDVHConstraint::~qSlicerMinMaxDVHConstraint() = default;

void qSlicerMinMaxDVHConstraint::initializeParameters()
{
  this->constraintParameters["doseRef"] = 30.0;
  this->constraintParameters["vMinPercent"] = 0.0;
  this->constraintParameters["vMaxPercent"] = 100.0;
}

int qSlicerMinMaxDVHConstraint::numConstraints(int /*numVoxels*/) const
{
  return 1;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxDVHConstraint::computeDoseConstraintFunction(const DoseType& dose)
{
  double dRef = this->constraintParameters["doseRef"].toDouble();
  int n = static_cast<int>(dose.size());
  double fraction = 0.0;
  for (int i = 0; i < n; ++i)
    if (dose[i] >= dRef) fraction += 1.0;
  fraction /= n;

  DoseType result(1);
  result[0] = fraction;
  return result;
}

Eigen::MatrixXd
qSlicerMinMaxDVHConstraint::computeDoseConstraintJacobian(const DoseType& dose)
{
  double dRef = this->constraintParameters["doseRef"].toDouble();
  int n = static_cast<int>(dose.size());

  // Logistic approximation of the step function
  std::vector<double> sorted(dose.data(), dose.data() + n);
  std::sort(sorted.begin(), sorted.end());

  int noVoxels = std::max(static_cast<int>(std::ceil(n / 2.0)), 10);
  std::vector<double> absDiff(n);
  for (int i = 0; i < n; ++i)
    absDiff[i] = std::abs(dRef - sorted[i]);
  std::sort(absDiff.begin(), absDiff.end());
  double deltaDoseMax = absDiff[std::min(static_cast<int>(std::ceil(noVoxels / 2.0)), n) - 1];

  static const double refScaling = 0.01;
  double dvhcScaling = std::min(std::log(1.0 / refScaling - 1.0) / (2.0 * deltaDoseMax), 250.0);

  Eigen::MatrixXd jac(1, n);
  for (int i = 0; i < n; ++i)
  {
    double d = dose[i] - dRef;
    double e = std::exp(2.0 * dvhcScaling * d);
    jac(0, i) = (2.0 / n) * dvhcScaling * e / ((e + 1.0) * (e + 1.0));
  }
  return jac;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxDVHConstraint::upperBounds(int /*numVoxels*/)
{
  DoseType ub(1);
  ub[0] = this->constraintParameters["vMaxPercent"].toDouble() / 100.0;
  return ub;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxDVHConstraint::lowerBounds(int /*numVoxels*/)
{
  DoseType lb(1);
  lb[0] = this->constraintParameters["vMinPercent"].toDouble() / 100.0;
  return lb;
}
