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

#include "plm_config.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "itk_tps.h"
#include "landmark_warp.h"
#include "logfile.h"
#include "plm_image.h"
#include "plm_math.h"
#include "raw_pointset.h"
#include "rbf_gauss.h"
#include "rbf_wendland.h"
#include "xform.h"

// this .h is generated from ...landwarp.xml file by GenerateCLP in Slicer3-build
#include "plastimatch_slicer_landwarpCLP.h"

static void
do_landmark_warp_itk_tps (Landmark_warp *lw)
{
    itk_tps_warp (lw);
}

static void
do_landmark_warp_wendland (Landmark_warp *lw)  
{
    rbf_wendland_warp (lw);
}

static void
do_landmark_warp_gauss (Landmark_warp *lw)  //formerly do_landmark_warp_nsh
{
   rbf_gauss_warp (lw);
}

static void
do_landmark_warp (Landmark_warp *lw, const char *algorithm)
{
    if (!strcmp (algorithm, "tps")) {
        do_landmark_warp_itk_tps (lw);
    }
    else if (!strcmp (algorithm, "gauss")) {
        do_landmark_warp_gauss (lw);
    }
    else if (!strcmp (algorithm, "wendland")) {
        do_landmark_warp_wendland (lw);
    }
/*    else {
    do_landmark_warp_gcs (lw);
    }
*/
}

int
main (int argc, char *argv[])
{    
  //PARSE_ARGS comes from ...CLP.h
  PARSE_ARGS;

  Landmark_warp *lw = landmark_warp_create ();

  /* Load moving image */
  lw->m_input_img = plm_image_load_native (
    plmslc_landwarp_moving_volume.c_str());

  /* Default geometry comes from moving image.
     But if user specified fixed image, we use that. */
  lw->m_pih.set_from_plm_image (lw->m_input_img);
  if (plmslc_landwarp_fixed_volume != "") {
    Plm_image::Pointer tmp = plm_image_load_native (
      plmslc_landwarp_fixed_volume.c_str());
    lw->m_pih.set_from_plm_image (tmp);
  }

  /* Set other parameters */
  lw->default_val=plmslc_landwarp_default_value;
  lw->rbf_radius=plmslc_landwarp_rbf_radius;
  lw->young_modulus=plmslc_landwarp_stiffness;
  lw->num_clusters=plmslc_landwarp_num_clusters;

  unsigned long num_fiducials = plmslc_landwarp_fixed_fiducials.size();
  if (num_fiducials == 0 || plmslc_landwarp_moving_fiducials.size() == 0) {
    return EXIT_FAILURE;
  }
  if (plmslc_landwarp_moving_fiducials.size() < num_fiducials) {
    num_fiducials = plmslc_landwarp_moving_fiducials.size();
  }

  /* NSh: pointset_load_fcsv assumes RAS, as does Slicer.
     For some reason, pointset_load_txt assumes LPS.
     Thus, we write out Slicer-style .fcsv
     pointset_add_point assumes RAS
  */

  Labeled_pointset& fix_ps = lw->m_fixed_landmarks;
  Labeled_pointset& mov_ps = lw->m_moving_landmarks;

  for (unsigned long i = 0; i < num_fiducials; i++) {
        
    float lm_fix[3] = { 
      plmslc_landwarp_fixed_fiducials[i][0],  
      plmslc_landwarp_fixed_fiducials[i][1],  
      plmslc_landwarp_fixed_fiducials[i][2]
    };
    fix_ps.insert_ras (lm_fix);
        
    float lm_mov[3] = { 
      plmslc_landwarp_moving_fiducials[i][0],  
      plmslc_landwarp_moving_fiducials[i][1],  
      plmslc_landwarp_moving_fiducials[i][2]
    };
    mov_ps.insert_ras (lm_mov);
  }

  do_landmark_warp (lw, plmslc_landwarp_rbf_type.c_str());

  if (lw->m_warped_img && plmslc_landwarp_warped_volume != "None") {
    lw->m_warped_img->save_image (plmslc_landwarp_warped_volume.c_str());
  }
  if (lw->m_vf && plmslc_landwarp_vector_field != "None") {
    xform_save (lw->m_vf, plmslc_landwarp_vector_field.c_str());
  }

  return EXIT_SUCCESS;
}
