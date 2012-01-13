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

// .NAME vtkSlicerDicomRtImportLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtImportLogic_h
#define __vtkSlicerDicomRtImportLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes
#include "vtkMRMLDisplayableNode.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerDicomRtImportModuleLogicExport.h"

class vtkStringArray;
class vtkSlicerVolumesLogic;
class vtkMRMLDisplayableNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_DICOMRTIMPORT_MODULE_LOGIC_EXPORT vtkSlicerDicomRtImportLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerDicomRtImportLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtImportLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Initialize listening to MRML events
  void InitializeEventListeners();

  /// Load DICOM RT series from file name
  /// /return True if loading successful (it contained RT)
  bool LoadDicomRT(const char* filename);

  /// Create new mrml node and associated storage node.
  vtkMRMLDisplayableNode* AddArchetypeDICOMObject(const char *filename, const char* name);

  /// Set Volumes module logic
  void SetVolumesLogic(vtkSlicerVolumesLogic* volumesLogic);

protected:
  vtkSlicerDicomRtImportLogic();
  virtual ~vtkSlicerDicomRtImportLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  /// Add an ROI point to the scene
  vtkMRMLDisplayableNode* AddRoiPoint(double *roiPosition, const char* roiLabel, double *roiColor);

  /// Add an ROI contour to the scene
  vtkMRMLDisplayableNode* AddRoiContour(vtkPolyData *roiPoly, const char* roiLabel, double *roiColor);

private:

  vtkSlicerDicomRtImportLogic(const vtkSlicerDicomRtImportLogic&); // Not implemented
  void operator=(const vtkSlicerDicomRtImportLogic&);               // Not implemented

private:
  vtkSlicerVolumesLogic* VolumesLogic;
};

#endif
