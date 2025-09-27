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

#ifndef __qSlicerPlanOptimizerPluginHandler_h
#define __qSlicerPlanOptimizerPluginHandler_h

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QList>

class qSlicerAbstractPlanOptimizer;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \class qSlicerPlanOptimizerPluginHandler
/// \brief Singleton class managing plan optimizer plugins for external beam planning
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerPlanOptimizerPluginHandler : public QObject
{
  Q_OBJECT

public:
  typedef QList<qSlicerAbstractPlanOptimizer*> PlanOptimizerListType;

  /// Instance getter for the singleton class
  /// \return Instance object
  Q_INVOKABLE static qSlicerPlanOptimizerPluginHandler* instance();

  /// Allows cleanup of the singleton at application exit
  static void setInstance(qSlicerPlanOptimizerPluginHandler* instance);

public:
  /// Add a plan optimizer.
  /// The handler will keep a reference to this plan optimizer object,
  /// and it will not be deleted until all these referring classes are deleted.
  Q_INVOKABLE bool registerPlanOptimizer(qSlicerAbstractPlanOptimizer* engine);

  /// Get plan optimizer by its name
  Q_INVOKABLE qSlicerAbstractPlanOptimizer* PlanOptimizerByName(QString name);

  /// Get all registered plan optimizers
  Q_INVOKABLE PlanOptimizerListType registeredPlanOptimizers();

protected:
  /// Registered converter rules
  PlanOptimizerListType m_RegisteredPlanOptimizers;

public:
  /// Private constructor made public to enable python wrapping
  /// IMPORTANT: Should not be used for creating effect handler! Use instance() instead.
  qSlicerPlanOptimizerPluginHandler(QObject* parent=nullptr);

  /// Private destructor made public to enable python wrapping
  ~qSlicerPlanOptimizerPluginHandler() override;

private:
  Q_DISABLE_COPY(qSlicerPlanOptimizerPluginHandler);
  friend class qSlicerPlanOptimizerPluginHandlerCleanup;

private:
  /// Instance of the singleton
  static qSlicerPlanOptimizerPluginHandler* m_Instance;
};

#endif
