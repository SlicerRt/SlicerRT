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

  This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ)
==============================================================================*/

// PlanOptimizers includes
#include "qSlicerScriptedPlanOptimizer.h"

// Beams includes
#include "vtkMRMLRTPlanNode.h"

// Objectives includes
#include "qSlicerSquaredDeviationObjective.h"

// SlicerQt includes
#include "qSlicerScriptedUtils_p.h"

// PythonQt includes
#include <PythonQt.h>
#include <PythonQtConversion.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLObjectiveNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPythonUtil.h>
#include <vtkCollection.h>

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
QString qSlicerScriptedPlanOptimizer::optimizePlanUsingOptimizer(vtkMRMLRTPlanNode* planNode, std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> objectives, vtkMRMLScalarVolumeNode* resultOptimizationVolumeNode)
{
  Q_D(const qSlicerScriptedPlanOptimizer);
  // transform objectives to python list
  PyObject* pyList = PyList_New(objectives.size());
  for (size_t i = 0; i < objectives.size(); i++)
  {
	  vtkMRMLObjectiveNode* objectiveNode = objectives[i];
      if (objectiveNode)
      {
          PyObject* pyDict = PyDict_New();
          PyDict_SetItemString(pyDict, "Objective", Py_BuildValue("s", objectiveNode->GetName()));
          std::vector<std::string> segments = objectiveNode->GetSegmentations();
          PyObject* pySegmentsList = PyList_New(segments.size());
          for (size_t j = 0; j < segments.size(); j++)
          {
              PyList_SetItem(pySegmentsList, j, Py_BuildValue("s", segments[j].c_str()));
          }
          PyDict_SetItemString(pyDict, "Segments", pySegmentsList);
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
   std::vector<vtkSmartPointer<vtkMRMLObjectiveNode>> objectives;

   vtkSmartPointer<vtkMRMLObjectiveNode> objective1 = vtkSmartPointer<vtkMRMLObjectiveNode>::New();
   vtkSmartPointer<vtkMRMLObjectiveNode> objective2 = vtkSmartPointer<vtkMRMLObjectiveNode>::New();

   // set names
   objective1->SetName("pyRad objective 1");
   objective2->SetName("pyRad objective 2");


   // set objective function
   qSlicerSquaredDeviationObjective* squaredDeviationObjective = new qSlicerSquaredDeviationObjective();
   qSlicerSquaredDeviationObjective::ObjectiveFunctionAndGradient computedDoseObjectiveFunctionAndGradient = squaredDeviationObjective->computeDoseObjectiveFunctionAndGradient();
   objective1->SetDoseObjectiveFunctionAndGradient(computedDoseObjectiveFunctionAndGradient);
   objective2->SetDoseObjectiveFunctionAndGradient(computedDoseObjectiveFunctionAndGradient);


   if (objective1) {
       objectives.push_back(objective1);
   }
   if (objective2) {
       objectives.push_back(objective2);
   }

   this->availableObjectives = objectives;
}


// execute objective: objectiveNode->GetObjectiveFunction()();