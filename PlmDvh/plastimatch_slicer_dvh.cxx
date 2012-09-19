/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <iostream>
#include <vector>
#include <stdio.h>
#include "plastimatch_slicer_dvhCLP.h"

#include "dvh.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Dvh dvh;

    /* Set input images */
    if (input_ss_list != "" && input_ss_list != "None") {
        dvh.set_structure_set_image (
            input_ss_image.c_str(), input_ss_list.c_str());
    } else {
        dvh.set_structure_set_image (input_ss_image.c_str(), 0);
    }
    dvh.set_dose_image (input_dose_image.c_str());

    /* Set input parameters */
    dvh.set_dose_units (Dvh::DVH_UNITS_CGY);
    Dvh::Dvh_normalization normalization = Dvh::DVH_NORMALIZATION_VOX;
    Dvh::Histogram_type htype = (histogram_type == "Cumulative") 
        ? Dvh::DVH_CUMULATIVE_HISTOGRAM : Dvh::DVH_DIFFERENTIAL_HISTOGRAM;
    dvh.set_dvh_parameters (normalization, htype, num_bins, bin_width);

    /* Process DVH */
    dvh.run();

    /* Save output to csv */
    dvh.save_csv (output_dvh_filename.c_str());

    return EXIT_SUCCESS;
}
