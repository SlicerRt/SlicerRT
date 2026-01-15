#include "qSlicerIpoptOptimizer.h"

// Qt includes
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

// STL includes
#include <iostream>
#include <regex>
#include <cassert>

// Static constants
const QString qSlicerIpoptOptimizer::NAME = "Interior Point Optimizer";
const QString qSlicerIpoptOptimizer::SHORT_NAME = "ipopt";
const bool qSlicerIpoptOptimizer::GPU_COMPATIBLE = false;
const bool qSlicerIpoptOptimizer::ALLOW_KEYBOARD_CANCEL = true;
const double qSlicerIpoptOptimizer::DEFAULT_TOLERANCE = 1e-10;
const double qSlicerIpoptOptimizer::DEFAULT_ABS_OBJ_TOL = 1e-6;
const int qSlicerIpoptOptimizer::DEFAULT_MAX_ITER = 1000;
const double qSlicerIpoptOptimizer::DEFAULT_MAX_TIME = 3600.0;

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerIpoptOptimizerPrivate
{
  Q_DECLARE_PUBLIC(qSlicerIpoptOptimizer);
protected:
  qSlicerIpoptOptimizer* const q_ptr;

public:
  qSlicerIpoptOptimizerPrivate(qSlicerIpoptOptimizer& object);
  ~qSlicerIpoptOptimizerPrivate();

  // Options and settings
  qSlicerIpoptOptimizer::Options options;
  qSlicerIpoptOptimizer::Result lastResult;

  // Function pointers
  qSlicerIpoptOptimizer::ObjectiveFunction objectiveFunction;
  qSlicerIpoptOptimizer::GradientFunction gradientFunction;
  qSlicerIpoptOptimizer::CallbackFunction intermediateCallback;

  // Cancellation support
  std::atomic<bool> cancellationRequested{false};
  bool keyboardCancelEnabled = true;

  // Thread safety
  mutable QMutex mutex;

  // Validate and create IPOPT problem
  SmartPtr<qSlicerIpoptOptimizer::IpoptProblem> validateIpoptProblem(
    const qSlicerIpoptOptimizer::Array& x0);

  // Version handling for options
  void normalizeTimingStatisticsOption();
};

//-----------------------------------------------------------------------------
/// Internal TNLP implementation for IPOPT
class qSlicerIpoptOptimizer::IpoptProblem : public TNLP
{
private:
  qSlicerIpoptOptimizerPrivate* d;
  Array x0;
  int n;

public:
  IpoptProblem(qSlicerIpoptOptimizerPrivate* dPtr, const Array& initialPoint)
    : d(dPtr), x0(initialPoint), n(static_cast<int>(initialPoint.size()))
  {}

  virtual ~IpoptProblem() = default;

