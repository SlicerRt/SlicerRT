#ifndef __qSlicerMinDVHObjective_h
#define __qSlicerMinDVHObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractObjective.h"

/// Penalized Min DVH objective.
/// Penalizes voxels between d_ref2 (dose at vMinPercent) and doseRef.
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMinDVHObjective
  : public qSlicerAbstractObjective
{
  Q_OBJECT

public:
  typedef qSlicerAbstractObjective Superclass;
  explicit qSlicerMinDVHObjective(QObject* parent = nullptr);
  ~qSlicerMinDVHObjective() override;

  Q_INVOKABLE float computeDoseObjectiveFunction(const DoseType& dose) override;
  Q_INVOKABLE DoseType computeDoseObjectiveGradient(const DoseType& dose) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerMinDVHObjective);
};

#endif
