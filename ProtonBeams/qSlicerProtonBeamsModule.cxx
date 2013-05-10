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

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// Beams Logic includes
#include <vtkSlicerProtonBeamsModuleLogic.h>

// Beams includes
#include "qSlicerProtonBeamsModule.h"
#include "qSlicerProtonBeamsModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerProtonBeamsModule, qSlicerProtonBeamsModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Beams
class qSlicerProtonBeamsModulePrivate
{
public:
  qSlicerProtonBeamsModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerProtonBeamsModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModulePrivate::qSlicerProtonBeamsModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerProtonBeamsModule methods

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModule::qSlicerProtonBeamsModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerProtonBeamsModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerProtonBeamsModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerProtonBeamsModule::~qSlicerProtonBeamsModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerProtonBeamsModule::helpText()const
{
  QString help = 
    "This module displays and handles beam geometry modules created from the loaded isocenter and source fiducials for proton beams. "
    /*"For more information see <a href=\"%1/Documentation/%2.%3/Modules/Beams\">%1/Documentation/%2.%3/Modules/Beams</a><br>";*/
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerProtonBeamsModule::acknowledgementText()const
{
  /*return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
*/
}

//-----------------------------------------------------------------------------
QStringList qSlicerProtonBeamsModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerProtonBeamsModule::icon()const
{
  return QIcon(":/Icons/ProtonBeams.png");
}

//-----------------------------------------------------------------------------
void qSlicerProtonBeamsModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerProtonBeamsModule::createWidgetRepresentation()
{
  return new qSlicerProtonBeamsModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerProtonBeamsModule::createLogic()
{
  return vtkSlicerProtonBeamsModuleLogic::New();
}
