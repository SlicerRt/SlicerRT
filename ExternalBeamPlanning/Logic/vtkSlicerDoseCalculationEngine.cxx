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

  /// Pointer that contains the dose --> MD Fix: vector of itk_images?
  itk::Image<float, 3>::Pointer rcVolumeItk;

  /// Pointer that contains the dose --> MD Fix: vector of itk_images?
  itk::Image<unsigned char, 3>::Pointer apertureVolumeItk;

  /// Pointer that contains the dose --> MD Fix: vector of itk_images?
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
  this->Internal->doseVolumeItk = itk_image_create<float>(Plm_image_header(referenceVolumeItk));
  this->Internal->TotalRx = 0.f;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseCalculationEngine::CalculateDose(
  vtkMRMLRTBeamNode* beamNode, 
  Plm_image::Pointer& plmTgt,
  double isocenter[],
  double src[],
  double RxDose)
{
  vtkMRMLRTProtonBeamNode* protonBeamNode = vtkMRMLRTProtonBeamNode::SafeDownCast (beamNode);
  
  this->Internal->plmRef->print ();
  plmTgt->print ();

  float origin[3] = {plmTgt->origin(0),plmTgt->origin(1), plmTgt->origin(2)};
  float spacing[3] = {plmTgt->spacing(0),plmTgt->spacing(1), plmTgt->spacing(2)};
  plm_long dim[3] = {(plm_long) plmTgt->dim(0), (plm_long) plmTgt->dim(1), (plm_long) plmTgt->dim(2)};

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

    // Create a beam
    rt_beam = rt_plan.append_beam ();

    // Assign inputs to dose calc logic

    // Update plan
    printf("\n ***PLAN PARAMETERS***\n");
    printf ("Setting reference volume\n");
    rt_plan.set_patient (referenceVolumeItk);
    printf ("Setting target volume\n");
    rt_plan.set_target (targetVolumeItk);
    printf ("Setting reference dose point -> ");
    rt_plan.set_ref_dose_point(isocenter); // MD Fix, for the moment, the reference dose point is the isocenter
    printf("Reference dose position: %lg %lg %lg\n", rt_plan.get_ref_dose_point()[0], rt_plan.get_ref_dose_point()[1], rt_plan.get_ref_dose_point()[2]);
    rt_plan.set_have_ref_dose_point(true);
    rt_plan.set_have_dose_norm(true);
    printf ("Setting dose prescription -> ");
    rt_plan.set_normalization_dose(RxDose);
    printf("Dose prescription = %lg\n", rt_plan.get_normalization_dose());

    // Not needed for dose calculation: 
    // Parameter Set, Plan Contour, Dose Volume, Dose Grid

    // Update beam
    printf("\n ***BEAM PARAMETERS***\n");
    printf ("Setting source position -> ");
    rt_beam->set_source_position(src);
    printf("Source position: %lg %lg %lg\n", rt_beam->get_source_position()[0], rt_beam->get_source_position()[1], rt_beam->get_source_position()[2]);
    printf ("Setting isocenter position -> ");
    rt_beam->set_isocenter_position(isocenter);
    printf("Isocenter position: %lg %lg %lg\n", rt_beam->get_isocenter_position()[0], rt_beam->get_isocenter_position()[1], rt_beam->get_isocenter_position()[2]);
    printf ("Setting dose calculation algorithm -> ");
    switch(protonBeamNode->GetAlgorithm())
    {
        case vtkMRMLRTProtonBeamNode::CGS:
            rt_beam->set_flavor('f');
            break;
        case vtkMRMLRTProtonBeamNode::DGS:
            rt_beam->set_flavor('g');
            break;
        case vtkMRMLRTProtonBeamNode::HGS:
            rt_beam->set_flavor('h');
            break;
        default:
            rt_beam->set_flavor('a');
            break;
    }
    printf("Algorithm Flavor = %c\n", rt_beam->get_flavor());
    printf ("Setting homo approximation -> ");
    if (protonBeamNode->GetLateralSpreadHomoApprox() == true)
    {
        rt_beam->set_homo_approx('y');
        printf("Homo approximation set to true\n");
    }
    else
    {
        rt_beam->set_homo_approx('n');
        printf("Homo approximation set to false\n");
    }
    printf ("Setting beam weight -> ");
    rt_beam->set_beam_weight(beamNode->GetBeamWeight());
    printf("Beam weight = %lg\n", rt_beam->get_beam_weight());
    printf ("Setting smearing -> ");
    rt_beam->set_smearing(beamNode->GetSmearing());
    printf("Smearing = %lg\n", rt_beam->get_smearing());
    printf ("Setting Highland model for range compensator\n");
    if (protonBeamNode->GetRangeCompensatorHighland() == true)
    {
        rt_beam->set_rc_MC_model('n');
        printf("Highland model for range compensator set to true\n");
    }
    else
    {
        rt_beam->set_rc_MC_model('y');
        printf("Highland model for range compensator set to false\n");
    }
    printf ("Setting source size -> ");
    rt_beam->set_source_size(protonBeamNode->GetSourceSize());
    printf("Source size = %lg\n", rt_beam->get_source_size());
    printf ("Setting step length -> ");
    rt_beam->set_step_length(protonBeamNode->GetStepLength());
    printf("Step length = %lg\n", rt_beam->get_step_length());
    // Not needed: BeamType, NominalEnergy, BeamOnTime, NominalmA, MLC_Array
    // To be added in the future: couchAngle

    // Update aperture parameters
    printf("\nAPERTURE PARAMETERS:\n");
    protonBeamNode->UpdateApertureParameters(beamNode->GetSAD()); // Sanity check before we set the parameters
    printf ("Setting aperture distance -> ");
    rt_beam->get_aperture()->set_distance(protonBeamNode->GetApertureOffset());
    printf("Aperture distance = %lg\n", rt_beam->get_aperture()->get_distance());
    printf ("Setting aperture origin -> ");
    rt_beam->get_aperture()->set_origin(protonBeamNode->GetApertureOrigin());
    printf("Aperture origin = %lg %lg\n", protonBeamNode->GetApertureOrigin(0), protonBeamNode->GetApertureOrigin(1));
    printf ("Setting aperture spacing -> ");
    rt_beam->get_aperture()->set_spacing(protonBeamNode->GetApertureSpacing());
    printf("Aperture Spacing = %lg %lg\n", rt_beam->get_aperture()->get_spacing(0),  rt_beam->get_aperture()->get_spacing(1));
    printf ("Setting aperture dim -> ");
    rt_beam->get_aperture()->set_dim(protonBeamNode->GetApertureDim() );
    printf("Aperture dim = %d %d\n", rt_beam->get_aperture()->get_dim(0), rt_beam->get_aperture()->get_dim(1) );
    // To be added in the future: collimatorAngle

    // Update mebs parameters
    printf("\nENERGY PARAMETERS:\n");
    printf("Setting beam line type -> ");
    if (protonBeamNode->GetBeamLineType() == true)
    {
        rt_beam->set_beam_line_type("active");      
        printf("beam line type set to active\n");
    }
    else
    {
        rt_beam->set_beam_line_type("passive");      
        printf("beam line type set to passive\n");
    }
    printf("Setting have prescription -> ");
    rt_beam->get_mebs()->set_have_prescription(protonBeamNode->GetManualEnergyLimits());
    printf("Manual energy prescription set to %d\n", rt_beam->get_mebs()->get_have_prescription());
    if (rt_beam->get_mebs()->get_have_prescription() == true)
    {
        rt_beam->get_mebs()->set_energy_min(protonBeamNode->GetMinimumEnergy());
        rt_beam->get_mebs()->set_energy_max(protonBeamNode->GetMaximumEnergy());
        printf("Energy min: %lg, Energy max: %lg\n", rt_beam->get_mebs()->get_energy_min(), rt_beam->get_mebs()->get_energy_max());
    }
    printf("Setting proximal margin -> ");
    rt_beam->get_mebs()->set_proximal_margin (protonBeamNode->GetProximalMargin());
    printf("Proximal margin = %lg\n", rt_beam->get_mebs()->get_proximal_margin());
    printf("Setting distal margin -> ");
    rt_beam->get_mebs()->set_distal_margin (protonBeamNode->GetDistalMargin());
    printf("Distal margin = %lg\n", rt_beam->get_mebs()->get_distal_margin());
    printf("Setting energy resolution -> ");
    rt_beam->get_mebs()->set_energy_resolution(protonBeamNode->GetEnergyResolution());
    printf("Energy resolution = %lg\n", rt_beam->get_mebs()->get_energy_resolution());
    printf("Setting energy spread -> ");
    rt_beam->get_mebs()->set_spread (protonBeamNode->GetEnergySpread());
    printf("Energy spread = %lg\n", rt_beam->get_mebs()->get_spread());

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
  Plm_image::Pointer& rc = rpl_vol->get_aperture()->get_range_compensator_image();
  this->Internal->rcVolumeItk = rc->itk_float();

  /* Compute the dose */
  try
  {
    if (rt_beam->get_beam_line_type() != "passive")
    {
        /* active */
        rt_beam->get_mebs()->compute_particle_number_matrix_from_target_active_slicerRt(rt_beam->rpl_vol, plmTgt, rt_beam->get_smearing());
    }
    else
    {
        /* passive */
        rt_beam->rpl_vol->compute_beam_modifiers_passive_scattering_slicerRt (plmTgt, rt_beam->get_smearing(), rt_beam->get_mebs()->get_proximal_margin(), rt_beam->get_mebs()->get_distal_margin());
        rt_beam->get_mebs()->set_prescription_depths(rt_beam->rpl_vol->get_min_wed(), rt_beam->rpl_vol->get_max_wed());
        rt_beam->rpl_vol->apply_beam_modifiers();
        rt_beam->get_mebs()->optimize_sobp();
        int ap_dim[2] = {rt_beam->rpl_vol->get_aperture()->get_dim()[0], rt_beam->rpl_vol->get_aperture()->get_dim()[1]};
        rt_beam->get_mebs()->generate_part_num_from_weight(ap_dim);
    }
    /* We can compute the dose */
    rt_plan.compute_dose (rt_beam);
  }
  catch (std::exception& ex)
  {
    vtkErrorMacro("ComputeDose: Plastimatch exception: " << ex.what());
    return;
  }
  /* Get dose as itk image */
  this->Internal->doseVolumeItk = rt_beam->get_dose()->itk_float();
  
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
