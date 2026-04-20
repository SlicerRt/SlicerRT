#include "qSlicerMinDVHObjective.h"
#include "qSlicerDVHUtils.h"

qSlicerMinDVHObjective::qSlicerMinDVHObjective(QObject* parent)
  : qSlicerAbstractObjective(parent)
{
  this->m_Name = "Min DVH";
  this->initializeParameters();
}

qSlicerMinDVHObjective::~qSlicerMinDVHObjective() = default;

void qSlicerMinDVHObjective::initializeParameters()
{
  this->objectivesParameters["doseRef"] = 60.0;
  this->objectivesParameters["vMinPercent"] = 95.0;
}

float qSlicerMinDVHObjective::computeDoseObjectiveFunction(const DoseType& dose)
{
  double dRef = this->objectivesParameters["doseRef"].toDouble();
  double vMinPercent = this->objectivesParameters["vMinPercent"].toDouble();
  double refVol = vMinPercent / 100.0;
  double dRef2 = qSlicerCalcInverseDVH(refVol, dose);

  // Penalize voxels with dose in (dRef2, dRef)
  DoseType deviation = dose.array() - dRef;
  for (int i = 0; i < dose.size(); ++i)
    if (dose[i] > dRef || dose[i] < dRef2) deviation[i] = 0.0;

  return static_cast<float>(deviation.squaredNorm() / dose.size());
}

qSlicerAbstractObjective::DoseType
qSlicerMinDVHObjective::computeDoseObjectiveGradient(const DoseType& dose)
{
  double dRef = this->objectivesParameters["doseRef"].toDouble();
  double vMinPercent = this->objectivesParameters["vMinPercent"].toDouble();
  double refVol = vMinPercent / 100.0;
  double dRef2 = qSlicerCalcInverseDVH(refVol, dose);

  DoseType deviation = dose.array() - dRef;
  for (int i = 0; i < dose.size(); ++i)
    if (dose[i] > dRef || dose[i] < dRef2) deviation[i] = 0.0;

  return (2.0 / dose.size()) * deviation;
}
