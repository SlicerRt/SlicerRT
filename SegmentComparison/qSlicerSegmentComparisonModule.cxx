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

// Qt includes
#include <QtPlugin>

// SegmentComparison Logic includes
#include <vtkSlicerSegmentComparisonModuleLogic.h>

// SegmentComparison includes
#include "qSlicerSegmentComparisonModule.h"
#include "qSlicerSegmentComparisonModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSegmentComparisonModule, qSlicerSegmentComparisonModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_SegmentComparison
class qSlicerSegmentComparisonModulePrivate
{
public:
  qSlicerSegmentComparisonModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSegmentComparisonModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentComparisonModulePrivate::qSlicerSegmentComparisonModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSegmentComparisonModule methods

//-----------------------------------------------------------------------------
qSlicerSegmentComparisonModule::qSlicerSegmentComparisonModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSegmentComparisonModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentComparisonModule::~qSlicerSegmentComparisonModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentComparisonModule::helpText()const
{
  QString help = 
    "This module computes segment similarity metrics. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/SegmentComparison\">%1/Documentation/%2.%3/Modules/SegmentComparison</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentComparisonModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentComparisonModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentComparisonModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSegmentComparisonModule::icon()const
{
  return QIcon(":/Icons/SegmentComparison.png");
}

//-----------------------------------------------------------------------------
void qSlicerSegmentComparisonModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSegmentComparisonModule::createWidgetRepresentation()
{
  return new qSlicerSegmentComparisonModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSegmentComparisonModule::createLogic()
{
  return vtkSlicerSegmentComparisonModuleLogic::New();
}
