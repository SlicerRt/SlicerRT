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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// .NAME vtkSlicerDicomRtExportReader -
// .SECTION Description
// This class manages the Reader associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerDicomRtExportReader_h
#define __vtkSlicerDicomRtExportReader_h

#include "vtkSlicerDicomRtImportExportModuleLogicExport.h"

#include "vtkObject.h"

#include "rt_study.h"

class vtkPolyData;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO #210: investigate why the wrapping fails
//BTX

/// \ingroup SlicerRt_QtModules_DicomRtExport
class VTK_SLICER_DICOMRTIMPORTEXPORT_LOGIC_EXPORT vtkSlicerDicomRtWriter :  public vtkObject
{
public:
  static vtkSlicerDicomRtWriter *New();
  vtkTypeMacro(vtkSlicerDicomRtWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set anatomical image to Plastimatch RT study for export
  void SetImage(const Plm_image::Pointer&);

  /// Set dose distribution image to Plastimatch RT study for export
  void SetDose(const Plm_image::Pointer&);

  /// Add structure as image data to Plastimatch RT study for export
  void AddStructure(UCharImageType::Pointer, const char* name, double* color);
  
  /// Add empty structure for direct polyline format to Plastimatch RT study for export.
  /// The three argument vectors contain the slice numbers, UIDs and contours, and need to
  /// contain the same number of elements.
  void AddStructure(const char *name, double *color,
                    std::vector<int> sliceNumbers,
                    std::vector<std::string> sliceUIDs,
                    std::vector<vtkPolyData*> sliceContours);
  /// Add 

  /// TODO: Description, argument names and descriptions
  void Write();

public:
  /// Get the DICOM Patient Name
  vtkGetStringMacro(PatientName);
  /// Set the DICOM Patient Name
  vtkSetStringMacro(PatientName);
  /// Get the DICOM Patient ID
  vtkGetStringMacro(PatientID);
  /// Set the DICOM Patient ID
  vtkSetStringMacro(PatientID);
  /// Get the DICOM Patient Sex
  vtkGetStringMacro(PatientSex);
  /// Set the DICOM Patient Sex
  vtkSetStringMacro(PatientSex);
  /// Get the DICOM Study Date
  vtkGetStringMacro(StudyDate);
  /// Set the DICOM Study Date
  vtkSetStringMacro(StudyDate);
  /// Get the DICOM Study Time
  vtkGetStringMacro(StudyTime);
  /// Set the DICOM Study Time
  vtkSetStringMacro(StudyTime);
  /// Get the DICOM Study Description
  vtkGetStringMacro(StudyDescription);
  /// Set the DICOM Study Description
  vtkSetStringMacro(StudyDescription);
  /// Get the DICOM Study Instance UID
  vtkGetStringMacro(StudyInstanceUid);
  /// Set the DICOM Study Instance UID
  vtkSetStringMacro(StudyInstanceUid);
  /// Get the DICOM Study ID
  vtkGetStringMacro(StudyID);
  /// Set the DICOM Study ID
  vtkSetStringMacro(StudyID);

  /// Get the DICOM Image Series Description 
  vtkGetStringMacro(ImageSeriesDescription);
  /// Set the DICOM Image Series Description 
  vtkSetStringMacro(ImageSeriesDescription);
  /// Get the DICOM Image Series Number
  vtkGetStringMacro(ImageSeriesNumber);
  /// Set the DICOM Image Series Number
  vtkSetStringMacro(ImageSeriesNumber);
  /// Get the DICOM Image Modality
  vtkGetStringMacro(ImageSeriesModality);
  /// Set the DICOM Image Modality
  vtkSetStringMacro(ImageSeriesModality);
  /// Get the DICOM Dose Series Description 
  vtkGetStringMacro(DoseSeriesDescription);
  /// Set the DICOM Dose Series Description 
  vtkSetStringMacro(DoseSeriesDescription);
  /// Get the DICOM Dose Series Number
  vtkGetStringMacro(DoseSeriesNumber);
  /// Set the DICOM Dose Series Number
  vtkSetStringMacro(DoseSeriesNumber);
  /// Get the DICOM Rtss Series Description 
  vtkGetStringMacro(RtssSeriesDescription);
  /// Set the DICOM Rtss Series Description 
  vtkSetStringMacro(RtssSeriesDescription);
  /// Get the DICOM Rtss Series Number
  vtkGetStringMacro(RtssSeriesNumber);
  /// Set the DICOM Rtss Series Number
  vtkSetStringMacro(RtssSeriesNumber);
  
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  
protected:
  std::string formatColorString (const double *color);
  vtkSlicerDicomRtWriter();
  virtual ~vtkSlicerDicomRtWriter();

  // Parameters that get written into DICOM header
  char* PatientName;
  char* PatientID;
  char* PatientSex;
  char* StudyDate;
  char* StudyTime;
  char* StudyDescription;
  char* StudyInstanceUid;
  char* StudyID;

  char* ImageSeriesDescription;
  char* ImageSeriesNumber;
  char* ImageSeriesModality;
  char* DoseSeriesDescription;
  char* DoseSeriesNumber;
  char* RtssSeriesDescription;
  char* RtssSeriesNumber;

  /// Output directory
  char* FileName;

  /// Plastimatch RT study structure
  Rt_study RtStudy;

private:
  vtkSlicerDicomRtWriter(const vtkSlicerDicomRtWriter&); // Not implemented
  void operator=(const vtkSlicerDicomRtWriter&);               // Not implemented
};

//ETX

#endif
