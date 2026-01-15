#include "qSlicerMaxDVHObjective.h"
#include "qSlicerDVHUtils.h"

qSlicerMaxDVHObjective::qSlicerMaxDVHObjective(QObject* parent)
  : qSlicerAbstractObjective(parent)
{
  this->m_Name = "Max DVH";
  this->initializeParameters();
}

qSlicerMaxDVHObjective::~qSlicerMaxDVHObjective() = default;

void qSlicerMaxDVHObjective::initializeParameters()
{
  this->objectivesParameters["doseRef"] = 30.0;
  this->objectivesParameters["vMaxPercent"] = 95.0;
}

float qSlicerMaxDVHObjective::computeDoseObjectiveFunction(const DoseType& dose)
{
  double dRef = this->objectivesParameters["doseRef"].toDouble();
  double vMaxPercent = this->objectivesParameters["vMaxPercent"].toDouble();
  double refVol = vMaxPercent / 100.0;
  double dRef2 = qSlicerCalcInverseDVH(refVol, dose);

  // Penalize voxels with dose in (dRef, dRef2)
  DoseType deviation = dose.array() - dRef;
  for (int i = 0; i < dose.size(); ++i)
    if (dose[i] < dRef || dose[i] > dRef2) deviation[i] = 0.0;

  return static_cast<float>(deviation.squaredNorm() / dose.size());
}

qSlicerAbstractObjective::DoseType
qSlicerMaxDVHObjective::computeDoseObjectiveGradient(const DoseType& dose)
{
  double dRef = this->objectivesParameters["doseRef"].toDouble();
  double vMaxPercent = this->objectivesParameters["vMaxPercent"].toDouble();
  double refVol = vMaxPercent / 100.0;
  double dRef2 = qSlicerCalcInverseDVH(refVol, dose);

  DoseType deviation = dose.array() - dRef;
  for (int i = 0; i < dose.size(); ++i)
    if (dose[i] < dRef || dose[i] > dRef2) deviation[i] = 0.0;

  return (2.0 / dose.size()) * deviation;
}
