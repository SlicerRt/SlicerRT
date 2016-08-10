/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QtPlugin>
#include <QDebug>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModule.h"
#include "qSlicerExternalBeamPlanningModuleWidget.h"
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"

// DoseEngines includes
#include "qSlicerDoseEnginePluginHandler.h"
#include "qSlicerPlastimatchProtonDoseEngine.h"
#include "qSlicerMockDoseEngine.h"

// SlicerRT includes
#include "vtkSlicerBeamsModuleLogic.h"

// SubjectHierarchy Plugins includes
#include <qSlicerSubjectHierarchyPluginHandler.h>
#include "qSlicerSubjectHierarchyRTPlanPlugin.h"
#include "qSlicerSubjectHierarchyRTBeamPlugin.h"

// PythonQt includes
#include "PythonQt.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerExternalBeamPlanningModule, qSlicerExternalBeamPlanningModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerExternalBeamPlanningModulePrivate
{
public:
  qSlicerExternalBeamPlanningModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModulePrivate::qSlicerExternalBeamPlanningModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerExternalBeamPlanningModule methods

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModule::qSlicerExternalBeamPlanningModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerExternalBeamPlanningModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerExternalBeamPlanningModule::~qSlicerExternalBeamPlanningModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerExternalBeamPlanningModule::helpText()const
{
  QString help = 
    "The External Beam Planning module facilitates basic EBRT planning. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/ExternalBeamPlanning\">%1/Documentation/%2.%3/Modules/ExternalBeamPlanning</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerExternalBeamPlanningModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerExternalBeamPlanningModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerExternalBeamPlanningModule::dependencies()const
{
  return QStringList() << "Beams";
}

//-----------------------------------------------------------------------------
QStringList qSlicerExternalBeamPlanningModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Greg Sharp (MGH)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Centre)");
  moduleContributors << QString("Maxime Desplanques (CNAO, Italy)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerExternalBeamPlanningModule::icon()const
{
  return QIcon(":/Icons/ExternalBeamPlanning.png");
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModule::setup()
{
  this->Superclass::setup();

  vtkSlicerExternalBeamPlanningModuleLogic* ebpLogic = vtkSlicerExternalBeamPlanningModuleLogic::SafeDownCast(this->logic());

  // Set beams logic to the logic
  qSlicerAbstractCoreModule* beamsModule = qSlicerCoreApplication::application()->moduleManager()->module("Beams");
  if (beamsModule)
  {
    vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(beamsModule->logic());
    ebpLogic->SetBeamsLogic(beamsLogic);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Beams module is not found";
  }

  // Register dose engines
  qSlicerDoseEnginePluginHandler::instance()->registerDoseEngine(new qSlicerPlastimatchProtonDoseEngine());
  qSlicerDoseEnginePluginHandler::instance()->registerDoseEngine(new qSlicerMockDoseEngine());

  // Python engines
  // (otherwise it would be the responsibility of the module that embeds the dose engine)
  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript( QString(
    "from DoseEngines import * \n"
    "import qSlicerExternalBeamPlanningDoseEnginesPythonQt as engines \n"
    "import traceback \n"
    "import logging \n"
    "try: \n"
    "  slicer.modules.doseenginenames \n"
    "except AttributeError: \n"
    "  slicer.modules.doseenginenames=[] \n"
    "for engineName in slicer.modules.doseenginenames: \n"
    "  try: \n"
    "    exec(\"{0}Instance = engines.qSlicerScriptedDoseEngine(None);{0}Instance.setPythonSource({0}.__file__.replace('\\\\\\\\','/'));{0}Instance.self().register()\".format(engineName)) \n"
    "  except Exception as e: \n"
    "    logging.error(traceback.format_exc()) \n") );
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerExternalBeamPlanningModule::createWidgetRepresentation()
{
  return new qSlicerExternalBeamPlanningModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerExternalBeamPlanningModule::createLogic()
{
  return vtkSlicerExternalBeamPlanningModuleLogic::New();
}

//-----------------------------------------------------------------------------
QStringList qSlicerExternalBeamPlanningModule::associatedNodeTypes() const
{
  return QStringList() << "vtkMRMLRTPlanNode";
}
