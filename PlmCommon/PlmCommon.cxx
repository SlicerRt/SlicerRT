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

#include "PlmCommon.h"

/// This workaround prevents a compilation fail with ITK-5.3
/// Something defines POSIX in preprocessor, and it conflicts
/// with ITK KWSys POSIX enumeration constant
#if (ITK_VERSION_MAJOR == 5) && (ITK_VERSION_MINOR > 2) && defined(POSIX)
#define PLMPOSIX_TMP (POSIX)
#undef POSIX
#endif

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

#ifdef PLMPOSIX_TMP
#define POSIX (PLMPOSIX_TMP)
#undef PLMPOSIX_TMP
#endif

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// Segmentations includes
#include "vtkOrientedImageData.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------
template<class T> 
static typename itk::Image<T,3>::Pointer
convert_to_itk (vtkMRMLScalarVolumeNode* inVolumeNode, bool applyWorldTransform)
{
  typename itk::Image<T,3>::Pointer image = itk::Image<T,3>::New ();
  if (!vtkSlicerRtCommon::ConvertVolumeNodeToItkImage<T>(inVolumeNode, image, applyWorldTransform, true))
  {
    vtkGenericWarningMacro("PlmCommon::convert_to_itk(vtkMRMLScalarVolumeNode): Failed to convert volume node to PlmImage!");
  }
  return image;
}

//----------------------------------------------------------------------------
template<class T> 
static typename itk::Image<T,3>::Pointer
convert_to_itk (vtkOrientedImageData* inImageData)
{
  typename itk::Image<T,3>::Pointer image = itk::Image<T,3>::New ();
  if (!vtkSlicerRtCommon::ConvertVtkOrientedImageDataToItkImage<T>(inImageData, image, true))
  {
    vtkGenericWarningMacro("PlmCommon::convert_to_itk(vtkOrientedImageData): Failed to convert oriented image data to PlmImage!");
  }
  return image;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
Plm_image::Pointer 
PlmCommon::ConvertVolumeNodeToPlmImage(vtkMRMLScalarVolumeNode* inVolumeNode, bool applyWorldTransform/* = true*/)
{
  Plm_image::Pointer image = Plm_image::New ();

  if (!inVolumeNode || !inVolumeNode->GetImageData())
  {
    vtkGenericWarningMacro("PlmCommon::ConvertVolumeNodeToPlmImage: Invalid input volume node!");
    return image;
  }

  vtkImageData* inVolume = inVolumeNode->GetImageData();
  int vtk_type = inVolume->GetScalarType ();

  switch (vtk_type) {
  case VTK_CHAR:
  case VTK_SIGNED_CHAR:
    image->set_itk (convert_to_itk<char> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_CHAR:
    image->set_itk (convert_to_itk<unsigned char> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_SHORT:
    image->set_itk (convert_to_itk<short> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_SHORT:
    image->set_itk (convert_to_itk<unsigned short> (inVolumeNode, applyWorldTransform));
    break;
  
#if (CMAKE_SIZEOF_UINT == 4)
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<int> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned int> (inVolumeNode, applyWorldTransform));
    break;
#else
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<long> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned long> (inVolumeNode, applyWorldTransform));
    break;
#endif
  
  case VTK_FLOAT:
    image->set_itk (convert_to_itk<float> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_DOUBLE:
    image->set_itk (convert_to_itk<double> (inVolumeNode, applyWorldTransform));
    break;

  default:
    vtkWarningWithObjectMacro (inVolumeNode, "Unsupported scalar type!");
    break;
  }

  return image;
}

//----------------------------------------------------------------------------
Plm_image::Pointer 
PlmCommon::ConvertVolumeNodeToPlmImage(vtkMRMLNode* inNode, bool applyWorldTransform/* = true*/)
{
  return PlmCommon::ConvertVolumeNodeToPlmImage(
    vtkMRMLScalarVolumeNode::SafeDownCast(inNode), applyWorldTransform);
}

//----------------------------------------------------------------------------
Plm_image::Pointer 
PlmCommon::ConvertVtkOrientedImageDataToPlmImage(vtkOrientedImageData* inImageData)
{
  Plm_image::Pointer image = Plm_image::New ();

  if (!inImageData)
  {
    vtkGenericWarningMacro("PlmCommon::ConvertVtkOrientedImageDataToPlmImage: Invalid input image data!");
    return image;
  }

  int vtk_type = inImageData->GetScalarType ();

  switch (vtk_type) {
  case VTK_CHAR:
  case VTK_SIGNED_CHAR:
    image->set_itk (convert_to_itk<char> (inImageData));
    break;
  
  case VTK_UNSIGNED_CHAR:
    image->set_itk (convert_to_itk<unsigned char> (inImageData));
    break;
  
  case VTK_SHORT:
    image->set_itk (convert_to_itk<short> (inImageData));
    break;
  
  case VTK_UNSIGNED_SHORT:
    image->set_itk (convert_to_itk<unsigned short> (inImageData));
    break;
  
#if (CMAKE_SIZEOF_UINT == 4)
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<int> (inImageData));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned int> (inImageData));
    break;
#else
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<long> (inImageData));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned long> (inImageData));
    break;
#endif
  
  case VTK_FLOAT:
    image->set_itk (convert_to_itk<float> (inImageData));
    break;
  
  case VTK_DOUBLE:
    image->set_itk (convert_to_itk<double> (inImageData));
    break;

  default:
    vtkWarningWithObjectMacro (inImageData, "Unsupported scalar type!");
    break;
  }

  return image;
}
