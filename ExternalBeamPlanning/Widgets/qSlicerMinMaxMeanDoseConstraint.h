#ifndef __qSlicerMinMaxMeanDoseConstraint_h
#define __qSlicerMinMaxMeanDoseConstraint_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractConstraint.h"

/// Mean dose constraint: minMeanDose <= mean(dose) <= maxMeanDose.
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMinMaxMeanDoseConstraint
  : public qSlicerAbstractConstraint
{
  Q_OBJECT

public:
  typedef qSlicerAbstractConstraint Superclass;
  explicit qSlicerMinMaxMeanDoseConstraint(QObject* parent = nullptr);
  ~qSlicerMinMaxMeanDoseConstraint() override;

  int numConstraints(int numVoxels) const override;

  Q_INVOKABLE DoseType computeDoseConstraintFunction(const DoseType& dose) override;
  Q_INVOKABLE Eigen::MatrixXd computeDoseConstraintJacobian(const DoseType& dose) override;
  Q_INVOKABLE DoseType upperBounds(int numVoxels) override;
  Q_INVOKABLE DoseType lowerBounds(int numVoxels) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerMinMaxMeanDoseConstraint);
};

#endif
