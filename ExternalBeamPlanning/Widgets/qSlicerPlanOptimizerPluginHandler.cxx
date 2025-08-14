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

  This file was originally developed by Nikals Wahl, German Cancer Research Center (DKFZ)

==============================================================================*/

// ExternalBeamPlanning includes
#include "qSlicerPlanOptimizerPluginHandler.h"
#include "qSlicerAbstractPlanOptimizer.h"

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
qSlicerPlanOptimizerPluginHandler *qSlicerPlanOptimizerPluginHandler::m_Instance = nullptr;

//----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerPlanOptimizerPluginHandlerCleanup
{
public:
  inline void use() { };

  ~qSlicerPlanOptimizerPluginHandlerCleanup()
  {
    if (qSlicerPlanOptimizerPluginHandler::m_Instance)
    {
      qSlicerPlanOptimizerPluginHandler::setInstance(nullptr);
    }
  }
};
static qSlicerPlanOptimizerPluginHandlerCleanup qSlicerPlanOptimizerPluginHandlerCleanupGlobal;

//-----------------------------------------------------------------------------
qSlicerPlanOptimizerPluginHandler* qSlicerPlanOptimizerPluginHandler::instance()
{
  if(!qSlicerPlanOptimizerPluginHandler::m_Instance)
  {
    if(!qSlicerPlanOptimizerPluginHandler::m_Instance)
    {
      qSlicerPlanOptimizerPluginHandlerCleanupGlobal.use();

      qSlicerPlanOptimizerPluginHandler::m_Instance = new qSlicerPlanOptimizerPluginHandler();
    }
  }
  // Return the instance
  return qSlicerPlanOptimizerPluginHandler::m_Instance;
}

//-----------------------------------------------------------------------------
void qSlicerPlanOptimizerPluginHandler::setInstance(qSlicerPlanOptimizerPluginHandler* instance)
{
  if (qSlicerPlanOptimizerPluginHandler::m_Instance==instance)
  {
    return;
  }
  // Preferably this will be nullptr
  if (qSlicerPlanOptimizerPluginHandler::m_Instance)
  {
    delete qSlicerPlanOptimizerPluginHandler::m_Instance;
  }
  qSlicerPlanOptimizerPluginHandler::m_Instance = instance;
  if (!instance)
  {
    return;
  }
}

//-----------------------------------------------------------------------------
qSlicerPlanOptimizerPluginHandler::qSlicerPlanOptimizerPluginHandler(QObject* parent)
  : QObject(parent)
{
  this->m_RegisteredPlanOptimizers.clear();
}

//-----------------------------------------------------------------------------
qSlicerPlanOptimizerPluginHandler::~qSlicerPlanOptimizerPluginHandler()
{
  foreach(qSlicerAbstractPlanOptimizer* engine, m_RegisteredPlanOptimizers)
  {
    delete engine;
  }
  this->m_RegisteredPlanOptimizers.clear();
}

//---------------------------------------------------------------------------
bool qSlicerPlanOptimizerPluginHandler::registerPlanOptimizer(qSlicerAbstractPlanOptimizer* engineToRegister)
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
  qSlicerAbstractPlanOptimizer* currentEngine = nullptr;
  foreach (currentEngine, this->m_RegisteredPlanOptimizers)
  {
    if (engineToRegister->name().compare(currentEngine->name()) == 0)
    {
      return false;
    }
  }

  // Add the engine to the list
  this->m_RegisteredPlanOptimizers << engineToRegister;

  return true;
}

//----------------------------------------------------------------------------
qSlicerAbstractPlanOptimizer* qSlicerPlanOptimizerPluginHandler::PlanOptimizerByName(QString name)
{
if (name.isEmpty())
  {
    return nullptr;
  }

  // Find engine with name
  qSlicerAbstractPlanOptimizer* currentEngine = nullptr;
  foreach (currentEngine, this->m_RegisteredPlanOptimizers)
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
qSlicerPlanOptimizerPluginHandler::PlanOptimizerListType qSlicerPlanOptimizerPluginHandler::registeredPlanOptimizers()
{
  return this->m_RegisteredPlanOptimizers;
}
