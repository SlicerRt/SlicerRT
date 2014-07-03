/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkSlicerPlastimatchPyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes

#ifndef __plmpyVectorFieldAnalysis_h
#define __plmpyVectorFieldAnalysis_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// PlastimatchPy includes
#include "vtkSlicerPlastimatchPyModuleLogicExport.h"

// ITK includes
#include "itkImage.h"

// Plastimatch includes
#include "landmark_warp.h"
#include "plm_config.h"
#include "plm_image.h"
#include "pointset.h"
#include "registration_data.h"
#include "registration_parms.h"
#include "vf_jacobian.h"

/// Class to wrap Plastimatch registration capability into the embedded Python shell in Slicer
class VTK_SLICER_PLASTIMATCHPY_MODULE_LOGIC_EXPORT vtkPlmpyVectorFieldAnalysis :
  public vtkSlicerModuleLogic
{
  typedef itk::Vector< float, 3 >  VectorType;
  typedef itk::Image< VectorType, 3 >  DeformationFieldType;

public:
  /// Constructor
  static vtkPlmpyVectorFieldAnalysis* New();
  vtkTypeMacro(vtkPlmpyVectorFieldAnalysis, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Compute Jacobian
  void RunJacobian();

public:
  /// Set the ID of the fixed image (\sa FixedImageID) (image data type must be "float").
  vtkSetStringMacro(FixedImageID);
  /// Get the ID of the fixed image (\sa FixedImageID) (image data type must be "float").
  vtkGetStringMacro(FixedImageID);

  /// Set the ID of the output image (\sa OutputVolumeID).
  vtkSetStringMacro(OutputVolumeID);
  /// Get the ID of the output image (\sa OutputVolumeID).
  vtkGetStringMacro(OutputVolumeID);


// NSh, Jacobian
  /// GetMin/MaxJacobian
  vtkGetStringMacro(JacobianMinString);
  vtkGetStringMacro(JacobianMaxString); 
  /// Set the ID of the vector field
  vtkSetStringMacro(VFImageID);
  /// Get the ID of the vector field
  vtkGetStringMacro(VFImageID);

protected:
  vtkPlmpyVectorFieldAnalysis();
  virtual ~vtkPlmpyVectorFieldanalysis();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

protected:
  /// ID of the fixed image
  /// This value is a required parameter to execute a registration.
  char* FixedImageID;

  /// ID of the registered image
  /// This value is a required parameter to execute a registration.
  char* OutputVolumeID;

  /// Vector filed computed by Plastimatch
  DeformationFieldType::Pointer MovingImageToFixedImageVectorField;

  /// NSh: Jacobian module - return values for Slicer GUI
  float jacobian_min;
  float jacobian_max;
  char* JacobianMinString;
  char* JacobianMaxString;
  /// ID of the vector field image to calculate the Jacobian of
  char* VFImageID;

private:
  vtkSlicerPlastimatchPyModuleLogic(const vtkSlicerPlastimatchPyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerPlastimatchPyModuleLogic&);            // Not implemented
};

#endif
