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

// DoseEngines includes
#include "qSlicerScriptedDoseEngine.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"

// SlicerQt includes
#include "qSlicerScriptedUtils_p.h"

// PythonQt includes
#include <PythonQt.h>
#include <PythonQtConversion.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPythonUtil.h>

// Qt includes
#include <QDebug>
#include <QFileInfo>

//-----------------------------------------------------------------------------
class qSlicerScriptedDoseEnginePrivate
{
public:
  typedef qSlicerScriptedDoseEnginePrivate Self;
  qSlicerScriptedDoseEnginePrivate();
  virtual ~qSlicerScriptedDoseEnginePrivate();

  enum {
    DefineBeamParametersMethod = 0,
    CalculateDoseUsingEngineMethod,
    CalculateDoseInfluenceMatrixUsingEngineMethod,
    UpdateBeamParametersForIonPlan
    };

  mutable qSlicerPythonCppAPI PythonCppAPI;

  QString PythonSource;
};

//-----------------------------------------------------------------------------
// qSlicerScriptedDoseEnginePrivate methods

//-----------------------------------------------------------------------------
qSlicerScriptedDoseEnginePrivate::qSlicerScriptedDoseEnginePrivate()
{
  this->PythonCppAPI.declareMethod(Self::DefineBeamParametersMethod, "defineBeamParameters");
  this->PythonCppAPI.declareMethod(Self::CalculateDoseUsingEngineMethod, "calculateDoseUsingEngine");
  this->PythonCppAPI.declareMethod(Self::CalculateDoseInfluenceMatrixUsingEngineMethod, "calculateDoseInfluenceMatrixUsingEngine");
  this->PythonCppAPI.declareMethod(Self::UpdateBeamParametersForIonPlan, "updateBeamParametersForIonPlan");
}

//-----------------------------------------------------------------------------
qSlicerScriptedDoseEnginePrivate::~qSlicerScriptedDoseEnginePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerScriptedDoseEngine methods

//-----------------------------------------------------------------------------
qSlicerScriptedDoseEngine::qSlicerScriptedDoseEngine(QObject *parent)
  : Superclass(parent)
  , d_ptr(new qSlicerScriptedDoseEnginePrivate)
{
  this->m_Name = QString("UnnamedScriptedEngine");
}

//-----------------------------------------------------------------------------
qSlicerScriptedDoseEngine::~qSlicerScriptedDoseEngine() = default;

//-----------------------------------------------------------------------------
QString qSlicerScriptedDoseEngine::pythonSource()const
{
  Q_D(const qSlicerScriptedDoseEngine);
  return d->PythonSource;
}

//-----------------------------------------------------------------------------
bool qSlicerScriptedDoseEngine::setPythonSource(const QString newPythonSource)
{
  Q_D(qSlicerScriptedDoseEngine);

  if (!Py_IsInitialized())
    {
    return false;
    }

  if (!newPythonSource.endsWith(".py") && !newPythonSource.endsWith(".pyc"))
    {
    return false;
    }

  // Extract moduleName from the provided filename
  QString moduleName = QFileInfo(newPythonSource).baseName();

  // In case the engine is within the main module file
  QString className = moduleName;
  if (!moduleName.endsWith("Engine"))
    {
    className.append("Engine");
    }

  // Get a reference to the main module and global dictionary
  PyObject * main_module = PyImport_AddModule("__main__");
  PyObject * global_dict = PyModule_GetDict(main_module);

  // Get a reference (or create if needed) the <moduleName> python module
  PyObject * module = PyImport_AddModule(moduleName.toUtf8());

  // Get a reference to the python module class to instantiate
  PythonQtObjectPtr classToInstantiate;
  if (PyObject_HasAttrString(module, className.toUtf8()))
    {
    classToInstantiate.setNewRef(PyObject_GetAttrString(module, className.toUtf8()));
    }
  if (!classToInstantiate)
    {
    PythonQtObjectPtr local_dict;
    local_dict.setNewRef(PyDict_New());
    if (!qSlicerScriptedUtils::loadSourceAsModule(moduleName, newPythonSource, global_dict, local_dict))
      {
      return false;
      }
    if (PyObject_HasAttrString(module, className.toUtf8()))
      {
      classToInstantiate.setNewRef(PyObject_GetAttrString(module, className.toUtf8()));
      }
    }

  if (!classToInstantiate)
    {
    PythonQt::self()->handleError();
    PyErr_SetString(PyExc_RuntimeError,
                    QString("qSlicerScriptedDoseEngine::setPythonSource - "
                            "Failed to load scripted dose engine: "
                            "class %1 was not found in %2").arg(className).arg(newPythonSource).toUtf8());
    PythonQt::self()->handleError();
    return false;
    }

  d->PythonCppAPI.setObjectName(className);

  PyObject* self = d->PythonCppAPI.instantiateClass(this, className, classToInstantiate);
  if (!self)
    {
    return false;
    }

  d->PythonSource = newPythonSource;

  if (!qSlicerScriptedUtils::setModuleAttribute(
        "slicer", className, self))
    {
    qCritical() << "Failed to set" << ("slicer." + className);
    }

  return true;
}

