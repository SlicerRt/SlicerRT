#include "qSlicerIpoptOptimizer.h"

// ExternalBeamPlanning includes
#include "qSlicerAbstractObjective.h"
#include "qSlicerAbstractConstraint.h"
#include "qSlicerSquaredDeviationObjective.h"
#include "qSlicerSquaredOverdosingObjective.h"
#include "qSlicerSquaredUnderdosingObjective.h"
#include "qSlicerMeanDoseObjective.h"
#include "qSlicerEUDObjective.h"
#include "qSlicerMaxDVHObjective.h"
#include "qSlicerMinDVHObjective.h"
#include "qSlicerMinMaxDoseConstraint.h"
#include "qSlicerMinMaxDVHConstraint.h"
#include "qSlicerMinMaxEUDConstraint.h"
#include "qSlicerMinMaxMeanDoseConstraint.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTObjectiveNode.h"
#include "vtkMRMLSegmentationNode.h"
#include <vtkMRMLScalarVolumeNode.h>

// Segmentations includes
#include "vtkSlicerSegmentationsModuleLogic.h"

// MRML includes (continued)
#include <vtkMRMLLabelMapVolumeNode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkStringArray.h>

// Eigen includes
#include <itkeigen/Eigen/Dense>
#include <itkeigen/Eigen/Sparse>

// Qt includes
#include <QDebug>

// STL includes
#include <iostream>
#include <regex>
#include <cassert>
#include <numeric>
#include <memory>

