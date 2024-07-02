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
#include <qSlicerModuleManager.h>

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyRTPlanPlugin.h"
#include "qSlicerSubjectHierarchyRTBeamPlugin.h"

// Beams Logic includes
#include <vtkSlicerBeamsModuleLogic.h>

// Beams includes
#include "qSlicerBeamsModule.h"
#include "qSlicerBeamsModuleWidget.h"

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerBeamsModule, qSlicerBeamsModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Beams
class qSlicerBeamsModulePrivate
{
public:
  qSlicerBeamsModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerBeamsModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerBeamsModulePrivate::qSlicerBeamsModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerBeamsModule methods

//-----------------------------------------------------------------------------
qSlicerBeamsModule::qSlicerBeamsModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerBeamsModulePrivate)
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerBeamsModule::categories()const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
qSlicerBeamsModule::~qSlicerBeamsModule() = default;

//-----------------------------------------------------------------------------
QStringList qSlicerBeamsModule::dependencies()const
{
  return QStringList() << "Models" << "Segmentations";
}

//-----------------------------------------------------------------------------
QString qSlicerBeamsModule::helpText()const
{
  return QString("This module displays and handles beam geometry models created from the loaded isocenter and source fiducials. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/Beams\">%1/Documentation/%2.%3/Modules/Beams</a><br>").arg(
    this->slicerWikiUrl()).arg(qSlicerCoreApplication::application()->majorVersion()).arg(qSlicerCoreApplication::application()->minorVersion());
}

//-----------------------------------------------------------------------------
QString qSlicerBeamsModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerBeamsModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Kevin Wang (Techna, UHN)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerBeamsModule::icon()const
{
  return QIcon(":/Icons/Beams.png");
}

//-----------------------------------------------------------------------------
void qSlicerBeamsModule::setup()
{
  this->Superclass::setup();

  // Register Subject Hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRTPlanPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchyRTBeamPlugin());
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerBeamsModule::createWidgetRepresentation()
{
  return new qSlicerBeamsModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerBeamsModule::createLogic()
{
  return vtkSlicerBeamsModuleLogic::New();
}

//-----------------------------------------------------------------------------
QStringList qSlicerBeamsModule::associatedNodeTypes() const
{
  return QStringList() << "vtkMRMLRTBeamNode";
}
