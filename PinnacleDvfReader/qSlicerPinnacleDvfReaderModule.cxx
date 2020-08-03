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
#include <QtPlugin>

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerIOManager.h>
#include <qSlicerNodeWriter.h>

// PinnacleDvfReader Logic includes
#include <vtkSlicerPinnacleDvfReaderLogic.h>

// PinnacleDvfReader QTModule includes
#include "qSlicerPinnacleDvfReaderPlugin.h"
#include "qSlicerPinnacleDvfReaderModule.h"
#include "qSlicerPinnacleDvfReaderPluginWidget.h"


//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerPinnacleDvfReaderModule, qSlicerPinnacleDvfReaderModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class qSlicerPinnacleDvfReaderModulePrivate
{
public:
  qSlicerPinnacleDvfReaderModulePrivate();
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderModulePrivate::qSlicerPinnacleDvfReaderModulePrivate() = default;


//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderModule::qSlicerPinnacleDvfReaderModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPinnacleDvfReaderModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderModule::~qSlicerPinnacleDvfReaderModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerPinnacleDvfReaderModule::helpText()const
{
  QString help = QString(
    "The PinnacleDvfReader module enables importing and loading .DVF files into Slicer.<br>"
    "The PinnacleDvfReader module is hidden and therefore does not require an application.<br>");
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerPinnacleDvfReaderModule::acknowledgementText()const
{
  QString acknowledgement = QString(
    "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).");
  return acknowledgement;
}

//-----------------------------------------------------------------------------
QStringList qSlicerPinnacleDvfReaderModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (Princess Margaret Cancer Center)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerPinnacleDvfReaderModule::categories()const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDvfReaderModule::setup()
{
  this->Superclass::setup();
  
  vtkSlicerPinnacleDvfReaderLogic* PinnacleDvfReaderLogic =  
    vtkSlicerPinnacleDvfReaderLogic::SafeDownCast(this->logic());

  // Adds the module to the IO Manager
  qSlicerCoreIOManager* ioManager =
    qSlicerCoreApplication::application()->coreIOManager();
  ioManager->registerIO(new qSlicerPinnacleDvfReaderPlugin(PinnacleDvfReaderLogic,this));
  ioManager->registerIO(new qSlicerNodeWriter(
    "Dvf", QString("Pinnacle DVF"),
    QStringList() << "vtkMRMLGridTransformNode", true, this));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPinnacleDvfReaderModule::createWidgetRepresentation()
{
  return new qSlicerPinnacleDvfReaderPluginWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPinnacleDvfReaderModule::createLogic()
{
  return vtkSlicerPinnacleDvfReaderLogic::New();
}

