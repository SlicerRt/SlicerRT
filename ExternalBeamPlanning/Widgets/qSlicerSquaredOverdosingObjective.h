#ifndef __qSlicerSquaredOverdosingObjective_h
#define __qSlicerSquaredOverdosingObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractObjective.h"

class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerSquaredOverdosingObjective
  : public qSlicerAbstractObjective
{
  Q_OBJECT

public:
  typedef qSlicerAbstractObjective Superclass;
  explicit qSlicerSquaredOverdosingObjective(QObject* parent = nullptr);
  ~qSlicerSquaredOverdosingObjective() override;

  Q_INVOKABLE float computeDoseObjectiveFunction(const DoseType& dose) override;
  Q_INVOKABLE DoseType computeDoseObjectiveGradient(const DoseType& dose) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerSquaredOverdosingObjective);
};

#endif
