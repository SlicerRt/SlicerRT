#ifndef __qSlicerMinMaxDVHConstraint_h
#define __qSlicerMinMaxDVHConstraint_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractConstraint.h"

/// DVH constraint: vMin <= V(doseRef) <= vMax  (volume fractions in percent).
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMinMaxDVHConstraint
  : public qSlicerAbstractConstraint
{
  Q_OBJECT

public:
  typedef qSlicerAbstractConstraint Superclass;
  explicit qSlicerMinMaxDVHConstraint(QObject* parent = nullptr);
  ~qSlicerMinMaxDVHConstraint() override;

  int numConstraints(int numVoxels) const override;

  Q_INVOKABLE DoseType computeDoseConstraintFunction(const DoseType& dose) override;
  Q_INVOKABLE Eigen::MatrixXd computeDoseConstraintJacobian(const DoseType& dose) override;
  Q_INVOKABLE DoseType upperBounds(int numVoxels) override;
  Q_INVOKABLE DoseType lowerBounds(int numVoxels) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerMinMaxDVHConstraint);
};

#endif
