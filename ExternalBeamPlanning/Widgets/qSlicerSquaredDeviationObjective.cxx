


// Optimizer includes
#include "qSlicerSquaredDeviationObjective.h"

#include "qSlicerAbstractObjective.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
//#include <vtkMRMLObjectiveNode.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>

// Slicer includes
#include <vtkSlicerVersionConfigure.h>

// Qt includes
#include <QDebug>


//----------------------------------------------------------------------------
qSlicerSquaredDeviationObjective::qSlicerSquaredDeviationObjective(QObject* parent)
    : qSlicerAbstractObjective(parent)
{
    this->m_Name = QString("Squared Deviation");
}

//----------------------------------------------------------------------------
qSlicerSquaredDeviationObjective::~qSlicerSquaredDeviationObjective() = default;

//---------------------------------------------------------------------------
std::function<QString(const qSlicerAbstractObjective::DoseType&, const qSlicerAbstractObjective::ObjectivesType&)> qSlicerSquaredDeviationObjective::computeDoseObjectiveFunction()
{
	return [this](const DoseType& doseMatrix, const ObjectivesType& objectives) -> QString
	{
		return QString("Squared Deviation Objective Function");
	};
}

//---------------------------------------------------------------------------
std::function<QString(const qSlicerAbstractObjective::DoseType&, const qSlicerAbstractObjective::ObjectivesType&)> qSlicerSquaredDeviationObjective::computeDoseObjectiveGradient()
{
	return [](const DoseType& doseMatrix, const ObjectivesType& objectives) -> QString
	{
		return QString("Squared Deviation Objective Gradient");
	};
}