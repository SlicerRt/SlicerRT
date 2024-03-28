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

  This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ)

==============================================================================*/

#ifndef __qSlicerAbstractPlanOptimizer_h
#define __qSlicerAbstractPlanOptimizer_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QStringList>

class qSlicerAbstractPlanOptimizerPrivate;
class vtkMRMLScalarVolumeNode;
class vtkMRMLRTBeamNode;
class vtkMRMLRTPlanNode;
class vtkMRMLNode;
class qMRMLBeamParametersTabWidget;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract Optimization calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific Optimization engine plugins
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerAbstractPlanOptimizer : public QObject
{
  Q_OBJECT

  /// This property stores the name of the Optimization engine.
  /// Cannot be empty.
  /// \sa name(), \sa setName()
  Q_PROPERTY(QString name READ name WRITE setName)

public:
    /// Maximum Gray value for visualization window/level of the newly created per-beam dose volumes
    static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerAbstractPlanOptimizer(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerAbstractPlanOptimizer() override;

  /// Get Optimization engine name
  virtual QString name()const;
  /// Set the name of the Optimization engine
  /// NOTE: name must be defined in constructor in C++ engines, this can only be used in python scripted ones
  virtual void setName(QString name);

// Optimization calculation related functions
public:
  /// Perform Optimization calculation for a single beam
  /// \param Beam node for which the Optimization is calculated
  /// \return Error message. Empty string on success
  QString optimizePlan(vtkMRMLRTPlanNode* planNode);

// API functions to implement in the subclass
protected:
  /// Calculate Optimization for a single beam. Called by \sa CalculateOptimization that performs actions generic
  /// to any Optimization engine before and after calculation.
  /// This is the method that needs to be implemented in each engine.
  ///
  /// \param planNode Plan for which the Optimization is carried out. 
  /// \param resultOptimizationVolumeNode Output volume node for the result Optimization. It is created by \sa optimizePlan
  virtual QString optimizePlanUsingOptimizer(
    vtkMRMLRTPlanNode* planNode,
    vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode ) = 0;

protected:
  /// Name of the engine. Must be set in Optimization engine constructor
  QString m_Name;

protected:
  QScopedPointer<qSlicerAbstractPlanOptimizerPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerAbstractPlanOptimizer);
  Q_DISABLE_COPY(qSlicerAbstractPlanOptimizer);
  friend class qSlicerPlanOptimizerPluginHandler;
  friend class qSlicerPlanOptimizerLogic;
  friend class qSlicerExternalBeamPlanningModuleWidget;


public:
    /// Add result per-beam dose volume to beam
    /// \param resultDose Dose volume to add to beam as result
    /// \param beamNode Beam node to add dose as result to
    /// \param replace Remove referenced dose volume if already exists. True by default
    Q_INVOKABLE void addResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTPlanNode* planNode, bool replace = true);
};

#endif
