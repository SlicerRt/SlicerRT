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

#ifndef __vtkPlmpyDicomSroExport_h
#define __vtkPlmpyDicomSroExport_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerTransformLogic.h"

// PlastimatchPy includes
#include "vtkSlicerPlastimatchPyModuleLogicExport.h"

// VTK includes
#include <vtkWeakPointer.h>

class VTK_SLICER_PLASTIMATCHPY_MODULE_LOGIC_EXPORT vtkPlmpyDicomSroExport :
  public vtkSlicerModuleLogic
{
public:
  static vtkPlmpyDicomSroExport *New();
  vtkTypeMacro(vtkPlmpyDicomSroExport, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

public:
  vtkSetStringMacro(FixedImageID);
  vtkGetStringMacro(FixedImageID);
  vtkSetStringMacro(MovingImageID);
  vtkGetStringMacro(MovingImageID);
  vtkSetStringMacro(XformID);
  vtkGetStringMacro(XformID);
  vtkSetStringMacro(OutputDirectory);
  vtkGetStringMacro(OutputDirectory);
  vtkSetObjectMacro(TransformsLogic, vtkSlicerTransformLogic);

  /// Export DICOM SRO to file
  /// \return Success flag. 0 in case of success, non-0 otherwise
  int DoExport();

protected:
  char* FixedImageID;
  char* MovingImageID;
  char* XformID;
  char* OutputDirectory;

protected:
  vtkPlmpyDicomSroExport();
  ~vtkPlmpyDicomSroExport() override;

  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

  void UpdateFromMRMLScene() override;

private:
  vtkPlmpyDicomSroExport(const vtkPlmpyDicomSroExport&);
  void operator=(const vtkPlmpyDicomSroExport&);

private:
  /// Transforms module logic instance
  vtkWeakPointer<vtkSlicerTransformLogic> TransformsLogic;
};

#endif