  // TNLP interface implementation
  virtual bool get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                           Index& nnz_h_lag, IndexStyleEnum& index_style) override
  {
    n = this->n;
    m = 0;  // No constraints for now
    nnz_jac_g = 0;  // No constraint jacobian
    nnz_h_lag = 0;  // Will use limited-memory BFGS
    index_style = TNLP::C_STYLE;  // 0-based indexing
    return true;
  }

  virtual bool get_bounds_info(Index n, Number* x_l, Number* x_u,
                              Index m, Number* g_l, Number* g_u) override
  {
    // Set variable bounds (unbounded for now)
    for (Index i = 0; i < n; i++) {
      x_l[i] = -1e19;
      x_u[i] = 1e19;
    }
    return true;
  }

  virtual bool get_starting_point(Index n, bool init_x, Number* x,
                                 bool init_z, Number* z_L, Number* z_U,
                                 Index m, bool init_lambda, Number* lambda) override
  {
    assert(init_x == true);
    assert(init_z == false);
    assert(init_lambda == false);

    // Set initial point
    for (Index i = 0; i < n; i++) {
      x[i] = (i < static_cast<Index>(x0.size())) ? x0[i] : 0.0;
    }
    return true;
  }

  virtual bool eval_f(Index n, const Number* x, bool new_x, Number& obj_value) override
  {
    if (!d->objectiveFunction) {
      return false;
    }

    Array xvec(x, x + n);
    obj_value = d->objectiveFunction(xvec);
    return true;
  }

  virtual bool eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f) override
  {
    if (!d->gradientFunction) {
      return false;
    }

    Array xvec(x, x + n);
    Array gradient = d->gradientFunction(xvec);

    if (gradient.size() != static_cast<size_t>(n)) {
      return false;
    }

    for (Index i = 0; i < n; i++) {
      grad_f[i] = gradient[i];
    }
    return true;
  }

  virtual bool eval_g(Index n, const Number* x, bool new_x, Index m, Number* g) override
  {
    // No constraints
    return true;
  }

  virtual bool eval_jac_g(Index n, const Number* x, bool new_x, Index m,
                         Index nele_jac, Index* iRow, Index* jCol, Number* values) override
  {
    // No constraints
    return true;
  }

  virtual bool intermediate_callback(AlgorithmMode mode, Index iter, Number obj_value,
                                    Number inf_pr, Number inf_du, Number mu,
                                    Number d_norm, Number regularization_size,
                                    Number alpha_du, Number alpha_pr, Index ls_trials,
                                    const IpoptData* ip_data,
                                    IpoptCalculatedQuantities* ip_cq) override
  {
    // Check for cancellation
    if (d->cancellationRequested.load()) {
      return false; // Abort optimization
    }

    // Call user callback if provided
    if (d->intermediateCallback) {
      Array currentX; // Would need to extract current x from ip_data if needed
      return d->intermediateCallback(static_cast<int>(iter), obj_value, currentX);
    }

    return true; // Continue optimization
  }

  virtual void finalize_solution(SolverReturn status, Index n, const Number* x,
                                const Number* z_L, const Number* z_U,
                                Index m, const Number* g, const Number* lambda,
                                Number obj_value, const IpoptData* ip_data,
                                IpoptCalculatedQuantities* ip_cq) override
  {
    // Store results in the private class
    QMutexLocker locker(&d->mutex);
    d->lastResult.solution = Array(x, x + n);
    d->lastResult.final_objective_value = obj_value;
    d->lastResult.status = static_cast<ApplicationReturnStatus>(status);
    d->lastResult.success = (status == SUCCESS);
  }
};

