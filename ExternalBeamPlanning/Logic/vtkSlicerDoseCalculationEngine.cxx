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


// Module includes
#include "vtkSlicerDoseCalculationEngine.h"

// Plastimatch includes
#include "itk_image_accumulate.h"
#include "itk_image_create.h"
#include "itk_image_save.h"
#include "itk_image_scale.h"
#include "itk_image_stats.h"
#include "plm_image.h"
#include "plm_image_header.h"
#include "rpl_volume.h"
#include "rt_beam.h"
#include "rt_plan.h"
#include "string_util.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
class vtkSlicerDoseCalculationEngine::vtkInternal
{
public:
  vtkInternal();

  /// Pointer that contains the dose
  Plm_image::Pointer plmRef;

  /// Pointer that contains the dose
  itk::Image<float, 3>::Pointer rcVolumeItk;

  /// Pointer that contains the dose
  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk;

  /// Pointer that contains the dose
  itk::Image<float, 3>::Pointer doseVolumeItk;

  /// Pointer that contains the accumulate dose
  itk::Image<float, 3>::Pointer accumulateVolumeItk;

  /// Pointer that contains the dose
  float TotalRx;
};

//----------------------------------------------------------------------------
vtkSlicerDoseCalculationEngine::vtkInternal::vtkInternal()
{
  this->TotalRx = 0.f;
}

vtkStandardNewMacro(vtkSlicerDoseCalculationEngine);

