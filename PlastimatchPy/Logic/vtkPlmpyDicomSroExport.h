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
#ifndef __vtkPlmpyDicomSroExport_h
#define __vtkPlmpyDicomSroExport_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// PlastimatchPy includes
#include "vtkSlicerPlastimatchPyModuleLogicExport.h"

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
#include "vf_jacobian.h"

class VTK_SLICER_PLASTIMATCHPY_MODULE_LOGIC_EXPORT vtkPlmpyDicomSroExport :
  public vtkSlicerModuleLogic
{
public:
  static vtkPlmpyDicomSroExport *New();
  vtkTypeMacro(vtkPlmpyDicomSroExport, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  vtkSetStringMacro(FixedImageID);
  vtkGetStringMacro(FixedImageID);
  vtkSetStringMacro(MovingImageID);
  vtkGetStringMacro(MovingImageID);
  vtkSetStringMacro(XformID);
  vtkGetStringMacro(XformID);
  vtkSetStringMacro(OutputDirectory);
  vtkGetStringMacro(OutputDirectory);

  void set_mrml_scene_hack(vtkMRMLScene * newScene);
  void DoExport ();

protected:
  char* FixedImageID;
  char* MovingImageID;
  char* XformID;
  char* OutputDirectory;

protected:
  vtkPlmpyDicomSroExport();
  virtual ~vtkPlmpyDicomSroExport();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();

private:
  vtkPlmpyDicomSroExport(const vtkPlmpyDicomSroExport&);
  void operator=(const vtkPlmpyDicomSroExport&);
};

#endif
