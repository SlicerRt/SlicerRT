/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

  This file is a modified version of vtkPolyDataToImageStencil.h from VTK

==============================================================================*/

#ifndef vtkFractionalPolyDataToImageStencil_h
#define vtkFractionalPolyDataToImageStencil_h

#include "vtkSlicerRtCommonWin32Header.h"
#include <vtkPolyDataToImageStencil.h>

class vtkMergePoints;
class vtkDataSet;
class vtkPolyData;

class VTK_SLICERRTCOMMON_EXPORT vtkFractionalPolyDataToImageStencil :
  public vtkPolyDataToImageStencil
{
public:
  static vtkFractionalPolyDataToImageStencil* New();
  vtkTypeMacro(vtkFractionalPolyDataToImageStencil, vtkPolyDataToImageStencil);

protected:
  vtkFractionalPolyDataToImageStencil();
  ~vtkFractionalPolyDataToImageStencil();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  void ThreadedExecute(vtkImageStencilData *output,
                               int extent[6], int threadId);

private:
  vtkFractionalPolyDataToImageStencil(const vtkFractionalPolyDataToImageStencil&);  // Not implemented.
  void operator=(const vtkFractionalPolyDataToImageStencil&);  // Not implemented.
};

#endif