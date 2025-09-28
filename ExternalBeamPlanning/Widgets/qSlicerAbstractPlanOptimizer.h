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

  This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ).

==============================================================================*/

#ifndef __qSlicerAbstractPlanOptimizer_h
#define __qSlicerAbstractPlanOptimizer_h

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"
#include "qSlicerAbstractObjective.h"

// Qt includes
#include <QObject>

// VTK includes
#include <vtkSmartPointer.h>

class qSlicerAbstractPlanOptimizerPrivate;
class vtkMRMLScalarVolumeNode;
class vtkMRMLRTBeamNode;
class vtkMRMLRTPlanNode;
class vtkMRMLNode;
class vtkMRMLRTObjectiveNode;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract plan optimization algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific plan optimizer plugins
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerAbstractPlanOptimizer : public QObject
{
  Q_OBJECT

  /// This property stores the name of the plan optimizer.
  /// Cannot be empty.
  /// \sa name(), \sa setName()
  Q_PROPERTY(QString name READ name WRITE setName)

public:
  /// Maximum Gray value for visualization window/level of the newly created dose volumes
  static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerAbstractPlanOptimizer(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerAbstractPlanOptimizer() override;

  /// Get plan optimizer name
  virtual QString name()const;
  /// Set the name of the plan optimizer
  /// NOTE: name must be defined in constructor in C++ engines, this can only be used in python scripted ones
  virtual void setName(QString name);

public:
  /// Structure to represent objective functions (only name and parameters)
  struct ObjectiveStruct
  {
    QString name;
    QMap<QString, QVariant> parameters;
  };

signals:
  void progressInfoUpdated(QString info);

// Optimization calculation related functions
public:
  /// Perform optimization on a plan
  /// \param planNode Plan which is optimized.
  /// \return Error message. Empty string on success
  QString optimizePlan(vtkMRMLRTPlanNode* planNode);

  /// Get available objective functions (only name and parameters) for the plan optimizer
  std::vector<ObjectiveStruct> getAvailableObjectives();
  /// Set available objective functions (only name and parameters) for the plan optimizer 
  virtual void setAvailableObjectives();

  /// Get saved objectives for the plan optimizer
  std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> getSavedObjectiveNodes();

  /// Save objectiveNode in optimizer (in savedObjectiveNodes)
  void saveObjectiveNodeInOptimizer(vtkSmartPointer<vtkMRMLRTObjectiveNode> objectiveNode);

  /// Remove all objectiveNodes from optimizer (clear savedObjectiveNodes)
  void removeAllObjectiveNodes();

// API functions to implement in the subclass
protected:
  /// Optimize for a plan. Called by \sa CalculateOptimization that performs actions generic
  /// to any plan optimizer before and after calculation.
  /// This is the method that needs to be implemented in each engine.
  ///
  /// \param planNode Plan which is optimized.
  /// \param objectives List of objective nodes defining the objectives for the plan optimization
  /// \param resultOptimizationVolumeNode Output volume node for the result optimized dose. It is created by \sa optimizePlan
  
  virtual QString optimizePlanUsingOptimizer(
    vtkMRMLRTPlanNode* planNode,
    std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives,
    vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode ) = 0;


protected:
  /// Name of the engine. Must be set in plan optimizer constructor
  QString m_Name;


protected:
  QScopedPointer<qSlicerAbstractPlanOptimizerPrivate> d_ptr;

  /// Available objective functions (only name and parameters)
  std::vector<ObjectiveStruct> availableObjectives;

  /// Saved objectiveNodes
  std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> savedObjectiveNodes;


private:
  Q_DECLARE_PRIVATE(qSlicerAbstractPlanOptimizer);
  Q_DISABLE_COPY(qSlicerAbstractPlanOptimizer);
  friend class qSlicerPlanOptimizerPluginHandler;
  friend class qSlicerPlanOptimizerLogic;
  friend class qSlicerExternalBeamPlanningModuleWidget;


public:
  /// Add result optimized dose volume to plan
  /// \param resultOptimizedDose Dose volume to add to plan as result
  /// \param planNode Plan node to add dose as result to
  /// \param replace Remove referenced dose volume if already exists. True by default
  Q_INVOKABLE void addResultOptimizedDose(vtkMRMLScalarVolumeNode* resultOptimizedDose, vtkMRMLRTPlanNode* planNode, bool replace = true);
};
Q_DECLARE_METATYPE(qSlicerAbstractPlanOptimizer::ObjectiveStruct)

#endif
