/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes
#include <QtPlugin>

// Isodose Logic includes
#include <vtkSlicerIsodoseModuleLogic.h>

// Isodose includes
#include "qSlicerIsodoseModule.h"
#include "qSlicerIsodoseModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerIsodoseModule, qSlicerIsodoseModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Isodose
class qSlicerIsodoseModulePrivate
{
public:
  qSlicerIsodoseModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModulePrivate::qSlicerIsodoseModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerIsodoseModule methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModule::qSlicerIsodoseModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerIsodoseModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerIsodoseModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModule::~qSlicerIsodoseModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModule::helpText()const
{
  QString help = 
    "This template module is meant to be used with the"
    "with the ModuleWizard.py script distributed with the"
    "Slicer source code (starting with version 4)."
    "Developers can generate their own source code using the"
    "wizard and then customize it to fit their needs.";
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModule::acknowledgementText()const
{
  return "This work was supported by NAMIC, NAC, and the Slicer Community...";
}

//-----------------------------------------------------------------------------
QStringList qSlicerIsodoseModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (Organization)");
  // moduleContributors << QString("Richard Roe (Organization2)");
  // ...
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerIsodoseModule::icon()const
{
  return QIcon(":/Icons/Isodose.png");
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerIsodoseModule::createWidgetRepresentation()
{
  return new qSlicerIsodoseModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerIsodoseModule::createLogic()
{
  return vtkSlicerIsodoseModuleLogic::New();
}
