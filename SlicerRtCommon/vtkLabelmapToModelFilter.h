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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkLabelmapToModelFilter - Converts PolyData model to Labelmap image data
// .SECTION Description


#ifndef __vtkLabelmapToModelFilter_h
#define __vtkLabelmapToModelFilter_h

// VTK includes
#include <vtkPolyData.h>
#include <vtkImageData.h>

// STD includes
#include <cstdlib>

/// \ingroup SlicerRt_SlicerRtCommon
class vtkLabelmapToModelFilter : public vtkObject
{
public:

  static vtkLabelmapToModelFilter *New();
  vtkTypeMacro(vtkLabelmapToModelFilter, vtkObject );
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkPolyData* GetOutput();

  virtual void Update();

  vtkSetObjectMacro(InputLabelmap, vtkImageData);

  vtkGetMacro(DecimateTargetReduction, double);
  vtkSetMacro(DecimateTargetReduction, double);

  vtkGetMacro(LabelValue, double);
  vtkSetMacro(LabelValue, double);

protected:
  vtkSetObjectMacro(OutputModel, vtkPolyData);

protected:
  vtkImageData* InputLabelmap;
  vtkPolyData* OutputModel;
  double DecimateTargetReduction;
  /// Use this value for the marching cubes
  double LabelValue;

protected:
  vtkLabelmapToModelFilter();
  virtual ~vtkLabelmapToModelFilter();

private:
  vtkLabelmapToModelFilter(const vtkLabelmapToModelFilter&); // Not implemented
  void operator=(const vtkLabelmapToModelFilter&);               // Not implemented
};

#endif 