/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"

#include "plastimatch_slicer_dose_comparisonCLP.h"

#include <string.h>
#include <stdlib.h>

#include "plmbase.h"
#include "plmutil.h"

#include "pstring.h"
#include "threshbox.h"

int
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Diff_parms parms;
    Threshbox_parms tparms;
    Gamma_parms gparms;

    bool have_dose1_img_input = false;
    bool have_dose2_img_input = false;
    bool have_dose_diff_output = false;

    /* Input dose */

    if (plmslc_need_dosediff) {

        if (plmslc_dose1_input_img != "" 
            && plmslc_dose1_input_img != "None")
        {
            have_dose1_img_input = true;
            parms.img_in_1_fn = plmslc_dose1_input_img.c_str();
        }

        if (plmslc_dose2_input_img != "" 
            && plmslc_dose2_input_img != "None")
        {
            have_dose2_img_input = true;
            parms.img_in_2_fn = plmslc_dose2_input_img.c_str();
        }

        if (!have_dose1_img_input && !have_dose2_img_input) {
            printf ("Error.  No input specified.\n");
            return EXIT_FAILURE;
        }

        /* Output dose difference */
        if (plmslc_dose_diff_output_img != "" 
            && plmslc_dose_diff_output_img != "None")
        {
            have_dose_diff_output = true;
            parms.img_out_fn = plmslc_dose_diff_output_img.c_str();
        }

    
        /* Output type 
           if (plmslc_xformwarp_output_type != "auto") {
           parms.output_type = plm_image_type_parse (
           plmslc_xformwarp_output_type.c_str());
           }
        */

        /* What is the input file type? 
           file_type = plm_file_format_deduce ((const char*) parms.input_fn);
        */

        /* Process diff; output image is set in diff_main */
        diff_main (&parms);

    } //end if need dosediff

    if ( plmslc_need_isodose ) {

        if ( plmslc_input_dose_image == "" || plmslc_input_dose_image == "None" ) {
            printf("Error. No input specified for dose multithresholding!\n");
            return EXIT_FAILURE;
        }

        tparms.img_in = plm_image_load_native ( plmslc_input_dose_image.c_str() );

        // can be changed into arrays if needed 
        tparms.isodose_value1 = plmslc_isodose_value1;
        tparms.isodose_value2 = plmslc_isodose_value2;
        tparms.isodose_value3 = plmslc_isodose_value3;
        tparms.isodose_value4 = plmslc_isodose_value4;
        tparms.isodose_value5 = plmslc_isodose_value5;

        do_multi_threshold( &tparms );

        UCharImageType::Pointer imgcompo = tparms.composite_labelmap->itk_uchar();
        if ( plmslc_isodose_composite != "" && plmslc_isodose_composite != "None" ) 
            itk_image_save (imgcompo, plmslc_isodose_composite.c_str());
/*
  UCharImageType::Pointer img1 = tparms.dose_labelmap1->itk_uchar();
  if ( plmslc_isodose_img_out1 != "" && plmslc_isodose_img_out1 != "None" ) 
  itk_image_save (img1, plmslc_isodose_img_out1.c_str());
        
  UCharImageType::Pointer img2 = tparms.dose_labelmap2->itk_uchar();
  if ( plmslc_isodose_img_out2 != "" && plmslc_isodose_img_out2 != "None" ) 
  itk_image_save (img2, plmslc_isodose_img_out2.c_str());
        
  UCharImageType::Pointer img3 = tparms.dose_labelmap3->itk_uchar();
  if ( plmslc_isodose_img_out3 != "" && plmslc_isodose_img_out3 != "None" ) 
  itk_image_save (img3, plmslc_isodose_img_out3.c_str());
        
  UCharImageType::Pointer img4 = tparms.dose_labelmap4->itk_uchar();
  if ( plmslc_isodose_img_out4 != "" && plmslc_isodose_img_out4 != "None" ) 
  itk_image_save (img4, plmslc_isodose_img_out4.c_str());
        
  UCharImageType::Pointer img5 = tparms.dose_labelmap5->itk_uchar();
  if ( plmslc_isodose_img_out5 != "" && plmslc_isodose_img_out5 != "None" ) 
  itk_image_save (img5, plmslc_isodose_img_out5.c_str());
*/
    } //end if need isodose
    
    if (plmslc_need_gamma) {
        if (plmslc_gamma_input_img1 == "" || plmslc_gamma_input_img1 == "None" ||
            plmslc_gamma_input_img2 == "" || plmslc_gamma_input_img2 == "None"  ) {
            printf("Error. No input specified for dose multithresholding!\n");
            return EXIT_FAILURE;
        }

        gparms.img_in1 = plm_image_load_native (plmslc_gamma_input_img1.c_str());
        gparms.img_in2 = plm_image_load_native (plmslc_gamma_input_img2.c_str());

        gparms.r_tol = plmslc_gamma_r_tol;
        gparms.gamma_max = plmslc_gamma_g;

        find_dose_threshold (&gparms); 

//   if (plmslc_gamma_d_tol_gy != "" )
        if (plmslc_gamma_d_tol_gy != 0) {
            gparms.d_tol = plmslc_gamma_d_tol_gy;
        }
//  else if (plmslc_gamma_d_tol_percent != "" )
        else if (plmslc_gamma_d_tol_percent != 0) {
            gparms.d_tol = plmslc_gamma_d_tol_percent/100.*gparms.dose_max;
        }

        /* Select labalmap output mode */
        if (!strcmp ("None", labelmap_mode.c_str())) {
            gparms.mode = NONE;
        }
        else if (!strcmp ("Pass", labelmap_mode.c_str())) {
            gparms.mode = PASS;
        }
        else if (!strcmp ("Fail", labelmap_mode.c_str())) {
            gparms.mode = FAIL;
        }

        do_gamma_analysis (&gparms); 

        /* for Gamma */
        FloatImageType::Pointer img_out = gparms.img_out->itk_float();
        itk_image_save (img_out, plmslc_gamma_output.c_str());

        /* for labelmap */
        if (strcmp ("None", labelmap_mode.c_str())) {
            UCharImageType::Pointer labelmap_out = gparms.labelmap_out->itk_uchar();
            itk_image_save (labelmap_out, plmslc_gamma_labelmap.c_str());
        }

    } // end if need gamma

    return EXIT_SUCCESS;
}
