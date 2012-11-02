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

// ProtonDose Logic includes
#include <vtkSlicerProtonDoseModuleLogic.h>

// ProtonDose includes
#include "qSlicerProtonDoseModule.h"
#include "qSlicerProtonDoseModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerProtonDoseModule, qSlicerProtonDoseModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ProtonDose
class qSlicerProtonDoseModulePrivate
{
public:
  qSlicerProtonDoseModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerProtonDoseModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerProtonDoseModulePrivate::qSlicerProtonDoseModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerProtonDoseModule methods

//-----------------------------------------------------------------------------
qSlicerProtonDoseModule::qSlicerProtonDoseModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerProtonDoseModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerProtonDoseModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerProtonDoseModule::~qSlicerProtonDoseModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerProtonDoseModule::helpText()const
{
  QString help = 
    "This module computes proton dose distributions. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/ProtonDose\">%1/Documentation/%2.%3/Modules/ProtonDose</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerProtonDoseModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerProtonDoseModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Greg Sharp (BWH, Boston)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerProtonDoseModule::icon()const
{
  return QIcon(":/Icons/ProtonDose.png");
}

//-----------------------------------------------------------------------------
void qSlicerProtonDoseModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerProtonDoseModule::createWidgetRepresentation()
{
  return new qSlicerProtonDoseModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerProtonDoseModule::createLogic()
{
  return vtkSlicerProtonDoseModuleLogic::New();
}