//----------------------------------------------------------------------------
vtkSlicerDoseCalculationEngine::vtkSlicerDoseCalculationEngine()
{
  this->Internal = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSlicerDoseCalculationEngine::~vtkSlicerDoseCalculationEngine()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::InitializeAccumulatedDose(Plm_image::Pointer plmRef)
{
  this->Internal->plmRef = plmRef;
  itk::Image<short, 3>::Pointer referenceVolumeItk = this->Internal->plmRef->itk_short();

  this->Internal->accumulateVolumeItk = itk_image_create<float>(Plm_image_header(referenceVolumeItk));
  this->Internal->TotalRx = 0.f;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::CalculateDose(
  vtkMRMLRTBeamNode* beamNode, 
  Plm_image::Pointer plmTgt,
  double isocenter[],
  double src[],
  double proximalMargin,
  double distalMargin,
  double beamRx)
{

  this->Internal->plmRef->print ();
  plmTgt->print ();

#if defined (commentout)
  /* This is debugging code, for checking the input volume */
  double min_val, max_val, avg;
  int non_zero, num_vox;
  itk_image_stats (Internal->plmRef->m_itk_int32, &min_val, &max_val, &avg, &non_zero, &num_vox);
  printf ("MIN %f AVE %f MAX %f NONZERO %d NUMVOX %d\n", 
    (float) min_val, (float) avg, (float) max_val, non_zero, num_vox);
#endif

  itk::Image<short, 3>::Pointer referenceVolumeItk = this->Internal->plmRef->itk_short();
  itk::Image<unsigned char, 3>::Pointer targetVolumeItk = plmTgt->itk_uchar();

  Rt_plan rt_plan;
  Rt_beam *rt_beam;

  try
  {
    /* Connection of the beam parameters to the rt_beam class used to calculate the dose in platimatch */
  
    /* Plastimatch settings */

    // Assign inputs to dose calc logic
    /* SETTINGS */

    printf("\n ***SETTING PARAMETERS***\n");
    printf ("Setting reference volume\n");
    rt_plan.set_patient (referenceVolumeItk);

    printf ("Setting target volume\n");
    rt_plan.set_target (targetVolumeItk);

    // Create a beam
    rt_beam = rt_plan.append_beam ();

    // the normalization dose will be added at the creation of the 
    // global dose matrix: D = N * (beam1 * w1 + beam2 * w2...)
    rt_plan.set_normalization_dose(1.0); 

    // see comment previous line
    rt_beam->set_beam_weight(1.0);

    rt_beam->set_flavor(beamNode->GetBeamFlavor());
    printf("Beam Flavor = %c\n", rt_beam->get_flavor());

    rt_beam->get_sobp()->set_energyResolution(beamNode->GetEnergyResolution());
    printf("Energy resolution = %lg\n ", rt_beam->get_sobp()->get_energyResolution());

    /* APERTURE SETTINGS */
    printf("\n***APERTURE PARAMETERS***\n");
    beamNode->UpdateApertureParameters();
    rt_beam->get_aperture()->set_distance(beamNode->GetApertureOffset());
    printf("Aperture offset = %lg\n", rt_beam->get_aperture()->get_distance());

    printf("SAD = %lg\n", beamNode->GetSAD() );

    rt_beam->get_aperture()->set_spacing(beamNode->GetApertureSpacing());
    printf("Aperture Spacing = %lg %lg\n", rt_beam->get_aperture()->get_spacing(0),  rt_beam->get_aperture()->get_spacing(1));

    rt_beam->get_aperture()->set_origin(beamNode->GetApertureOrigin());
    printf("Aperture Origin = %lg %lg\n", beamNode->GetApertureOrigin(0), beamNode->GetApertureOrigin(1));

    rt_beam->get_aperture()->set_dim(beamNode->GetApertureDim() );
    printf("Aperture dim = %d %d\n", rt_beam->get_aperture()->get_dim(0), rt_beam->get_aperture()->get_dim(1) );
  
    rt_beam->set_source_size(beamNode->GetSourceSize());
    printf("Source size = %lg\n", rt_beam->get_source_size() );

    /* Beam Parameters */
    printf("\n***BEAM PARAMETERS***\n");


    rt_beam->set_source_position (src);
    rt_beam->set_isocenter_position (isocenter);
    printf("Isocenter = %lg %lg %lg\n", rt_beam->get_isocenter_position(0), rt_beam->get_isocenter_position(1),rt_beam->get_isocenter_position(2) );
    printf("Source position = %lg %lg %lg\n", rt_beam->get_source_position(0), rt_beam->get_source_position(1),rt_beam->get_source_position(2) );

    rt_beam->set_step_length (1);
    printf("Step length = %lg\n", rt_beam->get_step_length() );

    rt_beam->set_smearing (beamNode->GetSmearing());
    printf("Smearing = %lg\n", beamNode->GetSmearing());
    // Distal and proximal margins are updated when the SOBP is created
    /* All the rt_beam parameters are updated to initiate the dose calculation */

    if (!rt_plan.prepare_beam_for_calc (rt_beam))
    {
      /* Failure.  How to notify the user?? */
      std::cerr << "Sorry, rt_plan.prepare_beam_for_calc() failed.\n";
      return;
    }

    /* A little warm fuzzy for the developers */
    rt_plan.print_verif ();
    printf ("Working...\n");
    fflush(stdout);

    /* Compute the aperture and range compensator */
    //vtkWarningMacro ("Computing beam modifier\n");
    printf ("Computing beam modifiers\n"); fflush (stdout);
    rt_beam->compute_beam_modifiers ();
    //vtkWarningMacro ("Computing beam modifier done!\n");
    printf ("Computing beam modifiers done\n"); fflush (stdout);
  }
  catch (std::exception& ex)
  {
    vtkErrorMacro("ComputeDose: Plastimatch exception: " << ex.what());
    return;
  }

  /* Get aperture as itk image */
  Rpl_volume *rpl_vol = rt_beam->rpl_vol;
  Plm_image::Pointer& ap = rpl_vol->get_aperture()->get_aperture_image();
  this->Internal->apertureVolumeItk = ap->itk_uchar();

  /* Get range compensator as itk image */
  Plm_image::Pointer& rc =
    rpl_vol->get_aperture()->get_range_compensator_image();
  this->Internal->rcVolumeItk = rc->itk_float();

  /* Compute the dose */
  try
  {
    vtkWarningMacro("ComputeDose: Applying beam modifiers");
    rt_beam->apply_beam_modifiers ();

    vtkWarningMacro ("Optimizing SOBP\n");
    rt_beam->set_proximal_margin (proximalMargin);
    rt_beam->set_distal_margin (distalMargin);
    rt_beam->set_sobp_prescription_min_max (
      rpl_vol->get_min_wed(), rpl_vol->get_max_wed());
    rt_beam->optimize_sobp ();
    rt_plan.compute_dose (rt_beam);
  }
  catch (std::exception& ex)
  {
    vtkErrorMacro("ComputeDose: Plastimatch exception: " << ex.what());
    return;
  }

  /* Get dose as itk image */
  this->Internal->doseVolumeItk = rt_plan.get_dose_itk();

  this->Internal->TotalRx += beamRx;
  itk_image_scale (this->Internal->doseVolumeItk, beamRx);

  /* Add into accumulate image */
  itk_image_accumulate (this->Internal->accumulateVolumeItk, 1.0, this->Internal->doseVolumeItk);
}

//---------------------------------------------------------------------------
itk::Image<float, 3>::Pointer vtkSlicerDoseCalculationEngine::GetRangeCompensatorVolume()
{
  return this->Internal->rcVolumeItk;
}

//---------------------------------------------------------------------------
itk::Image<unsigned char, 3>::Pointer vtkSlicerDoseCalculationEngine::GetApertureVolume()
{
  return this->Internal->apertureVolumeItk;
}

//---------------------------------------------------------------------------
itk::Image<float, 3>::Pointer vtkSlicerDoseCalculationEngine::GetComputedDose()
{
  return this->Internal->doseVolumeItk;
}

//---------------------------------------------------------------------------
itk::Image<float, 3>::Pointer vtkSlicerDoseCalculationEngine::GetAccumulatedDose()
{
  return this->Internal->accumulateVolumeItk;
}

//---------------------------------------------------------------------------
double vtkSlicerDoseCalculationEngine::GetTotalRx()
{
  return this->Internal->TotalRx; 
}

//---------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::FinalizeAccumulatedDose()
{
  /* Free up memory for reference CT */
  Internal->plmRef.reset();
}
