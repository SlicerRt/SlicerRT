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
  return "This DoseVolumeHistogram module illustrates how a loadable module should "
      "be implemented.";
}

//-----------------------------------------------------------------------------
QString qSlicerDoseVolumeHistogramModule::acknowledgementText()const
{
  return "This work was supported by ...";
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
