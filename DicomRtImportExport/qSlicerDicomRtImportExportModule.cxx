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

// DicomRtImportExport includes
#include "qSlicerDicomRtImportExportModule.h"
#include "qSlicerDicomRtImportExportModuleWidget.h"
#include "vtkSlicerDicomRtImportExportModuleLogic.h"

// Qt includes
#include <QDebug> 

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyRtImagePlugin.h"
#include "qSlicerSubjectHierarchyRtDoseVolumePlugin.h"

// SlicerRT includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkSlicerPlanarImageModuleLogic.h"
#include "vtkSlicerBeamsModuleLogic.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerDicomRtImportExportModule, qSlicerDicomRtImportExportModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DicomRtImportExport
class qSlicerDicomRtImportExportModulePrivate
{
public:
  qSlicerDicomRtImportExportModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportExportModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModulePrivate::qSlicerDicomRtImportExportModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportExportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModule::qSlicerDicomRtImportExportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomRtImportExportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModule::~qSlicerDicomRtImportExportModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportExportModule::helpText()const
{
  return QString("The DicomRtImportExport module enables importing and loading DICOM RT files into the Slicer DICOM database and the Slicer scene, and exporting MRML nodes into DICOM-RT files"
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DicomRtImportExport\">%1/Documentation/%2.%3/Modules/Models</a><br>").arg(
    this->slicerWikiUrl()).arg(qSlicerCoreApplication::application()->majorVersion()).arg(qSlicerCoreApplication::application()->minorVersion());
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportExportModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtImportExportModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtImportExportModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomRtImportExportModule::dependencies()const
{
  return QStringList() << "Volumes" << "Isodose" << "PlanarImage" << "Beams";
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportExportModule::setup()
{
  this->Superclass::setup();

  vtkSlicerDicomRtImportExportModuleLogic* dicomRtImportExportLogic = vtkSlicerDicomRtImportExportModuleLogic::SafeDownCast(this->logic());

  // Set isodose logic to the logic
  qSlicerAbstractCoreModule* isodoseModule = qSlicerCoreApplication::application()->moduleManager()->module("Isodose");
  if (isodoseModule)
  {
    vtkSlicerIsodoseModuleLogic* isodoseLogic = vtkSlicerIsodoseModuleLogic::SafeDownCast(isodoseModule->logic());
    dicomRtImportExportLogic->SetIsodoseLogic(isodoseLogic);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Isodose module is not found";
  } 

  // Set planar image logic to the logic
  qSlicerAbstractCoreModule* planarImageModule = qSlicerCoreApplication::application()->moduleManager()->module("PlanarImage");
  if (planarImageModule)
  {
    vtkSlicerPlanarImageModuleLogic* planarImageLogic = vtkSlicerPlanarImageModuleLogic::SafeDownCast(planarImageModule->logic());
    dicomRtImportExportLogic->SetPlanarImageLogic(planarImageLogic);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Planar Image module is not found";
  } 

  // Set beams logic to the logic
  qSlicerAbstractCoreModule* beamsModule = qSlicerCoreApplication::application()->moduleManager()->module("Beams");
  if (beamsModule)
  {
    vtkSlicerBeamsModuleLogic* beamsLogic = vtkSlicerBeamsModuleLogic::SafeDownCast(beamsModule->logic());
    dicomRtImportExportLogic->SetBeamsLogic(beamsLogic);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Beams module is not found";
  } 

  // Register Subject Hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRtImagePlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRtDoseVolumePlugin());
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDicomRtImportExportModule::createWidgetRepresentation()
{
  return new qSlicerDicomRtImportExportModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDicomRtImportExportModule::createLogic()
{
  return vtkSlicerDicomRtImportExportModuleLogic::New();
}
