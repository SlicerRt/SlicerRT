#include "qSlicerSquaredUnderdosingObjective.h"

qSlicerSquaredUnderdosingObjective::qSlicerSquaredUnderdosingObjective(QObject* parent)
  : qSlicerAbstractObjective(parent)
{
  this->m_Name = "Squared Underdosing";
  this->initializeParameters();
}

qSlicerSquaredUnderdosingObjective::~qSlicerSquaredUnderdosingObjective() = default;

void qSlicerSquaredUnderdosingObjective::initializeParameters()
{
  this->objectivesParameters["minDose"] = 60.0;
}

float qSlicerSquaredUnderdosingObjective::computeDoseObjectiveFunction(const DoseType& dose)
{
  double dMin = this->objectivesParameters["minDose"].toDouble();
  DoseType underdose = (dose.array() - dMin).min(0.0).matrix();
  return static_cast<float>(underdose.squaredNorm() / dose.size());
}

qSlicerAbstractObjective::DoseType
qSlicerSquaredUnderdosingObjective::computeDoseObjectiveGradient(const DoseType& dose)
{
  double dMin = this->objectivesParameters["minDose"].toDouble();
  DoseType underdose = (dose.array() - dMin).min(0.0).matrix();
  return 2.0 * underdose / dose.size();
}
