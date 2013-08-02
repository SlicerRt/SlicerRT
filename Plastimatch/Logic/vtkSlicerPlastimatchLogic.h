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

// .NAME vtkSlicerPlastimatchLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes

#ifndef __vtkSlicerPlastimatchLogic_h
#define __vtkSlicerPlastimatchLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// Plastimatch Module Logic
#include "vtkSlicerPlastimatchModuleLogicExport.h"

// STD includes
#include <cstdlib>

// ITK includes
#include "itkImage.h"

// VTK includes
#include <vtkPoints.h>

// Plastimatch includes
#include "landmark_warp.h"
#include "plm_config.h"
#include "plm_image.h"
#include "plm_stages.h"
#include "pointset.h"
#include "registration_data.h"
#include "registration_parms.h"

/// Class to wrap Plastimatch registration capability into the embedded Python shell in Slicer
class VTK_SLICER_PLASTIMATCH_MODULE_LOGIC_EXPORT vtkSlicerPlastimatchLogic :
  public vtkSlicerModuleLogic
{
  typedef itk::Vector< float, 3 >  VectorType;
  typedef itk::Image< VectorType, 3 >  DeformationFieldType;

public:
  /// Create a new object
  static vtkSlicerPlastimatchLogic* New();

  //
  vtkTypeMacro(vtkSlicerPlastimatchLogic, vtkSlicerModuleLogic);
  
  //
  void PrintSelf(ostream& os, vtkIndent indent);
  
  /// Add a registration stage in the Plastimatch workflow
  void AddStage();

  /// Set parameter in stage
  void SetPar(char* key, char* value);

  /// Execute registration
  void RunRegistration();

public:
  // Description:
  // Set/get the ID of the fixed image (image data type must be "float").
  // This value is a required parameter to execute a registration.
  vtkSetStringMacro(FixedID);
  vtkGetStringMacro(FixedID);

  // Description:
  // Set/get the ID of the moving image (image data type must be "float").
  // This value is a required parameter to execute a registration.
  vtkSetStringMacro(MovingID);
  vtkGetStringMacro(MovingID);
  
  // Description:
  // Set/get the fixed landmarks using a vtkPoints object.
  // The number of the fixed landmarks must be the same of the number of the moving landmarks.
  // Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  // Is not possible mix landmarks from vtkPoints and from files.
  // If no fcsv files are passing this value is a required parameter to execute a landmark based registration.
  vtkSetMacro(FixedLandmarks, vtkPoints*);
  vtkGetMacro(FixedLandmarks, vtkPoints*);

  // Description:
  // Set/get the moving landmarks using a vtkPoints object.
  // The number of the moving landmarks must be the same of the number of fixed landmarks.
  // Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  // Is not possible mix landmarks from vtkPoints and from files.
  // If no fcsv files are passing this value is a required parameter to execute a landmark based registration.
  vtkSetMacro(MovingLandmarks, vtkPoints*);
  vtkGetMacro(MovingLandmarks, vtkPoints*);

  // Description:
  // Set/get the fcsv file name containing the fixed landmarks.
  // The number of the fixed landmarks must be the same of the number of the moving landmarks.
  // Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  // Is not possible mix landmarks from vtkPoints and from files.
  // If no vtkPoints objects are passing this value is a required parameter to execute a landmark based registration.
  vtkSetStringMacro(FixedLandmarksFileName);
  vtkGetStringMacro(FixedLandmarksFileName);

  // Description:
  // Set/get the fcsv file name containing the moving landmarks.
  // The number of the moving landmarks must be the same of the number of the fixed landmarks.
  // Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  // Is not possible mix landmarks from vtkPoints and from files.
  // If no vtkPoints objects are passing this value is a required parameter to execute a landmark based registration.
  vtkSetStringMacro(MovingLandmarksFileName);
  vtkGetStringMacro(MovingLandmarksFileName);

  // Description:
  // Set/Get the warped landmarks using a vtkPoints object.
  // This value is a required parameter to execute a landmark based registration.
  vtkGetMacro(WarpedLandmarks, vtkPoints*);
  vtkSetMacro(WarpedLandmarks, vtkPoints*);
  
  // Description:
  // Set/get the ID of a precomputed rigid/affine transformation.
  // This transformation will be used as initialization for the Plastimatch registration.
  // This value is an optional parameter to execute a registration.
  vtkSetStringMacro(InputTransformationID);
  vtkGetStringMacro(InputTransformationID);

  // Description:
  // Set/Get the ID of the output image.
  // This value is a required parameter to execute a registration.
  vtkSetStringMacro(OutputVolumeID);
  vtkGetStringMacro(OutputVolumeID);
 
protected:
  vtkSlicerPlastimatchLogic();
  virtual ~vtkSlicerPlastimatchLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

private:
  //
  vtkSlicerPlastimatchLogic(const vtkSlicerPlastimatchLogic&); // Not implemented
  
  //
  void operator=(const vtkSlicerPlastimatchLogic&);              // Not implemented

  /// This function sets the vtkPoints as input landmarks for Plastimatch registration
  void SetLandmarksFromSlicer();

  /// This function reads the fcsv files containing the landmarks and sets them as input landmarks for Plastimatch registration
  void SetLandmarksFromFiles();

  /// This function applies an initial affine trasformation modifing the moving image before the Plastimatch registration
  void ApplyInitialLinearTransformation();

  /// This function applies a linear/deformable transformation at an image.
  /// It is used from ApplyInitialLinearTransformation() and RunRegistration().
  void ApplyWarp(
    Plm_image* warpedImage,                        /*!< Output image as Plm_image pointer */
    DeformationFieldType::Pointer vectorFieldOut, /*!< Output vector field (optional) as DeformationFieldType::Pointer */
    Xform* inputTransformation,                    /*!< Input transformation as Xform pointer */
    Plm_image* fixedImage,                         /*!< Fixed image as Plm_image pointer */
    Plm_image* inputImage,                         /*!< Input image to warp as Plm_image pointer */
    float defaultValue,                            /*!< Value (float) for pixels without match */
    int useItk,                                    /*!< Int to choose between itk (1) or Plastimatch (0) algorithm for the warp task */
    int interpolationLinear                        /*!< Int to choose between trilinear interpolation (1) on nearest neighbor (0) */
    );

  /// This function shows the output image into the Slicer scene
  void GetOutputImage();

  /// This function warps the landmarks according to OutputTransformation
  void WarpLandmarks();

private:
  
  /// ID of the fixed image
  char* FixedID;

  /// ID of the moving image
  char* MovingID;

  /// vtkPoints object containing the fixed landmarks
  vtkPoints* FixedLandmarks;
  
  /// vtkPoints object containing the moving landmarks
  vtkPoints* MovingLandmarks;
  
  /// Name of the fcsv containing the fixed landmarks
  char* FixedLandmarksFileName;
  
  /// Name of the fcsv containing the moving landmarks
  char* MovingLandmarksFileName;
  
  /// vtkPoints object containing the warped landmarks
  vtkPoints* WarpedLandmarks;
  
  /// Plastimatch registration parameters
  Registration_parms* RegistrationParameters;

  /// Plastimatch registration data
  Registration_data* RegistrationData;
  
  /// ID of the affine registration used as initialization for the Plastimatch registration
  char* InputTransformationID;

  /// Initial affine transformation
  Xform* InputTransformation;

  /// Transformation (linear or deformable) computed by Plastimatch
  Xform* OutputTransformation;

  /// Vector filed computed by Plastimatch
  DeformationFieldType::Pointer OutputVectorField;

  /// Image deformed by Plastimatch
  Plm_image* WarpedImage;
  
  /// ID of the registered image
  char* OutputVolumeID;
};

#endif
