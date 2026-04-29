#ifndef __qSlicerMeanDoseObjective_h
#define __qSlicerMeanDoseObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractObjective.h"

/// Penalized mean dose objective.
/// mode 0 = linear |mean(dose) - dRef|, mode 1 = quadratic (mean(dose) - dRef)^2
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMeanDoseObjective
  : public qSlicerAbstractObjective
{
  Q_OBJECT

public:
  typedef qSlicerAbstractObjective Superclass;
  explicit qSlicerMeanDoseObjective(QObject* parent = nullptr);
  ~qSlicerMeanDoseObjective() override;

  Q_INVOKABLE float computeDoseObjectiveFunction(const DoseType& dose) override;
  Q_INVOKABLE DoseType computeDoseObjectiveGradient(const DoseType& dose) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerMeanDoseObjective);
};

#endif
