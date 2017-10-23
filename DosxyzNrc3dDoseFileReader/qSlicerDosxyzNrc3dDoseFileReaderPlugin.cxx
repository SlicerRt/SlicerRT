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
#include "qSlicerDosxyzNrc3dDoseFileReaderPlugin.h"
#include "qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget.h"

// Logic includes
#include "vtkSlicerDosxyzNrc3dDoseFileReaderLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class qSlicerDosxyzNrc3dDoseFileReaderPluginPrivate
{
  public:
  vtkSmartPointer<vtkSlicerDosxyzNrc3dDoseFileReaderLogic> Logic;
};

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderPlugin::qSlicerDosxyzNrc3dDoseFileReaderPlugin(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDosxyzNrc3dDoseFileReaderPluginPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderPlugin::qSlicerDosxyzNrc3dDoseFileReaderPlugin(vtkSlicerDosxyzNrc3dDoseFileReaderLogic* logic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDosxyzNrc3dDoseFileReaderPluginPrivate)
{
  this->setLogic(logic);
}

//-----------------------------------------------------------------------------
qSlicerDosxyzNrc3dDoseFileReaderPlugin::~qSlicerDosxyzNrc3dDoseFileReaderPlugin()
{
}

//-----------------------------------------------------------------------------
void qSlicerDosxyzNrc3dDoseFileReaderPlugin::setLogic(vtkSlicerDosxyzNrc3dDoseFileReaderLogic* logic)
{
  Q_D(qSlicerDosxyzNrc3dDoseFileReaderPlugin);
  d->Logic = logic;
}

//-----------------------------------------------------------------------------
vtkSlicerDosxyzNrc3dDoseFileReaderLogic* qSlicerDosxyzNrc3dDoseFileReaderPlugin::logic()const
{
  Q_D(const qSlicerDosxyzNrc3dDoseFileReaderPlugin);
  return d->Logic.GetPointer();
}

//-----------------------------------------------------------------------------
QString qSlicerDosxyzNrc3dDoseFileReaderPlugin::description()const
{
  return "DosxyzNrc3dDose";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerDosxyzNrc3dDoseFileReaderPlugin::fileType()const
{
  return QString("DosxyzNrc3dDoseFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDosxyzNrc3dDoseFileReaderPlugin::extensions()const
{
  return QStringList() << "DosxyzNrc3dDose (*.3ddose)";
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerDosxyzNrc3dDoseFileReaderPlugin::options()const
{
  return new qSlicerDosxyzNrc3dDoseFileReaderOptionsWidget;
}

//-----------------------------------------------------------------------------
bool qSlicerDosxyzNrc3dDoseFileReaderPlugin::load(const IOProperties& properties) 
{
  Q_D(qSlicerDosxyzNrc3dDoseFileReaderPlugin);
  
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();
  Q_ASSERT(d->Logic);

  bool useImageIntensityScaleAndOffsetFromFile = properties["imageIntensityScaleAndOffset"].toBool();
  d->Logic->LoadDosxyzNrc3dDoseFile(fileName.toLatin1().data());

  this->setLoadedNodes(QStringList());

  return true;
}
