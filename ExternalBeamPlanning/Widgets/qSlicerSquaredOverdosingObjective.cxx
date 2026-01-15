#include "qSlicerSquaredOverdosingObjective.h"

qSlicerSquaredOverdosingObjective::qSlicerSquaredOverdosingObjective(QObject* parent)
  : qSlicerAbstractObjective(parent)
{
  this->m_Name = "Squared Overdosing";
  this->initializeParameters();
}

qSlicerSquaredOverdosingObjective::~qSlicerSquaredOverdosingObjective() = default;

void qSlicerSquaredOverdosingObjective::initializeParameters()
{
  this->objectivesParameters["maxDose"] = 30.0;
}

float qSlicerSquaredOverdosingObjective::computeDoseObjectiveFunction(const DoseType& dose)
{
  double dMax = this->objectivesParameters["maxDose"].toDouble();
  DoseType overdose = (dose.array() - dMax).max(0.0).matrix();
  return static_cast<float>(overdose.squaredNorm() / dose.size());
}

qSlicerAbstractObjective::DoseType
qSlicerSquaredOverdosingObjective::computeDoseObjectiveGradient(const DoseType& dose)
{
  double dMax = this->objectivesParameters["maxDose"].toDouble();
  DoseType overdose = (dose.array() - dMax).max(0.0).matrix();
  return 2.0 * overdose / dose.size();
}
