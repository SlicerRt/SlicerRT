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

// .NAME vtkSlicerDicomSroImportModuleLogic - slicer logic class for SRO importing
// .SECTION Description
// This class manages the logic associated with reading Dicom spatial registration object


#ifndef __vtkSlicerDicomSroImportLogic_h
#define __vtkSlicerDicomSroImportLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <vector>

#include "vtkSlicerDicomSroImportModuleLogicExport.h"

class vtkDICOMImportInfo;
class vtkMatrix4x4;
class vtkSlicerDicomSroReader;

/// \ingroup SlicerRt_DicomSroImportLogic
class VTK_SLICER_DICOMSROIMPORT_MODULE_LOGIC_EXPORT vtkSlicerDicomSroImportModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerDicomSroImportModuleLogic *New();
  vtkTypeMacro(vtkSlicerDicomSroImportModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Examine a list of file lists and determine what objects can be loaded from them
  void Examine(vtkDICOMImportInfo *importInfo);

  /// Load DICOM Sro series from file name
  /// \return True if loading successful
  bool LoadDicomSro(vtkDICOMImportInfo *loadInfo);

protected:
  vtkSlicerDicomSroImportModuleLogic();
  virtual ~vtkSlicerDicomSroImportModuleLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /// Load Dicom spatial registration objects into the MRML scene
  /// \return Success flag
  bool LoadSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo* loadInfo);

  /// Load Dicom spatial fiducial objects into the MRML scene
  /// \return Success flag
  bool LoadSpatialFiducials(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo* loadInfo);

  /// Load Dicom deformable spatial registration objects into the MRML scene
  /// \return Success flag
  bool LoadDeformableSpatialRegistration(vtkSlicerDicomSroReader* regReader, vtkDICOMImportInfo* loadInfo);

private:
  vtkSlicerDicomSroImportModuleLogic(const vtkSlicerDicomSroImportModuleLogic&); // Not implemented
  void operator=(const vtkSlicerDicomSroImportModuleLogic&);              // Not implemented
};

#endif
