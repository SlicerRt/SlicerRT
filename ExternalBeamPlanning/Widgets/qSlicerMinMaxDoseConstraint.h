#ifndef __qSlicerMinMaxDoseConstraint_h
#define __qSlicerMinMaxDoseConstraint_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractConstraint.h"

/// Min/Max dose constraint using log-sum-exp smoothing (mode 0) or
/// exact voxel-wise bounds (mode 1).
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMinMaxDoseConstraint
  : public qSlicerAbstractConstraint
{
  Q_OBJECT

public:
  typedef qSlicerAbstractConstraint Superclass;
  explicit qSlicerMinMaxDoseConstraint(QObject* parent = nullptr);
  ~qSlicerMinMaxDoseConstraint() override;

  int numConstraints(int numVoxels) const override;

  Q_INVOKABLE DoseType computeDoseConstraintFunction(const DoseType& dose) override;
  Q_INVOKABLE Eigen::MatrixXd computeDoseConstraintJacobian(const DoseType& dose) override;
  Q_INVOKABLE DoseType upperBounds(int numVoxels) override;
  Q_INVOKABLE DoseType lowerBounds(int numVoxels) override;

protected:
  void initializeParameters() override;

private:
  bool hasMinConstraint() const;
  bool hasMaxConstraint() const;

  Q_DISABLE_COPY(qSlicerMinMaxDoseConstraint);
};

#endif
