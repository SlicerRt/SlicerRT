/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada and
  Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerDicomRtImportExportModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtImportExportModuleLogic_h
#define __vtkSlicerDicomRtImportExportModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerDicomRtImportExportModuleLogicExport.h"

class vtkSlicerVolumesLogic;
class vtkSlicerIsodoseModuleLogic;
class vtkSlicerPlanarImageModuleLogic;
class vtkMRMLScalarVolumeNode;
class vtkMRMLMarkupsFiducialNode;
class vtkMRMLSegmentationNode;
class vtkSlicerDICOMLoadable;
class vtkSlicerDICOMExportable;
class vtkStringArray;
class vtkPolyData;
class vtkSlicerDicomRtReader;
class vtkCollection;

/// \ingroup SlicerRt_QtModules_DicomRtImport
class VTK_SLICER_DICOMRTIMPORTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtImportExportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomRtImportExportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtImportExportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Examine a list of file lists and determine what objects can be loaded from them
  /// \param fileList List of files to examine and generate loadables from
  /// \param loadables Collection to store generated (output) loadables
  void ExamineForLoad(vtkStringArray* fileList, vtkCollection* loadables);

  /// Load DICOM RT series from file name
  /// /return True if loading successful
  bool LoadDicomRT(vtkSlicerDICOMLoadable* loadable);

  /// Set Volumes module logic
  void SetVolumesLogic(vtkSlicerVolumesLogic* volumesLogic);

  /// Set Isodose module logic
  void SetIsodoseLogic(vtkSlicerIsodoseModuleLogic* isodoseLogic);

  /// Set Planar Image module logic
  void SetPlanarImageLogic(vtkSlicerPlanarImageModuleLogic* planarImage);

  /// Export RT study (list of RT exportables) to DICOM files
  /// \return Error message, empty string if success
  std::string ExportDicomRTStudy(vtkCollection* exportables);

  /// Get referenced volume for a segmentation according to subject hierarchy attributes
  /// \return The reference volume for the segmentation if any, NULL otherwise
  static vtkMRMLScalarVolumeNode* GetReferencedVolumeByDicomForSegmentation(vtkMRMLSegmentationNode* segmentationNode);

public:
  vtkSetMacro(BeamModelsInSeparateBranch, bool);
  vtkGetMacro(BeamModelsInSeparateBranch, bool);
  vtkBooleanMacro(BeamModelsInSeparateBranch, bool);

  vtkGetStringMacro(DefaultDoseColorTableNodeId);
  vtkSetStringMacro(DefaultDoseColorTableNodeId);

protected:
  vtkSlicerDicomRtImportExportModuleLogic();
  virtual ~vtkSlicerDicomRtImportExportModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void OnMRMLSceneEndClose();

protected:
  /// Load RT Structure Set and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtStructureSet(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable);

  /// Add an ROI point to the scene
  vtkMRMLMarkupsFiducialNode* AddRoiPoint(double* roiPosition, std::string baseName, double* roiColor);

  /// Load RT Dose and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtDose(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable);

  /// Load RT Plan and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtPlan(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable);

  /// Load RT Image and related objects into the MRML scene
  /// \return Success flag
  bool LoadRtImage(vtkSlicerDicomRtReader* rtReader, vtkSlicerDICOMLoadable* loadable);

  /// Insert currently loaded series in the proper place in subject hierarchy
  void InsertSeriesInSubjectHierarchy(vtkSlicerDicomRtReader* rtReader);

  /// Creates default dose color table.
  /// Should not be called, except when updating the default dose color table file manually, or when the file cannot be found (\sa LoadDefaultDoseColorTable)
  void CreateDefaultDoseColorTable();

  /// Compute and set geometry of an RT image
  /// \param node Either the volume node of the loaded RT image, or the isocenter fiducial node (corresponding to an RT image). This function is called both when
  ///    loading an RT image and when loading a beam. Sets up the RT image geometry only if both information (the image itself and the isocenter data) are available
  void SetupRtImageGeometry(vtkMRMLNode* node);

private:
  vtkSlicerDicomRtImportExportModuleLogic(const vtkSlicerDicomRtImportExportModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDicomRtImportExportModuleLogic&);              // Not implemented

private:
  /// Volumes logic instance
  vtkSlicerVolumesLogic* VolumesLogic;

  /// Isodose logic instance
  vtkSlicerIsodoseModuleLogic* IsodoseLogic;

  /// Planar Image logic instance
  vtkSlicerPlanarImageModuleLogic* PlanarImageLogic;

  /// Flag determining whether the generated beam models are arranged in a separate subject hierarchy
  /// branch, or each beam model is added under its corresponding isocenter fiducial
  bool BeamModelsInSeparateBranch;

  /// Default dose color table ID. Loaded on Slicer startup.
  char* DefaultDoseColorTableNodeId;
};

#endif
