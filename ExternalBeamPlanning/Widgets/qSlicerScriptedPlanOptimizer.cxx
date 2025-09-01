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
    OptimizePlanUsingOptimizerMethod = 0
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

  return PyString_AsString(result);
}


//-----------------------------------------------------------------------------
void qSlicerScriptedPlanOptimizer::setAvailableObjectives()
{
  if (!Py_IsInitialized())
  {
    qCritical() << "Python is not initialized!";
    return;
  }

  // requires pyRadPlan being installed
  PyObject* pName = PyUnicode_DecodeFSDefault("pyRadPlan.optimization.objectives");
  PyObject* pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  std::vector<ObjectiveStruct> objectives;
  if (pModule != nullptr)
  {
    // Get the function from the module
    PyObject* pFunc = PyObject_GetAttrString(pModule, "get_available_objectives");
    if (pFunc && PyCallable_Check(pFunc))
    {
      // Call the function
      PyObject* pValue = PyObject_CallObject(pFunc, nullptr);
      if (pValue != nullptr)
      {
        // Assuming the function returns a dictionary-like object
        if (PyMapping_Check(pValue))
        {
          PyObject* pKeys = PyMapping_Keys(pValue);
          if (pKeys && PyList_Check(pKeys))
          {
            Py_ssize_t numKeys = PyList_Size(pKeys);
            for (Py_ssize_t i = 0; i < numKeys; ++i)
            {
              PyObject* pKey = PyList_GetItem(pKeys, i);
              if (PyUnicode_Check(pKey))
              {
                const char* keyStr = PyUnicode_AsUTF8(pKey);
                ObjectiveStruct objective;
                objective.name = keyStr;

                PyObject* pVal = PyMapping_GetItemString(pValue, keyStr);
                // Check if pVal has __annotations__
                if (PyObject_HasAttrString(pVal, "__annotations__"))
                {
                  PyObject* pAnnotations = PyObject_GetAttrString(pVal, "__annotations__");
                  if (PyMapping_Check(pAnnotations))
                  {
                    // Print all keys in pAnnotations
                    PyObject* pAnnotationsKeys = PyMapping_Keys(pAnnotations);
                    if (pAnnotationsKeys && PyList_Check(pAnnotationsKeys))
                    {
                      Py_ssize_t numKeys = PyList_Size(pAnnotationsKeys);
                      for (Py_ssize_t j = 0; j < numKeys; ++j)
                      {
                        PyObject* pKey = PyList_GetItem(pAnnotationsKeys, j);
                        if (PyUnicode_Check(pKey))
                        {
                          const char* paramName = PyUnicode_AsUTF8(pKey);
                          objective.parameters[std::string(paramName)] = "";
                        }
                      }
                      Py_DECREF(pAnnotationsKeys);
                    }
                  }
                }
                objectives.push_back(objective);
              }
            }
            Py_DECREF(pKeys); // free memory
          }
        }
        Py_DECREF(pValue); // free memory
      }
    }
    Py_XDECREF(pFunc); // free memory
    Py_DECREF(pModule); // free memory
  }
  else
  {
    qCritical() << "Failed to import pyRadPlan.optimization.objectives module!";
  }

  this->availableObjectives = objectives;
}
