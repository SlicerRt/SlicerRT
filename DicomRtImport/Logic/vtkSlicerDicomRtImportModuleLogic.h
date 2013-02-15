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

// .NAME vtkSlicerDicomRtImportModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtImportLogic_h
#define __vtkSlicerDicomRtImportLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerVolumesLogic.h"

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkMRMLDisplayableNode;
class vtkDICOMImportInfo;
class vtkPolyData;
class vtkSlicerDicomRtReader;

/// \ingroup SlicerRt_DicomRtImportLogic
class VTK_SLICER_DICOMRTIMPORT_LOGIC_EXPORT vtkSlicerDicomRtImportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomRtImportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtImportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Examine a list of file lists and determine what objects can be loaded from them
  void Examine(vtkDICOMImportInfo *importInfo);

  /// Load DICOM RT series from file name
  /// /return True if loading successful
  bool LoadDicomRT(vtkDICOMImportInfo *loadInfo);

  /// Set Volumes module logic
  void SetVolumesLogic(vtkSlicerVolumesLogic* volumesLogic);

public:
  vtkSetMacro(AutoContourOpacity, bool);
  vtkGetMacro(AutoContourOpacity, bool);
  vtkBooleanMacro(AutoContourOpacity, bool);

protected:
  vtkSlicerDicomRtImportModuleLogic();
  virtual ~vtkSlicerDicomRtImportModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /// Load RT Structure Set and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtStructureSet(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo);

  /// Add an ROI point to the scene
  vtkMRMLDisplayableNode* AddRoiPoint(double *roiPosition, std::string baseName, double *roiColor);

  /// Add an ROI contour to the scene
  vtkMRMLDisplayableNode* AddRoiContour(vtkPolyData *roiPoly, std::string baseName, double *roiColor);

  /// Load RT Dose and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtDose(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo);

  /// Load RT Plan and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtPlan(vtkSlicerDicomRtReader* rtReader, vtkDICOMImportInfo *loadInfo);

  /// Insert currently loaded series in the proper place in patient hierarchy
  void InsertSeriesInPatientHierarchy(vtkSlicerDicomRtReader* rtReader);

private:
  vtkSlicerDicomRtImportModuleLogic(const vtkSlicerDicomRtImportModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDicomRtImportModuleLogic&);              // Not implemented

private:
  vtkSlicerVolumesLogic* VolumesLogic;

  /// Flag indicating whether opacity values for the loaded contours are automatically determined
  bool AutoContourOpacity;
};

#endif
