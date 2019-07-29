/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre
  and was supported by Cancer Care Ontario (CCO)'s ACRU program
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/


// .NAME vtkSlicerDicomSroReader -
// .SECTION Description
// This class manages the Reader associated with reading DICOM Spatial Registration Object
// The reader load DICOM SRO in LPS and convert it to RAS coordinate system mainly due to
// it is used for Slicer.

#ifndef __vtkSlicerDicomSroReader_h
#define __vtkSlicerDicomSroReader_h

// SlicerRtCommon includes
#include "vtkSlicerDicomReaderBase.h"

#include "vtkSlicerDicomSroImportExportModuleLogicExport.h"

class DcmDataset;
class vtkMatrix4x4;
class vtkImageData;

/// \ingroup SlicerRt_DicomSroImportExport
class VTK_SLICER_DICOMSROIMPORTEXPORT_MODULE_LOGIC_EXPORT vtkSlicerDicomSroReader : public vtkSlicerDicomReaderBase
{
public:
  static vtkSlicerDicomSroReader *New();
  vtkTypeMacro(vtkSlicerDicomSroReader, vtkSlicerDicomReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Do reading
  void Update();

  /// Get number of referenced series UIDs of the loaded registration object
  int GetNumberOfReferencedSeriesUids();
  /// Get referenced series UID with given index
  std::string GetReferencedSeriesUid(int index);

public:
  /// Get spatial registration matrix
  vtkGetObjectMacro(SpatialRegistrationMatrix, vtkMatrix4x4);

  /// Get post deformation registration matrix
  vtkGetObjectMacro(PostDeformationRegistrationMatrix, vtkMatrix4x4);

  /// Get deformable registration grid (displacement vector field)
  vtkGetObjectMacro(DeformableRegistrationGrid, vtkImageData);

  /// Get deformable registration grid orientation matrix
  vtkGetObjectMacro(DeformableRegistrationGridOrientationMatrix, vtkMatrix4x4);


  /// Get load spatial registration successful flag
  vtkGetMacro(LoadSpatialRegistrationSuccessful, bool);

  /// Get load spatial fiducials successful flag
  vtkGetMacro(LoadSpatialFiducialsSuccessful, bool);

  /// Get load deformable spatial registration successful flag
  vtkGetMacro(LoadDeformableSpatialRegistrationSuccessful, bool);

protected:
  /// Load spatial registration
  void LoadSpatialRegistration(DcmDataset*);

  /// Load deformable spatial registration
  void LoadDeformableSpatialRegistration(DcmDataset*);

  /// Load spatial fiducials
  /// NOTE: Not implemented yet
  void LoadSpatialFiducials(DcmDataset*);

protected:
  /// Spatial registration matrix
  vtkMatrix4x4* SpatialRegistrationMatrix;

  /// Post deformation registration matrix
  vtkMatrix4x4* PostDeformationRegistrationMatrix;

  /// Deformable registration grid
  vtkImageData* DeformableRegistrationGrid;

  /// Deformable registration grid orientation matrix
  vtkMatrix4x4* DeformableRegistrationGridOrientationMatrix;


  /// Flag indicating if spatial registration object has been successfully read from the input dataset
  bool LoadSpatialRegistrationSuccessful;

  /// Flag indicating if spatial fiducial object has been successfully read from the input dataset
  bool LoadSpatialFiducialsSuccessful;

  /// Flag indicating if deformable spatial registration object has been successfully read from the input dataset
  bool LoadDeformableSpatialRegistrationSuccessful;

protected:
  vtkSlicerDicomSroReader();
  ~vtkSlicerDicomSroReader();

private:
  vtkSlicerDicomSroReader(const vtkSlicerDicomSroReader&) = delete;
  void operator=(const vtkSlicerDicomSroReader&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
