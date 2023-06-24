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

// Slicer includes
#include <qSlicerCoreApplication.h>

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyGammaPlugin.h"

// DoseComparison Logic includes
#include <vtkSlicerDoseComparisonModuleLogic.h>

// DoseComparison includes
#include "qSlicerDoseComparisonModule.h"
#include "qSlicerDoseComparisonModuleWidget.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerDoseComparisonModule, qSlicerDoseComparisonModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_DoseComparison
class qSlicerDoseComparisonModulePrivate
{
public:
  qSlicerDoseComparisonModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModulePrivate::qSlicerDoseComparisonModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerDoseComparisonModule methods

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModule::qSlicerDoseComparisonModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerDoseComparisonModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDoseComparisonModule::~qSlicerDoseComparisonModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerDoseComparisonModule::helpText()const
{
  return QString("This module computes and displays dose comparison metrics. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/DoseComparison\">%1/Documentation/%2.%3/Modules/DoseComparison</a><br>").arg(
    qSlicerCoreApplication::application()->majorVersion()).arg(qSlicerCoreApplication::application()->minorVersion());
}

//-----------------------------------------------------------------------------
QString qSlicerDoseComparisonModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseComparisonModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDoseComparisonModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerDoseComparisonModule::icon()const
{
  return QIcon(":/Icons/DoseComparison.png");
}

//-----------------------------------------------------------------------------
void qSlicerDoseComparisonModule::setup()
{
  this->Superclass::setup();

  // Register Subject Hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyGammaPlugin());
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDoseComparisonModule::createWidgetRepresentation()
{
  return new qSlicerDoseComparisonModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDoseComparisonModule::createLogic()
{
  return vtkSlicerDoseComparisonModuleLogic::New();
}
