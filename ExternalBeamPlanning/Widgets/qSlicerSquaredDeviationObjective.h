/*==============================================================================

  Copyright (c) German Cancer Research Center (DKFZ),
  Heidelberg, Germany. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Lina Bucher, German Cancer
  Research Center (DKFZ) and Institute for Biomedical Engineering (IBT),
  Karlsruhe Institute of Technology (KIT).

==============================================================================*/

#ifndef __qSlicerSquaredDeviationObjective_h
#define __qSlicerSquaredDeviationObjective_h

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
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
  Q_INVOKABLE float computeDoseObjectiveFunction(const DoseType& doseMatrix) override;
  Q_INVOKABLE DoseType computeDoseObjectiveGradient(const DoseType& doseMatrix) override;

protected:
  void initializeParameters() override;

private:
  Q_DISABLE_COPY(qSlicerSquaredDeviationObjective);
};

#endif
