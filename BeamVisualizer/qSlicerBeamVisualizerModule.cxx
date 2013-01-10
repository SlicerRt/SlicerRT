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

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// BeamVisualizer Logic includes
#include <vtkSlicerBeamVisualizerModuleLogic.h>

// BeamVisualizer includes
#include "qSlicerBeamVisualizerModule.h"
#include "qSlicerBeamVisualizerModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerBeamVisualizerModule, qSlicerBeamVisualizerModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_BeamVisualizer
class qSlicerBeamVisualizerModulePrivate
{
public:
  qSlicerBeamVisualizerModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerBeamVisualizerModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModulePrivate::qSlicerBeamVisualizerModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerBeamVisualizerModule methods

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModule::qSlicerBeamVisualizerModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerBeamVisualizerModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerBeamVisualizerModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerBeamVisualizerModule::~qSlicerBeamVisualizerModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerBeamVisualizerModule::helpText()const
{
  QString help = 
    "This module displays and handles beam geometry modules created from the loaded isocenter and source fiducials. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/BeamVisualizer\">%1/Documentation/%2.%3/Modules/BeamVisualizer</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerBeamVisualizerModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerBeamVisualizerModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerBeamVisualizerModule::icon()const
{
  return QIcon(":/Icons/BeamVisualizer.png");
}

//-----------------------------------------------------------------------------
void qSlicerBeamVisualizerModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerBeamVisualizerModule::createWidgetRepresentation()
{
  return new qSlicerBeamVisualizerModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerBeamVisualizerModule::createLogic()
{
  return vtkSlicerBeamVisualizerModuleLogic::New();
}
