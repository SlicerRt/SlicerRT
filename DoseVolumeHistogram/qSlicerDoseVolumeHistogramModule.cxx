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
//#include <QDebug>

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerDoseVolumeHistogramLogic.h>

// ExtensionTemplate includes
#include "qSlicerDoseVolumeHistogramModule.h"
#include "qSlicerDoseVolumeHistogramModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDoseVolumeHistogramModule, qSlicerDoseVolumeHistogramModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerDoseVolumeHistogramModulePrivate
{
public:
  qSlicerDoseVolumeHistogramModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModulePrivate::qSlicerDoseVolumeHistogramModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModule methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModule::qSlicerDoseVolumeHistogramModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDoseVolumeHistogramModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModule::~qSlicerDoseVolumeHistogramModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerDoseVolumeHistogramModule::helpText()const
{
  return "Compute dose volume histogram (DVH) from a dose map and structure set.";
}

//-----------------------------------------------------------------------------
QString qSlicerDoseVolumeHistogramModule::acknowledgementText()const
{
  return "This work was supported through the Applied Cancer Research Unit program of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care.";
}

//-----------------------------------------------------------------------------
QIcon qSlicerDoseVolumeHistogramModule::icon()const
{
  return QIcon(":/Icons/DoseVolumeHistogram.png");
}

//-----------------------------------------------------------------------------
void qSlicerDoseVolumeHistogramModule::setup()
{
  this->Superclass::setup();

  //qSlicerAbstractCoreModule* modelToLabelmapModule =
  //  qSlicerCoreApplication::application()->moduleManager()->module("ModelToLabelMap");
  //if (modelToLabelmapModule)
  //  {
  //  vtkSlicerModuleLogic* modelToLabelmapModuleLogic = 
  //    vtkSlicerModuleLogic::SafeDownCast(modelToLabelmapModule->logic());
  //  }
  //else
  //  {
  //  qWarning() << "ModelToLabelMap module is not found";
  //  }
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseVolumeHistogramModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseVolumeHistogramModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDoseVolumeHistogramModule::createWidgetRepresentation()
{
  return new qSlicerDoseVolumeHistogramModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDoseVolumeHistogramModule::createLogic()
{
  return vtkSlicerDoseVolumeHistogramLogic::New();
}
