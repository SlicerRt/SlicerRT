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

// STD includes
#include <cstdlib>

#include "vtkSlicerPlastimatchModuleLogicExport.h"

// Plastimatch includes 
#include "plm_config.h"
#include "plm_image.h"
#include "plm_stages.h"
#include "pointset.h"
#include "registration_data.h"
#include "registration_parms.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLASTIMATCH_MODULE_LOGIC_EXPORT vtkSlicerPlastimatchLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPlastimatchLogic *New();
  vtkTypeMacro(vtkSlicerPlastimatchLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  void AddStage();
  void SetPar(char* key, char* val);
  void RunRegistration();

public:
  vtkSetStringMacro(FixedId);
  vtkGetStringMacro(FixedId);
  vtkSetStringMacro(MovingId);
  vtkGetStringMacro(MovingId);

  vtkSetStringMacro(FixedLandmarksFn);
  vtkGetStringMacro(FixedLandmarksFn);
  vtkSetStringMacro(MovingLandmarksFn);
  vtkGetStringMacro(MovingLandmarksFn);

  vtkSetStringMacro(OutputImageName);
  vtkGetStringMacro(OutputImageName);
 
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
  vtkSlicerPlastimatchLogic(const vtkSlicerPlastimatchLogic&); // Not implemented
  void operator=(const vtkSlicerPlastimatchLogic&);               // Not implemented
  void ApplyWarp(Plm_image *WarpedImg,   /* Output: Output image */
    Xform * XfIn,          /* Input:  Input image warped by this xform */
    Plm_image * FixedImg,   /* Input:  Size of output image */
    Plm_image * InImg,       /* Input:  Input image */
    float DefaultVal,     /* Input:  Value for pixels without match */
    int UseItk,           /* Input:  Force use of itk (1) or not (0) */
    int InterpLin);
  void GetOutputImg(char* PublicOutputImageName);

private:
  Registration_parms *regp;
  Registration_data *regd;
  Xform* XfOut;
  char* FixedId;
  char* MovingId;
  Plm_image * WarpedImg;
  char* FixedLandmarksFn;
  Labeled_pointset* FixedLandmarks;
  char* MovingLandmarksFn;
  Labeled_pointset* MovingLandmarks;
  char* OutputImageName;
};

#endif
