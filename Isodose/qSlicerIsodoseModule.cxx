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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Isodose Logic includes
#include <vtkSlicerIsodoseModuleLogic.h>

// Isodose includes
#include "qSlicerIsodoseModule.h"
#include "qSlicerIsodoseModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Isodose
class qSlicerIsodoseModulePrivate
{
public:
  qSlicerIsodoseModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerIsodoseModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModulePrivate::qSlicerIsodoseModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerIsodoseModule methods

//-----------------------------------------------------------------------------
qSlicerIsodoseModule::qSlicerIsodoseModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerIsodoseModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerIsodoseModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerIsodoseModule::~qSlicerIsodoseModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModule::helpText()const
{
  QString help = 
    "This module generates iso dose surface models using user defined dose levels. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/Isodose\">%1/Documentation/%2.%3/Modules/Isodose</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerIsodoseModule::acknowledgementText()const
{
  return "This work was funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO), and CANARIE.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerIsodoseModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (PMH, Toronto)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerIsodoseModule::icon()const
{
  return QIcon(":/Icons/Isodose.png");
}

//-----------------------------------------------------------------------------
void qSlicerIsodoseModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerIsodoseModule::createWidgetRepresentation()
{
  return new qSlicerIsodoseModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerIsodoseModule::createLogic()
{
  return vtkSlicerIsodoseModuleLogic::New();
}
