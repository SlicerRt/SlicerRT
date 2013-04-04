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
#include <vtkSlicerRTPlanModuleLogic.h>

// RTPlan includes
#include "qSlicerRTPlanModule.h"
#include "qSlicerRTPlanModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerRTPlanModule, qSlicerRTPlanModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_RTPlan
class qSlicerRTPlanModulePrivate
{
public:
  qSlicerRTPlanModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRTPlanModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerRTPlanModulePrivate::qSlicerRTPlanModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerRTPlanModule methods

//-----------------------------------------------------------------------------
qSlicerRTPlanModule::qSlicerRTPlanModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerRTPlanModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerRTPlanModule::~qSlicerRTPlanModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerRTPlanModule::helpText()const
{
  QString help = 
    "The RTPlan module manages RTPlan. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/RTPlan\">%1/Documentation/%2.%3/Modules/RTPlan</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerRTPlanModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTPlanModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRTPlanModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (PMH, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerRTPlanModule::icon()const
{
  return QIcon(":/Icons/RTPlan.png");
}

//-----------------------------------------------------------------------------
void qSlicerRTPlanModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerRTPlanModule::createWidgetRepresentation()
{
  return new qSlicerRTPlanModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerRTPlanModule::createLogic()
{
  return vtkSlicerRTPlanModuleLogic::New();
}
