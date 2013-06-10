/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// Qt includes
#include <QtPlugin>

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
Q_EXPORT_PLUGIN2(qSlicerVffFileReaderModule, qSlicerVffFileReaderModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_VffFileReader
class qSlicerVffFileReaderModulePrivate
{
public:
  qSlicerVffFileReaderModulePrivate();
};

//-----------------------------------------------------------------------------
qSlicerVffFileReaderModulePrivate::qSlicerVffFileReaderModulePrivate()
{
}


//-----------------------------------------------------------------------------
qSlicerVffFileReaderModule::qSlicerVffFileReaderModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerVffFileReaderModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderModule::~qSlicerVffFileReaderModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerVffFileReaderModule::helpText()const
{
  QString help = QString(
    "The VffFileReader module enables importing and loading VFF files into Slicer.<br>"
    "The VffFileReader module is hidden and therefore does not require an application.<br>");
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
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
    QStringList() << "vtkMRMLScalarVolumeNode", this));
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

