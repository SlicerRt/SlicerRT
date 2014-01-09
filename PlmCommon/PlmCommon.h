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

class vtkMRMLTransformableNode;
class vtkMRMLNode;
class vtkMRMLScalarVolumeNode;
class vtkMRMLModelNode;
class vtkMRMLColorTableNode;

class vtkImageData;
class vtkGeneralTransform;
class vtkMatrix4x4;

/// \ingroup SlicerRt_PlmCommon
class VTK_PLMCOMMON_EXPORT PlmCommon
{
  //----------------------------------------------------------------------------
  // Utility functions
  //----------------------------------------------------------------------------
public:
  static Plm_image::Pointer CreatePlmImage(vtkMRMLScalarVolumeNode* inVolumeNode);

};

#endif
