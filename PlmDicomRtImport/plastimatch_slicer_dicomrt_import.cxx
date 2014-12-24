/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Gregory C. Sharp, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#include <stdio.h>
#include <iostream>
#include <vector>
#include "plastimatch_slicer_dicomrt_importCLP.h"

#include "plm_config.h"
#include "rt_study.h"
#include "rt_study_warp.h"
#include "warp_parms.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Plm_file_format file_type;
    Warp_parms parms;
    Rt_study rtds;

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
    rt_study_warp (&rtds, file_type, &parms);

    return EXIT_SUCCESS;
}
