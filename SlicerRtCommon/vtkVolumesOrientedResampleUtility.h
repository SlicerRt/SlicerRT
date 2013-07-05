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

  This file was originally developed by Kevin Wang, RMP, Princess Margaret Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkVolumesOrientedResampleUtility - Performs image resampling according to a reference volume
// .SECTION Description


#ifndef __vtkVolumesOrientedResampleUtility_h
#define __vtkVolumesOrientedResampleUtility_h

// MRML includes
#include <vtkMRMLVolumeNode.h>

// ITK includes
#include "itkImage.h"

// STD includes
#include <cstdlib>

/// \ingroup SlicerRt_SlicerRtCommon
class vtkVolumesOrientedResampleUtility : public vtkObject
{
public:
  // TODO Do we need a real class or should it be more a namespace for static functions?
  // TODO or do we put the static functions in VolumesLogic class?
  static vtkVolumesOrientedResampleUtility *New();
  vtkTypeMacro(vtkVolumesOrientedResampleUtility, vtkObject );
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Perform MRML volume --> ITK image conversion
  /// Note: Supports only 3-dimensional ITK images
  /// \param inVolumeNode Input MRML volume node that needs to be converted
  /// \param outItkVolume Output ITK image that contains the same data with the same lattice as the input MRML volume
  static bool ResampleInputVolumeNodeToReferenceVolumeNode(vtkMRMLVolumeNode*, 
                                                           vtkMRMLVolumeNode*,
                                                           vtkMRMLVolumeNode*);

protected:
  vtkVolumesOrientedResampleUtility();
  virtual ~vtkVolumesOrientedResampleUtility();

private:
  vtkVolumesOrientedResampleUtility(const vtkVolumesOrientedResampleUtility&); // Not implemented
  void operator=(const vtkVolumesOrientedResampleUtility&);               // Not implemented
};

#endif // __vtkVolumesOrientedResampleUtility_h