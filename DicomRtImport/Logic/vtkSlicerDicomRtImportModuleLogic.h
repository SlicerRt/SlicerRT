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

// STD includes
#include <cstdlib>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkMRMLDisplayableNode;
class vtkDICOMImportInfo;
class vtkPolyData;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTIMPORT_LOGIC_EXPORT vtkSlicerDicomRtImportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomRtImportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtImportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Initialize listening to MRML events
  void InitializeEventListeners();

  /// Examine a list of file lists and determine what objects can be loaded from them
  void Examine(vtkDICOMImportInfo *importInfo);

  /// Load DICOM RT series from file name
  /// /return True if loading successful
  bool LoadDicomRT(const char *filename, const char* seriesname);

  /// Set Volumes module logic
  void SetVolumesLogic(vtkSlicerVolumesLogic* volumesLogic);

protected:
  vtkSlicerDicomRtImportModuleLogic();
  virtual ~vtkSlicerDicomRtImportModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Add an ROI point to the scene
  vtkMRMLDisplayableNode* AddRoiPoint(double *roiPosition, std::string baseName, double *roiColor);

  /// Add an ROI contour to the scene
  vtkMRMLDisplayableNode* AddRoiContour(vtkPolyData *roiPoly, std::string baseName, double *roiColor);

private:

  vtkSlicerDicomRtImportModuleLogic(const vtkSlicerDicomRtImportModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDicomRtImportModuleLogic&);              // Not implemented

private:
  vtkSlicerVolumesLogic* VolumesLogic;
};

#endif
