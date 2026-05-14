#include "qSlicerMinMaxDoseConstraint.h"

#include <cmath>
#include <limits>

static const double EPSILON = 1e-3;

qSlicerMinMaxDoseConstraint::qSlicerMinMaxDoseConstraint(QObject* parent)
  : qSlicerAbstractConstraint(parent)
{
  this->m_Name = "Min/Max Dose";
  this->initializeParameters();
}

qSlicerMinMaxDoseConstraint::~qSlicerMinMaxDoseConstraint() = default;

void qSlicerMinMaxDoseConstraint::initializeParameters()
{
  this->constraintParameters["minDose"] = 0.0;
  this->constraintParameters["maxDose"] = 30.0;
  // 0 = log-sum-exp approximation, 1 = voxel-wise
  this->constraintParameters["mode"] = 0;
}

bool qSlicerMinMaxDoseConstraint::hasMinConstraint() const
{
  return this->constraintParameters["minDose"].toDouble() > 0.0;
}

bool qSlicerMinMaxDoseConstraint::hasMaxConstraint() const
{
  double maxDose = this->constraintParameters["maxDose"].toDouble();
  return std::isfinite(maxDose);
}

int qSlicerMinMaxDoseConstraint::numConstraints(int numVoxels) const
{
  int mode = this->constraintParameters["mode"].toInt();
  if (mode == 1)
    return numVoxels;

  // log-sum-exp: 0, 1, or 2 scalar constraints
  int count = 0;
  if (hasMinConstraint()) ++count;
  if (hasMaxConstraint()) ++count;
  return count;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxDoseConstraint::computeDoseConstraintFunction(const DoseType& dose)
{
  int mode = this->constraintParameters["mode"].toInt();
  double dMin = this->constraintParameters["minDose"].toDouble();
  double dMax = this->constraintParameters["maxDose"].toDouble();

  if (mode == 1)
    return dose; // voxel-wise: g_i = dose_i, bounds enforce limits

  // log-sum-exp approximation
  DoseType result;
  int idx = 0;
  bool doMin = hasMinConstraint();
  bool doMax = hasMaxConstraint();
  result.resize(doMin && doMax ? 2 : (doMin || doMax ? 1 : 0));

  if (doMin)
  {
    double doseMin = dose.minCoeff();
    double g = doseMin - EPSILON * std::log(((doseMin - dose.array()) / EPSILON).exp().sum());
    result[idx++] = g;
  }
  if (doMax)
  {
    double doseMax = dose.maxCoeff();
    double g = doseMax + EPSILON * std::log(((dose.array() - doseMax) / EPSILON).exp().sum());
    result[idx++] = g;
  }
  return result;
}

Eigen::MatrixXd
qSlicerMinMaxDoseConstraint::computeDoseConstraintJacobian(const DoseType& dose)
{
  int mode = this->constraintParameters["mode"].toInt();
  int n = static_cast<int>(dose.size());

  if (mode == 1)
    return Eigen::MatrixXd::Identity(n, n);

  int m = numConstraints(n);
  Eigen::MatrixXd jac = Eigen::MatrixXd::Zero(m, n);
  int row = 0;

  if (hasMinConstraint())
  {
    double doseMin = dose.minCoeff();
    Eigen::VectorXd w = ((doseMin - dose.array()) / EPSILON).exp().matrix();
    jac.row(row++) = (w / w.sum()).transpose();
  }
  if (hasMaxConstraint())
  {
    double doseMax = dose.maxCoeff();
    Eigen::VectorXd w = ((dose.array() - doseMax) / EPSILON).exp().matrix();
    jac.row(row++) = (w / w.sum()).transpose();
  }
  return jac;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxDoseConstraint::upperBounds(int numVoxels)
{
  int mode = this->constraintParameters["mode"].toInt();
  double dMax = this->constraintParameters["maxDose"].toDouble();

  if (mode == 1)
    return DoseType::Constant(numVoxels, dMax);

  int m = numConstraints(numVoxels);
  DoseType ub(m);
  int idx = 0;
  if (hasMinConstraint()) ub[idx++] = std::numeric_limits<double>::infinity();
  if (hasMaxConstraint()) ub[idx++] = dMax;
  return ub;
}

qSlicerAbstractConstraint::DoseType
qSlicerMinMaxDoseConstraint::lowerBounds(int numVoxels)
{
  int mode = this->constraintParameters["mode"].toInt();
  double dMin = this->constraintParameters["minDose"].toDouble();

  if (mode == 1)
    return DoseType::Constant(numVoxels, dMin);

  int m = numConstraints(numVoxels);
  DoseType lb(m);
  int idx = 0;
  if (hasMinConstraint()) lb[idx++] = dMin;
  if (hasMaxConstraint()) lb[idx++] = 0.0;
  return lb;
}
