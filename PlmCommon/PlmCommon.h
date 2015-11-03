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

#ifndef __PlmCommon_h
#define __PlmCommon_h

// STD includes
#include <cstdlib>
#include <string>

// ITK includes
#include "itkImage.h"

// Plastimatch includes
#include "plm_image.h"

#include "vtkPlmCommonWin32Header.h"

class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;

class vtkOrientedImageData;

/// \ingroup SlicerRt_PlmCommon
class VTK_PLMCOMMON_EXPORT PlmCommon
{
  //----------------------------------------------------------------------------
  // Utility functions
  //----------------------------------------------------------------------------
public:
  /// Convert MRML volume node to Plm image using typed scalar volume node
  /// \param inVolumeNode Scalar volume node to convert
  /// \param applyWorldTransform Flag determining if parent transform is applied to volume node when converting to Plm image. True by default
  static Plm_image::Pointer ConvertVolumeNodeToPlmImage(vtkMRMLScalarVolumeNode* inVolumeNode, bool applyWorldTransform = true);

  /// Convert MRML volume node to Plm image using generic MRML node type
  /// \param inNode Node to convert (must be scalar volume node type)
  /// \param applyWorldTransform Flag determining if parent transform is applied to volume node when converting to Plm image. True by default
  static Plm_image::Pointer ConvertVolumeNodeToPlmImage(vtkMRMLNode* inNode, bool applyWorldTransform = true);

  /// Convert VTK oriented image data to Plm image
  static Plm_image::Pointer ConvertVtkOrientedImageDataToPlmImage(vtkOrientedImageData* inImageData);
};

#endif
