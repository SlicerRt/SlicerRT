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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Qt includes
#include <QtPlugin>

// DoseComparison Logic includes
#include <vtkSlicerDoseComparisonModuleLogic.h>

// DoseComparison includes
#include "qSlicerDoseComparisonModule.h"
#include "qSlicerDoseComparisonModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDoseComparisonModule, qSlicerDoseComparisonModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoseComparison
class qSlicerDoseComparisonModulePrivate
{
public:
  qSlicerDoseComparisonModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModulePrivate::qSlicerDoseComparisonModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModule methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModule::qSlicerDoseComparisonModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDoseComparisonModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModule::~qSlicerDoseComparisonModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDoseComparisonModule::helpText()const
{
  QString help = 
    "Compute and display dose comparison metrics.";
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerDoseComparisonModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseComparisonModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseComparisonModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDoseComparisonModule::icon()const
{
  return QIcon(":/Icons/DoseComparison.png");
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDoseComparisonModule::createWidgetRepresentation()
{
  return new qSlicerDoseComparisonModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDoseComparisonModule::createLogic()
{
  return vtkSlicerDoseComparisonModuleLogic::New();
}
