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

// vtk includes
#include <vtkSmartPointer.h>

// MRML includes
#include <vtkMRMLObjectiveNode.h>

// Eigen includes
#include <itkeigen/Eigen/SparseCore>


class qSlicerAbstractObjectivePrivate;
// class vtkMRMLScalarVolumeNode;
class vtkMRMLNode;
// class qMRMLBeamParametersTabWidget;
class vtkMRMLObjectiveNode;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \brief Abstract Optimization calculation algorithm that can be used in the
///        External Beam Planning SlicerRT module as a base class for specific Optimization engine plugins
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerAbstractObjective : public QObject
{
  Q_OBJECT

  /// This property stores the name of the Optimization engine.
  /// Cannot be empty.
  /// \sa name(), \sa setName()
  Q_PROPERTY(QString name READ name WRITE setName)

public:
    /// Maximum Gray value for visualization window/level of the newly created per-beam dose volumes
    static double DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM;
    /// Type definitions for dose and objectives
    using DoseType = vtkMRMLObjectiveNode::DoseType;
    using ObjectivesType = vtkMRMLObjectiveNode::ObjectivesType;
	using ObjectiveFunctionAndGradient = vtkMRMLObjectiveNode::ObjectiveFunctionAndGradient;

public:
  typedef QObject Superclass;
  /// Constructor
  explicit qSlicerAbstractObjective(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerAbstractObjective() override;

  /// Get Optimization engine name
  virtual QString name()const;
  /// Set the name of the Optimization engine
  /// NOTE: name must be defined in constructor in C++ engines, this can only be used in python scripted ones
  virtual void setName(QString name);

  virtual ObjectiveFunctionAndGradient computeDoseObjectiveFunctionAndGradient();

protected:
  /// Name of the engine. Must be set in Optimization engine constructor
  QString m_Name;



protected:
  QScopedPointer<qSlicerAbstractObjectivePrivate> d_ptr;


private:
  Q_DECLARE_PRIVATE(qSlicerAbstractObjective);
  Q_DISABLE_COPY(qSlicerAbstractObjective);
  friend class qSlicerObjectivePluginHandler;
  friend class qSlicerObjectiveLogic;
  friend class qSlicerExternalBeamPlanningModuleWidget;

protected:
    virtual float computeDoseObjectiveFunction(const DoseType&, const ObjectivesType&) = 0;
    virtual DoseType& computeDoseObjectiveGradient(const DoseType&, const ObjectivesType&) = 0;
    

};

Q_DECLARE_METATYPE(qSlicerAbstractObjective::DoseType)
Q_DECLARE_METATYPE(qSlicerAbstractObjective::ObjectivesType)

#endif
