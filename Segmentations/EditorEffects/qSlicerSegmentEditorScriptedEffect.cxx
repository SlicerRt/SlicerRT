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

// SubjectHierarchy includes
#include "qSlicerSegmentEditorScriptedEffect.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "qSlicerSubjectHierarchyPluginHandler.h"

// Qt includes
#include <QDebug>
#include <QFileInfo>
#include <QAction>

// SlicerQt includes
#include "qSlicerScriptedUtils_p.h"

// PythonQt includes
#include <PythonQt.h>
#include <PythonQtConversion.h>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkPythonUtil.h>

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorScriptedEffectPrivate
{
public:
  typedef qSlicerSegmentEditorScriptedEffectPrivate Self;
  qSlicerSegmentEditorScriptedEffectPrivate();
  virtual ~qSlicerSegmentEditorScriptedEffectPrivate();

  enum {
    IconMethod = 0,
    HelpTextMethod,
    CloneMethod,
    ActivateMethod,
    DeactivateMethod,
    ApplyMethod,
    SetupOptionsFrameMethod,
    CreateCursorMethod,
    ProcessInteractionEventsMethod,
    ProcessViewNodeEventsMethod,
    SetMRMLDefaultsMethod,
    EditedLabelmapChangedMethod,
    MasterVolumeNodeChangedMethod,
    UpdateGUIFromMRMLMethod,
    UpdateMRMLFromGUIMethod,
    AddOptionsWidgetMethod
    };

  mutable qSlicerPythonCppAPI PythonCppAPI;

  QString PythonSource;
};

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorScriptedEffectPrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedEffectPrivate::qSlicerSegmentEditorScriptedEffectPrivate()
{
  this->PythonCppAPI.declareMethod(Self::IconMethod, "icon");
  this->PythonCppAPI.declareMethod(Self::HelpTextMethod, "helpText");
  this->PythonCppAPI.declareMethod(Self::CloneMethod, "clone");
  this->PythonCppAPI.declareMethod(Self::ActivateMethod, "activate");
  this->PythonCppAPI.declareMethod(Self::DeactivateMethod, "deactivate");
  this->PythonCppAPI.declareMethod(Self::ApplyMethod, "apply");
  this->PythonCppAPI.declareMethod(Self::SetupOptionsFrameMethod, "setupOptionsFrame");
  this->PythonCppAPI.declareMethod(Self::CreateCursorMethod, "createCursor");
  this->PythonCppAPI.declareMethod(Self::ProcessInteractionEventsMethod, "processInteractionEvents");
  this->PythonCppAPI.declareMethod(Self::ProcessViewNodeEventsMethod, "processViewNodeEvents");
  this->PythonCppAPI.declareMethod(Self::SetMRMLDefaultsMethod, "setMRMLDefaults");
  this->PythonCppAPI.declareMethod(Self::EditedLabelmapChangedMethod, "editedLabelmapChanged");
  this->PythonCppAPI.declareMethod(Self::MasterVolumeNodeChangedMethod, "masterVolumeNodeChanged");
  this->PythonCppAPI.declareMethod(Self::UpdateGUIFromMRMLMethod, "updateGUIFromMRML");
  this->PythonCppAPI.declareMethod(Self::UpdateMRMLFromGUIMethod, "updateMRMLFromGUI");
  this->PythonCppAPI.declareMethod(Self::AddOptionsWidgetMethod, "addOptionsWidget");
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedEffectPrivate::~qSlicerSegmentEditorScriptedEffectPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSegmentEditorScriptedEffect methods

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedEffect::qSlicerSegmentEditorScriptedEffect(QObject *parent)
  : Superclass(parent)
  , d_ptr(new qSlicerSegmentEditorScriptedEffectPrivate)
{
  this->m_Name = QString("UnnamedScriptedPlugin");
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScriptedEffect::~qSlicerSegmentEditorScriptedEffect()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorScriptedEffect::pythonSource()const
{
  Q_D(const qSlicerSegmentEditorScriptedEffect);
  return d->PythonSource;
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorScriptedEffect::setPythonSource(const QString newPythonSource)
{
  Q_D(qSlicerSegmentEditorScriptedEffect);

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

  // In case the effect is within the main module file
  QString className = moduleName;
  if (!moduleName.endsWith("Effect"))
    {
    className.append("Effect");
    }

  // Get a reference to the main module and global dictionary
  PyObject * main_module = PyImport_AddModule("__main__");
  PyObject * global_dict = PyModule_GetDict(main_module);

  // Get a reference (or create if needed) the <moduleName> python module
  PyObject * module = PyImport_AddModule(moduleName.toLatin1());

  // Get a reference to the python module class to instantiate
  PythonQtObjectPtr classToInstantiate;
  if (PyObject_HasAttrString(module, className.toLatin1()))
    {
    classToInstantiate.setNewRef(PyObject_GetAttrString(module, className.toLatin1()));
    }
  if (!classToInstantiate)
    {
    PythonQtObjectPtr local_dict;
    local_dict.setNewRef(PyDict_New());
    if (!qSlicerScriptedUtils::loadSourceAsModule(moduleName, newPythonSource, global_dict, local_dict))
      {
      return false;
      }
    if (PyObject_HasAttrString(module, className.toLatin1()))
      {
      classToInstantiate.setNewRef(PyObject_GetAttrString(module, className.toLatin1()));
      }
    }

  if (!classToInstantiate)
    {
    PythonQt::self()->handleError();
    PyErr_SetString(PyExc_RuntimeError,
                    QString("qSlicerSegmentEditorScriptedEffect::setPythonSource - "
                            "Failed to load segment editor scripted effect: "
                            "class %1 was not found in %2").arg(className).arg(newPythonSource).toLatin1());
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
PyObject* qSlicerSegmentEditorScriptedEffect::self() const
{
  Q_D(const qSlicerSegmentEditorScriptedEffect);
  return d->PythonCppAPI.pythonSelf();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::setName(QString name)
{
  this->m_Name = name;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSegmentEditorScriptedEffect::icon()
{
  Q_D(const qSlicerSegmentEditorScriptedEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->IconMethod);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::icon();
    }

  // Parse result
  QVariant resultVariant = PythonQtConv::PyObjToQVariant(result, QVariant::Icon);
  if (resultVariant.isNull())
    {
    return this->Superclass::icon(node);
    }
  return resultVariant.value<QIcon>();
}

//-----------------------------------------------------------------------------
const QString qSlicerSegmentEditorScriptedEffect::helpText()const
{
  Q_D(const qSlicerSegmentEditorScriptedEffect);
  PyObject* result = d->PythonCppAPI.callMethod(d->HelpTextMethod);
  if (!result)
    {
    // Method call failed (probably an omitted function), call default implementation
    return this->Superclass::helpText();
    }

  // Parse result
  if (!PyString_Check(result))
    {
    qWarning() << d->PythonSource << ": qSlicerSegmentEditorScriptedEffect: Function 'helpText' is expected to return a string!";
    return this->Superclass::helpText();
    }

  const char* role = PyString_AsString(result);
  return QString::fromLocal8Bit(role);
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorScriptedEffect::clone()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::activate()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::deactivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::apply()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::setupOptionsFrame()
{
}

//-----------------------------------------------------------------------------
QCursor qSlicerSegmentEditorScriptedEffect::createCursor(qMRMLWidget* viewWidget)
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::processInteractionEvents(vtkRenderWindowInteractor* callerInteractor, unsigned long eid, qMRMLWidget* viewWidget)
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::processViewNodeEvents(vtkMRMLAbstractViewNode* callerViewNode, unsigned long eid, qMRMLWidget* viewWidget)
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::setMRMLDefaults()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::editedLabelmapChanged()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::masterVolumeNodeChanged()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::updateGUIFromMRML()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::updateMRMLFromGUI()
{
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScriptedEffect::addOptionsWidget(QWidget* newOptionsWidget)
{
}
