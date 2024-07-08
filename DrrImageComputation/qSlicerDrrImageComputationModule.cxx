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

// SlicerRT DrrImageComputation Logic includes
#include <vtkSlicerDrrImageComputationLogic.h>

// SlicerRT DrrImageComputation includes
#include "qSlicerDrrImageComputationModule.h"
#include "qSlicerDrrImageComputationModuleWidget.h"

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// SlicerRT includes
#include <vtkSlicerPlanarImageModuleLogic.h>
#include <vtkSlicerBeamsModuleLogic.h>

// Slicer includes
#include <vtkSlicerCLIModuleLogic.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DrrImageComputation
class qSlicerDrrImageComputationModulePrivate
{
public:
  qSlicerDrrImageComputationModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModulePrivate::qSlicerDrrImageComputationModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDrrImageComputationModule methods

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModule::qSlicerDrrImageComputationModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDrrImageComputationModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDrrImageComputationModule::~qSlicerDrrImageComputationModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDrrImageComputationModule::helpText() const
{
  return "This is a loadable module that calculates a digitally " \
  "reconstructed radiograph (DRR) using the plastimatch reconstruct library.";
}

//-----------------------------------------------------------------------------
QString qSlicerDrrImageComputationModule::acknowledgementText() const
{
  return "This work was supported by Slicer Community.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDrrImageComputationModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Mikhail Polkovnikov (NRC \"Kurchatov Institute\" - IHEP)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDrrImageComputationModule::icon() const
{
  return QIcon(":/Icons/DrrImageComputation.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDrrImageComputationModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDrrImageComputationModule::dependencies() const
{
  return QStringList() << "Beams" << "PlanarImage" << "plastimatch_slicer_drr";
}

//-----------------------------------------------------------------------------
void qSlicerDrrImageComputationModule::setup()
{
  this->Superclass::setup();

  vtkSlicerDrrImageComputationLogic* drrImageComputationLogic = vtkSlicerDrrImageComputationLogic::SafeDownCast(this->logic());

  // Set plastimatch DRR computation logic to the logic
  qSlicerAbstractCoreModule* plastimatchDrrModule = qSlicerCoreApplication::application()->moduleManager()->module("plastimatch_slicer_drr");
  if (plastimatchDrrModule && drrImageComputationLogic)
  {
    vtkSlicerCLIModuleLogic* plastimatchDrrLogic = vtkSlicerCLIModuleLogic::SafeDownCast(plastimatchDrrModule->logic());
    drrImageComputationLogic->SetDRRComputationLogic(plastimatchDrrLogic);
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": Plastimatch DRR module is not found";
  }
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerDrrImageComputationModule
::createWidgetRepresentation()
{
  return new qSlicerDrrImageComputationModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDrrImageComputationModule::createLogic()
{
  return vtkSlicerDrrImageComputationLogic::New();
}
