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
#ifndef __plmpyDicomSroExport_h
#define __plmpyDicomSroExport_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// PlastimatchPy includes
#include "vtkSlicerPlastimatchPyModuleLogicExport.h"

class VTK_SLICER_PLASTIMATCHPY_MODULE_LOGIC_EXPORT plmpyDicomSroExport 
  : public vtkSlicerModuleLogic
{
public:
  static plmpyDicomSroExport *New();
  vtkTypeMacro(plmpyDicomSroExport,vtkSlicerModuleLogic);
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

  void DoExport ();

protected:
  char* FixedImageID;
  char* MovingImageID;
  char* XformID;
  char* OutputDirectory;

protected:
  plmpyDicomSroExport();
  ~plmpyDicomSroExport();

private:
  plmpyDicomSroExport(const plmpyDicomSroExport&);
  void operator=(const plmpyDicomSroExport&);
};

#endif
