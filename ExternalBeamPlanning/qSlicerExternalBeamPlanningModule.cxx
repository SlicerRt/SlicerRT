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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QtPlugin>

// ExtensionTemplate Logic includes
#include <vtkSlicerExternalBeamPlanningModuleLogic.h>

// ExternalBeamPlanning includes
#include "qSlicerExternalBeamPlanningModule.h"
#include "qSlicerExternalBeamPlanningModuleWidget.h"

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
QStringList qSlicerExternalBeamPlanningModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Centre)");
  moduleContributors << QString("Maxime Desplanques (CNAO, Italy)");
  moduleContributors << QString("Greg Sharp (MGH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerExternalBeamPlanningModule::icon()const
{
  return this->Superclass::icon();
  //return QIcon(":/Icons/ExternalBeamPlanning.png");
}

//-----------------------------------------------------------------------------
void qSlicerExternalBeamPlanningModule::setup()
{
  this->Superclass::setup();
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
