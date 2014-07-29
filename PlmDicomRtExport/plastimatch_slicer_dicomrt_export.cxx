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
#include "plastimatch_slicer_dicomrt_exportCLP.h"

#include "plm_config.h"
#include "rt_study.h"
#include "rtds_warp.h"
#include "warp_parms.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Plm_file_format file_type;
    Warp_parms parms;
    Rt_study rtds;
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
