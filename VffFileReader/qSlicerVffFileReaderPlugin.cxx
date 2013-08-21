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

// SlicerQt includes
#include "qSlicerVffFileReaderPlugin.h"
#include "qSlicerVffFileReaderOptionsWidget.h"

// Logic includes
#include "vtkSlicerVffFileReaderLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

//-----------------------------------------------------------------------------
class qSlicerVffFileReaderPluginPrivate
{
  public:
  vtkSmartPointer<vtkSlicerVffFileReaderLogic> Logic;
};

//-----------------------------------------------------------------------------
qSlicerVffFileReaderPlugin::qSlicerVffFileReaderPlugin(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerVffFileReaderPluginPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderPlugin::qSlicerVffFileReaderPlugin(vtkSlicerVffFileReaderLogic* logic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerVffFileReaderPluginPrivate)
{
  this->setLogic(logic);
}

//-----------------------------------------------------------------------------
qSlicerVffFileReaderPlugin::~qSlicerVffFileReaderPlugin()
{
}

//-----------------------------------------------------------------------------
void qSlicerVffFileReaderPlugin::setLogic(vtkSlicerVffFileReaderLogic* logic)
{
  Q_D(qSlicerVffFileReaderPlugin);
  d->Logic = logic;
}

//-----------------------------------------------------------------------------
vtkSlicerVffFileReaderLogic* qSlicerVffFileReaderPlugin::logic()const
{
  Q_D(const qSlicerVffFileReaderPlugin);
  return d->Logic.GetPointer();
}

//-----------------------------------------------------------------------------
QString qSlicerVffFileReaderPlugin::description()const
{
  return "Vff";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerVffFileReaderPlugin::fileType()const
{
  return QString("VffFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerVffFileReaderPlugin::extensions()const
{
  return QStringList()
	<< "Vff (*.vff)";
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerVffFileReaderPlugin::options()const
{
  return new qSlicerVffFileReaderOptionsWidget;
}

//-----------------------------------------------------------------------------
bool qSlicerVffFileReaderPlugin::load(const IOProperties& properties) 
{
  Q_D(qSlicerVffFileReaderPlugin);
  
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();
  Q_ASSERT(d->Logic);

  bool useImageIntensityScaleAndOffsetFromFile = properties["imageIntensityScaleAndOffset"].toBool();
  d->Logic->LoadVffFile(fileName.toLatin1().data(), useImageIntensityScaleAndOffsetFromFile);

  this->setLoadedNodes(QStringList());

  return true;
}