//-----------------------------------------------------------------------------
// qSlicerIpoptOptimizerPrivate methods

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizerPrivate::qSlicerIpoptOptimizerPrivate(qSlicerIpoptOptimizer& object)
  : q_ptr(&object)
{
  // Initialize default options
  options.acceptable_tol = qSlicerIpoptOptimizer::DEFAULT_ABS_OBJ_TOL;
  options.max_iter = qSlicerIpoptOptimizer::DEFAULT_MAX_ITER;
  options.max_cpu_time = qSlicerIpoptOptimizer::DEFAULT_MAX_TIME;
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizerPrivate::~qSlicerIpoptOptimizerPrivate() = default;

//-----------------------------------------------------------------------------
SmartPtr<qSlicerIpoptOptimizer::IpoptProblem> qSlicerIpoptOptimizerPrivate::validateIpoptProblem(
  const qSlicerIpoptOptimizer::Array& x0)
{
  // Validate required functions
  if (!objectiveFunction) {
    qCritical() << "Objective function not set";
    return nullptr;
  }

  if (!gradientFunction) {
    qCritical() << "Gradient function not set";
    return nullptr;
  }

  if (x0.empty()) {
    qCritical() << "Initial point is empty";
    return nullptr;
  }

  // Handle version-specific options
  normalizeTimingStatisticsOption();

  return SmartPtr<qSlicerIpoptOptimizer::IpoptProblem>(new qSlicerIpoptOptimizer::IpoptProblem(this, x0));
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizerPrivate::normalizeTimingStatisticsOption()
{
  // This mimics the Python version's handling of timing statistics option names
  // For now, we assume the newer "print_timing_statistics" is supported
  // In a real implementation, you might want to check the IPOPT version
}

//-----------------------------------------------------------------------------
// qSlicerIpoptOptimizer methods

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::qSlicerIpoptOptimizer(QObject* parent)
  : Superclass(parent)
  , d_ptr(new qSlicerIpoptOptimizerPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::~qSlicerIpoptOptimizer() = default;

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setMaxIterations(int maxIter)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->options.max_iter = maxIter;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setMaxTime(double maxTime)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->options.max_cpu_time = maxTime;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setAbsoluteObjectiveTolerance(double tol)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->options.acceptable_tol = tol;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setObjectiveFunction(ObjectiveFunction func)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->objectiveFunction = func;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setGradientFunction(GradientFunction func)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->gradientFunction = func;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setIntermediateCallback(CallbackFunction callback)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->intermediateCallback = callback;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setKeyboardCancelEnabled(bool enabled)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->keyboardCancelEnabled = enabled;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::requestCancellation()
{
  Q_D(qSlicerIpoptOptimizer);
  d->cancellationRequested.store(true);
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::Result qSlicerIpoptOptimizer::solveProblem(const Array& x0)
{
  Q_D(qSlicerIpoptOptimizer);

  // Reset cancellation flag
  d->cancellationRequested.store(false);

  // Update options with current values
  d->options.max_iter = d->options.max_iter;
  d->options.max_cpu_time = d->options.max_cpu_time;
  d->options.acceptable_tol = d->options.acceptable_tol;

  // Create and validate IPOPT problem
  SmartPtr<IpoptProblem> nlp = d->validateIpoptProblem(x0);
  if (IsNull(nlp)) {
    Result result;
    result.success = false;
    result.status = Invalid_Problem_Definition;
    return result;
  }

  // Create IPOPT application
  SmartPtr<IpoptApplication> app = IpoptApplicationFactory();

  // Set options
  app->Options()->SetNumericValue("tol", d->options.tol);
  app->Options()->SetNumericValue("dual_inf_tol", d->options.dual_inf_tol);
  app->Options()->SetNumericValue("constr_viol_tol", d->options.constr_viol_tol);
  app->Options()->SetNumericValue("compl_inf_tol", d->options.compl_inf_tol);
  app->Options()->SetIntegerValue("acceptable_iter", d->options.acceptable_iter);
  app->Options()->SetNumericValue("acceptable_tol", d->options.acceptable_tol);
  app->Options()->SetNumericValue("acceptable_constr_viol_tol", d->options.acceptable_constr_viol_tol);
  app->Options()->SetNumericValue("acceptable_dual_inf_tol", d->options.acceptable_dual_inf_tol);
  app->Options()->SetNumericValue("acceptable_compl_inf_tol", d->options.acceptable_compl_inf_tol);
  app->Options()->SetNumericValue("acceptable_obj_change_tol", d->options.acceptable_obj_change_tol);
  app->Options()->SetIntegerValue("max_iter", d->options.max_iter);
  app->Options()->SetNumericValue("max_cpu_time", d->options.max_cpu_time);
  app->Options()->SetStringValue("mu_strategy", d->options.mu_strategy.toStdString());
  app->Options()->SetStringValue("hessian_approximation", d->options.hessian_approximation.toStdString());
  app->Options()->SetIntegerValue("limited_memory_max_history", d->options.limited_memory_max_history);
  app->Options()->SetStringValue("limited_memory_initialization", d->options.limited_memory_initialization.toStdString());
  app->Options()->SetStringValue("linear_solver", d->options.linear_solver.toStdString());
  app->Options()->SetIntegerValue("print_level", d->options.print_level);
  app->Options()->SetStringValue("print_user_options", d->options.print_user_options.toStdString());
  app->Options()->SetStringValue("print_options_documentation", d->options.print_options_documentation.toStdString());
  app->Options()->SetStringValue("print_timing_statistics", d->options.print_timing_statistics.toStdString());

  // Initialize the application
  ApplicationReturnStatus status = app->Initialize();
  if (status != Solve_Succeeded) {
    qCritical() << "IPOPT initialization failed with status:" << status;
    Result result;
    result.success = false;
    result.status = status;
    return result;
  }

  // Solve the problem
  status = app->OptimizeTNLP(nlp);

  // Get results
  Result result = d->lastResult;
  result.status = status;
  result.success = (status == Solve_Succeeded);

  if (result.success) {
    result.iteration_count = app->Statistics()->IterationCount();
  }

  // Emit completion signal
  emit optimizationCompleted(result.success,
    result.success ? "Optimization completed successfully" : "Optimization failed");

  return result;
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::Options qSlicerIpoptOptimizer::getOptions() const
{
  Q_D(const qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  return d->options;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setOptions(const Options& options)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  d->options = options;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setOption(const QString& key, const QVariant& value)
{
  Q_D(qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);

  // Handle individual option setting
  if (key == "print_level") {
    d->options.print_level = value.toInt();
  } else if (key == "tol") {
    d->options.tol = value.toDouble();
  } else if (key == "max_iter") {
    d->options.max_iter = value.toInt();
  } else if (key == "max_cpu_time") {
    d->options.max_cpu_time = value.toDouble();
  } else if (key == "acceptable_tol") {
    d->options.acceptable_tol = value.toDouble();
  }
  // Add more option handling as needed
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::Result qSlicerIpoptOptimizer::getLastResult() const
{
  Q_D(const qSlicerIpoptOptimizer);
  QMutexLocker locker(&d->mutex);
  return d->lastResult;
}
