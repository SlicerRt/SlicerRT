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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerDoseEnginePluginHandler_h
#define __qSlicerDoseEnginePluginHandler_h

#include "qSlicerExternalBeamPlanningModuleWidgetsExport.h"

// Qt includes
#include <QObject>
#include <QList>

class qSlicerAbstractDoseEngine;

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \class qSlicerDoseEnginePluginHandler
/// \brief Singleton class managing dose engine plugins for external beam planning
class Q_SLICER_MODULE_EXTERNALBEAMPLANNING_WIDGETS_EXPORT qSlicerDoseEnginePluginHandler : public QObject
{
  Q_OBJECT

public:
  typedef QList<qSlicerAbstractDoseEngine*> DoseEngineListType;

  /// Instance getter for the singleton class
  /// \return Instance object
  Q_INVOKABLE static qSlicerDoseEnginePluginHandler* instance();

  /// Cleanup of the singleton at application exit
  static void cleanup();

public:
  /// Add a dose engine.
  /// The handler will keep a reference to this dose engine object,
  /// and it will not be deleted until all these referring classes are deleted.
  Q_INVOKABLE bool registerDoseEngine(qSlicerAbstractDoseEngine* engine);

  /// Get dose engine by its name
  Q_INVOKABLE qSlicerAbstractDoseEngine* doseEngineByName(QString name);

  /// Get all registered dose engines
  Q_INVOKABLE DoseEngineListType registeredDoseEngines();

protected:
  /// Registered converter rules
  DoseEngineListType m_RegisteredDoseEngines;

public:
  /// Private constructor made public to enable python wrapping
  /// IMPORTANT: Should not be used for creating effect handler! Use instance() instead.
  qSlicerDoseEnginePluginHandler(QObject* parent=nullptr);

  /// Private destructor made public to enable python wrapping
  ~qSlicerDoseEnginePluginHandler() override;

private:
  Q_DISABLE_COPY(qSlicerDoseEnginePluginHandler);
  friend class qSlicerDoseEnginePluginHandlerCleanup;

private:
  /// Instance of the singleton
  static qSlicerDoseEnginePluginHandler* m_Instance;
};

#endif
