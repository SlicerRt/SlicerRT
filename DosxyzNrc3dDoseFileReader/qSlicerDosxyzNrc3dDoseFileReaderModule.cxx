/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Anna Ilina, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario.

==============================================================================*/

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerIOManager.h>
#include <qSlicerNodeWriter.h>

// DosxyzNrc3dDoseFileReader Logic includes
#include <vtkSlicerDosxyzNrc3dDoseFileReaderLogic.h>

// DosxyzNrc3dDoseFileReader QTModule includes
#include "qSlicerDosxyzNrc3dDoseFileReaderPlugin.h"
#include "qSlicerDosxyzNrc3dDoseFileReaderModule.h"
#include "qSlicerDosxyzNrc3dDoseFileReaderPluginWidget.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerDosxyzNrc3dDoseFileReaderModule, qSlicerDosxyzNrc3dDoseFileReaderModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class qSlicerDosxyzNrc3dDoseFileReaderModulePrivate
{
public:
  qSlicerDosxyzNrc3dDoseFileReaderModulePrivate();
};

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderModulePrivate::qSlicerDosxyzNrc3dDoseFileReaderModulePrivate()
{
}


//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderModule::qSlicerDosxyzNrc3dDoseFileReaderModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDosxyzNrc3dDoseFileReaderModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderModule::~qSlicerDosxyzNrc3dDoseFileReaderModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDosxyzNrc3dDoseFileReaderModule::helpText()const
{
  QString help = QString(
    "The DosxyzNrc3dDoseFileReader module enables importing and loading .3ddose files into Slicer.<br>"
    "The DosxyzNrc3dDoseFileReader module is hidden and therefore does not require an application.<br>");
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerDosxyzNrc3dDoseFileReaderModule::acknowledgementText()const
{
  QString acknowledgement = QString(
    "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).");
  return acknowledgement;
}

//-----------------------------------------------------------------------------
QStringList qSlicerDosxyzNrc3dDoseFileReaderModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Anna Ilina (Queen's)") << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerDosxyzNrc3dDoseFileReaderModule::categories()const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
void qSlicerDosxyzNrc3dDoseFileReaderModule::setup()
{
  this->Superclass::setup();
  
  vtkSlicerDosxyzNrc3dDoseFileReaderLogic* DosxyzNrc3dDoseFileReaderLogic =  
    vtkSlicerDosxyzNrc3dDoseFileReaderLogic::SafeDownCast(this->logic());

  // Adds the module to the IO Manager
  qSlicerCoreIOManager* ioManager =
    qSlicerCoreApplication::application()->coreIOManager();
  ioManager->registerIO(new qSlicerDosxyzNrc3dDoseFileReaderPlugin(DosxyzNrc3dDoseFileReaderLogic,this));
  ioManager->registerIO(new qSlicerNodeWriter(
    "DosxyzNrc3dDose", QString("DosxyzNrc3dDoseFile"),
    QStringList() << "vtkMRMLScalarVolumeNode", true, this));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerDosxyzNrc3dDoseFileReaderModule::createWidgetRepresentation()
{
  return new qSlicerDosxyzNrc3dDoseFileReaderPluginWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDosxyzNrc3dDoseFileReaderModule::createLogic()
{
  return vtkSlicerDosxyzNrc3dDoseFileReaderLogic::New();
}
