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

// .NAME vtkSlicerMockDoseEngine - 
// .SECTION Description
// This class represents the Plastimatch proton dose calculation algorithm

#ifndef __vtkSlicerMockDoseEngine_h
#define __vtkSlicerMockDoseEngine_h

#include "vtkSlicerExternalBeamPlanningDoseEnginesExport.h"

// ExternalBeamPlanning includes
#include "vtkSlicerAbstractDoseEngine.h"

/// \ingroup SlicerRt_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT vtkSlicerMockDoseEngine : public vtkSlicerAbstractDoseEngine
{
public:
  static vtkSlicerMockDoseEngine *New();
  vtkTypeMacro(vtkSlicerMockDoseEngine, vtkSlicerAbstractDoseEngine);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create beam node of type the dose engine uses.
  /// Dose engines need to override this to return beam node of type they use.
  /// Note: Need to take ownership of the created object! For example using vtkSmartPointer<vtkDataObject>::Take
  virtual vtkMRMLRTBeamNode* CreateBeamForEngine();

protected:
  /// Calculate dose for a single beam. Called by \sa CalculateDose that performs actions generic
  /// to any dose engine before and after calculation.
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  virtual std::string CalculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode);

protected:
  vtkSlicerMockDoseEngine();
  virtual ~vtkSlicerMockDoseEngine();

private:
  vtkSlicerMockDoseEngine(const vtkSlicerMockDoseEngine&); // Not implemented
  void operator=(const vtkSlicerMockDoseEngine&);         // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
