/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Paolo Zaffino, Universita' degli Studi
  "Magna Graecia" di Catanzaro and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Natural Sciences and Engineering Research Council of Canada.

==========================================================================*/

// PlastimatchPy Logic includes
//#include "vtkSlicerPlastimatchPyModuleLogic.h"

// PlastimatchPy includes
#include "qSlicerPlastimatchPyModule.h"
#include "qSlicerPlastimatchPyModuleWidget.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerPlastimatchPyModule, qSlicerPlastimatchPyModule);
#endif

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
qSlicerPlastimatchPyModulePrivate::qSlicerPlastimatchPyModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerPlastimatchPyModule methods

//-----------------------------------------------------------------------------
qSlicerPlastimatchPyModule::qSlicerPlastimatchPyModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlastimatchPyModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlastimatchPyModule::~qSlicerPlastimatchPyModule() = default;

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
  moduleContributors << QString("Csaba Pinter (Queen's University)");
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
  return 0;
  //return vtkSlicerPlastimatchPyModuleLogic::New();
  //return plmpyDicomSroExport::New();
}
