/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/
#include "PlmCommon.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// SlicerRT includes
#include "SlicerRtCommon.h"

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------
template<class T> 
typename itk::Image<T,3>::Pointer
convert_to_itk (vtkMRMLScalarVolumeNode* inVolumeNode)
{
  typename itk::Image<T,3>::Pointer image = itk::Image<T,3>::New ();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<T>(inVolumeNode, image, true);
  return image;
}

Plm_image::Pointer 
PlmCommon::ConvertVolumeNodeToPlmImage(vtkMRMLScalarVolumeNode* inVolumeNode)
{
  Plm_image::Pointer image = Plm_image::New ();

  vtkImageData* inVolume = inVolumeNode->GetImageData();
  int vtk_type = inVolume->GetScalarType ();

  switch (vtk_type) {
  case VTK_CHAR:
  case VTK_SIGNED_CHAR:
    image->set_itk (convert_to_itk<char> (inVolumeNode));
    break;
  
  case VTK_UNSIGNED_CHAR:
    image->set_itk (convert_to_itk<unsigned char> (inVolumeNode));
    break;
  
  case VTK_SHORT:
    image->set_itk (convert_to_itk<short> (inVolumeNode));
    break;
  
  case VTK_UNSIGNED_SHORT:
    image->set_itk (convert_to_itk<unsigned short> (inVolumeNode));
    break;
  
#if (CMAKE_SIZEOF_UINT == 4)
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<int> (inVolumeNode));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned int> (inVolumeNode));
    break;
#else
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<long> (inVolumeNode));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned long> (inVolumeNode));
    break;
#endif
  
  case VTK_FLOAT:
    image->set_itk (convert_to_itk<float> (inVolumeNode));
    break;
  
  case VTK_DOUBLE:
    image->set_itk (convert_to_itk<double> (inVolumeNode));
    break;

  default:
    vtkWarningWithObjectMacro (inVolumeNode, "Got something else\n");
    break;
  }

  return image;
}

Plm_image::Pointer 
PlmCommon::ConvertVolumeNodeToPlmImage(vtkMRMLNode* inNode)
{
  return PlmCommon::ConvertVolumeNodeToPlmImage(
    vtkMRMLScalarVolumeNode::SafeDownCast(inNode));
}
