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

// PlastimatchPy Logic includes
#include "vtkSlicerPlastimatchPyModuleLogic.h"

// PlastimatchPy includes
#include "qSlicerPlastimatchPyModule.h"
#include "qSlicerPlastimatchPyModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPlastimatchPyModule, qSlicerPlastimatchPyModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_PlastimatchPy
class qSlicerPlastimatchPyModulePrivate
{
public:
  qSlicerPlastimatchPyModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlastimatchPyModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlastimatchPyModulePrivate
::qSlicerPlastimatchPyModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlastimatchPyModule methods

//-----------------------------------------------------------------------------
qSlicerPlastimatchPyModule
::qSlicerPlastimatchPyModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlastimatchPyModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlastimatchPyModule::~qSlicerPlastimatchPyModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPlastimatchPyModule::helpText()const
{
  return "This module facilitates python access of numerous Plastimatch functions.";
}

//-----------------------------------------------------------------------------
QString qSlicerPlastimatchPyModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlastimatchPyModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Paolo Zaffino (University 'Magna Graecia' of Catanzaro)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlastimatchPyModule::categories() const
{
  return QStringList() << "Plastimatch.Utilities";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlastimatchPyModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerPlastimatchPyModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerPlastimatchPyModule
::createWidgetRepresentation()
{
  return new qSlicerPlastimatchPyModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlastimatchPyModule::createLogic()
{
  return vtkSlicerPlastimatchPyModuleLogic::New();
  //return plmpyDicomSroExport::New();
}
