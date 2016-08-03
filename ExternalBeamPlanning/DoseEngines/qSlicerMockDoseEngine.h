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

#ifndef __qSlicerMockDoseEngine_h
#define __qSlicerMockDoseEngine_h

#include "qSlicerExternalBeamPlanningDoseEnginesExport.h"

// ExternalBeamPlanning includes
#include "qSlicerAbstractDoseEngine.h"

/// \ingroup SlicerRt_QtModules_ExternalBeamPlanning
/// \class qSlicerMockDoseEngine
/// \brief Mock dose calculation algorithm. Simply fills the beam apertures with prescription dose adding some noise.
///        Used for testing.
class Q_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT qSlicerMockDoseEngine : public qSlicerAbstractDoseEngine
{
  Q_OBJECT

public:
  typedef qSlicerAbstractDoseEngine Superclass;
  /// Constructor
  explicit qSlicerMockDoseEngine(QObject* parent=NULL);
  /// Destructor
  virtual ~qSlicerMockDoseEngine();

public:
  /// Calculate dose for a single beam. Called by \sa CalculateDose that performs actions generic
  /// to any dose engine before and after calculation.
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  Q_INVOKABLE QString calculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode);

  /// Define engine-specific beam parameters
  void defineBeamParameters();

private:
  Q_DISABLE_COPY(qSlicerMockDoseEngine);
};

#endif
