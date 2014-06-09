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

  This file was originally developed by Adam Rankin, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Qt includes
#include <QFileInfo>

// Module includes
#include "qSlicerContoursReader.h"

// Logic includes
#include "vtkSlicerContoursModuleLogic.h"

// MRML includes
#include "vtkMRMLContourNode.h"
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
class qSlicerContoursReaderPrivate
{
public:
  vtkSmartPointer<vtkSlicerContoursModuleLogic> ContoursLogic;
};

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Contours
qSlicerContoursReader::qSlicerContoursReader(vtkSlicerContoursModuleLogic* contoursLogic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerContoursReaderPrivate)
{
  this->setContoursLogic(contoursLogic);
}

//-----------------------------------------------------------------------------
qSlicerContoursReader::~qSlicerContoursReader()
{
}

//-----------------------------------------------------------------------------
void qSlicerContoursReader::setContoursLogic(vtkSlicerContoursModuleLogic* newContoursLogic)
{
  Q_D(qSlicerContoursReader);
  d->ContoursLogic = newContoursLogic;
}

//-----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic* qSlicerContoursReader::contoursLogic()const
{
  Q_D(const qSlicerContoursReader);
  return d->ContoursLogic;
}

//-----------------------------------------------------------------------------
QString qSlicerContoursReader::description()const
{
  return "Contour";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerContoursReader::fileType()const
{
  return QString("ContourFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerContoursReader::extensions()const
{
  return QStringList()
    << "Contour (*.ctr)";
}

//-----------------------------------------------------------------------------
bool qSlicerContoursReader::load(const IOProperties& properties)
{
  Q_D(qSlicerContoursReader);
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();

  this->setLoadedNodes(QStringList());
  if (d->ContoursLogic.GetPointer() == 0)
  {
    return false;
  }
  vtkMRMLContourNode* node = d->ContoursLogic->LoadContourFromFile( fileName.toLatin1() );
  if (!node)
  {
    return false;
  }
  this->setLoadedNodes( QStringList(QString(node->GetID())) );
  if (properties.contains("name"))
  {
    std::string uname = this->mrmlScene()->GetUniqueNameByString(
      properties["name"].toString().toLatin1());
    node->SetName(uname.c_str());
  }
  return true;
}
