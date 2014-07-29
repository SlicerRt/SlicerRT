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
#include "plastimatch_slicer_mabsCLP.h"

#include "mabs.h"
#include "mabs_parms.h"

int 
main (int argc, char * argv [])
{
    PARSE_ARGS;

    Mabs_parms parms;

    /* Required input */
    parms.atlas_dir = atlas_directory;
    parms.labeling_input_fn = input_image.c_str();

    /* GCS FIX: Need registration config */

    /* Output image */
    parms.labeling_output_fn = output_image.c_str();

    /* Process mabs */
    Mabs mabs;
    mabs.run (parms);

    return EXIT_SUCCESS;
}
