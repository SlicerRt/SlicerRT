#ifndef __qSlicerSquaredDeviationObjective_h
#define __qSlicerSquaredDeviationObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// ExternalBeamPlanning includes
#include "qSlicerAbstractObjective.h"

class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerSquaredDeviationObjective : public qSlicerAbstractObjective
{
  Q_OBJECT

public:
typedef qSlicerAbstractObjective Superclass;
/// Constructor
explicit qSlicerSquaredDeviationObjective(QObject* parent=nullptr);
/// Destructor
~qSlicerSquaredDeviationObjective() override;


public:
  Q_INVOKABLE std::function<QString(const DoseType&, const ObjectivesType&)> computeDoseObjectiveFunction();

  Q_INVOKABLE std::function<QString(const DoseType&, const ObjectivesType&)> computeDoseObjectiveGradient();

private:
  Q_DISABLE_COPY(qSlicerSquaredDeviationObjective);
};

#endif