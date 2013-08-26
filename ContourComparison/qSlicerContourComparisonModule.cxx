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

// ContourComparison Logic includes
#include <vtkSlicerContourComparisonModuleLogic.h>

// ContourComparison includes
#include "qSlicerContourComparisonModule.h"
#include "qSlicerContourComparisonModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerContourComparisonModule, qSlicerContourComparisonModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ContourComparison
class qSlicerContourComparisonModulePrivate
{
public:
  qSlicerContourComparisonModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerContourComparisonModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerContourComparisonModulePrivate::qSlicerContourComparisonModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerContourComparisonModule methods

//-----------------------------------------------------------------------------
qSlicerContourComparisonModule::qSlicerContourComparisonModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerContourComparisonModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerContourComparisonModule::~qSlicerContourComparisonModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerContourComparisonModule::helpText()const
{
  QString help = 
    "This module computes contour similarity metrics. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/ContourComparison\">%1/Documentation/%2.%3/Modules/ContourComparison</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerContourComparisonModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerContourComparisonModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerContourComparisonModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerContourComparisonModule::icon()const
{
  return this->Superclass::icon();
  //return QIcon(":/Icons/ContourComparison.png");
}

//-----------------------------------------------------------------------------
void qSlicerContourComparisonModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerContourComparisonModule::createWidgetRepresentation()
{
  return new qSlicerContourComparisonModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerContourComparisonModule::createLogic()
{
  return vtkSlicerContourComparisonModuleLogic::New();
}
