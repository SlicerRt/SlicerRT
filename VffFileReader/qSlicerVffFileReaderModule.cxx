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

  This file was originally developed by Jennifer Andrea, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==============================================================================*/

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerIOManager.h>
#include <qSlicerNodeWriter.h>

// VffFileReader Logic includes
#include <vtkSlicerVffFileReaderLogic.h>

// VffFileReader QTModule includes
#include "qSlicerVffFileReaderPlugin.h"
#include "qSlicerVffFileReaderModule.h"
#include "qSlicerVffFileReaderPluginWidget.h"


//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerVffFileReaderModule, qSlicerVffFileReaderModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_VffFileReader
class qSlicerVffFileReaderModulePrivate
{
public:
  qSlicerVffFileReaderModulePrivate();
};

//-----------------------------------------------------------------------------
qSlicerVffFileReaderModulePrivate::qSlicerVffFileReaderModulePrivate() = default;


//-----------------------------------------------------------------------------
qSlicerVffFileReaderModule::qSlicerVffFileReaderModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerVffFileReaderModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderModule::~qSlicerVffFileReaderModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerVffFileReaderModule::helpText()const
{
  QString help = QString(
    "The VffFileReader module enables importing and loading VFF files into Slicer.<br>"
    "The VffFileReader module is hidden and therefore does not require an application.<br>");
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerVffFileReaderModule::acknowledgementText()const
{
  QString acknowledgement = QString(
    "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).");
  return acknowledgement;
}

//-----------------------------------------------------------------------------
QStringList qSlicerVffFileReaderModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Jennifer Andrea (Queen's)") << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerVffFileReaderModule::categories()const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
void qSlicerVffFileReaderModule::setup()
{
  this->Superclass::setup();
  
  vtkSlicerVffFileReaderLogic* vffFileReaderLogic =  
    vtkSlicerVffFileReaderLogic::SafeDownCast(this->logic());

  // Adds the module to the IO Manager
  qSlicerCoreIOManager* ioManager =
    qSlicerCoreApplication::application()->coreIOManager();
  ioManager->registerIO(new qSlicerVffFileReaderPlugin(vffFileReaderLogic,this));
  ioManager->registerIO(new qSlicerNodeWriter(
    "Vff", QString("VffFile"),
    QStringList() << "vtkMRMLScalarVolumeNode", true, this));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerVffFileReaderModule::createWidgetRepresentation()
{
  return new qSlicerVffFileReaderPluginWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerVffFileReaderModule::createLogic()
{
  return vtkSlicerVffFileReaderLogic::New();
}

