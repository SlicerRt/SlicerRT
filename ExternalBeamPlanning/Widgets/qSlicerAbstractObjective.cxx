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

This file was originally developed by Niklas Wahl, German Cancer Research Center (DKFZ)

==============================================================================*/

// Optimization engines includes
#include "qSlicerAbstractObjective.h"


// SlicerRT includes
#include "vtkSlicerRtCommon.h"
#include "vtkSlicerIsodoseModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLColorTableNode.h>
#include "vtkMRMLObjectiveNode.h"

// VTK includes
#include <vtkSmartPointer.h>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractModuleWidget.h"

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
class qSlicerAbstractObjectivePrivate
{
  Q_DECLARE_PUBLIC(qSlicerAbstractObjective);
protected:
  qSlicerAbstractObjective* const q_ptr;
public:
  qSlicerAbstractObjectivePrivate(qSlicerAbstractObjective& object);
};

//-----------------------------------------------------------------------------
// qSlicerAbstractObjectivePrivate methods

//-----------------------------------------------------------------------------
qSlicerAbstractObjectivePrivate::qSlicerAbstractObjectivePrivate(qSlicerAbstractObjective& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
// qSlicerAbstractObjective methods

//----------------------------------------------------------------------------
qSlicerAbstractObjective::qSlicerAbstractObjective(QObject* parent)
  : Superclass(parent)
  , m_Name(QString())
  , d_ptr( new qSlicerAbstractObjectivePrivate(*this) )
{
}

//----------------------------------------------------------------------------
qSlicerAbstractObjective::~qSlicerAbstractObjective() = default;

//----------------------------------------------------------------------------
QString qSlicerAbstractObjective::name() const
{
  if (m_Name.isEmpty())
  {
    qCritical() << Q_FUNC_INFO << ": Empty Optimization engine name";
  }
  return this->m_Name;
}

//-----------------------------------------------------------------------------
void qSlicerAbstractObjective::setName(QString name)
{
  Q_UNUSED(name);
  qCritical() << Q_FUNC_INFO << ": Cannot set Optimization engine name by method, only in constructor";
}

//----------------------------------------------------------------------------
qSlicerAbstractObjective::ObjectiveFunctionAndGradient qSlicerAbstractObjective::computeDoseObjectiveFunctionAndGradient()
{
	qSlicerAbstractObjective::ObjectiveFunctionAndGradient functions;
	functions.objectiveFunction = [this](const DoseType& dose, const ObjectivesType& objectives) -> float
	{
		return this->computeDoseObjectiveFunction(dose, objectives);
	};
	functions.objectiveGradient = [this](const DoseType& dose, const ObjectivesType& objectives)->DoseType &
	{
		return this->computeDoseObjectiveGradient(dose, objectives);
	};
    return functions;
}
