#ifndef __qSlicerAbstractConstraint_h
#define __qSlicerAbstractConstraint_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QString>

#include <itkeigen/Eigen/Dense>

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract base class for dose constraint functions used in plan optimization.
///
/// Mirrors the matRad_DoseConstraint interface. Each concrete constraint provides:
///   - The number of scalar constraint equations it contributes
///   - The constraint function values g(dose)
///   - The Jacobian dg/d(dose)  (rows = constraints, cols = voxels)
///   - Upper and lower bounds for g
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerAbstractConstraint : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString name READ name WRITE setName)

public:
  using DoseType = Eigen::VectorXd;

  typedef QObject Superclass;
  explicit qSlicerAbstractConstraint(QObject* parent = nullptr);
  ~qSlicerAbstractConstraint() override;

  virtual QString name() const;
  virtual void setName(QString name);

  QMap<QString, QVariant> getConstraintParameters() const;
  void setConstraintParameters(QMap<QString, QVariant> parameters);

  /// Number of scalar constraint equations this instance contributes.
  virtual int numConstraints(int numVoxels) const = 0;

  /// Constraint function values: vector of length numConstraints(n).
  Q_INVOKABLE virtual DoseType computeDoseConstraintFunction(const DoseType& dose) = 0;

  /// Jacobian: matrix of shape (numConstraints, numVoxels).
  Q_INVOKABLE virtual Eigen::MatrixXd computeDoseConstraintJacobian(const DoseType& dose) = 0;

  /// Upper bounds for each constraint equation.
  Q_INVOKABLE virtual DoseType upperBounds(int numVoxels) = 0;

  /// Lower bounds for each constraint equation.
  Q_INVOKABLE virtual DoseType lowerBounds(int numVoxels) = 0;

protected:
  QString m_Name;
  QMap<QString, QVariant> constraintParameters;
  virtual void initializeParameters() = 0;

private:
  Q_DISABLE_COPY(qSlicerAbstractConstraint);
};

#endif
