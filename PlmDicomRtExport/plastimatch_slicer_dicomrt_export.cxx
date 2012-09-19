/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include <stdio.h>
#include <iostream>
#include <vector>
#include "plastimatch_slicer_dicomrt_exportCLP.h"

#include "plmbase.h"
#include "plmutil.h"

#include "plm_config.h"
#include "rtds_warp.h"
#include "warp_parms.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Plm_file_format file_type;
    Warp_parms parms;
    Rtds rtds;
    int have_input = 0;

    /* Optional inputs */
    if (ct_image.compare ("None") != 0) {
	have_input = 1;
	parms.input_fn = ct_image.c_str();
    }
    if (dose_image.compare ("None") != 0) {
	have_input = 1;
	parms.input_dose_img_fn = dose_image.c_str();
    }
    if (ss_image.compare ("None") != 0) {
	have_input = 1;
	parms.input_ss_img_fn = ss_image.c_str();
    }
    if (input_ss_list != "" && input_ss_list != "None") {
	parms.input_ss_list_fn = input_ss_list.c_str();
    }

    /* Required input */
    if (have_input && output_directory.compare ("None") != 0) {
	parms.output_dicom = output_directory.c_str();

	/* Process warp */
	file_type = PLM_FILE_FMT_IMG;
	rtds_warp (&rtds, file_type, &parms);
    }


    return EXIT_SUCCESS;
}
