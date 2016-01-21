/*==============================================================================

  Program: 3D Slicer

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

// SubjectHierarchy plugins includes
#include "qSlicerSegmentEditorEffectHandler.h"
#include "qSlicerSegmentEditorAbstractEffect.h"

// Qt includes
#include <QDebug>
#include <QStringList>

// MRML includes
#include "vtkMRMLScene.h"

//----------------------------------------------------------------------------
qSlicerSegmentEditorEffectHandler *qSlicerSegmentEditorEffectHandler::m_Instance = NULL;

//----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Segmentations
class qSlicerSegmentEditorEffectHandlerCleanup
{
public:
  inline void use()   {   }

  ~qSlicerSegmentEditorEffectHandlerCleanup()
    {
    if (qSlicerSegmentEditorEffectHandler::m_Instance)
      {
      qSlicerSegmentEditorEffectHandler::setInstance(NULL);
      }
    }
};
static qSlicerSegmentEditorEffectHandlerCleanup qSlicerSegmentEditorEffectHandlerCleanupGlobal;

//-----------------------------------------------------------------------------
qSlicerSegmentEditorEffectHandler* qSlicerSegmentEditorEffectHandler::instance()
{
  if(!qSlicerSegmentEditorEffectHandler::m_Instance)
    {
    if(!qSlicerSegmentEditorEffectHandler::m_Instance)
      {
      qSlicerSegmentEditorEffectHandlerCleanupGlobal.use();

      qSlicerSegmentEditorEffectHandler::m_Instance = new qSlicerSegmentEditorEffectHandler();
      }
    }
  // Return the instance
  return qSlicerSegmentEditorEffectHandler::m_Instance;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorEffectHandler::setInstance(qSlicerSegmentEditorEffectHandler* instance)
{
  if (qSlicerSegmentEditorEffectHandler::m_Instance==instance)
    {
    return;
    }
  // Preferably this will be NULL
  if (qSlicerSegmentEditorEffectHandler::m_Instance)
    {
    delete qSlicerSegmentEditorEffectHandler::m_Instance;
    }
  qSlicerSegmentEditorEffectHandler::m_Instance = instance;
  if (!instance)
    {
    return;
    }
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorEffectHandler::qSlicerSegmentEditorEffectHandler(QObject* parent)
  : QObject(parent)
  , m_ActiveEffect(NULL)
{
  this->m_RegisteredEffects.clear();
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorEffectHandler::~qSlicerSegmentEditorEffectHandler()
{
  QList<qSlicerSegmentEditorAbstractEffect*>::iterator pluginIt;
  for (pluginIt = this->m_RegisteredEffects.begin(); pluginIt != this->m_RegisteredEffects.end(); ++pluginIt)
    {
    delete (*pluginIt);
    }
  this->m_RegisteredEffects.clear();

  this->m_ActiveEffect = NULL;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorEffectHandler::setScene(vtkMRMLScene* scene)
{
  m_Scene = scene;
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qSlicerSegmentEditorEffectHandler::scene()
{
  return m_Scene;
}

//---------------------------------------------------------------------------
bool qSlicerSegmentEditorEffectHandler::registerEffect(qSlicerSegmentEditorAbstractEffect* effectToRegister)
{
  if (effectToRegister == NULL)
    {
    qCritical() << "qSlicerSegmentEditorEffectHandler::registerEffect: Invalid effect to register!";
    return false;
    }
  if (effectToRegister->name().isEmpty())
    {
    qCritical() << "qSlicerSegmentEditorEffectHandler::registerEffect: Segment editor effect cannot be registered with empty name!";
    return false;
    }

  // Check if the same effect has already been registered
  qSlicerSegmentEditorAbstractEffect* currentEffect = NULL;
  foreach (currentEffect, this->m_RegisteredEffects)
    {
    if (effectToRegister->name().compare(currentEffect->name()) == 0)
      {
      qDebug() << "qSlicerSegmentEditorEffectHandler::registerEffect: "
                    "Segment editor effect " << effectToRegister->name() << " is already registered";
      return false;
      }
    }

  // Add the effect to the list
  this->m_RegisteredEffects << effectToRegister;

  return true;
}

//---------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorEffectHandler::effectByName(QString name)
{
  if (name.isEmpty())
    {
    return NULL;
    }

  // Find effect with name
  qSlicerSegmentEditorAbstractEffect* currentEffect = NULL;
  foreach (currentEffect, this->m_RegisteredEffects)
    {
    if (currentEffect->name().compare(name) == 0)
      {
      return currentEffect;
      }
    }

  qWarning() << "qSlicerSegmentEditorEffectHandler::effectByName: Effect named '" << name << "' cannot be found!";
  return NULL;
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorEffectHandler::activeEffect()const
{
  return m_ActiveEffect;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorEffectHandler::setActiveEffect(qSlicerSegmentEditorAbstractEffect* effect)
{
  m_ActiveEffect = effect;
}
