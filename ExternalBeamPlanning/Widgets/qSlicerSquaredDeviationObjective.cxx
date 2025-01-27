


// Objective includes
#include "qSlicerSquaredDeviationObjective.h"
#include "qSlicerAbstractObjective.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"

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

//

//---------------------------------------------------------------------------
float qSlicerSquaredDeviationObjective::computeDoseObjectiveFunction(const DoseType& doseMatrix)
{
	// Get the preferred dose from the objectives
	QMap<QString, QVariant> parameters = this->getObjectiveParameters();
	float preferredDose = parameters["preferredDose"].toFloat();
	DoseType preferredDoseVector = DoseType::Constant(doseMatrix.size(), preferredDose);
			
		
	// Compute the squared deviation
	DoseType deviation = doseMatrix - preferredDoseVector;
	float squaredDeviation = deviation.squaredNorm() / doseMatrix.size();

	return squaredDeviation;
}

//---------------------------------------------------------------------------
qSlicerAbstractObjective::DoseType& qSlicerSquaredDeviationObjective::computeDoseObjectiveGradient(const DoseType& doseMatrix)
{
	// Get the preferred dose from the objectives
	QMap<QString, QVariant> parameters = this->getObjectiveParameters();
	float preferredDose = parameters["preferredDose"].toFloat();
	DoseType preferredDoseVector = DoseType::Constant(doseMatrix.size(), preferredDose);

	// Compute gradient
	DoseType deviation = doseMatrix - preferredDoseVector;
	DoseType gradient = 2 * deviation / doseMatrix.size();

	return gradient;
}