/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include <stdio.h>
#include <iostream>
#include <vector>
#include "plastimatch_slicer_dicomrt_importCLP.h"

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

    /* Required input */
    parms.input_fn = input_dicomrt_ss.c_str();

    /* Optional inputs */
    if (output_labelmap.compare ("None") != 0) {
	parms.output_labelmap_fn = output_labelmap.c_str();
    }
    if (output_ss_img.compare ("None") != 0) {
	parms.output_ss_img_fn = output_ss_img.c_str();
    }
    if (output_dose.compare ("None") != 0) {
	parms.output_dose_img_fn = output_dose.c_str();
    }
    if (output_image.compare ("None") != 0) {
	parms.output_img_fn = output_image.c_str();
    }
    if (reference_vol.compare ("None") != 0) {
	parms.fixed_img_fn = reference_vol.c_str();
    }
    if (output_ss_list != "" && output_ss_list != "None") {
	parms.output_ss_list_fn = output_ss_list.c_str();
    }

    if (overlapping_contours == "XOR") {
	parms.xor_contours = true;
    } else {
	parms.xor_contours = false;
    }

    /* Process warp */
    file_type = PLM_FILE_FMT_DICOM_DIR;
    rtds_warp (&rtds, file_type, &parms);

    return EXIT_SUCCESS;
}
