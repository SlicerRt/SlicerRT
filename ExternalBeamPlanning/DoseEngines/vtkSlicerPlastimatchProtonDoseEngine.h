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

// .NAME vtkSlicerPlastimatchProtonDoseEngine - 
// .SECTION Description
// This class represents the Plastimatch proton dose calculation algorithm

#ifndef __vtkSlicerPlastimatchProtonDoseEngine_h
#define __vtkSlicerPlastimatchProtonDoseEngine_h

#include "vtkSlicerExternalBeamPlanningDoseEnginesExport.h"

// ExternalBeamPlanning includes
#include "vtkSlicerAbstractDoseEngine.h"

// Beams includes
#include "vtkMRMLRTProtonBeamNode.h"

// Plastimatch includes
#include "plm_image.h"
#include "plm_image_header.h"

/// \ingroup SlicerRt_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_DOSE_ENGINES_EXPORT vtkSlicerPlastimatchProtonDoseEngine : public vtkSlicerAbstractDoseEngine
{
public:
  static vtkSlicerPlastimatchProtonDoseEngine *New();
  vtkTypeMacro(vtkSlicerPlastimatchProtonDoseEngine, vtkSlicerAbstractDoseEngine);
  void PrintSelf(ostream& os, vtkIndent indent);
  /*
  /// Initialize accumulated dose
  void InitializeAccumulatedDose(Plm_image::Pointer);

  /// Do dose calculation for a single beam
  void CalculateDose(
    vtkMRMLRTBeamNode* beamNode, 
    Plm_image::Pointer& plmTgt,
    double isocenter[],
    double src[], 
    double RxDose);

  /// Get range compensator volume
  itk::Image<float, 3>::Pointer GetRangeCompensatorVolume();

  /// Get aperture volume
  itk::Image<unsigned char, 3>::Pointer GetApertureVolume();

  /// Get computed dose
  itk::Image<float, 3>::Pointer GetComputedDose();

  /// Get accumulated dose
  itk::Image<float, 3>::Pointer GetAccumulatedDose();

  /// Do dose calculation
  void CleanUp();
  */

protected:
  /// Calculate dose for a single beam. Called by \sa CalculateDose that performs actions generic
  /// to any dose engine before and after calculation.
  /// \param beamNode Beam for which the dose is calculated. Each beam has a parent plan from which the
  ///   plan-specific parameters are got
  /// \param resultDoseVolumeNode Output volume node for the result dose. It is created by \sa CalculateDose
  virtual std::string CalculateDoseUsingEngine(vtkMRMLRTBeamNode* beamNode, vtkMRMLScalarVolumeNode* resultDoseVolumeNode);

protected:
  vtkSlicerPlastimatchProtonDoseEngine();
  virtual ~vtkSlicerPlastimatchProtonDoseEngine();

private:
  vtkSlicerPlastimatchProtonDoseEngine(const vtkSlicerPlastimatchProtonDoseEngine&); // Not implemented
  void operator=(const vtkSlicerPlastimatchProtonDoseEngine&);         // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
