#ifndef __qSlicerMaxDVHObjective_h
#define __qSlicerMaxDVHObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractObjective.h"

/// Penalized Max DVH objective.
/// Penalizes voxels between doseRef and the dose at which volume fraction = vMaxPercent.
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMaxDVHObjective
  : public qSlicerAbstractObjective
{
  Q_OBJECT

public:
  typedef qSlicerAbstractObjective Superclass;
  explicit qSlicerMaxDVHObjective(QObject* parent = nullptr);
  ~qSlicerMaxDVHObjective() override;

  Q_INVOKABLE float computeDoseObjectiveFunction(const DoseType& dose) override;
  Q_INVOKABLE DoseType computeDoseObjectiveGradient(const DoseType& dose) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerMaxDVHObjective);
};

#endif