// Static constants
const QString qSlicerIpoptOptimizer::NAME = "Interior Point Optimizer";
const QString qSlicerIpoptOptimizer::SHORT_NAME = "ipopt";
const bool qSlicerIpoptOptimizer::GPU_COMPATIBLE = false;
const bool qSlicerIpoptOptimizer::ALLOW_KEYBOARD_CANCEL = false;
const double qSlicerIpoptOptimizer::DEFAULT_ABS_OBJ_TOL = 1e-6;
const int qSlicerIpoptOptimizer::DEFAULT_MAX_ITER = 10000;
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

  // Function pointers — objectives
  qSlicerIpoptOptimizer::ObjectiveFunction objectiveFunction;
  qSlicerIpoptOptimizer::GradientFunction gradientFunction;

  // Function pointers — constraints
  // constraintFunction: w → g(w), size numConstraints
  // constraintJacobianFunction: w → row-major Jacobian, size numConstraints*|w|
  // constraintBoundsFunction: () → {lb, ub}, each size numConstraints
  std::function<qSlicerIpoptOptimizer::Array(const qSlicerIpoptOptimizer::Array&)> constraintFunction;
  std::function<qSlicerIpoptOptimizer::Array(const qSlicerIpoptOptimizer::Array&)> constraintJacobianFunction;
  std::function<std::pair<qSlicerIpoptOptimizer::Array,
                          qSlicerIpoptOptimizer::Array>()>                         constraintBoundsFunction;
  int numConstraints = 0;

  SmartPtr<qSlicerIpoptOptimizer::IpoptProblem> validateIpoptProblem(
    const qSlicerIpoptOptimizer::Array& x0);

  struct StructureTerm {
    Eigen::SparseMatrix<double> D;
    QString objectiveType;
    double bound;
    double weight;
  };
  std::vector<StructureTerm> structureTerms;
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
    m = d->numConstraints;
    nnz_jac_g = m * n;  // dense Jacobian
    nnz_h_lag = 0;      // limited-memory BFGS — no explicit Hessian
    index_style = TNLP::C_STYLE;
    return true;
  }

  virtual bool get_bounds_info(Index n, Number* x_l, Number* x_u,
                              Index m, Number* g_l, Number* g_u) override
  {
    for (Index i = 0; i < n; i++) {
      x_l[i] = 0.0;   // fluence weights must be non-negative
      x_u[i] = 1e19;
    }
    if (m > 0 && d->constraintBoundsFunction) {
      auto [lb, ub] = d->constraintBoundsFunction();
      for (Index i = 0; i < m; i++) {
        g_l[i] = (i < static_cast<Index>(lb.size())) ? lb[i] : -1e19;
        g_u[i] = (i < static_cast<Index>(ub.size())) ? ub[i] :  1e19;
      }
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
    if (m == 0) return true;
    if (!d->constraintFunction) return false;
    Array xvec(x, x + n);
    Array gvec = d->constraintFunction(xvec);
    for (Index i = 0; i < m; i++)
      g[i] = (i < static_cast<Index>(gvec.size())) ? gvec[i] : 0.0;
    return true;
  }

  virtual bool eval_jac_g(Index n, const Number* x, bool new_x, Index m,
                          Index nele_jac, Index* iRow, Index* jCol, Number* values) override
  {
    if (m == 0) return true;
    if (values == nullptr) {
      // Return sparsity structure — dense: row i, col j → entry i*n+j
      Index k = 0;
      for (Index i = 0; i < m; i++)
        for (Index j = 0; j < n; j++) {
          iRow[k] = i;
          jCol[k] = j;
          ++k;
        }
    } else {
      if (!d->constraintJacobianFunction) return false;
      Array xvec(x, x + n);
      Array jac = d->constraintJacobianFunction(xvec);
      for (Index k2 = 0; k2 < nele_jac; k2++)
        values[k2] = (k2 < static_cast<Index>(jac.size())) ? jac[k2] : 0.0;
    }
    return true;
  }

  virtual void finalize_solution(SolverReturn status, Index n, const Number* x,
                                const Number* z_L, const Number* z_U,
                                Index m, const Number* g, const Number* lambda,
                                Number obj_value, const IpoptData* ip_data,
                                IpoptCalculatedQuantities* ip_cq) override
  {
    d->lastResult.solution = Array(x, x + n);
    d->lastResult.final_objective_value = obj_value;
    d->lastResult.status = static_cast<ApplicationReturnStatus>(status);
    d->lastResult.success = (status == SUCCESS || status == STOP_AT_ACCEPTABLE_POINT);
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

  return SmartPtr<qSlicerIpoptOptimizer::IpoptProblem>(new qSlicerIpoptOptimizer::IpoptProblem(this, x0));
}

//-----------------------------------------------------------------------------
// qSlicerIpoptOptimizer methods

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::qSlicerIpoptOptimizer(QObject* parent)
  : Superclass(parent)
  , d_ptr(new qSlicerIpoptOptimizerPrivate(*this))
{
  this->m_Name = NAME;
  this->setAvailableObjectives();
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::~qSlicerIpoptOptimizer() = default;

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setMaxIterations(int maxIter)
{
  Q_D(qSlicerIpoptOptimizer);

  d->options.max_iter = maxIter;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setMaxTime(double maxTime)
{
  Q_D(qSlicerIpoptOptimizer);

  d->options.max_cpu_time = maxTime;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setAbsoluteObjectiveTolerance(double tol)
{
  Q_D(qSlicerIpoptOptimizer);

  d->options.acceptable_tol = tol;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setObjectiveFunction(ObjectiveFunction func)
{
  Q_D(qSlicerIpoptOptimizer);

  d->objectiveFunction = func;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setGradientFunction(GradientFunction func)
{
  Q_D(qSlicerIpoptOptimizer);

  d->gradientFunction = func;
}


//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::addStructureTerm(
  const QList<double>& data, const QList<int>& rowInd, const QList<int>& colInd,
  int nVoxels, int nBixels, const QString& objectiveType, double bound, double weight)
{
  Q_D(qSlicerIpoptOptimizer);

  using Triplet = Eigen::Triplet<double>;
  std::vector<Triplet> triplets;
  triplets.reserve(data.size());
  for (int k = 0; k < data.size(); ++k)
    triplets.emplace_back(rowInd[k], colInd[k], data[k]);

  qSlicerIpoptOptimizerPrivate::StructureTerm term;
  term.D.resize(nVoxels, nBixels);
  term.D.setFromTriplets(triplets.begin(), triplets.end());
  term.objectiveType = objectiveType;
  term.bound = bound;
  term.weight = weight;

  d->structureTerms.push_back(std::move(term));
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::clearStructureTerms()
{
  Q_D(qSlicerIpoptOptimizer);
  d->structureTerms.clear();
  d->objectiveFunction = nullptr;
  d->gradientFunction  = nullptr;
}

//-----------------------------------------------------------------------------
bool qSlicerIpoptOptimizer::solve(const QList<double>& x0)
{
  Array x0vec(x0.begin(), x0.end());
  Result result = solveProblem(x0vec);
  return result.success;
}

//-----------------------------------------------------------------------------
QList<double> qSlicerIpoptOptimizer::getSolution() const
{
  Q_D(const qSlicerIpoptOptimizer);
  QList<double> sol;
  sol.reserve(static_cast<int>(d->lastResult.solution.size()));
  for (double v : d->lastResult.solution)
    sol.append(v);
  return sol;
}

//-----------------------------------------------------------------------------
qSlicerIpoptOptimizer::Result qSlicerIpoptOptimizer::solveProblem(const Array& x0)
{
  Q_D(qSlicerIpoptOptimizer);

  // Auto-wire objective/gradient from structure terms when no explicit function is set
  if (!d->structureTerms.empty())
  {
    auto sharedTerms = std::make_shared<std::vector<qSlicerIpoptOptimizerPrivate::StructureTerm>>(
      d->structureTerms);

    d->objectiveFunction = [sharedTerms](const Array& w) -> double
    {
      Eigen::VectorXd wv = Eigen::Map<const Eigen::VectorXd>(w.data(), w.size());
      double total = 0.0;
      for (const auto& t : *sharedTerms)
      {
        Eigen::VectorXd dose = t.D * wv;
        double f = 0.0;
        if (t.objectiveType == "SquaredOverdosing") {
          for (int i = 0; i < dose.size(); ++i) {
            double diff = dose[i] - t.bound;
            if (diff > 0.0) f += diff * diff;
          }
        } else if (t.objectiveType == "SquaredUnderdosing") {
          for (int i = 0; i < dose.size(); ++i) {
            double diff = t.bound - dose[i];
            if (diff > 0.0) f += diff * diff;
          }
        } else if (t.objectiveType == "SquaredDeviation") {
          for (int i = 0; i < dose.size(); ++i) {
            double diff = dose[i] - t.bound;
            f += diff * diff;
          }
        }
        total += t.weight * f / dose.size();
      }
      return total;
    };

    d->gradientFunction = [sharedTerms](const Array& w) -> Array
    {
      int n = static_cast<int>(w.size());
      Eigen::VectorXd wv = Eigen::Map<const Eigen::VectorXd>(w.data(), n);
      Eigen::VectorXd grad = Eigen::VectorXd::Zero(n);
      for (const auto& t : *sharedTerms)
      {
        Eigen::VectorXd dose = t.D * wv;
        Eigen::VectorXd gDose = Eigen::VectorXd::Zero(dose.size());
        if (t.objectiveType == "SquaredOverdosing") {
          for (int i = 0; i < dose.size(); ++i) {
            double diff = dose[i] - t.bound;
            if (diff > 0.0) gDose[i] = 2.0 * diff;
          }
        } else if (t.objectiveType == "SquaredUnderdosing") {
          for (int i = 0; i < dose.size(); ++i) {
            double diff = t.bound - dose[i];
            if (diff > 0.0) gDose[i] = -2.0 * diff;
          }
        } else if (t.objectiveType == "SquaredDeviation") {
          for (int i = 0; i < dose.size(); ++i)
            gDose[i] = 2.0 * (dose[i] - t.bound);
        }
        grad += t.weight * (t.D.transpose() * gDose) / dose.size();
      }
      return Array(grad.data(), grad.data() + n);
    };
  }

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
  app->Options()->SetIntegerValue("max_resto_iter", d->options.max_resto_iter);
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
  result.success = (status == Solve_Succeeded || status == Solved_To_Acceptable_Level);

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

  return d->options;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setOptions(const Options& options)
{
  Q_D(qSlicerIpoptOptimizer);

  d->options = options;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setOption(const QString& key, const QVariant& value)
{
  Q_D(qSlicerIpoptOptimizer);


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

  return d->lastResult;
}

//-----------------------------------------------------------------------------
void qSlicerIpoptOptimizer::setAvailableObjectives()
{
  availableObjectives.clear();

  auto addObjective = [this](qSlicerAbstractObjective* obj) {
    ObjectiveStruct s;
    s.name = obj->name();
    s.parameters = obj->getObjectiveParameters();
    s.isConstraint = false;
    availableObjectives.push_back(s);
    delete obj;
  };
  auto addConstraint = [this](qSlicerAbstractConstraint* c) {
    ObjectiveStruct s;
    s.name = c->name();
    s.parameters = c->getConstraintParameters();
    s.isConstraint = true;
    availableObjectives.push_back(s);
    delete c;
  };

  // Objectives
  addObjective(new qSlicerSquaredDeviationObjective());
  addObjective(new qSlicerSquaredOverdosingObjective());
  addObjective(new qSlicerSquaredUnderdosingObjective());
  addObjective(new qSlicerMeanDoseObjective());
  addObjective(new qSlicerEUDObjective());
  addObjective(new qSlicerMaxDVHObjective());
  addObjective(new qSlicerMinDVHObjective());

  // Constraints
  addConstraint(new qSlicerMinMaxDoseConstraint());
  addConstraint(new qSlicerMinMaxDVHConstraint());
  addConstraint(new qSlicerMinMaxEUDConstraint());
  addConstraint(new qSlicerMinMaxMeanDoseConstraint());
}

//-----------------------------------------------------------------------------
// Helper: create a constraint object by name and apply parameters from node attributes.
static std::unique_ptr<qSlicerAbstractConstraint> createConstraintByName(
  const QString& name, vtkMRMLRTObjectiveNode* node)
{
  std::unique_ptr<qSlicerAbstractConstraint> c;
  if      (name == "Min/Max Dose") c = std::make_unique<qSlicerMinMaxDoseConstraint>();
  else if (name == "DVH")         c = std::make_unique<qSlicerMinMaxDVHConstraint>();
  else if (name == "EUD")         c = std::make_unique<qSlicerMinMaxEUDConstraint>();
  else if (name == "Mean Dose")   c = std::make_unique<qSlicerMinMaxMeanDoseConstraint>();
  else return nullptr;

  QMap<QString, QVariant> params = c->getConstraintParameters();
  for (const QString& key : params.keys())
  {
    const char* val = node->GetAttribute(key.toStdString().c_str());
    if (val) params[key] = QString(val).toDouble();
  }
  c->setConstraintParameters(params);
  return c;
}

//-----------------------------------------------------------------------------
// Helper: create an objective object by name and apply parameters from a node attribute map.
static std::unique_ptr<qSlicerAbstractObjective> createObjectiveByName(
  const QString& name, vtkMRMLRTObjectiveNode* node)
{
  std::unique_ptr<qSlicerAbstractObjective> obj;
  if      (name == "Squared Deviation")   obj = std::make_unique<qSlicerSquaredDeviationObjective>();
  else if (name == "Squared Overdosing")  obj = std::make_unique<qSlicerSquaredOverdosingObjective>();
  else if (name == "Squared Underdosing") obj = std::make_unique<qSlicerSquaredUnderdosingObjective>();
  else if (name == "Mean Dose")           obj = std::make_unique<qSlicerMeanDoseObjective>();
  else if (name == "EUD")                 obj = std::make_unique<qSlicerEUDObjective>();
  else if (name == "Max DVH")             obj = std::make_unique<qSlicerMaxDVHObjective>();
  else if (name == "Min DVH")             obj = std::make_unique<qSlicerMinDVHObjective>();
  else return nullptr;

  // Copy parameter values from node attributes
  QMap<QString, QVariant> params = obj->getObjectiveParameters();
  for (const QString& key : params.keys())
  {
    const char* val = node->GetAttribute(key.toStdString().c_str());
    if (val) params[key] = QString(val).toDouble();
  }
  obj->setObjectiveParameters(params);
  return obj;
}

//-----------------------------------------------------------------------------
// Helper: extract flat voxel indices (row indices in D) for a segment.
// Mirrors slicer.util.arrayFromSegmentBinaryLabelmap: exports the segment to a
// temporary labelmap node resampled to the reference volume geometry.
static std::vector<int> getSegmentVoxelIndices(
  vtkMRMLRTObjectiveNode* objNode,
  vtkMRMLVolumeNode* referenceVolumeNode,
  int numVoxels)
{
  std::vector<int> indices;

  vtkMRMLSegmentationNode* segNode = objNode->GetSegmentationNode();
  const char* segID = objNode->GetSegmentID();
  if (!segNode || !segID || !referenceVolumeNode) return indices;

  vtkMRMLScene* scene = segNode->GetScene();
  if (!scene) return indices;

  vtkNew<vtkMRMLLabelMapVolumeNode> tempLabelmap;
  scene->AddNode(tempLabelmap);

  vtkNew<vtkStringArray> segIds;
  segIds->InsertNextValue(segID);

  bool ok = vtkSlicerSegmentationsModuleLogic::ExportSegmentsToLabelmapNode(
    segNode, segIds, tempLabelmap, referenceVolumeNode);

  if (!ok || !tempLabelmap->GetImageData())
  {
    scene->RemoveNode(tempLabelmap);
    qWarning() << "ExportSegmentsToLabelmapNode failed for" << objNode->GetName();
    return indices;
  }

  vtkImageData* img = tempLabelmap->GetImageData();
  int dims[3];
  img->GetDimensions(dims);
  int total = dims[0] * dims[1] * dims[2];

  if (total != numVoxels)
  {
    scene->RemoveNode(tempLabelmap);
    qWarning() << "Labelmap/dose grid size mismatch for" << objNode->GetName()
               << ":" << total << "vs" << numVoxels;
    return indices;
  }

  void* rawPtr = img->GetScalarPointer();
  int scalarType = img->GetScalarType();
  indices.reserve(total / 4);
  for (int i = 0; i < total; ++i)
  {
    double val = 0.0;
    if      (scalarType == VTK_UNSIGNED_CHAR)  val = static_cast<unsigned char*>(rawPtr)[i];
    else if (scalarType == VTK_SHORT)          val = static_cast<short*>(rawPtr)[i];
    else if (scalarType == VTK_UNSIGNED_SHORT) val = static_cast<unsigned short*>(rawPtr)[i];
    else if (scalarType == VTK_INT)            val = static_cast<int*>(rawPtr)[i];
    if (val > 0.0) indices.push_back(i);
  }

  scene->RemoveNode(tempLabelmap);
  return indices;
}

//-----------------------------------------------------------------------------
QString qSlicerIpoptOptimizer::optimizePlanUsingOptimizer(
  vtkMRMLRTPlanNode* planNode,
  std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives,
  vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode)
{
  if (!planNode || !resultOptimizationVolumeNode)
    return "Invalid plan or result volume node";
  if (objectives.empty())
    return "No objectives defined";

  vtkMRMLScalarVolumeNode* refVol = planNode->GetReferenceVolumeNode();
  if (!refVol)
    return "No reference volume on plan";

  // ── Step 1: collect beams and build combined D (numVoxels × totalBixels) ──

  std::vector<vtkMRMLRTBeamNode*> beams;
  planNode->GetBeams(beams);
  if (beams.empty())
    return "No beams in plan";

  // Use dose grid from first beam; fall back to reference volume if not set.
  int doseGridDim[3] = {0, 0, 0};
  double doseGridSpacing[3] = {0.0, 0.0, 0.0};
  beams[0]->GetDoseGridDim(doseGridDim);
  beams[0]->GetDoseGridSpacing(doseGridSpacing);
  bool useRefVolGrid = (doseGridDim[0] <= 0);
  if (useRefVolGrid)
  {
    refVol->GetImageData()->GetDimensions(doseGridDim);
    refVol->GetSpacing(doseGridSpacing);
  }
  int numVoxels = doseGridDim[0] * doseGridDim[1] * doseGridDim[2];

  // Build a temporary volume node representing the dose grid geometry so that
  // ExportSegmentsToLabelmapNode resamples segments to dose-grid resolution,
  // not CT resolution.  Origin and direction are taken from the reference volume;
  // spacing and dims come from the beam's dose grid (or the ref vol when useRefVolGrid).
  vtkMRMLScene* scene = planNode->GetScene();
  vtkNew<vtkMRMLScalarVolumeNode> doseGridVolNode;
  vtkNew<vtkMatrix4x4> doseIJKToRAS; // saved for use when writing the result dose volume
  scene->AddNode(doseGridVolNode);
  {
    vtkNew<vtkImageData> img;
    img->SetDimensions(doseGridDim);
    img->AllocateScalars(VTK_FLOAT, 1);
    doseGridVolNode->SetAndObserveImageData(img);
    doseGridVolNode->SetSpacing(doseGridSpacing);
    doseGridVolNode->SetOrigin(refVol->GetOrigin());

    // Direction: same as refVol but scaled to dose grid spacing
    vtkNew<vtkMatrix4x4> refIJKToRAS;
    refVol->GetIJKToRASMatrix(refIJKToRAS);
    double refSpacing[3];
    refVol->GetSpacing(refSpacing);
    doseIJKToRAS->DeepCopy(refIJKToRAS);
    for (int col = 0; col < 3; ++col)
    {
      double scale = doseGridSpacing[col] / refSpacing[col];
      for (int row = 0; row < 3; ++row)
        doseIJKToRAS->SetElement(row, col, refIJKToRAS->GetElement(row, col) * scale);
    }
    doseGridVolNode->SetIJKToRASMatrix(doseIJKToRAS);
  }

  using SparseD = vtkMRMLRTBeamNode::DoseInfluenceMatrixType; // Eigen ColMajor sparse
  using Triplet = Eigen::Triplet<double>;

  std::vector<Triplet> triplets;
  int colOffset = 0;
  for (vtkMRMLRTBeamNode* beam : beams)
  {
    const SparseD& Di = beam->GetDoseInfluenceMatrix();
    if (Di.rows() == 0 || Di.cols() == 0)
      return QString("Dose influence matrix empty for beam '%1'. "
                     "Run dose calculation first.").arg(beam->GetName());
    for (int k = 0; k < Di.outerSize(); ++k)
      for (SparseD::InnerIterator it(Di, k); it; ++it)
        triplets.emplace_back(static_cast<int>(it.row()),
                              colOffset + static_cast<int>(it.col()),
                              it.value());
    colOffset += static_cast<int>(Di.cols());
  }
  int totalBixels = colOffset;

  SparseD D(numVoxels, totalBixels);
  D.setFromTriplets(triplets.begin(), triplets.end());

  // ── Step 2: map each node → objective or constraint object, voxel indices, penalty ──

  struct StructObj {
    std::unique_ptr<qSlicerAbstractObjective> obj;
    std::vector<int> voxelIndices;
    double penalty;
  };
  struct StructConstraint {
    std::unique_ptr<qSlicerAbstractConstraint> constraint;
    std::vector<int> voxelIndices;
  };

  std::vector<StructObj> structObjs;
  std::vector<StructConstraint> structConstraints;

  for (auto& objNodePtr : objectives)
  {
    vtkMRMLRTObjectiveNode* node = objNodePtr.GetPointer();
    if (!node) continue;

    QString typeName(node->GetName());

    std::vector<int> voxelIdx = getSegmentVoxelIndices(node, doseGridVolNode, numVoxels);
    if (voxelIdx.empty())
    {
      qWarning() << "Could not extract voxel mask for" << node->GetName()
                 << "— applying to all voxels";
      voxelIdx.resize(numVoxels);
      std::iota(voxelIdx.begin(), voxelIdx.end(), 0);
    }

    // Try objective first
    auto obj = createObjectiveByName(typeName, node);
    if (obj)
    {
      double penalty = 1.0;
      const char* penAttr = node->GetAttribute("penalty");
      if (penAttr) penalty = atof(penAttr);
      structObjs.push_back({std::move(obj), std::move(voxelIdx), penalty});
      continue;
    }

    // Try constraint
    auto con = createConstraintByName(typeName, node);
    if (con)
    {
      structConstraints.push_back({std::move(con), std::move(voxelIdx)});
      continue;
    }

    qWarning() << "Unknown objective type:" << node->GetName() << "— skipping";
  }

  scene->RemoveNode(doseGridVolNode);

  if (structObjs.empty() && structConstraints.empty())
    return "No valid objectives or constraints could be configured";

  // ── Step 3: wire IPOPT objective and gradient as lambdas ──
  // f(w) = Σ_i penalty_i * f_i( D[struct_i, :] * w )
  // ∇f/∂w = Σ_i penalty_i * D[struct_i, :]ᵀ * ∇f_i( D[struct_i, :] * w )

  auto sharedSO = std::make_shared<std::vector<StructObj>>(std::move(structObjs));
  auto sharedD  = std::make_shared<SparseD>(std::move(D));

  setObjectiveFunction([sharedSO, sharedD](const Array& w) -> double
  {
    Eigen::VectorXd wv = Eigen::Map<const Eigen::VectorXd>(w.data(), w.size());
    Eigen::VectorXd dose = (*sharedD) * wv;
    double total = 0.0;
    for (auto& so : *sharedSO)
    {
      Eigen::VectorXd dStruct(so.voxelIndices.size());
      for (size_t j = 0; j < so.voxelIndices.size(); ++j)
        dStruct[j] = dose[so.voxelIndices[j]];
      total += so.penalty * static_cast<double>(so.obj->computeDoseObjectiveFunction(dStruct));
    }
    return total;
  });

  setGradientFunction([sharedSO, sharedD](const Array& w) -> Array
  {
    int n = static_cast<int>(w.size());
    Eigen::VectorXd wv = Eigen::Map<const Eigen::VectorXd>(w.data(), n);
    Eigen::VectorXd dose = (*sharedD) * wv;
    Eigen::VectorXd gradW = Eigen::VectorXd::Zero(n);

    for (auto& so : *sharedSO)
    {
      Eigen::VectorXd dStruct(so.voxelIndices.size());
      for (size_t j = 0; j < so.voxelIndices.size(); ++j)
        dStruct[j] = dose[so.voxelIndices[j]];

      Eigen::VectorXd gDoseStruct =
        so.penalty * so.obj->computeDoseObjectiveGradient(dStruct);

      // Scatter structure gradient back to full voxel space, then chain-rule through D.
      Eigen::VectorXd gDoseFull = Eigen::VectorXd::Zero(sharedD->rows());
      for (size_t j = 0; j < so.voxelIndices.size(); ++j)
        gDoseFull[so.voxelIndices[j]] += gDoseStruct[j];

      gradW += sharedD->transpose() * gDoseFull;
    }
    return Array(gradW.data(), gradW.data() + n);
  });

  // ── Step 3b: wire constraints ──
  {
    Q_D(qSlicerIpoptOptimizer);

    // Compute total constraint count
    int totalCon = 0;
    for (auto& sc : structConstraints)
      totalCon += sc.constraint->numConstraints(static_cast<int>(sc.voxelIndices.size()));
    d->numConstraints = totalCon;

    if (totalCon > 0)
    {
      auto sharedSC = std::make_shared<std::vector<StructConstraint>>(std::move(structConstraints));

      d->constraintBoundsFunction = [sharedSC, totalCon]()
        -> std::pair<qSlicerIpoptOptimizer::Array, qSlicerIpoptOptimizer::Array>
      {
        qSlicerIpoptOptimizer::Array lb, ub;
        lb.reserve(totalCon);
        ub.reserve(totalCon);
        for (auto& sc : *sharedSC)
        {
          int nv = static_cast<int>(sc.voxelIndices.size());
          Eigen::VectorXd lbv = sc.constraint->lowerBounds(nv);
          Eigen::VectorXd ubv = sc.constraint->upperBounds(nv);
          for (int i = 0; i < lbv.size(); ++i) { lb.push_back(lbv[i]); ub.push_back(ubv[i]); }
        }
        return {lb, ub};
      };

      d->constraintFunction = [sharedSC, sharedD, totalCon](const qSlicerIpoptOptimizer::Array& w)
        -> qSlicerIpoptOptimizer::Array
      {
        Eigen::VectorXd wv = Eigen::Map<const Eigen::VectorXd>(w.data(), w.size());
        Eigen::VectorXd dose = (*sharedD) * wv;
        qSlicerIpoptOptimizer::Array g;
        g.reserve(totalCon);
        for (auto& sc : *sharedSC)
        {
          Eigen::VectorXd dStruct(sc.voxelIndices.size());
          for (size_t j = 0; j < sc.voxelIndices.size(); ++j)
            dStruct[j] = dose[sc.voxelIndices[j]];
          Eigen::VectorXd gv = sc.constraint->computeDoseConstraintFunction(dStruct);
          for (int i = 0; i < gv.size(); ++i) g.push_back(gv[i]);
        }
        return g;
      };

      d->constraintJacobianFunction = [sharedSC, sharedD, totalCon](const qSlicerIpoptOptimizer::Array& w)
        -> qSlicerIpoptOptimizer::Array
      {
        int nBixels = static_cast<int>(sharedD->cols());
        Eigen::VectorXd wv = Eigen::Map<const Eigen::VectorXd>(w.data(), w.size());
        Eigen::VectorXd dose = (*sharedD) * wv;
        qSlicerIpoptOptimizer::Array jac(totalCon * nBixels, 0.0);
        int rowOffset = 0;
        for (auto& sc : *sharedSC)
        {
          int nv = static_cast<int>(sc.voxelIndices.size());
          Eigen::VectorXd dStruct(nv);
          for (int j = 0; j < nv; ++j) dStruct[j] = dose[sc.voxelIndices[j]];
          Eigen::MatrixXd Jdose = sc.constraint->computeDoseConstraintJacobian(dStruct);
          int numCon = static_cast<int>(Jdose.rows());
          for (int ci = 0; ci < numCon; ++ci)
          {
            Eigen::VectorXd jFull = Eigen::VectorXd::Zero(sharedD->rows());
            for (int j = 0; j < nv; ++j) jFull[sc.voxelIndices[j]] = Jdose(ci, j);
            Eigen::VectorXd jW = sharedD->transpose() * jFull;
            for (int j = 0; j < nBixels; ++j)
              jac[(rowOffset + ci) * nBixels + j] = jW[j];
          }
          rowOffset += numCon;
        }
        return jac;
      };
    }
  }

  // ── Step 4: solve ──
  emit progressInfoUpdated("Starting IPOPT optimization...");
  Array w0(totalBixels, 1.0 / totalBixels); // uniform initial fluence
  Result result = solveProblem(w0);
  if (!result.success)
    return QString("IPOPT solver did not converge (status %1)").arg(result.status);

  // ── Step 5: compute and store result dose volume ──
  Eigen::VectorXd wOpt = Eigen::Map<Eigen::VectorXd>(
    result.solution.data(), result.solution.size());
  Eigen::VectorXd doseOpt = (*sharedD) * wOpt;

  vtkNew<vtkImageData> doseImg;
  doseImg->SetDimensions(doseGridDim);
  doseImg->AllocateScalars(VTK_FLOAT, 1);
  float* ptr = static_cast<float*>(doseImg->GetScalarPointer());
  for (int i = 0; i < numVoxels; ++i)
    ptr[i] = static_cast<float>(doseOpt[i]);

  resultOptimizationVolumeNode->SetAndObserveImageData(doseImg);
  resultOptimizationVolumeNode->SetIJKToRASMatrix(doseIJKToRAS);
  resultOptimizationVolumeNode->SetName(
    (std::string(planNode->GetName()) + "_IpoptOptimizedDose").c_str());

  emit progressInfoUpdated("IPOPT optimization complete.");
  return QString(); // empty = success
}
