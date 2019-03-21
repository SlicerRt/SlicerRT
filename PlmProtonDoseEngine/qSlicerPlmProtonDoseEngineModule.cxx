/*==============================================================================

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
  and was supported through CANARIE.

==============================================================================*/

// PlmProtonDoseEngine Logic includes
#include <vtkSlicerPlmProtonDoseEngineLogic.h>

// PlmProtonDoseEngine includes
#include "qSlicerPlmProtonDoseEngineModule.h"
#include "qSlicerPlmProtonDoseEngineModuleWidget.h"

// ExternalBeamPlanning includes
#include "qSlicerDoseEnginePluginHandler.h"

// DoseEngines includes
#include "qSlicerPlmProtonDoseEngine.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerPlmProtonDoseEngineModule, qSlicerPlmProtonDoseEngineModule);
#endif

//-----------------------------------------------------------------------------
// qSlicerPlmProtonDoseEngineModule methods

//-----------------------------------------------------------------------------
qSlicerPlmProtonDoseEngineModule::qSlicerPlmProtonDoseEngineModule(QObject* _parent)
  : Superclass(_parent)
{
}

//-----------------------------------------------------------------------------
qSlicerPlmProtonDoseEngineModule::~qSlicerPlmProtonDoseEngineModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerPlmProtonDoseEngineModule::helpText() const
{
  return "This is a hidden module providing the Plastimatch proton dose engine";
}

//-----------------------------------------------------------------------------
QString qSlicerPlmProtonDoseEngineModule::acknowledgementText() const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmProtonDoseEngineModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Greg Sharp (MGH)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Centre)");
  moduleContributors << QString("Maxime Desplanques (CNAO, Italy)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmProtonDoseEngineModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlmProtonDoseEngineModule::dependencies() const
{
  return QStringList() << "ExternalBeamPlanning";
}

//-----------------------------------------------------------------------------
void qSlicerPlmProtonDoseEngineModule::setup()
{
  this->Superclass::setup();

  // Register dose engines
  qSlicerDoseEnginePluginHandler::instance()->registerDoseEngine(new qSlicerPlmProtonDoseEngine());
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPlmProtonDoseEngineModule
::createWidgetRepresentation()
{
  return new qSlicerPlmProtonDoseEngineModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlmProtonDoseEngineModule::createLogic()
{
  return vtkSlicerPlmProtonDoseEngineLogic::New();
}
