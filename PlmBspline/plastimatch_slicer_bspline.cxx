/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "plmregister.h"

#include "plastimatch_slicer_bsplineCLP.h"
#include "plm_stages.h"

#include "warp_parms.h"

#include "landmark_warp.h"
#include "logfile.h"
#include "plm_image.h"
#include "plm_math.h"
#include "raw_pointset.h"
#include "xform.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    std::ostringstream command_string;
    Warp_parms parms;
	
	command_string <<
	"[GLOBAL]\n"
	"fixed=" << plmslc_fixed_volume << "\n"
	"moving=" << plmslc_moving_volume << "\n";

    /* Get xform either from MRML scene or file */
    if (plmslc_xformwarp_input_xform_s != "" 
	&& plmslc_xformwarp_input_xform_s != "None")
    {
	parms.xf_in_fn = plmslc_xformwarp_input_xform_s.c_str();
    }
    else if (plmslc_xformwarp_input_vf_s != "" 
	&& plmslc_xformwarp_input_vf_s != "None")
    {
	parms.xf_in_fn = plmslc_xformwarp_input_vf_s.c_str();
    }
    else if (plmslc_xformwarp_input_xform_f != "" 
	&& plmslc_xformwarp_input_xform_f != "None")
    {
	parms.xf_in_fn = plmslc_xformwarp_input_xform_f.c_str();
    }

    printf ("xf_in_fn = %s\n", (const char*) parms.xf_in_fn);

	if (plmslc_output_warped != "" && plmslc_output_warped != "None") {
	command_string << 
	    "img_out=" << plmslc_output_warped << "\n";
    }
    	command_string <<
	    "warped_landmarks=" << plmslc_warped_fiducials_f << "\n";

    /* GCS FIX: You can specify two output bspline xforms, but not 
       (yet) two output vector field files */
    if (plmslc_output_vf != "" && plmslc_output_vf != "None") {
	command_string << 
	    "vf_out=" << plmslc_output_vf << "\n";
    }
    else if (plmslc_output_vf_f != "" && plmslc_output_vf_f != "None") {
	command_string << 
	    "vf_out=" << plmslc_output_vf_f << "\n";
    }

    if (plmslc_output_bsp != "" && plmslc_output_bsp != "None") {
	command_string << 
	    "xf_out_itk=true\n"
	    "xf_out=" << plmslc_output_bsp << "\n";
    }
    if (plmslc_output_bsp_f != "" && plmslc_output_bsp_f != "None") {
	command_string << 
	    "xf_out_itk=true\n"
	    "xf_out=" << plmslc_output_bsp_f << "\n";
    }

    if (output_type != "auto") {
	command_string << "img_out_type = " << output_type << "\n";
    }

    /* Figure out fiducials */
    unsigned long num_fiducials = plmslc_fixed_fiducials.size();
    if (plmslc_moving_fiducials.size() < num_fiducials) {
	num_fiducials = plmslc_moving_fiducials.size();
    }
    printf ("Num fiducials = %lu\n", num_fiducials);
    if (num_fiducials == 0) {
        printf (">> No fiducials.\n");
    }
	/* Stage 0 */
    if (enable_stage_0) {
	command_string << 
	    "[STAGE]\n"
	    "metric=" 
	    << metric << "\n"
	    "xform=translation\n"
	    "optim=rsg\n"
	    "impl=itk\n"
	    "max_its=" << stage_0_its << "\n"
	    "convergence_tol=5\n"
	    "grad_tol=1.5\n"
	    "res=" 
	    << stage_0_resolution[0] << " "
	    << stage_0_resolution[1] << " "
	    << stage_0_resolution[2] << "\n";
    }
    if (num_fiducials > 0) {
        command_string << "fixed_landmark_list=";
        for (unsigned long i = 0; i < num_fiducials; i++) {
            command_string << 
                plmslc_fixed_fiducials[i][0] << "," <<
                plmslc_fixed_fiducials[i][1] << "," <<
                plmslc_fixed_fiducials[i][2];
            if (i < num_fiducials - 1) {
                command_string << ";";
            } else {
                command_string << "\n";
            }
        }
        command_string << "moving_landmark_list=";
        for (unsigned long i = 0; i < num_fiducials; i++) {
            command_string << 
                plmslc_moving_fiducials[i][0] << "," <<
                plmslc_moving_fiducials[i][1] << "," <<
                plmslc_moving_fiducials[i][2];
            if (i < num_fiducials - 1) {
                command_string << ";";
            } else {
                command_string << "\n";
            }
        }
    }

    /* Stage 1 */
    command_string << 
	"[STAGE]\n"
	"metric=" 
	<< metric << "\n"
	"xform=bspline\n"	
	"optim=lbfgsb\n"
	"impl=plastimatch\n"
	"threading=" 
	<< (strcmp (hardware.c_str(),"CPU") ? "cuda" : "openmp")
	<< "\n"
	"max_its=" << stage_1_its << "\n"
	"convergence_tol=5\n"
	"grad_tol=1.5\n"
	"regularization_lambda=" << stage_1_regularization << "\n"
	"landmark_stiffness=" << stage_1_landm_penalty << "\n"
	"res=" 
	<< stage_1_resolution[0] << " "
	<< stage_1_resolution[1] << " "
	<< stage_1_resolution[2] << "\n"
	"grid_spac="
	<< stage_1_grid_size << " "
	<< stage_1_grid_size << " "
	<< stage_1_grid_size << "\n"
	;
    if (plmslc_output_warped_1 != "" && plmslc_output_warped_1 != "None") {
	command_string << 
	    "img_out=" << plmslc_output_warped_1 << "\n";
    }
		

    if (enable_stage_2) {
	command_string << 
	    "[STAGE]\n" 
	    "max_its=" << stage_2_its << "\n"
	    "regularization_lambda=" << stage_2_regularization << "\n"
	    "landmark_stiffness=" << stage_2_landm_penalty << "\n"
	    "res=" 
	    << stage_2_resolution[0] << " "
	    << stage_2_resolution[1] << " "
	    << stage_2_resolution[2] << "\n"
	    "grid_spac="
	    << stage_2_grid_size << " "
	    << stage_2_grid_size << " "
	    << stage_2_grid_size << "\n"
	    ;
   if (plmslc_output_warped_2 != "" && plmslc_output_warped_2 != "None") {
	command_string << 
	    "img_out=" << plmslc_output_warped_2 << "\n";
    }   
}


	if (enable_stage_3) {
	    command_string << 
	    	"[STAGE]\n"
	    	"max_its=" << stage_3_its << "\n"
	    	"regularization_lambda=" << stage_3_regularization << "\n"
	    	"landmark_stiffness=" << stage_3_landm_penalty << "\n"
	    	"res=" 
	    	<< stage_3_resolution[0] << " "
	    	<< stage_3_resolution[1] << " "
	    	<< stage_3_resolution[2] << "\n"
	    	"grid_spac="
	    	<< stage_3_grid_size << " "
	    	<< stage_3_grid_size << " "
	    	<< stage_3_grid_size << "\n"
	    	;
	if (plmslc_output_warped_3 != "" && plmslc_output_warped_3 != "None") {
	command_string << 
	    "img_out=" << plmslc_output_warped_3 << "\n";
    }
    	}

    std::cout << command_string.str() << "\n";

    Registration_parms regp;
    if (regp.set_command_string (command_string.str()) < 0) {
	return EXIT_FAILURE;
    }
    

//  if (!plmslc_interactive_registration) 
	do_registration (&regp);
//		else { 
//		prepare config for interactive registration
//		do_interactive_registration();
// 		}

    return EXIT_SUCCESS;
}
