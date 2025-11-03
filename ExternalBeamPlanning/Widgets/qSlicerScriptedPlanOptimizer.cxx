/*==============================================================================

  Program: 3D Slicer

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

// ExternalBeamPlanning includes
#include "qSlicerScriptedPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"

// SlicerQt includes
#include "qSlicerScriptedUtils_p.h"

// PythonQt includes
#include <PythonQt.h>

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLRTObjectiveNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPythonUtil.h>

// Qt includes
#include <QDebug>
#include <QFileInfo>

//-----------------------------------------------------------------------------
class qSlicerScriptedPlanOptimizerPrivate
{
public:
  typedef qSlicerScriptedPlanOptimizerPrivate Self;
  qSlicerScriptedPlanOptimizerPrivate();
  virtual ~qSlicerScriptedPlanOptimizerPrivate();

  enum {
    OptimizePlanUsingOptimizerMethod = 0,
    SetAvailableObjectivesMethod = 1,
    };

  mutable qSlicerPythonCppAPI PythonCppAPI;

  QString PythonSource;
};

//-----------------------------------------------------------------------------
// qSlicerScriptedPlanOptimizerPrivate methods

//-----------------------------------------------------------------------------
qSlicerScriptedPlanOptimizerPrivate::qSlicerScriptedPlanOptimizerPrivate()
{
  this->PythonCppAPI.declareMethod(Self::OptimizePlanUsingOptimizerMethod, "optimizePlanUsingOptimizer");
  this->PythonCppAPI.declareMethod(Self::SetAvailableObjectivesMethod, "setAvailableObjectives");
}

//-----------------------------------------------------------------------------
qSlicerScriptedPlanOptimizerPrivate::~qSlicerScriptedPlanOptimizerPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerScriptedPlanOptimizer methods

//-----------------------------------------------------------------------------
qSlicerScriptedPlanOptimizer::qSlicerScriptedPlanOptimizer(QObject *parent)
  : Superclass(parent)
  , d_ptr(new qSlicerScriptedPlanOptimizerPrivate)
{
  this->m_Name = QString("UnnamedScriptedPlanOptimizer");
}

//-----------------------------------------------------------------------------
qSlicerScriptedPlanOptimizer::~qSlicerScriptedPlanOptimizer() = default;

//-----------------------------------------------------------------------------
QString qSlicerScriptedPlanOptimizer::pythonSource()const
{
  Q_D(const qSlicerScriptedPlanOptimizer);
  return d->PythonSource;
}

//-----------------------------------------------------------------------------
bool qSlicerScriptedPlanOptimizer::setPythonSource(const QString newPythonSource)
{
  Q_D(qSlicerScriptedPlanOptimizer);

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
  if (!moduleName.endsWith("PlanOptimizer"))
    {
    className.append("PlanOptimizer");
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
                    QString("qSlicerScriptedPlanOptimizer::setPythonSource - "
                            "Failed to load scripted Optimization engine: "
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
PyObject* qSlicerScriptedPlanOptimizer::self() const
{
  Q_D(const qSlicerScriptedPlanOptimizer);
  return d->PythonCppAPI.pythonSelf();
}

//-----------------------------------------------------------------------------
void qSlicerScriptedPlanOptimizer::setName(QString name)
{
  this->m_Name = name;
}

//-----------------------------------------------------------------------------
QString qSlicerScriptedPlanOptimizer::optimizePlanUsingOptimizer(vtkMRMLRTPlanNode* planNode, std::vector<vtkSmartPointer<vtkMRMLRTObjectiveNode>> objectives, vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode)
{
  Q_D(const qSlicerScriptedPlanOptimizer);
  // transform objectives to python list
  PyObject* pyList = PyList_New(objectives.size());
  for (size_t i = 0; i < objectives.size(); i++)
  {
    vtkMRMLRTObjectiveNode* objectiveNode = objectives[i];
    if (objectiveNode)
    {
      PyObject* pyDict = PyDict_New();
      PyObject* pyObjectiveNode = vtkPythonUtil::GetObjectFromPointer(objectiveNode);
      PyDict_SetItemString(pyDict, "ObjectiveNode", pyObjectiveNode);
      PyList_SetItem(pyList, i, pyDict);
    }
  }

  PyObject* arguments = PyTuple_New(3);
  PyTuple_SET_ITEM(arguments, 0, vtkPythonUtil::GetObjectFromPointer(planNode));
  PyTuple_SET_ITEM(arguments, 1, pyList);
  PyTuple_SET_ITEM(arguments, 2, vtkPythonUtil::GetObjectFromPointer(resultOptimizationVolumeNode));
  qDebug() << d->PythonSource << ": Calling optimizePlanUsingOptimizer from Python Plan Optimizer";
  PyObject* result = d->PythonCppAPI.callMethod(d->OptimizePlanUsingOptimizerMethod, arguments);
  Py_DECREF(arguments);
  if (!result)
  {
    qCritical() << d->PythonSource << ": clone: Failed to call mandatory optimizePlanUsingOptimizer method! If it is implemented, please see python output for errors.";
    return QString();
  }

  // Parse result
  if (!PyFloat_Check(result))
  {
    qWarning() << d->PythonSource << ": qSlicerScriptedPlanOptimizer: Function 'optimizePlanUsingOptimizer' is expected to return a string!";
    return QString();
  }

  return PyUnicode_AsUTF8(result);
}

//-----------------------------------------------------------------------------
void qSlicerScriptedPlanOptimizer::setAvailableObjectives()
{
 Q_D(qSlicerScriptedPlanOptimizer);

 if (!Py_IsInitialized())
 {
   qCritical() << "Python is not initialized!";
   return;
 }

 // Call Python method `setAvailableObjectives
 // Expect a dict: { 'Objective Name': { 'param1': {'default': value1}, 'param2': {'default': value2}, ... }, ... }
 PyObject* result = d->PythonCppAPI.callMethod(d->SetAvailableObjectivesMethod);
 if (!result)
 {
   qCritical() << d->PythonSource << ": Failed to call Python method 'setAvailableObjectives'.";
   return;
 }

 // Iterate over the returned dict (result) and populate availableObjectives
 std::vector<ObjectiveStruct> objectives;
 if (PyMapping_Check(result))
 {
   // Get all keys (objective names)
   PyObject* keys = PyMapping_Keys(result);
   if (keys && PyList_Check(keys))
   {
     // Iterate over objective names
     Py_ssize_t numKeys = PyList_Size(keys);
     for (Py_ssize_t i = 0; i < numKeys; ++i)
     {
       PyObject* key = PyList_GetItem(keys, i);
       if (PyUnicode_Check(key))
       {
         // Create an ObjectiveStruct for each objective
         const char* keyStr = PyUnicode_AsUTF8(key);
         ObjectiveStruct obj;
         obj.name = keyStr;

         // Create parameters dict for each objective
         PyObject* paramsDict = PyMapping_GetItemString(result, keyStr);
         if (paramsDict && PyMapping_Check(paramsDict))
         {
           // Iterate over parameter names
           PyObject* paramKeys = PyMapping_Keys(paramsDict);
           for (Py_ssize_t j = 0; j < PyList_Size(paramKeys); ++j)
           {
             PyObject* paramKey = PyList_GetItem(paramKeys, j);
             if (PyUnicode_Check(paramKey))
             {
               const char* paramName = PyUnicode_AsUTF8(paramKey);

               // Get default values for each parameter as strings from python dict (parameter: {'default': value, ...})
               PyObject* paramInfoDict = PyMapping_GetItemString(paramsDict, paramName);
               QString defaultValueStr;
               if (paramInfoDict && PyMapping_Check(paramInfoDict))
               {
                 PyObject* defaultValueObj = PyMapping_GetItemString(paramInfoDict, "default");
                 if (defaultValueObj)
                 {
                   PyObject* defaultValueRepr = PyObject_Repr(defaultValueObj);
                   if (defaultValueRepr)
                   {
                     const char* reprStr = PyUnicode_AsUTF8(defaultValueRepr);
                     if (reprStr)
                       defaultValueStr = QString(reprStr);
                     Py_DECREF(defaultValueRepr); // Free memory
                   }
                   Py_DECREF(defaultValueObj); // Free memory
                 }
               }
               // Store parameter name and its default value string in the ObjectiveStruct
               obj.parameters[QString(paramName)] = defaultValueStr;
               Py_XDECREF(paramInfoDict); // Free memory
             }
           }
           Py_XDECREF(paramKeys); // Free memory
         }
         Py_XDECREF(paramsDict); // Free memory
         objectives.push_back(obj);
       }
     }
     Py_XDECREF(keys); // Free memory
   }
 }
 else
 {
   qWarning() << "Python setAvailableObjectives() did not return a dictionary!";
 }

 Py_XDECREF(result);

 this->availableObjectives = objectives;
}