//-----------------------------------------------------------------------------
PyObject* qSlicerScriptedDoseEngine::self() const
{
  Q_D(const qSlicerScriptedDoseEngine);
  return d->PythonCppAPI.pythonSelf();
}

//-----------------------------------------------------------------------------
void qSlicerScriptedDoseEngine::setName(QString name)
{
  this->m_Name = name;
}

//-----------------------------------------------------------------------------
void qSlicerScriptedDoseEngine::setIsInverse(bool isInverse)
{
  this->m_IsInverse = isInverse;
}

//-----------------------------------------------------------------------------
void qSlicerScriptedDoseEngine::setCanDoIonPlan(bool canDoIonPlan)
{
	this->m_CanDoIonPlan = canDoIonPlan;
}

//-----------------------------------------------------------------------------
QString qSlicerScriptedDoseEngine::calculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode)
{
  Q_D(const qSlicerScriptedDoseEngine);
  PyObject* arguments = PyTuple_New(2);
  PyTuple_SET_ITEM(arguments, 0, vtkPythonUtil::GetObjectFromPointer(beamNode));
  PyTuple_SET_ITEM(arguments, 1, vtkPythonUtil::GetObjectFromPointer(resultDoseVolumeNode));
  qDebug() << d->PythonSource << ": Calling calculateDoseUsingEngine from Python dose engine";
  PyObject* result = d->PythonCppAPI.callMethod(d->CalculateDoseUsingEngineMethod, arguments);
  Py_DECREF(arguments);
  if (!result)
    {
    qCritical() << d->PythonSource << ": clone: Failed to call mandatory calculateDoseUsingEngine method! If it is implemented, please see python output for errors.";
    return QString();
    }

  // Parse result
  if (!PyFloat_Check(result))
    {
    qWarning() << d->PythonSource << ": qSlicerScriptedDoseEngine: Function 'calculateDoseUsingEngine' is expected to return a string!";
    return QString();
    }

  return PyString_AsString(result);
}

QString qSlicerScriptedDoseEngine::calculateDoseInfluenceMatrixUsingEngine(vtkMRMLRTBeamNode* beamNode)
{
  Q_D(const qSlicerScriptedDoseEngine);
  PyObject* arguments = PyTuple_New(1);
  PyTuple_SET_ITEM(arguments, 0, vtkPythonUtil::GetObjectFromPointer(beamNode));
  //PyTuple_SET_ITEM(arguments, 1, vtkPythonUtil::GetObjectFromPointer(resultDoseVolumeNode));
  qDebug() << d->PythonSource << ": Calling calculateDoseInfluenceMatrixUsingEngine from Python dose engine";
  PyObject* result = d->PythonCppAPI.callMethod(d->CalculateDoseInfluenceMatrixUsingEngineMethod, arguments);
  Py_DECREF(arguments);
  if (!result)
  {
    qCritical() << d->PythonSource << ": clone: Failed to call mandatory calculateDoseInfluenceMatrixUsingEngine method! If it is implemented, please see python output for errors.";
    return QString();
  }

  // Parse result
  if (!PyFloat_Check(result))
  {
    qWarning() << d->PythonSource << ": qSlicerScriptedDoseEngine: Function 'calculateDoseInfluenceMatrixUsingEngine' is expected to return a string!";
    return QString();
  }

  return PyString_AsString(result);
}

//-----------------------------------------------------------------------------
void qSlicerScriptedDoseEngine::defineBeamParameters()
{
  Q_D(const qSlicerScriptedDoseEngine);
  d->PythonCppAPI.callMethod(d->DefineBeamParametersMethod);
}

//-----------------------------------------------------------------------------
void qSlicerScriptedDoseEngine::updateBeamParametersForIonPlan(bool isIonPlanActive)
{
  Q_D(const qSlicerScriptedDoseEngine);

  PyObject* arguments = PyTuple_New(1);
	 PyTuple_SET_ITEM(arguments, 0, PyBool_FromLong(isIonPlanActive));
	 d->PythonCppAPI.callMethod(d->UpdateBeamParametersForIonPlan, arguments);
}
