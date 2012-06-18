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

// GammaDoseComparison Logic includes
#include <vtkSlicerGammaDoseComparisonLogic.h>

// GammaDoseComparison includes
#include "qSlicerGammaDoseComparisonModule.h"
#include "qSlicerGammaDoseComparisonModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerGammaDoseComparisonModule, qSlicerGammaDoseComparisonModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_GammaDoseComparison
class qSlicerGammaDoseComparisonModulePrivate
{
public:
  qSlicerGammaDoseComparisonModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerGammaDoseComparisonModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerGammaDoseComparisonModulePrivate::qSlicerGammaDoseComparisonModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerGammaDoseComparisonModule methods

//-----------------------------------------------------------------------------
qSlicerGammaDoseComparisonModule::qSlicerGammaDoseComparisonModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerGammaDoseComparisonModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerGammaDoseComparisonModule::categories()const
{
  return QStringList() << "Developer Tools";
}

//-----------------------------------------------------------------------------
qSlicerGammaDoseComparisonModule::~qSlicerGammaDoseComparisonModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerGammaDoseComparisonModule::helpText()const
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
QString qSlicerGammaDoseComparisonModule::acknowledgementText()const
{
  return "This work was supported by NAMIC, NAC, and the Slicer Community...";
}

//-----------------------------------------------------------------------------
QStringList qSlicerGammaDoseComparisonModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("John Doe (Organization)");
  // moduleContributors << QString("Richard Roe (Organization2)");
  // ...
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerGammaDoseComparisonModule::icon()const
{
  return QIcon(":/Icons/GammaDoseComparison.png");
}

//-----------------------------------------------------------------------------
void qSlicerGammaDoseComparisonModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerGammaDoseComparisonModule::createWidgetRepresentation()
{
  return new qSlicerGammaDoseComparisonModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerGammaDoseComparisonModule::createLogic()
{
  return vtkSlicerGammaDoseComparisonLogic::New();
}
