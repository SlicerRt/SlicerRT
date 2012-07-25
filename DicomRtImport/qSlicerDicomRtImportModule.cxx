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
#include <QDebug> 
#include <QtPlugin>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <vtkSlicerVolumesLogic.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerDicomRtImportModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerDicomRtImportModule.h"
#include "qSlicerDicomRtImportModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDicomRtImportModule, qSlicerDicomRtImportModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerDicomRtImportModulePrivate
{
public:
  qSlicerDicomRtImportModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModulePrivate::qSlicerDicomRtImportModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModule::qSlicerDicomRtImportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomRtImportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModule::~qSlicerDicomRtImportModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportModule::helpText()const
{
  return "This DicomRtImport module enables importing and loading DICOM RT files into the Slicer DICOM database and the Slicer scene";
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtImportModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDicomRtImportModule::icon()const
{
  return QIcon(":/Icons/DicomRtImport.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtImportModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportModule::setup()
{
  this->Superclass::setup();

  vtkSlicerDicomRtImportLogic* dicomRtImportLogic = vtkSlicerDicomRtImportLogic::SafeDownCast(this->logic());

  qSlicerAbstractCoreModule* volumesModule = qSlicerCoreApplication::application()->moduleManager()->module("Volumes");
  if (volumesModule)
  {
    vtkSlicerVolumesLogic* volumesLogic = vtkSlicerVolumesLogic::SafeDownCast(volumesModule->logic());
    dicomRtImportLogic->SetVolumesLogic(volumesLogic);
  }
  else
  {
    qWarning() << "Volumes module is not found";
  } 
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDicomRtImportModule::createWidgetRepresentation()
{
  return new qSlicerDicomRtImportModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDicomRtImportModule::createLogic()
{
  return vtkSlicerDicomRtImportLogic::New();
}
