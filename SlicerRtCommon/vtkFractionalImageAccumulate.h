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

  This file is a modified version of vtkImageAccumulate.h from VTK

==============================================================================*/

#ifndef __vtkFractionalImageAccumulate_h
#define __vtkFractionalImageAccumulate_h

#include "vtkSlicerRtCommonWin32Header.h"

#include <vtkImageAccumulate.h>
#include <vtkImageData.h>

class VTK_SLICERRTCOMMON_EXPORT vtkFractionalImageAccumulate: public vtkImageAccumulate
{
public:
  static vtkFractionalImageAccumulate* New();
  vtkTypeMacro(vtkFractionalImageAccumulate, vtkImageAccumulate);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(MinimumFractionalValue, double);
  vtkSetMacro(MaximumFractionalValue, double);
  vtkGetMacro(MinimumFractionalValue, double);
  vtkGetMacro(MaximumFractionalValue, double);
  vtkSetMacro(FractionalLabelmap, vtkImageData*);
  vtkGetMacro(FractionalLabelmap, vtkImageData*);
  vtkGetMacro(FractionalVoxelCount, double);
  vtkSetMacro(UseFractionalLabelmap, bool);
  vtkGetMacro(UseFractionalLabelmap, bool);
  vtkBooleanMacro(UseFractionalLabelmap, bool);
    
protected:
  vtkFractionalImageAccumulate();
  ~vtkFractionalImageAccumulate() override;

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  int RequestInformation (
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector);

protected:
  double MinimumFractionalValue;
  double MaximumFractionalValue;
  vtkImageData* FractionalLabelmap;
  double FractionalVoxelCount;
  bool UseFractionalLabelmap;

private:
  vtkFractionalImageAccumulate(const vtkFractionalImageAccumulate&);  // Not implemented.
  void operator=(const vtkFractionalImageAccumulate&);  // Not implemented.
};

#endif // __vtkFractionalImageAccumulate_h
