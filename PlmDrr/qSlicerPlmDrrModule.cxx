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

// PlmDrr includes
#include "qSlicerPlmDrrModule.h"
#include "qSlicerPlmDrrModuleWidget.h"

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// SlicerRT includes
#include <vtkSlicerPlanarImageModuleLogic.h>

// Qt includes
#include <QDebug> 

// PlmDrr Logic includes
#include <vtkSlicerPlmDrrLogic.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlmDrrModulePrivate
{
public:
  qSlicerPlmDrrModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlmDrrModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlmDrrModulePrivate::qSlicerPlmDrrModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlmDrrModule methods

//-----------------------------------------------------------------------------
qSlicerPlmDrrModule::qSlicerPlmDrrModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlmDrrModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlmDrrModule::~qSlicerPlmDrrModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPlmDrrModule::helpText() const
{
  return "This is a loadable module that calculated a digitally " \
  "reconstructed radiograph (DRR) using the plastimatch application.";
}

//-----------------------------------------------------------------------------
QString qSlicerPlmDrrModule::acknowledgementText() const
{
  return QString();
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmDrrModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Mikhail Polkovnikov (NRC \"Kurchatov Institute\" - IHEP)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerPlmDrrModule::icon() const
{
  return QIcon(":/Icons/PlmDrr.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmDrrModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmDrrModule::dependencies() const
{
  return QStringList() << "Volumes" << "PlanarImage" << "Beams";
}

//-----------------------------------------------------------------------------
void qSlicerPlmDrrModule::setup()
{
  this->Superclass::setup();

  vtkSlicerPlmDrrLogic* drrLogic = vtkSlicerPlmDrrLogic::SafeDownCast(this->logic());

  // Set planar image logic to the logic
  qSlicerAbstractCoreModule* planarImageModule = qSlicerCoreApplication::application()->moduleManager()->module("PlanarImage");
  if (planarImageModule && drrLogic)
  {
    vtkSlicerPlanarImageModuleLogic* planarImageLogic = vtkSlicerPlanarImageModuleLogic::SafeDownCast(planarImageModule->logic());
    drrLogic->SetPlanarImageLogic(planarImageLogic);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Planar Image module is not found";
  }
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPlmDrrModule
::createWidgetRepresentation()
{
  return new qSlicerPlmDrrModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlmDrrModule::createLogic()
{
  return vtkSlicerPlmDrrLogic::New();
}
