#include "qSlicerEUDObjective.h"

#include <QDebug>
#include <cmath>

qSlicerEUDObjective::qSlicerEUDObjective(QObject* parent)
  : qSlicerAbstractObjective(parent)
{
  this->m_Name = "EUD";
  this->initializeParameters();
}

qSlicerEUDObjective::~qSlicerEUDObjective() = default;

void qSlicerEUDObjective::initializeParameters()
{
  this->objectivesParameters["eudRef"] = 0.0;
  this->objectivesParameters["exponent"] = 3.5;
}

float qSlicerEUDObjective::computeDoseObjectiveFunction(const DoseType& dose)
{
  double eudRef = this->objectivesParameters["eudRef"].toDouble();
  double k = this->objectivesParameters["exponent"].toDouble();

  double powerSum = dose.array().pow(k).sum();
  double eud = std::pow(powerSum / dose.size(), 1.0 / k);
  double diff = eud - eudRef;
  return static_cast<float>(diff * diff);
}

qSlicerAbstractObjective::DoseType
qSlicerEUDObjective::computeDoseObjectiveGradient(const DoseType& dose)
{
  double eudRef = this->objectivesParameters["eudRef"].toDouble();
  double k = this->objectivesParameters["exponent"].toDouble();
  int n = static_cast<int>(dose.size());

  // Replace zeros for numerical stability
  DoseType d = dose;
  for (int i = 0; i < n; ++i)
    if (d[i] == 0.0) d[i] = 1e-3;

  double powerSum = d.array().pow(k).sum();
  double eud = std::pow(powerSum / n, 1.0 / k);

  // grad_i = 2 * (n)^(-1/k) * powerSum^((1-k)/k) * d_i^(k-1) * (EUD - eudRef)
  double scale = 2.0 * std::pow(1.0 / n, 1.0 / k) * std::pow(powerSum, (1.0 - k) / k) * (eud - eudRef);
  DoseType grad = scale * d.array().pow(k - 1.0).matrix();

  if (!grad.allFinite())
  {
    qWarning() << "EUD gradient computation failed (non-finite values). Reduce exponent.";
    return DoseType::Zero(n);
  }
  return grad;
}
