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
  University Health Network and Csaba Pinter, PerkLab, Queen's University and
  Andras Lasso, PerkLab, Queen's University, and was supported by Cancer Care
  Ontario (CCO)'s ACRU program with funds provided by the Ontario Ministry of
  Health and Long-Term Care and Ontario Consortium for Adaptive Interventions in
  Radiation Oncology (OCAIRO).

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

class vtkCollection;
class vtkMRMLScalarVolumeNode;
class vtkMRMLScene;
class vtkMRMLSegmentationNode;
class vtkSlicerBeamsModuleLogic;
class vtkSlicerDICOMLoadable;
class vtkSlicerDicomReaderBase;
class vtkSlicerIsodoseModuleLogic;
class vtkSlicerPlanarImageModuleLogic;
class vtkStringArray;

/// \ingroup SlicerRt_QtModules_DicomRtImport
class VTK_SLICER_DICOMRTIMPORTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtImportExportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomRtImportExportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomRtImportExportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Examine a list of file lists and determine what objects can be loaded from them
  /// \param fileList List of files to examine and generate loadables from
  /// \param loadables Collection to store generated (output) loadables
  void ExamineForLoad(vtkStringArray* fileList, vtkCollection* loadables);

  /// Load DICOM RT series from file name
  /// /return True if loading successful
  bool LoadDicomRT(vtkSlicerDICOMLoadable* loadable);

  /// Export RT study (list of RT exportables) to DICOM files
  /// \return Error message, empty string if success
  std::string ExportDicomRTStudy(vtkCollection* exportables);

  /// Get referenced volume for a segmentation according to subject hierarchy attributes
  /// \return The reference volume for the segmentation if any, nullptr otherwise
  static vtkMRMLScalarVolumeNode* GetReferencedVolumeByDicomForSegmentation(vtkMRMLSegmentationNode* segmentationNode);

  /// Insert currently loaded series in the proper place in subject hierarchy
  static void InsertSeriesInSubjectHierarchy(vtkSlicerDicomReaderBase* reader, vtkMRMLScene* scene);

public:
  /// Set Isodose module logic
  void SetIsodoseLogic(vtkSlicerIsodoseModuleLogic* isodoseLogic);
  /// Set Planar Image module logic
  void SetPlanarImageLogic(vtkSlicerPlanarImageModuleLogic* planarImageLogic);
  /// Set Beams module logic
  void SetBeamsLogic(vtkSlicerBeamsModuleLogic* beamsLogic);

public:
  vtkSetMacro(BeamModelsInSeparateBranch, bool);
  vtkGetMacro(BeamModelsInSeparateBranch, bool);
  vtkBooleanMacro(BeamModelsInSeparateBranch, bool);

protected:
  void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  void OnMRMLSceneEndClose() override;

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  void RegisterNodes() override;

protected:
  vtkSlicerDicomRtImportExportModuleLogic();
  ~vtkSlicerDicomRtImportExportModuleLogic() override;
  vtkSlicerDicomRtImportExportModuleLogic(const vtkSlicerDicomRtImportExportModuleLogic&) = delete;
  void operator=(const vtkSlicerDicomRtImportExportModuleLogic&) = delete;

private:
  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal; // For access from the callback function

private:
  /// Isodose logic instance
  vtkSlicerIsodoseModuleLogic* IsodoseLogic;

  /// Planar Image logic instance
  vtkSlicerPlanarImageModuleLogic* PlanarImageLogic;

  /// Beams module logic instance
  vtkSlicerBeamsModuleLogic* BeamsLogic;

  /// Flag determining whether the generated beam models are arranged in a separate subject hierarchy
  /// branch, or each beam model is added under its corresponding isocenter fiducial
  bool BeamModelsInSeparateBranch;
};

#endif
