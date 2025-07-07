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

#ifndef __qSlicerObjectivePluginHandler_h
#define __qSlicerObjectivePluginHandler_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QList>

class qSlicerAbstractObjective;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \class qSlicerObjectivePluginHandler
/// \brief Singleton class managing objectives plugins for external beam planning
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerObjectivePluginHandler : public QObject
{
  Q_OBJECT

public:
  typedef QList<qSlicerAbstractObjective*> ObjectiveListType;

  /// Instance getter for the singleton class
  /// \return Instance object
  Q_INVOKABLE static qSlicerObjectivePluginHandler* instance();

  /// Allows cleanup of the singleton at application exit
  static void setInstance(qSlicerObjectivePluginHandler* instance);

public:
  /// Add a Objective.
  /// The handler will keep a reference to this Objective object,
  /// and it will not be deleted until all these referring classes are deleted.
  Q_INVOKABLE bool registerObjective(qSlicerAbstractObjective* engine);

  /// Get Objective by its name
  Q_INVOKABLE qSlicerAbstractObjective* ObjectiveByName(QString name);

  /// Get all registered Objectives
  Q_INVOKABLE ObjectiveListType registeredObjectives();

protected:
  /// Registered converter rules
  ObjectiveListType m_RegisteredObjectives;

public:
  /// Private constructor made public to enable python wrapping
  /// IMPORTANT: Should not be used for creating effect handler! Use instance() instead.
  qSlicerObjectivePluginHandler(QObject* parent=nullptr);

  /// Private destructor made public to enable python wrapping
  ~qSlicerObjectivePluginHandler() override;

private:
  Q_DISABLE_COPY(qSlicerObjectivePluginHandler);
  friend class qSlicerObjectivePluginHandlerCleanup;

private:
  /// Instance of the singleton
  static qSlicerObjectivePluginHandler* m_Instance;
};

#endif
