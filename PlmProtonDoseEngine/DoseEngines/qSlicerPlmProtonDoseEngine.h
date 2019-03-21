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

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre
  and was supported by Cancer Care Ontario (CCO)'s ACRU program
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

#ifndef __qSlicerPlmProtonDoseEngine_h
#define __qSlicerPlmProtonDoseEngine_h

#include "qSlicerPlmProtonDoseEngineDoseEnginesExport.h"

// ExternalBeamPlanning includes
#include "qSlicerAbstractDoseEngine.h"

/// \ingroup SlicerRt_PlmProtonDoseEngine
/// \brief Plastimatch proton dose calculation algorithm
class Q_SLICER_PLMPROTONDOSEENGINE_DOSE_ENGINES_EXPORT qSlicerPlmProtonDoseEngine : public qSlicerAbstractDoseEngine
{
  Q_OBJECT

public:
  typedef qSlicerAbstractDoseEngine Superclass;
  /// Constructor
  explicit qSlicerPlmProtonDoseEngine(QObject* parent=nullptr);
  /// Destructor
  ~qSlicerPlmProtonDoseEngine() override;

protected:
  /// Calculate dose for a single beam. Called by \sa CalculateDose that performs actions generic
  /// to any dose engine before and after calculation.
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  QString calculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode) override;

  /// Define engine-specific beam parameters
  void defineBeamParameters();

private:
  Q_DISABLE_COPY(qSlicerPlmProtonDoseEngine);
};

#endif
