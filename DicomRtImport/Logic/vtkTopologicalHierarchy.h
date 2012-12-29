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

// .NAME vtkTopologicalHierarchy - Assigns hierarchy level values to the elements of a poly data collection
// .SECTION Description


#ifndef __vtkTopologicalHierarchy_h
#define __vtkTopologicalHierarchy_h

// VTK includes
#include <vtkPolyDataCollection.h>

// STD includes
#include <cstdlib>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkIntArray;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTIMPORT_LOGIC_EXPORT vtkTopologicalHierarchy : public vtkObject
{
public:

  static vtkTopologicalHierarchy *New();
  vtkTypeMacro(vtkTopologicalHierarchy, vtkObject );
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkIntArray* GetOutput();

  virtual void Update();

  vtkSetObjectMacro(InputPolyDatas, vtkPolyDataCollection);

  vtkSetMacro(ContainConstraintFactor, double);
  vtkGetMacro(ContainConstraintFactor, double);

protected:
  vtkSetObjectMacro(OutputLevels, vtkIntArray);

protected:
  /// Determines if polyOut contains polyIn
  bool Contains(vtkPolyData* polyOut, vtkPolyData* polyIn);

  /// Determines if there are empty entries in the output level array
  bool OutputContainsEmptyLevels();

protected:
  /// Collection of poly datas to determine the hierarchy for
  vtkPolyDataCollection* InputPolyDatas;

  /// Array containing the levels for the input poly datas
  /// Update function needs to be called to compute the array
  /// The level values correspond to the poly data with the same index in the input collection
  vtkIntArray* OutputLevels;

  /// Constraint factor used when determining if a poly data contains another
  /// It defines a 'gap' that is needed between the outer and inner poly datas. The gap is computed
  /// as this factor multiplied by the bounding box length at each dimension.
  /// In case of positive value, the inner poly data has to be that much smaller than the outer one
  /// In case of negative value, it is rather an allowance by which the inner polydata can reach
  /// outside the other
  double ContainConstraintFactor;

  /// Maximum level that can be assigned to a poly data
  unsigned int MaximumLevel;

protected:
  vtkTopologicalHierarchy();
  virtual ~vtkTopologicalHierarchy();

private:
  vtkTopologicalHierarchy(const vtkTopologicalHierarchy&); // Not implemented
  void operator=(const vtkTopologicalHierarchy&);               // Not implemented
};

#endif 