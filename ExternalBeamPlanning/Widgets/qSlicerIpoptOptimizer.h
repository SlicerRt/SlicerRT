#ifndef __qSlicerIpoptOptimizer_h
#define __qSlicerIpoptOptimizer_h

// Qt includes
#include <QObject>
#include <QMap>
#include <QVariant>
#include <QString>

// IPOPT includes
#include "IpIpoptApplication.hpp"
#include "IpTNLP.hpp"
#include "IpSolveStatistics.hpp"

// STL includes
#include <vector>
#include <map>
#include <functional>
#include <memory>


#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

using namespace Ipopt;

class qSlicerIpoptOptimizerPrivate;

/// \brief Interior Point Optimizer using IPOPT
///
/// This class provides a C++ interface to the IPOPT optimization library,
/// translating the functionality from the Python OptimizerIpopt class.
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerIpoptOptimizer : public QObject
{
  Q_OBJECT

public:
  typedef QObject Superclass;

  /// Constructor
  explicit qSlicerIpoptOptimizer(QObject* parent = nullptr);

  /// Destructor
  ~qSlicerIpoptOptimizer() override;

  /// Optimizer properties
  static const QString NAME;
  static const QString SHORT_NAME;
  static const bool GPU_COMPATIBLE;
  static const bool ALLOW_KEYBOARD_CANCEL;

  /// Default tolerances and limits
  static const double DEFAULT_TOLERANCE;
  static const double DEFAULT_ABS_OBJ_TOL;
  static const int DEFAULT_MAX_ITER;
  static const double DEFAULT_MAX_TIME;

public:
  /// Type definitions for arrays and callbacks
  using Array = std::vector<double>;
  using ObjectiveFunction = std::function<double(const Array&)>;
  using GradientFunction = std::function<Array(const Array&)>;
  using CallbackFunction = std::function<bool(int, double, const Array&)>;

  /// Options container
  struct Options {
    int print_level = 5;
    QString print_user_options = "no";
    QString print_options_documentation = "no";
    double tol = 1e-10;
    double dual_inf_tol = 1e-4;
    double constr_viol_tol = 1e-4;
    double compl_inf_tol = 1e-4;
    int acceptable_iter = 5;
    double acceptable_tol = DEFAULT_ABS_OBJ_TOL;
    double acceptable_constr_viol_tol = 1e-2;
    double acceptable_dual_inf_tol = 1e10;
    double acceptable_compl_inf_tol = 1e10;
    double acceptable_obj_change_tol = 1e-4;
    int max_iter = DEFAULT_MAX_ITER;
    double max_cpu_time = DEFAULT_MAX_TIME;
    QString mu_strategy = "adaptive";
    QString hessian_approximation = "limited-memory";
    int limited_memory_max_history = 20;
    QString limited_memory_initialization = "scalar2";
    QString linear_solver = "mumps";
    QString print_timing_statistics = "yes";
  };

  /// Result structure
  struct Result {
    Array solution;
    ApplicationReturnStatus status;
    double final_objective_value = 0.0;
    int iteration_count = 0;
    bool success = false;
  };

public slots:
  /// Set optimization parameters
  void setMaxIterations(int maxIter);
  void setMaxTime(double maxTime);
  void setAbsoluteObjectiveTolerance(double tol);

  /// Set objective and gradient functions
  void setObjectiveFunction(ObjectiveFunction func);
  void setGradientFunction(GradientFunction func);
  void setIntermediateCallback(CallbackFunction callback);



public:
  /// Solve the optimization problem
  Result solveProblem(const Array& x0);

  /// Get/set options
  Options getOptions() const;
  void setOptions(const Options& options);
  void setOption(const QString& key, const QVariant& value);

  /// Get the last result
  Result getLastResult() const;

signals:
  /// Emitted during optimization iterations
  void iterationUpdate(int iteration, double objectiveValue);

  /// Emitted when optimization completes
  void optimizationCompleted(bool success, const QString& message);

private:
  /// Private implementation
  QScopedPointer<qSlicerIpoptOptimizerPrivate> d_ptr;
  Q_DECLARE_PRIVATE(qSlicerIpoptOptimizer);
  Q_DISABLE_COPY(qSlicerIpoptOptimizer);

  /// Internal TNLP implementation
  class IpoptProblem;
  friend class IpoptProblem;
};

#endif
