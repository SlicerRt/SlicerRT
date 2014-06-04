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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QtPlugin>

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerIOManager.h>
#include <qSlicerNodeWriter.h>

// PinnacleDVFReader Logic includes
#include <vtkSlicerPinnacleDVFReaderLogic.h>

// PinnacleDVFReader QTModule includes
#include "qSlicerPinnacleDVFReaderPlugin.h"
#include "qSlicerPinnacleDVFReaderModule.h"
#include "qSlicerPinnacleDVFReaderPluginWidget.h"


//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPinnacleDVFReaderModule, qSlicerPinnacleDVFReaderModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class qSlicerPinnacleDVFReaderModulePrivate
{
public:
  qSlicerPinnacleDVFReaderModulePrivate();
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderModulePrivate::qSlicerPinnacleDVFReaderModulePrivate()
{
}


//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderModule::qSlicerPinnacleDVFReaderModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPinnacleDVFReaderModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderModule::~qSlicerPinnacleDVFReaderModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPinnacleDVFReaderModule::helpText()const
{
  QString help = QString(
    "The PinnacleDVFReader module enables importing and loading .DVF files into Slicer.<br>"
    "The PinnacleDVFReader module is hidden and therefore does not require an application.<br>");
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerPinnacleDVFReaderModule::acknowledgementText()const
{
  QString acknowledgement = QString(
    "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).");
  return acknowledgement;
}

//-----------------------------------------------------------------------------
QStringList qSlicerPinnacleDVFReaderModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Center)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerPinnacleDVFReaderModule::categories()const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDVFReaderModule::setup()
{
  this->Superclass::setup();
  
  vtkSlicerPinnacleDVFReaderLogic* PinnacleDVFReaderLogic =  
    vtkSlicerPinnacleDVFReaderLogic::SafeDownCast(this->logic());

  // Adds the module to the IO Manager
  qSlicerCoreIOManager* ioManager =
    qSlicerCoreApplication::application()->coreIOManager();
  ioManager->registerIO(new qSlicerPinnacleDVFReaderPlugin(PinnacleDVFReaderLogic,this));
  ioManager->registerIO(new qSlicerNodeWriter(
    "Dvf", QString("Pinnacle DVF"),
    QStringList() << "vtkMRMLGridTransformNode", this));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPinnacleDVFReaderModule::createWidgetRepresentation()
{
  return new qSlicerPinnacleDVFReaderPluginWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPinnacleDVFReaderModule::createLogic()
{
  return vtkSlicerPinnacleDVFReaderLogic::New();
}

