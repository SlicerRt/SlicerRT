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
Technik at the Karlsruher Institut fuer Technologie (IBT-KIT) and German Cancer
Research Center (DKFZ)

==============================================================================*/

// Objectives includes
#include "qSlicerObjectivePluginHandler.h"
#include "qSlicerAbstractObjective.h"

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
qSlicerObjectivePluginHandler *qSlicerObjectivePluginHandler::m_Instance = nullptr;

//----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerObjectivePluginHandlerCleanup
{
public:
  inline void use() { };

  ~qSlicerObjectivePluginHandlerCleanup()
  {
    if (qSlicerObjectivePluginHandler::m_Instance)
    {
      qSlicerObjectivePluginHandler::setInstance(nullptr);
    }
  }
};
static qSlicerObjectivePluginHandlerCleanup qSlicerObjectivePluginHandlerCleanupGlobal;

//-----------------------------------------------------------------------------
qSlicerObjectivePluginHandler* qSlicerObjectivePluginHandler::instance()
{
  if(!qSlicerObjectivePluginHandler::m_Instance)
  {
    if(!qSlicerObjectivePluginHandler::m_Instance)
    {
      qSlicerObjectivePluginHandlerCleanupGlobal.use();

      qSlicerObjectivePluginHandler::m_Instance = new qSlicerObjectivePluginHandler();
    }
  }
  // Return the instance
  return qSlicerObjectivePluginHandler::m_Instance;
}

//-----------------------------------------------------------------------------
void qSlicerObjectivePluginHandler::setInstance(qSlicerObjectivePluginHandler* instance)
{
  if (qSlicerObjectivePluginHandler::m_Instance==instance)
  {
    return;
  }
  // Preferably this will be nullptr
  if (qSlicerObjectivePluginHandler::m_Instance)
  {
    delete qSlicerObjectivePluginHandler::m_Instance;
  }
  qSlicerObjectivePluginHandler::m_Instance = instance;
  if (!instance)
  {
    return;
  }
}

//-----------------------------------------------------------------------------
qSlicerObjectivePluginHandler::qSlicerObjectivePluginHandler(QObject* parent)
  : QObject(parent)
{
  this->m_RegisteredObjectives.clear();
}

//-----------------------------------------------------------------------------
qSlicerObjectivePluginHandler::~qSlicerObjectivePluginHandler()
{
  foreach(qSlicerAbstractObjective* engine, m_RegisteredObjectives)
  {
    delete engine;
  }
  this->m_RegisteredObjectives.clear();
}

//---------------------------------------------------------------------------
bool qSlicerObjectivePluginHandler::registerObjective(qSlicerAbstractObjective* engineToRegister)
{
  if (engineToRegister == nullptr)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid Optimization engine to register!";
    return false;
  }
  if (engineToRegister->name().isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Optimization engine cannot be registered with empty name!";
    return false;
  }

  // Check if the same engine has already been registered
  qSlicerAbstractObjective* currentEngine = nullptr;
  foreach (currentEngine, this->m_RegisteredObjectives)
  {
    if (engineToRegister->name().compare(currentEngine->name()) == 0)
    {
      return false;
    }
  }

  // Add the engine to the list
  this->m_RegisteredObjectives << engineToRegister;

  return true;
}

//----------------------------------------------------------------------------
qSlicerAbstractObjective* qSlicerObjectivePluginHandler::ObjectiveByName(QString name)
{
if (name.isEmpty())
  {
    return nullptr;
  }

  // Find engine with name
  qSlicerAbstractObjective* currentEngine = nullptr;
  foreach (currentEngine, this->m_RegisteredObjectives)
  {
    if (currentEngine->name().compare(name) == 0)
    {
      return currentEngine;
    }
  }

  qWarning() << Q_FUNC_INFO << ": Engine named '" << name << "' cannot be found!";
  return nullptr;
}

//----------------------------------------------------------------------------
qSlicerObjectivePluginHandler::ObjectiveListType qSlicerObjectivePluginHandler::registeredObjectives()
{
  return this->m_RegisteredObjectives;
}
