#ifndef __qSlicerMockPlanOptimizer_h
#define __qSlicerMockPlanOptimizer_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// ExternalBeamPlanning includes
#include "qSlicerAbstractPlanOptimizer.h"




class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerMockPlanOptimizer : public qSlicerAbstractPlanOptimizer
{
  Q_OBJECT

public:
typedef qSlicerAbstractPlanOptimizer Superclass;
/// Constructor
explicit qSlicerMockPlanOptimizer(QObject* parent=nullptr);
/// Destructor
~qSlicerMockPlanOptimizer() override;


public:
  /// Calculate Optimization for a single beam. Called by \sa CalculateOptimization that performs actions generic
  /// to any Optimization engine before and after calculation.
  /// \param planNode Plan for which the Optimization is carried out.
  /// \param resultOptimizationVolumeNode Output volume node for the result Optimization. It is created by \sa optimizePlan
  Q_INVOKABLE QString optimizePlanUsingOptimizer(vtkMRMLRTPlanNode* planNode, vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode);

private:
  Q_DISABLE_COPY(qSlicerMockPlanOptimizer);
};

#endif