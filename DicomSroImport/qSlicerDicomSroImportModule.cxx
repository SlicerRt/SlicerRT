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
#include <QDebug> 
#include <QtPlugin>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <vtkSlicerVolumesLogic.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerDicomSroImportModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerDicomSroImportModule.h"
#include "qSlicerDicomSroImportModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDicomSroImportModule, qSlicerDicomSroImportModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomSroImport
class qSlicerDicomSroImportModulePrivate
{
public:
  qSlicerDicomSroImportModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportModulePrivate::qSlicerDicomSroImportModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportModule::qSlicerDicomSroImportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomSroImportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomSroImportModule::~qSlicerDicomSroImportModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDicomSroImportModule::helpText()const
{
  QString help = 
    "The DicomSroImport module enables importing and loading DICOM Spatial Registration Objects into the Slicer DICOM database and the Slicer scene. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DicomSroImport\">%1/Documentation/%2.%3/Modules/Models</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerDicomSroImportModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomSroImportModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Centre)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDicomSroImportModule::icon()const
{
  return QIcon(":/Icons/DicomSroImport.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomSroImportModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomSroImportModule::dependencies()const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
void qSlicerDicomSroImportModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDicomSroImportModule::createWidgetRepresentation()
{
  return new qSlicerDicomSroImportModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDicomSroImportModule::createLogic()
{
  return vtkSlicerDicomSroImportModuleLogic::New();
}
