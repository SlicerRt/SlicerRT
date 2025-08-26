/*==============================================================================

		Copyright (c) German Cancer Research Center (DKFZ),
		Heidelberg, Germany. All Rights Reserved.

		See COPYRIGHT.txt
		or http://www.slicer.org/copyright/copyright.txt for details.

		Unless required by applicable law or agreed to in writing, software
		distributed under the License is distributed on an "AS IS" BASIS,
		WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
		See the License for the specific language governing permissions and
		limitations under the License.

		This file was originally developed by Lina Bucher, German Cancer
		Research Center (DKFZ) and Institute for Biomedical Engineering (IBT),
		Karlsruhe Institute of Technology (KIT).

==============================================================================*/

// ExternalBeamPlanning includes
#include "qSlicerSquaredDeviationObjective.h"
#include "qSlicerAbstractObjective.h"

//----------------------------------------------------------------------------
qSlicerSquaredDeviationObjective::qSlicerSquaredDeviationObjective(QObject* parent)
		: qSlicerAbstractObjective(parent)
{
  this->m_Name = QString("Squared Deviation");
		this->initializeParameters();
}

//----------------------------------------------------------------------------
qSlicerSquaredDeviationObjective::~qSlicerSquaredDeviationObjective() = default;

//----------------------------------------------------------------------------
void qSlicerSquaredDeviationObjective::initializeParameters()
{
		this->objectivesParameters["preferredDose"] = 0.0;
}

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
qSlicerAbstractObjective::DoseType qSlicerSquaredDeviationObjective::computeDoseObjectiveGradient(const DoseType& doseMatrix)
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
