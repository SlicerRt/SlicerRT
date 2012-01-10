/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes
#include <QtPlugin>

// ExtensionTemplate Logic includes
#include <vtkSlicerDicomRtImportLogic.h>

// ExtensionTemplate includes
#include "qSlicerDicomRtImportModule.h"
#include "qSlicerDicomRtImportModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDicomRtImportModule, qSlicerDicomRtImportModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerDicomRtImportModulePrivate
{
public:
  qSlicerDicomRtImportModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModulePrivate::qSlicerDicomRtImportModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDicomRtImportModule methods

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModule::qSlicerDicomRtImportModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDicomRtImportModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDicomRtImportModule::~qSlicerDicomRtImportModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportModule::helpText()const
{
  return "This DicomRtImport module illustrates how a loadable module should "
      "be implemented.";
}

//-----------------------------------------------------------------------------
QString qSlicerDicomRtImportModule::acknowledgementText()const
{
  return "This work was supported by ...";
}

//-----------------------------------------------------------------------------
QIcon qSlicerDicomRtImportModule::icon()const
{
  return QIcon(":/Icons/DicomRtImport.png");
}

//-----------------------------------------------------------------------------
void qSlicerDicomRtImportModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDicomRtImportModule::createWidgetRepresentation()
{
  return new qSlicerDicomRtImportModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDicomRtImportModule::createLogic()
{
  return vtkSlicerDicomRtImportLogic::New();
}
