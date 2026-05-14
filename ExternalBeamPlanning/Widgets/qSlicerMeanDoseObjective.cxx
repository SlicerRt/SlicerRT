#include "qSlicerMeanDoseObjective.h"

#include <cmath>

qSlicerMeanDoseObjective::qSlicerMeanDoseObjective(QObject* parent)
  : qSlicerAbstractObjective(parent)
{
  this->m_Name = "Mean Dose";
  this->initializeParameters();
}

qSlicerMeanDoseObjective::~qSlicerMeanDoseObjective() = default;

void qSlicerMeanDoseObjective::initializeParameters()
{
  this->objectivesParameters["meanDoseRef"] = 0.0;
  // 0 = Linear, 1 = Quadratic
  this->objectivesParameters["mode"] = 0;
}

float qSlicerMeanDoseObjective::computeDoseObjectiveFunction(const DoseType& dose)
{
  double dRef = this->objectivesParameters["meanDoseRef"].toDouble();
  int mode = this->objectivesParameters["mode"].toInt();
  double meanDose = dose.mean();

  if (mode == 1)
  {
    double diff = meanDose - dRef;
    return static_cast<float>(diff * diff);
  }
  // Linear (default)
  return static_cast<float>(std::abs(meanDose - dRef));
}

qSlicerAbstractObjective::DoseType
qSlicerMeanDoseObjective::computeDoseObjectiveGradient(const DoseType& dose)
{
  double dRef = this->objectivesParameters["meanDoseRef"].toDouble();
  int mode = this->objectivesParameters["mode"].toInt();
  int n = static_cast<int>(dose.size());

  if (mode == 1)
  {
    double meanDose = dose.mean();
    return DoseType::Constant(n, 2.0 * (meanDose - dRef) / n);
  }
  // Linear: (1/n) * sign(dose_i - dRef) per-element as in matRad
  return (1.0 / n) * (dose.array() - dRef).sign().matrix();
}
