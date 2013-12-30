/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// Qt includes
#include <QtPlugin>

// RTHierarchy Logic includes
#include <vtkSlicerRTHierarchyModuleLogic.h>

// RTHierarchy includes
#include "qSlicerRTHierarchyModule.h"
#include "qSlicerRTHierarchyModuleWidget.h"
#include "qSlicerSubjectHierarchyRTPlugin.h"

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyPluginHandler.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerRTHierarchyModule, qSlicerRTHierarchyModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_RtHierarchy
class qSlicerRTHierarchyModulePrivate
{
public:
  qSlicerRTHierarchyModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRTHierarchyModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerRTHierarchyModulePrivate
::qSlicerRTHierarchyModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerRTHierarchyModule methods

//-----------------------------------------------------------------------------
qSlicerRTHierarchyModule
::qSlicerRTHierarchyModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerRTHierarchyModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerRTHierarchyModule::~qSlicerRTHierarchyModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerRTHierarchyModule::helpText()const
{
  QString help = 
    "The RT Hierarchy module manages and displays radiation therapy data in the Subject Hierarchy module (this module does not have its own separate user interface)."
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/RTHierarchy\">%1/Documentation/%2.%3/Modules/RTHierarchy</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerRTHierarchyModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTHierarchyModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTHierarchyModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTHierarchyModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerRTHierarchyModule::setup()
{
  this->Superclass::setup();

  // Register Subject Hierarchy RT plugin
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRTPlugin());
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerRTHierarchyModule
::createWidgetRepresentation()
{
  return new qSlicerRTHierarchyModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerRTHierarchyModule::createLogic()
{
  return vtkSlicerRTHierarchyModuleLogic::New();
}
