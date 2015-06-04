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
#include "vtkRibbonModelToBinaryLabelmapConversionRule.h"
#include "vtkPlanarContourToRibbonModelConversionRule.h"

// Qt includes
#include <QDebug> 
#include <QtPlugin>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <vtkSlicerVolumesLogic.h>

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyRtImagePlugin.h"
#include "qSlicerSubjectHierarchyRtDoseVolumePlugin.h"
#include "qSlicerSubjectHierarchyRtPlanPlugin.h"
#include "qSlicerSubjectHierarchyRtBeamPlugin.h"

// SlicerRT includes
#include "vtkSlicerIsodoseModuleLogic.h"
#include "vtkSlicerPlanarImageModuleLogic.h"

// SegmentationCore includes
#include "vtkSegmentationConverterFactory.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDicomRtImportExportModule, qSlicerDicomRtImportExportModule);

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
qSlicerDicomRtImportExportModulePrivate::qSlicerDicomRtImportExportModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportExportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModule::qSlicerDicomRtImportExportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomRtImportExportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtImportExportModule::~qSlicerDicomRtImportExportModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportExportModule::helpText()const
{
  QString help = 
    "The DicomRtImportExport module enables importing and loading DICOM RT files into the Slicer DICOM database and the Slicer scene, and exporting MRML nodes into DICOM-RT files"
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DicomRtImportExport\">%1/Documentation/%2.%3/Modules/Models</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
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
  return QStringList() << "Volumes" << "Isodose" << "PlanarImage";
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportExportModule::setup()
{
  this->Superclass::setup();

  // Create DICOM-RT import logic
  vtkSlicerDicomRtImportExportModuleLogic* dicomRtImportExportLogic = vtkSlicerDicomRtImportExportModuleLogic::SafeDownCast(this->logic());

  // Set volumes logic to the logic
  qSlicerAbstractCoreModule* volumesModule = qSlicerCoreApplication::application()->moduleManager()->module("Volumes");
  if (volumesModule)
  {
    vtkSlicerVolumesLogic* volumesLogic = vtkSlicerVolumesLogic::SafeDownCast(volumesModule->logic());
    dicomRtImportExportLogic->SetVolumesLogic(volumesLogic);
  }
  else
  {
    qCritical() << "qSlicerDicomRtImportExportModule::setup: Volumes module is not found";
  } 

  // Set isodose logic to the logic
  qSlicerAbstractCoreModule* isodoseModule = qSlicerCoreApplication::application()->moduleManager()->module("Isodose");
  if (isodoseModule)
  {
    vtkSlicerIsodoseModuleLogic* isodoseLogic = vtkSlicerIsodoseModuleLogic::SafeDownCast(isodoseModule->logic());
    dicomRtImportExportLogic->SetIsodoseLogic(isodoseLogic);
  }
  else
  {
    qCritical() << "qSlicerDicomRtImportExportModule::setup: Isodose module is not found";
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
    qCritical() << "qSlicerDicomRtImportExportModule::setup: Planar Image module is not found";
  } 

  // Register Subject Hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRtImagePlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRtDoseVolumePlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRtPlanPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRtBeamPlugin());

  // Register converter rules
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkRibbonModelToBinaryLabelmapConversionRule>::New() );
  vtkSegmentationConverterFactory::GetInstance()->RegisterConverterRule(
    vtkSmartPointer<vtkPlanarContourToRibbonModelConversionRule>::New() );
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
