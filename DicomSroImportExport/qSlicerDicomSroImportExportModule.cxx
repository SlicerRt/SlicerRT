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

// Qt includes
#include <QDebug> 

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerDicomSroImportExportModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerDicomSroImportExportModule.h"
#include "qSlicerDicomSroImportExportModuleWidget.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerDicomSroImportExportModule, qSlicerDicomSroImportExportModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_DicomSroImportExport
class qSlicerDicomSroImportExportModulePrivate
{
public:
  qSlicerDicomSroImportExportModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportExportModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportExportModulePrivate::qSlicerDicomSroImportExportModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerDicomSroImportExportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomSroImportExportModule::qSlicerDicomSroImportExportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomSroImportExportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomSroImportExportModule::~qSlicerDicomSroImportExportModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerDicomSroImportExportModule::helpText()const
{
  return QString("The DicomSroImportExport module enables importing and loading DICOM Spatial Registration Objects into the Slicer DICOM database and the Slicer scene, and exporting transformations as DICOM SROs. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DicomSroImport\">%1/Documentation/%2.%3/Modules/Models</a><br>").arg(
    this->slicerWikiUrl()).arg(qSlicerCoreApplication::application()->majorVersion()).arg(qSlicerCoreApplication::application()->minorVersion());
}

//-----------------------------------------------------------------------------
QString qSlicerDicomSroImportExportModule::acknowledgementText()const
{
  return "This work was funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO), and CANARIE.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomSroImportExportModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Centre)") << QString("Csaba Pinter (Queen's University)") << QString("Greg Sharp (Massachusetts General Hospital)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDicomSroImportExportModule::icon()const
{
  return QIcon(":/Icons/DicomSroImportExport.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomSroImportExportModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDicomSroImportExportModule::dependencies()const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerDicomSroImportExportModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDicomSroImportExportModule::createWidgetRepresentation()
{
  return new qSlicerDicomSroImportExportModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDicomSroImportExportModule::createLogic()
{
  return vtkSlicerDicomSroImportExportModuleLogic::New();
}
