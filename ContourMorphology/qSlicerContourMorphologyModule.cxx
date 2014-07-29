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

// ExtensionTemplate Logic includes
#include <vtkSlicerContourMorphologyModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerContourMorphologyModule.h"
#include "qSlicerContourMorphologyModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerContourMorphologyModule, qSlicerContourMorphologyModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ContourMorphology
class qSlicerContourMorphologyModulePrivate
{
public:
  qSlicerContourMorphologyModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerContourMorphologyModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModulePrivate::qSlicerContourMorphologyModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerContourMorphologyModule methods

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModule::qSlicerContourMorphologyModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerContourMorphologyModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerContourMorphologyModule::~qSlicerContourMorphologyModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerContourMorphologyModule::helpText()const
{
  QString help = 
    "The Contour morphology module performs simualtion study of patient motion. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/ContourMorphology\">%1/Documentation/%2.%3/Modules/ContourMorphology</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerContourMorphologyModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerContourMorphologyModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerContourMorphologyModule::icon()const
{
  return this->Superclass::icon();
  //return QIcon(":/Icons/ContourMorphology.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerContourMorphologyModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerContourMorphologyModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerContourMorphologyModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerContourMorphologyModule::createWidgetRepresentation()
{
  return new qSlicerContourMorphologyModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerContourMorphologyModule::createLogic()
{
  return vtkSlicerContourMorphologyModuleLogic::New();
}
