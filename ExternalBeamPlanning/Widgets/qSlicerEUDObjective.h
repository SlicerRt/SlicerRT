#ifndef __qSlicerEUDObjective_h
#define __qSlicerEUDObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractObjective.h"

/// Penalized Equivalent Uniform Dose (EUD) objective.
/// f = (EUD - eudRef)^2  where  EUD = (mean(dose^k))^(1/k)
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerEUDObjective
  : public qSlicerAbstractObjective
{
  Q_OBJECT

public:
  typedef qSlicerAbstractObjective Superclass;
  explicit qSlicerEUDObjective(QObject* parent = nullptr);
  ~qSlicerEUDObjective() override;

  Q_INVOKABLE float computeDoseObjectiveFunction(const DoseType& dose) override;
  Q_INVOKABLE DoseType computeDoseObjectiveGradient(const DoseType& dose) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerEUDObjective);
};

#endif
