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

// Contours includes
#include "qSlicerContoursModule.h"
#include "qSlicerContoursModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerContoursModule, qSlicerContoursModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
class qSlicerContoursModulePrivate
{
public:
  qSlicerContoursModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerContoursModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerContoursModulePrivate::qSlicerContoursModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerContoursModule methods

//-----------------------------------------------------------------------------
qSlicerContoursModule::qSlicerContoursModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerContoursModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerContoursModule::~qSlicerContoursModule()
{
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerContoursModule::createLogic()
{
  return NULL;
}

//-----------------------------------------------------------------------------
QString qSlicerContoursModule::helpText()const
{
  QString help = 
    "The Contours module manages contours and contour hierarchies. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/Contours\">%1/Documentation/%2.%3/Modules/Contours</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerContoursModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerContoursModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerContoursModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerContoursModule::icon()const
{
  return QIcon(":/Icons/Contours.png");
}

//-----------------------------------------------------------------------------
void qSlicerContoursModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerContoursModule::createWidgetRepresentation()
{
  return new qSlicerContoursModuleWidget;
}
