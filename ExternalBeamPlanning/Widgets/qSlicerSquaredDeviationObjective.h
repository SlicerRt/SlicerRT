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


private:
  float computeDoseObjectiveFunction(const DoseType&, const ObjectivesType&);

  DoseType& computeDoseObjectiveGradient(const DoseType&, const ObjectivesType&);

private:
  Q_DISABLE_COPY(qSlicerSquaredDeviationObjective);
};

#endif