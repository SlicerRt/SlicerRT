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

  This file was originally developed by ...

==============================================================================*/

#ifndef __qSlicerAbstractObjective_h
#define __qSlicerAbstractObjective_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QStringList>
#include <QMap>
#include <QVariant>

// Eigen includes
#include <itkeigen/Eigen/SparseCore>


class qSlicerAbstractObjectivePrivate;
// class vtkMRMLScalarVolumeNode;
class vtkMRMLNode;
// class qMRMLBeamParametersTabWidget;
class vtkMRMLRTObjectiveNode;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract Optimization calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific Optimization engine plugins
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerAbstractObjective : public QObject
{
  Q_OBJECT

  /// This property stores the name of the OObjective.
  /// Cannot be empty.
  /// \sa name(), \sa setName()
  Q_PROPERTY(QString name READ name WRITE setName)

public:
    /// Maximum Gray value for visualization window/level of the newly created per-beam dose volumes
    static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;
    /// Type definitions for dose and objectives
    using DoseType = Eigen::VectorXd;

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerAbstractObjective(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerAbstractObjective() override;

  /// Get Objective name
  virtual QString name()const;
  /// Set the name of the Objective
  /// NOTE: name must be defined in constructor in C++ engines, this can only be used in python scripted ones
  virtual void setName(QString name);

  /// Get Objective parameters
  Q_INVOKABLE QMap<QString, QVariant> getObjectiveParameters() const;
  /// Set Objective parameters
  void setObjectiveParameters(QMap<QString, QVariant> parameters);


protected:
  /// Name of the engine. Must be set in Objective constructor
  QString m_Name;
  QMap<QString, QVariant> objectivesParameters;

  /// Pure virtual method to initialize parameters, must be implemented by subclasses
  virtual void initializeParameters() = 0;

protected:
  QScopedPointer<qSlicerAbstractObjectivePrivate> d_ptr;


private:
  Q_DECLARE_PRIVATE(qSlicerAbstractObjective);
  Q_DISABLE_COPY(qSlicerAbstractObjective);
  friend class qSlicerObjectivePluginHandler;
  friend class qSlicerObjectiveLogic;
  friend class qSlicerExternalBeamPlanningModuleWidget;

public:
    Q_INVOKABLE virtual float computeDoseObjectiveFunction(const DoseType&) = 0;
    Q_INVOKABLE virtual DoseType computeDoseObjectiveGradient(const DoseType&) = 0;
};

Q_DECLARE_METATYPE(qSlicerAbstractObjective::DoseType)

#endif
