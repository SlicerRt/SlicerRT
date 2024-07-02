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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDoseVolumeHistogramPlugin.h"

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>

// ExtensionTemplate Logic includes
#include <vtkSlicerDoseVolumeHistogramModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerDoseVolumeHistogramModule.h"
#include "qSlicerDoseVolumeHistogramModuleWidget.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerDoseVolumeHistogramModule, qSlicerDoseVolumeHistogramModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DoseVolumeHistogram
class qSlicerDoseVolumeHistogramModulePrivate
{
public:
  qSlicerDoseVolumeHistogramModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModulePrivate::qSlicerDoseVolumeHistogramModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerDoseVolumeHistogramModule methods

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModule::qSlicerDoseVolumeHistogramModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDoseVolumeHistogramModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDoseVolumeHistogramModule::~qSlicerDoseVolumeHistogramModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerDoseVolumeHistogramModule::helpText()const
{
  return QString("This module computes dose volume histogram (DVH) and metrics from a dose map and segmentation. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DoseVolumeHistogram\">%1/Documentation/%2.%3/Modules/DoseVolumeHistogram</a><br>").arg(
    this->slicerWikiUrl()).arg(qSlicerCoreApplication::application()->majorVersion()).arg(qSlicerCoreApplication::application()->minorVersion());
}

//-----------------------------------------------------------------------------
QString qSlicerDoseVolumeHistogramModule::acknowledgementText()const
{
  return "This work was funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO), and CANARIE.";
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

  // Register Subject Hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyDoseVolumeHistogramPlugin());
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
  return vtkSlicerDoseVolumeHistogramModuleLogic::New();
}
