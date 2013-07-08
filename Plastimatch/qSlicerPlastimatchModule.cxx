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

// Plastimatch Logic includes
#include <vtkSlicerPlastimatchLogic.h>

// Plastimatch includes
#include "qSlicerPlastimatchModule.h"
#include "qSlicerPlastimatchModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerPlastimatchModule, qSlicerPlastimatchModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlastimatchModulePrivate
{
public:
  qSlicerPlastimatchModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlastimatchModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlastimatchModulePrivate
::qSlicerPlastimatchModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlastimatchModule methods

//-----------------------------------------------------------------------------
qSlicerPlastimatchModule
::qSlicerPlastimatchModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlastimatchModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlastimatchModule::~qSlicerPlastimatchModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPlastimatchModule::helpText()const
{
  return "This is a loadable module bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerPlastimatchModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlastimatchModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Jean-Christophe Fillion-Robin (Kitware)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerPlastimatchModule::icon()const
{
  return QIcon(":/Icons/Plastimatch.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlastimatchModule::categories() const
{
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlastimatchModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerPlastimatchModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerPlastimatchModule
::createWidgetRepresentation()
{
  return new qSlicerPlastimatchModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlastimatchModule::createLogic()
{
  return vtkSlicerPlastimatchLogic::New();
}
