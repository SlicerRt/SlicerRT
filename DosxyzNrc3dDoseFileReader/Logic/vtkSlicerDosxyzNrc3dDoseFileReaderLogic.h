/*==========================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Anna Ilina, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario.

==========================================================================*/

#ifndef __vtkSlicerDosxyzNrc3dDoseFileReaderLogic_h
#define __vtkSlicerDosxyzNrc3dDoseFileReaderLogic_h

#define MAX_TOLERANCE_SPACING 0.01

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <vector>

// DosxyzNrc3dDoseFileReader includes
#include "vtkSlicerDosxyzNrc3dDoseFileReaderLogicExport.h"

class vtkMRMLScalarVolumeNode;
class vtkMRMLScalarVolumeDisplayNode;
class vtkMRMLVolumeHeaderlessStorageNode;
class vtkStringArray;

/// \ingroup SlicerRt_QtModules_DosxyzNrc3dDoseFileReader
class VTK_SLICER_DOSXYZNRC3DDOSEFILEREADER_LOGIC_EXPORT vtkSlicerDosxyzNrc3dDoseFileReaderLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDosxyzNrc3dDoseFileReaderLogic *New();
  vtkTypeMacro(vtkSlicerDosxyzNrc3dDoseFileReaderLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Load DosxyzNrc3dDose volume from file
  /// \param filename Path and filename of the DosxyzNrc3dDose file
  void LoadDosxyzNrc3dDoseFile(char* filename, float intensityScalingFactor=1.0);

  /// Determine if two numbers are equal within a small tolerance (0.001)
  static bool AreEqualWithTolerance(double a, double b);

protected:
  vtkSlicerDosxyzNrc3dDoseFileReaderLogic();
  ~vtkSlicerDosxyzNrc3dDoseFileReaderLogic() override;

private:
  vtkSlicerDosxyzNrc3dDoseFileReaderLogic(const vtkSlicerDosxyzNrc3dDoseFileReaderLogic&);  // Not implemented
  void operator=(const vtkSlicerDosxyzNrc3dDoseFileReaderLogic&);  // Not implemented
};

#endif