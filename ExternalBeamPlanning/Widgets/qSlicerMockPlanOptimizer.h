/*==============================================================================

Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
Queen's University, Kingston, ON, Canada. All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file was originally developed by Lina Bucher, Institut fuer Biomedizinische
Technik am Karlsruher Institut fuer Technologie (IBT-KIT) and German Cancer
Research Center (DKFZ)

==============================================================================*/

#ifndef __qSlicerMockPlanOptimizer_h
#define __qSlicerMockPlanOptimizer_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// ExternalBeamPlanning includes
#include "qSlicerAbstractPlanOptimizer.h"

// VTK includes
#include <vtkSmartPointer.h>

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
  Q_INVOKABLE QString optimizePlanUsingOptimizer(vtkMRMLRTPlanNode* planNode, std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives, vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode);

  void setAvailableObjectives();

private:
  Q_DISABLE_COPY(qSlicerMockPlanOptimizer);
};

#endif