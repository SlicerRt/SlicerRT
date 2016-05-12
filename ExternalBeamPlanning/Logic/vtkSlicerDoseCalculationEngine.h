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

// .NAME vtkSlicerDoseCalculationEngine - 
// .SECTION Description
// This class represents the dose calculation algorithm that can be used in SlicerRT modules
// The class can implement a dose calculation algorithm on its own or uses implementation from 
// a toolkit/library.

#ifndef __vtkSlicerDoseCalculationEngine_h
#define __vtkSlicerDoseCalculationEngine_h

#include "plm_image.h"
#include "plm_image_header.h"

// ExternalBeamPlanning includes
#include "vtkMRMLRTProtonBeamNode.h"
#include "vtkSlicerExternalBeamPlanningModuleLogicExport.h"

// VTK includes
#include "vtkObject.h"

class vtkImageData;

/// \ingroup SlicerRt_ExternalBeamPlanning
class VTK_SLICER_EXTERNALBEAMPLANNING_MODULE_LOGIC_EXPORT vtkSlicerDoseCalculationEngine : public vtkObject
{
public:
  static vtkSlicerDoseCalculationEngine *New();
  vtkTypeMacro(vtkSlicerDoseCalculationEngine, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

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

protected:
  vtkSlicerDoseCalculationEngine();
  virtual ~vtkSlicerDoseCalculationEngine();

private:
  vtkSlicerDoseCalculationEngine(const vtkSlicerDoseCalculationEngine&); // Not implemented
  void operator=(const vtkSlicerDoseCalculationEngine&);         // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
