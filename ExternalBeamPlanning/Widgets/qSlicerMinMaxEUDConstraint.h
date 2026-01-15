#ifndef __qSlicerMinMaxEUDConstraint_h
#define __qSlicerMinMaxEUDConstraint_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractConstraint.h"

/// EUD constraint: eudMin <= EUD <= eudMax  where EUD = (mean(dose^k))^(1/k).
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMinMaxEUDConstraint
  : public qSlicerAbstractConstraint
{
  Q_OBJECT

public:
  typedef qSlicerAbstractConstraint Superclass;
  explicit qSlicerMinMaxEUDConstraint(QObject* parent = nullptr);
  ~qSlicerMinMaxEUDConstraint() override;

  int numConstraints(int numVoxels) const override;

  Q_INVOKABLE DoseType computeDoseConstraintFunction(const DoseType& dose) override;
  Q_INVOKABLE Eigen::MatrixXd computeDoseConstraintJacobian(const DoseType& dose) override;
  Q_INVOKABLE DoseType upperBounds(int numVoxels) override;
  Q_INVOKABLE DoseType lowerBounds(int numVoxels) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerMinMaxEUDConstraint);
};

#endif
