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

// SlicerQt includes
#include "qSlicerPinnacleDVFReaderPlugin.h"
#include "qSlicerPinnacleDVFReaderOptionsWidget.h"

// Logic includes
#include "vtkSlicerPinnacleDVFReaderLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PinnacleDVFReader
class qSlicerPinnacleDVFReaderPluginPrivate
{
  public:
  vtkSmartPointer<vtkSlicerPinnacleDVFReaderLogic> Logic;
};

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderPlugin::qSlicerPinnacleDVFReaderPlugin(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPinnacleDVFReaderPluginPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderPlugin::qSlicerPinnacleDVFReaderPlugin(vtkSlicerPinnacleDVFReaderLogic* logic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPinnacleDVFReaderPluginPrivate)
{
  this->setLogic(logic);
}

//-----------------------------------------------------------------------------
qSlicerPinnacleDVFReaderPlugin::~qSlicerPinnacleDVFReaderPlugin()
{
}

//-----------------------------------------------------------------------------
void qSlicerPinnacleDVFReaderPlugin::setLogic(vtkSlicerPinnacleDVFReaderLogic* logic)
{
  Q_D(qSlicerPinnacleDVFReaderPlugin);
  d->Logic = logic;
}

//-----------------------------------------------------------------------------
vtkSlicerPinnacleDVFReaderLogic* qSlicerPinnacleDVFReaderPlugin::logic()const
{
  Q_D(const qSlicerPinnacleDVFReaderPlugin);
  return d->Logic.GetPointer();
}

//-----------------------------------------------------------------------------
QString qSlicerPinnacleDVFReaderPlugin::description()const
{
  return "Dvf";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerPinnacleDVFReaderPlugin::fileType()const
{
  return QString("Pinnacle DVF");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPinnacleDVFReaderPlugin::extensions()const
{
  return QStringList() << "Dvf (*.dvf)";
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerPinnacleDVFReaderPlugin::options()const
{
  return new qSlicerPinnacleDVFReaderOptionsWidget;
}

//-----------------------------------------------------------------------------
bool qSlicerPinnacleDVFReaderPlugin::load(const IOProperties& properties) 
{
  Q_D(qSlicerPinnacleDVFReaderPlugin);
  
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();
  Q_ASSERT(d->Logic);

  double gridOriginX = properties["gridOriginX"].toDouble();
  double gridOriginY = properties["gridOriginY"].toDouble();
  double gridOriginZ = properties["gridOriginZ"].toDouble();
  d->Logic->LoadPinnacleDVF(fileName.toLatin1().data(), gridOriginX, gridOriginY, gridOriginZ);

  this->setLoadedNodes(QStringList());

  return true;
}
