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

// DoseEngines includes
#include "qSlicerDoseEnginePluginHandler.h"
#include "qSlicerAbstractDoseEngine.h"

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
qSlicerDoseEnginePluginHandler *qSlicerDoseEnginePluginHandler::m_Instance = NULL;

//----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerDoseEnginePluginHandlerCleanup
{
public:
  inline void use() { }

  ~qSlicerDoseEnginePluginHandlerCleanup()
  {
    if (qSlicerDoseEnginePluginHandler::m_Instance)
    {
      qSlicerDoseEnginePluginHandler::setInstance(NULL);
    }
  }
};
static qSlicerDoseEnginePluginHandlerCleanup qSlicerDoseEnginePluginHandlerCleanupGlobal;

//-----------------------------------------------------------------------------
qSlicerDoseEnginePluginHandler* qSlicerDoseEnginePluginHandler::instance()
{
  if(!qSlicerDoseEnginePluginHandler::m_Instance)
  {
    if(!qSlicerDoseEnginePluginHandler::m_Instance)
    {
      qSlicerDoseEnginePluginHandlerCleanupGlobal.use();

      qSlicerDoseEnginePluginHandler::m_Instance = new qSlicerDoseEnginePluginHandler();
    }
  }
  // Return the instance
  return qSlicerDoseEnginePluginHandler::m_Instance;
}

//-----------------------------------------------------------------------------
void qSlicerDoseEnginePluginHandler::setInstance(qSlicerDoseEnginePluginHandler* instance)
{
  if (qSlicerDoseEnginePluginHandler::m_Instance==instance)
  {
    return;
  }
  // Preferably this will be NULL
  if (qSlicerDoseEnginePluginHandler::m_Instance)
  {
    delete qSlicerDoseEnginePluginHandler::m_Instance;
  }
  qSlicerDoseEnginePluginHandler::m_Instance = instance;
  if (!instance)
  {
    return;
  }
}

//-----------------------------------------------------------------------------
qSlicerDoseEnginePluginHandler::qSlicerDoseEnginePluginHandler(QObject* parent)
  : QObject(parent)
{
  this->m_RegisteredDoseEngines.clear();
}

//-----------------------------------------------------------------------------
qSlicerDoseEnginePluginHandler::~qSlicerDoseEnginePluginHandler()
{
  foreach(qSlicerAbstractDoseEngine* effect, m_RegisteredDoseEngines)
  {
    delete effect;
  }
  this->m_RegisteredDoseEngines.clear();
}

//---------------------------------------------------------------------------
bool qSlicerDoseEnginePluginHandler::registerDoseEngine(qSlicerAbstractDoseEngine* engineToRegister)
{
  if (engineToRegister == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid dose engine to register!";
    return false;
  }
  if (engineToRegister->name().isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Dose engine cannot be registered with empty name!";
    return false;
  }

  // Check if the same engine has already been registered
  qSlicerAbstractDoseEngine* currentEffect = NULL;
  foreach (currentEffect, this->m_RegisteredDoseEngines)
  {
    if (engineToRegister->name().compare(currentEffect->name()) == 0)
    {
      return false;
    }
  }

  // Add the effect to the list
  this->m_RegisteredDoseEngines << engineToRegister;

  return true;
}

//----------------------------------------------------------------------------
void qSlicerDoseEnginePluginHandler::unregisterDoseEngine(qSlicerAbstractDoseEngine* engine)
{
  //TODO: Implement
  qCritical() << Q_FUNC_INFO << ": Not implemented yet!";
}

//----------------------------------------------------------------------------
qSlicerAbstractDoseEngine* qSlicerDoseEnginePluginHandler::doseEngineByName(QString name)
{
if (name.isEmpty())
  {
    return NULL;
  }

  // Find engine with name
  qSlicerAbstractDoseEngine* currentEngine = NULL;
  foreach (currentEngine, this->m_RegisteredDoseEngines)
  {
    if (currentEngine->name().compare(name) == 0)
    {
      return currentEngine;
    }
  }

  qWarning() << Q_FUNC_INFO << ": Engine named '" << name << "' cannot be found!";
  return NULL;
}

//----------------------------------------------------------------------------
qSlicerDoseEnginePluginHandler::DoseEngineListType qSlicerDoseEnginePluginHandler::registeredDoseEngines()
{
  return this->m_RegisteredDoseEngines;
}
