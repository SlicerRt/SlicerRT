/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include "plastimatch_slicer_synthCLP.h"

#include "plmbase.h"
#include "plmutil.h"

#include "itk_image.h"
#include "itk_image_save.h"
#include "rtss.h"
#include "synthetic_mha.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Synthetic_mha_parms sm_parms;
    if (plmslc_dim.size() >= 3) {
	sm_parms.dim[0] = plmslc_dim[0];
	sm_parms.dim[1] = plmslc_dim[1];
	sm_parms.dim[2] = plmslc_dim[2];
    } else if (plmslc_dim.size() >= 1) {
	sm_parms.dim[0] = plmslc_dim[0];
	sm_parms.dim[1] = plmslc_dim[0];
	sm_parms.dim[2] = plmslc_dim[0];
    }
    if (plmslc_origin.size() >= 3) {
	sm_parms.origin[0] = plmslc_origin[0];
	sm_parms.origin[1] = plmslc_origin[1];
	sm_parms.origin[2] = plmslc_origin[2];
    } else if (plmslc_origin.size() >= 1) {
	sm_parms.origin[0] = plmslc_origin[0];
	sm_parms.origin[1] = plmslc_origin[0];
	sm_parms.origin[2] = plmslc_origin[0];
    }
    if (plmslc_spacing.size() >= 3) {
	sm_parms.spacing[0] = plmslc_spacing[0];
	sm_parms.spacing[1] = plmslc_spacing[1];
	sm_parms.spacing[2] = plmslc_spacing[2];
    } else if (plmslc_spacing.size() >= 1) {
	sm_parms.spacing[0] = plmslc_spacing[0];
	sm_parms.spacing[1] = plmslc_spacing[0];
	sm_parms.spacing[2] = plmslc_spacing[0];
    }

    /* Pattern options */
    if (plmslc_pattern == "Gauss") {
	sm_parms.pattern = PATTERN_GAUSS;
    } else if (plmslc_pattern == "Rectangle") {
	sm_parms.pattern = PATTERN_RECT;
    } else if (plmslc_pattern == "Sphere") {
	sm_parms.pattern = PATTERN_SPHERE;
    } 
	
    if (create_enclosed){
	sm_parms.pattern = PATTERN_ENCLOSED_RECT;
    }

    sm_parms.pattern_ss = PATTERN_SS_ONE;
    if (plmslc_pattern_ss == 1) sm_parms.pattern_ss = PATTERN_SS_ONE;
    if (plmslc_pattern_ss == 2) sm_parms.pattern_ss = PATTERN_SS_TWO_APART;
    if (plmslc_pattern_ss == 3) sm_parms.pattern_ss = PATTERN_SS_TWO_OVERLAP_PLUS_ONE;
    if (plmslc_pattern_ss == 4) sm_parms.pattern_ss = PATTERN_SS_TWO_OVERLAP_PLUS_ONE_PLUS_EMBED;
    
    sm_parms.enclosed_intens_f1 = plmslc_intensity1;
    sm_parms.enclosed_intens_f2 = plmslc_intensity2;

    sm_parms.foreground = plmslc_foreground;
    sm_parms.background = plmslc_background;

    if (create_objstructdose){
	sm_parms.pattern = PATTERN_OBJSTRUCTDOSE;
    }

    if (create_objstrucmha && create_objstructdose) { 
	sm_parms.m_want_ss_img = true;
    }

    if (create_objdosemha && create_objdosemha){
	sm_parms.m_want_dose_img = true;
    }


    /* Gauss options */
    if (plmslc_gausscenter.size() >= 3) {
	sm_parms.gauss_center[0] = plmslc_gausscenter[0];
	sm_parms.gauss_center[1] = plmslc_gausscenter[1];
	sm_parms.gauss_center[2] = plmslc_gausscenter[2];
    } else if (plmslc_gausscenter.size() >= 1) {
	sm_parms.gauss_center[0] = plmslc_gausscenter[0];
	sm_parms.gauss_center[1] = plmslc_gausscenter[0];
	sm_parms.gauss_center[2] = plmslc_gausscenter[0];
    }
    if (plmslc_gausswidth.size() >= 3) {
	sm_parms.gauss_std[0] = plmslc_gausswidth[0];
	sm_parms.gauss_std[1] = plmslc_gausswidth[1];
	sm_parms.gauss_std[2] = plmslc_gausswidth[2];
    } else if (plmslc_gausswidth.size() >= 1) {
	sm_parms.gauss_std[0] = plmslc_gausswidth[0];
	sm_parms.gauss_std[1] = plmslc_gausswidth[0];
	sm_parms.gauss_std[2] = plmslc_gausswidth[0];
    }

    /* Rect options */
    if (plmslc_rectsize.size() >= 6) {
	sm_parms.rect_size[0] = plmslc_rectsize[0];
	sm_parms.rect_size[1] = plmslc_rectsize[1];
	sm_parms.rect_size[2] = plmslc_rectsize[2];
	sm_parms.rect_size[3] = plmslc_rectsize[3];
	sm_parms.rect_size[4] = plmslc_rectsize[4];
	sm_parms.rect_size[5] = plmslc_rectsize[5];
    } else if (plmslc_rectsize.size() >= 3) {
	sm_parms.rect_size[0] = - 0.5 * plmslc_rectsize[0];
	sm_parms.rect_size[2] = - 0.5 * plmslc_rectsize[1];
	sm_parms.rect_size[4] = - 0.5 * plmslc_rectsize[2];
	sm_parms.rect_size[1] = - sm_parms.rect_size[0];
	sm_parms.rect_size[3] = - sm_parms.rect_size[2];
	sm_parms.rect_size[5] = - sm_parms.rect_size[4];
    }
    else if (plmslc_rectsize.size() >= 1) {
	sm_parms.rect_size[0] = - 0.5 * plmslc_rectsize[0];
	sm_parms.rect_size[1] = - sm_parms.rect_size[0];
	sm_parms.rect_size[2] = + sm_parms.rect_size[0];
	sm_parms.rect_size[3] = - sm_parms.rect_size[0];
	sm_parms.rect_size[4] = + sm_parms.rect_size[0];
	sm_parms.rect_size[5] = - sm_parms.rect_size[0];
    }

    /* Sphere options */
    if (plmslc_spherecenter.size() >= 3) {
	sm_parms.sphere_center[0] = plmslc_spherecenter[0];
	sm_parms.sphere_center[1] = plmslc_spherecenter[1];
	sm_parms.sphere_center[2] = plmslc_spherecenter[2];
    } else if (plmslc_spherecenter.size() >= 1) {
	sm_parms.sphere_center[0] = plmslc_spherecenter[0];
	sm_parms.sphere_center[1] = plmslc_spherecenter[0];
	sm_parms.sphere_center[2] = plmslc_spherecenter[0];
    }
    if (plmslc_spheresize.size() >= 3) {
	sm_parms.sphere_radius[0] = plmslc_spheresize[0];
	sm_parms.sphere_radius[1] = plmslc_spheresize[1];
	sm_parms.sphere_radius[2] = plmslc_spheresize[2];
    } else if (plmslc_spheresize.size() >= 1) {
	sm_parms.sphere_radius[0] = plmslc_spheresize[0];
	sm_parms.sphere_radius[1] = plmslc_spheresize[0];
	sm_parms.sphere_radius[2] = plmslc_spheresize[0];
    }

    /* Create Volume 1 */
    /* Also write out dose and structure set image if requested */
    if (plmslc_output_one != "" && plmslc_output_one != "None") {
	Rtds rtds;
	synthetic_mha (&rtds, &sm_parms);
	FloatImageType::Pointer img = rtds.m_img->itk_float();
	itk_image_save_float (img, plmslc_output_one.c_str());

	if (plmslc_output_dosemha != "" && plmslc_output_dosemha != "None" && sm_parms.m_want_dose_img) {
	    rtds.m_dose->save_image (plmslc_output_dosemha.c_str());
	}

	if (create_objstrucmha && create_objstructdose && plmslc_output_ssmha != "" && plmslc_output_ssmha != "None") {
	    rtds.m_rtss->save_ss_image (plmslc_output_ssmha.c_str());
	}
    }

    /* Create Volume 2 */
    if (plmslc_output_two != "" && plmslc_output_two != "None") {

	float xlat[3] = { 0.f, 0.f, 0.f };
	if (plmslc_vol2xlat.size() >= 1) {
	    xlat[0] = plmslc_vol2xlat[0];
	}
	if (plmslc_vol2xlat.size() >= 3) {
	    xlat[1] = plmslc_vol2xlat[1];
	    xlat[2] = plmslc_vol2xlat[2];
	}

	/* Translate volume */
	sm_parms.rect_size[0] += xlat[0];
	sm_parms.rect_size[1] += xlat[0];
	sm_parms.rect_size[2] += xlat[1];
	sm_parms.rect_size[3] += xlat[1];
	sm_parms.rect_size[4] += xlat[2];
	sm_parms.rect_size[5] += xlat[2];
	sm_parms.sphere_center[0] += xlat[0];
	sm_parms.sphere_center[1] += xlat[1];
	sm_parms.sphere_center[2] += xlat[2];
	sm_parms.gauss_center[0] += xlat[0];
	sm_parms.gauss_center[1] += xlat[1];
	sm_parms.gauss_center[2] += xlat[2];

	sm_parms.enclosed_xlat1[0]=plmslc_xlat_struct1[0];
	sm_parms.enclosed_xlat1[1]=plmslc_xlat_struct1[1];
	sm_parms.enclosed_xlat1[2]=plmslc_xlat_struct1[2];
	sm_parms.enclosed_xlat2[0]=plmslc_xlat_struct2[0];
	sm_parms.enclosed_xlat2[1]=plmslc_xlat_struct2[1];
	sm_parms.enclosed_xlat2[2]=plmslc_xlat_struct2[2];

	Rtds rtds;
	synthetic_mha (&rtds, &sm_parms);
	FloatImageType::Pointer img = rtds.m_img->itk_float();
	itk_image_save_float (img, plmslc_output_two.c_str());
    }

    return EXIT_SUCCESS;
}
