/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ)
==============================================================================*/

#ifndef __qSlicerScriptedPlanOptimizer_h
#define __qSlicerScriptedPlanOptimizer_h

// Optimization engines includes
#include "qSlicerAbstractPlanOptimizer.h"

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// VTK includes
#include <vtkSmartPointer.h>

// MRML includes
#include <vtkMRMLRTObjectiveNode.h>

// Forward Declare PyObject*
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif
class qSlicerScriptedPlanOptimizerPrivate;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Scripted abstract engine for implementing Optimization engines in python
///
/// This class provides an interface to engines implemented in python.
/// USAGE: Subclass AbstractScriptedPlanOptimizer in Python subfolder,
///   and register engine by creating this class and setting python source to implemented
///   engine subclass. One example is the MockScriptedPlanOptimizer.
///
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerScriptedPlanOptimizer
  : public qSlicerAbstractPlanOptimizer
{
  Q_OBJECT

public:
  typedef qSlicerAbstractPlanOptimizer Superclass;
  qSlicerScriptedPlanOptimizer(QObject* parent = nullptr);
  ~qSlicerScriptedPlanOptimizer() override;

  Q_INVOKABLE QString pythonSource()const;

  /// Set python source for the implemented engine
  /// \param newPythonSource Python file path
  Q_INVOKABLE bool setPythonSource(const QString newPythonSource);

  /// Convenience method allowing to retrieve the associated scripted instance
  Q_INVOKABLE PyObject* self() const;

  /// Set the name property value.
  /// \sa name
  void setName(QString name) override;

  /// Get the available objectives for the optimizer
  virtual void setAvailableObjectives();
  /// Set the available objectives for the optimizer
  //virtual void setAvailableObjectives(std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives);

// Optimization related functions (API functions to call from the subclass)
protected:
  /// Optimize Treatment Plan Called by \sa optimizePlan that performs actions generic
  /// to any Optimization engine before and after optimization.
  /// This is the method that needs to be implemented in each engine.
  ///
  /// \param planNode Plan which is optimized. 
  /// \param resultOptimizationVolumeNode Output volume node for the Optimization result. It is created by \sa optimizePlan
  virtual QString optimizePlanUsingOptimizer(
    vtkMRMLRTPlanNode* planNode,
    std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives,
    vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode );


protected:
  QScopedPointer<qSlicerScriptedPlanOptimizerPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerScriptedPlanOptimizer);
  Q_DISABLE_COPY(qSlicerScriptedPlanOptimizer);

  //std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> availableObjectives;
};

#endif
