/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Qt includes
#include <QtPlugin>

// ExtensionTemplate Logic includes
#include <vtkSlicerSegmentMorphologyModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerSegmentMorphologyModule.h"
#include "qSlicerSegmentMorphologyModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSegmentMorphologyModule, qSlicerSegmentMorphologyModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_SegmentMorphology
class qSlicerSegmentMorphologyModulePrivate
{
public:
  qSlicerSegmentMorphologyModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSegmentMorphologyModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModulePrivate::qSlicerSegmentMorphologyModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSegmentMorphologyModule methods

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModule::qSlicerSegmentMorphologyModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSegmentMorphologyModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentMorphologyModule::~qSlicerSegmentMorphologyModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentMorphologyModule::helpText()const
{
  QString help = 
    "The Segment morphology module performs simualtion study of patient motion. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/SegmentMorphology\">%1/Documentation/%2.%3/Modules/SegmentMorphology</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentMorphologyModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentMorphologyModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (UHN, Toronto)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSegmentMorphologyModule::icon()const
{
  return this->Superclass::icon();
  //return QIcon(":/Icons/SegmentMorphology.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentMorphologyModule::categories() const
{
  return QStringList() << "Radiotherapy";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentMorphologyModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentMorphologyModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSegmentMorphologyModule::createWidgetRepresentation()
{
  return new qSlicerSegmentMorphologyModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSegmentMorphologyModule::createLogic()
{
  return vtkSlicerSegmentMorphologyModuleLogic::New();
}
