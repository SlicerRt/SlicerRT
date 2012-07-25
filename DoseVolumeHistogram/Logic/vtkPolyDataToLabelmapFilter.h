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

// .NAME vtkPolyDataToLabelmapFilter - Converts PolyData model to Labelmap image data
// .SECTION Description
// !!! Copied from ModelToLabelMap CLI module !!! TODO: Make the DVH plugin use that module and depend on it instead of having this class (or both use this class)


#ifndef __vtkPolyDataToLabelmapFilter_h
#define __vtkPolyDataToLabelmapFilter_h

// VTK includes
#include <vtkPolyData.h>
#include <vtkImageData.h>

// STD includes
#include <cstdlib>

#include "vtkSlicerDoseVolumeHistogramModuleLogicExport.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DOSEVOLUMEHISTOGRAM_LOGIC_EXPORT vtkPolyDataToLabelmapFilter :
  public vtkObject
{
public:

  static vtkPolyDataToLabelmapFilter *New();
  vtkTypeMacro(vtkPolyDataToLabelmapFilter, vtkObject );
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetReferenceImage(vtkImageData* reference);
  virtual vtkImageData* GetOutput();

  virtual void Update();

  vtkSetObjectMacro(InputPolyData, vtkPolyData);

  vtkGetMacro(LabelValue, unsigned short);
  vtkSetMacro(LabelValue, unsigned short);

  vtkGetMacro(BackgroundValue, double);
  vtkSetMacro(BackgroundValue, double);

  vtkGetMacro(UseReferenceValues, bool);
  vtkSetMacro(UseReferenceValues, bool);
  vtkBooleanMacro(UseReferenceValues, bool);

protected:
  vtkSetObjectMacro(OutputLabelmap, vtkImageData);
  vtkSetObjectMacro(ReferenceImageData, vtkImageData);

protected:
  vtkPolyData* InputPolyData;
  vtkImageData* OutputLabelmap;
  vtkImageData* ReferenceImageData;
  unsigned short LabelValue;
  double BackgroundValue;
  bool UseReferenceValues;

protected:
  vtkPolyDataToLabelmapFilter();
  virtual ~vtkPolyDataToLabelmapFilter();

private:

  vtkPolyDataToLabelmapFilter(const vtkPolyDataToLabelmapFilter&); // Not implemented
  void operator=(const vtkPolyDataToLabelmapFilter&);               // Not implemented
};

#endif
