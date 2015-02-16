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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkCalculateOversamplingFactor - Calculate oversampling factor based on contour properties
// .SECTION Description

#ifndef __vtkCalculateOversamplingFactor_h
#define __vtkCalculateOversamplingFactor_h

// VTK includes
#include "vtkObject.h"

// Contours includes
#include "vtkMRMLContourNode.h"

#include "vtkSlicerContoursModuleMRMLExport.h"

class vtkMRMLModelNode;
class vtkMRMLScalarVolumeNode;

/// \ingroup SlicerRt_QtModules_Contours
/// \brief Calculate oversampling factor based on contour properties using fuzzy logics
class VTK_SLICER_CONTOURS_MODULE_MRML_EXPORT vtkCalculateOversamplingFactor : public vtkObject
{
public:
  static vtkCalculateOversamplingFactor *New();
  vtkTypeMacro(vtkCalculateOversamplingFactor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Calculate oversampling factor for the input contour and its rasterization reference volume
  /// based on contour properties using fuzzy logics.
  bool CalculateOversamplingFactor();

public:
  vtkGetObjectMacro(ContourNode, vtkMRMLContourNode);
  vtkSetObjectMacro(ContourNode, vtkMRMLContourNode);

  vtkGetMacro(OutputOversamplingFactor, int);
  vtkSetMacro(OutputOversamplingFactor, int);

  vtkGetMacro(LogSpeedMeasurements, bool);
  vtkSetMacro(LogSpeedMeasurements, bool);
  vtkBooleanMacro(LogSpeedMeasurements, bool);

protected:
  /// Input contour node
  vtkMRMLContourNode* ContourNode;

  /// Calculated oversampling factor for the contour node and its reference volume
  int OutputOversamplingFactor;

  /// Flag telling whether the speed measurements are logged on standard output
  bool LogSpeedMeasurements;
  
protected:
  vtkCalculateOversamplingFactor();
  virtual ~vtkCalculateOversamplingFactor();

private:
  vtkCalculateOversamplingFactor(const vtkCalculateOversamplingFactor&); // Not implemented
  void operator=(const vtkCalculateOversamplingFactor&);               // Not implemented
//ETX
};

#endif 