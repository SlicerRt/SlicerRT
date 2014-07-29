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

// SlicerQt includes
#include "qSlicerPinnacleDvfReaderPlugin.h"
#include "qSlicerPinnacleDvfReaderOptionsWidget.h"

// Logic includes
#include "vtkSlicerPinnacleDvfReaderLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDvfReader
class qSlicerPinnacleDvfReaderPluginPrivate
{
  public:
  vtkSmartPointer<vtkSlicerPinnacleDvfReaderLogic> Logic;
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderPlugin::qSlicerPinnacleDvfReaderPlugin(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPinnacleDvfReaderPluginPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderPlugin::qSlicerPinnacleDvfReaderPlugin(vtkSlicerPinnacleDvfReaderLogic* logic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPinnacleDvfReaderPluginPrivate)
{
  this->setLogic(logic);
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDvfReaderPlugin::~qSlicerPinnacleDvfReaderPlugin()
{
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDvfReaderPlugin::setLogic(vtkSlicerPinnacleDvfReaderLogic* logic)
{
  Q_D(qSlicerPinnacleDvfReaderPlugin);
  d->Logic = logic;
}

//-----------------------------------------------------------------------------
vtkSlicerPinnacleDvfReaderLogic* qSlicerPinnacleDvfReaderPlugin::logic()const
{
  Q_D(const qSlicerPinnacleDvfReaderPlugin);
  return d->Logic.GetPointer();
}

//-----------------------------------------------------------------------------
QString qSlicerPinnacleDvfReaderPlugin::description()const
{
  return "Dvf";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerPinnacleDvfReaderPlugin::fileType()const
{
  return QString("Pinnacle DVF");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPinnacleDvfReaderPlugin::extensions()const
{
  return QStringList() << "Dvf (*.dvf)";
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerPinnacleDvfReaderPlugin::options()const
{
  return new qSlicerPinnacleDvfReaderOptionsWidget;
}

//-----------------------------------------------------------------------------
bool qSlicerPinnacleDvfReaderPlugin::load(const IOProperties& properties) 
{
  Q_D(qSlicerPinnacleDvfReaderPlugin);
  
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();
  Q_ASSERT(d->Logic);

  double gridOriginX = properties["gridOriginX"].toDouble();
  double gridOriginY = properties["gridOriginY"].toDouble();
  double gridOriginZ = properties["gridOriginZ"].toDouble();
  d->Logic->LoadPinnacleDvf(fileName.toLatin1().data(), gridOriginX, gridOriginY, gridOriginZ);

  this->setLoadedNodes(QStringList());

  return true;
}
