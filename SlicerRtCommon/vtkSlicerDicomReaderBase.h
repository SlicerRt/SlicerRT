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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported by CANARIE.

==============================================================================*/

#ifndef __vtkSlicerDicomReaderBase_h
#define __vtkSlicerDicomReaderBase_h

#include "vtkSlicerRtCommonWin32Header.h"

// VTK includes
#include <vtkObject.h>

/// \ingroup SlicerRt_SlicerRtCommon
class VTK_SLICERRTCOMMON_EXPORT vtkSlicerDicomReaderBase : public vtkObject
{
public:
  static const std::string DICOMREADER_DICOM_DATABASE_FILENAME;
  static const std::string DICOMREADER_DICOM_CONNECTION_NAME;

public:
  static vtkSlicerDicomReaderBase *New();
  vtkTypeMacro(vtkSlicerDicomReaderBase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set input file name
  vtkSetStringMacro(FileName);

  /// Get patient name
  vtkGetStringMacro(PatientName);
  /// Get patient ID
  vtkGetStringMacro(PatientId);
  /// Get patient sex
  vtkGetStringMacro(PatientSex);
  /// Get patient birth date
  vtkGetStringMacro(PatientBirthDate);
  /// Get patient comments
  vtkGetStringMacro(PatientComments);
  /// Get study instance UID
  vtkGetStringMacro(StudyInstanceUid);
  /// Get study ID
  vtkGetStringMacro(StudyId);
  /// Get study description
  vtkGetStringMacro(StudyDescription);
  /// Get study date
  vtkGetStringMacro(StudyDate);
  /// Get study time
  vtkGetStringMacro(StudyTime);
  /// Get series instance UID
  vtkGetStringMacro(SeriesInstanceUid);
  /// Get series description
  vtkGetStringMacro(SeriesDescription);
  /// Get series modality
  vtkGetStringMacro(SeriesModality);
  /// Get series number
  vtkGetStringMacro(SeriesNumber);

  /// Get DICOM database file name
  vtkGetStringMacro(DatabaseFile);

protected:
  /// Set patient name
  vtkSetStringMacro(PatientName);
  /// Set patient ID
  vtkSetStringMacro(PatientId);
  /// Set patient sex
  vtkSetStringMacro(PatientSex);
  /// Set patient birth date
  vtkSetStringMacro(PatientBirthDate);
  /// Set patient comments
  vtkSetStringMacro(PatientComments);
  /// Set study instance UID
  vtkSetStringMacro(StudyInstanceUid);
  /// Set study ID
  vtkSetStringMacro(StudyId);
  /// Set study description
  vtkSetStringMacro(StudyDescription);
  /// Set study date
  vtkSetStringMacro(StudyDate);
  /// Set study time
  vtkSetStringMacro(StudyTime);
  /// Set series instance UID
  vtkSetStringMacro(SeriesInstanceUid);
  /// Set series description
  vtkSetStringMacro(SeriesDescription);
  /// Set series modality
  vtkSetStringMacro(SeriesModality);
  /// Set series number
  vtkSetStringMacro(SeriesNumber);

  /// Set DICOM database file name
  vtkSetStringMacro(DatabaseFile);

protected:
  template<class T> void GetAndStoreRtHierarchyInformation(T* dcmtkRtIodObject);

  template<class T> void GetAndStorePatientInformation(T* dcmtkIodObject);
  template<class T> void GetAndStoreStudyInformation(T* dcmtkIodObject);
  template<class T> void GetAndStoreSeriesInformation(T* dcmtkIodObject);

protected:
  /// Input file name
  char* FileName;

  /// Patient name
  char* PatientName;

  /// Patient ID
  char* PatientId;

  /// Patient sex
  char* PatientSex;

  /// Patient birth date
  char* PatientBirthDate;

  /// Patient comments
  char* PatientComments;

  /// Study instance UID
  char* StudyInstanceUid;

  /// Study ID
  char* StudyId;

  /// Study description
  char* StudyDescription;

  /// Study date
  char* StudyDate;

  /// Study time
  char* StudyTime;

  /// Series instance UID
  char* SeriesInstanceUid;

  /// Series description
  char* SeriesDescription;

  /// Series modality
  char* SeriesModality;

  /// Series number
  char* SeriesNumber;

  /// DICOM database file name
  char* DatabaseFile;

protected:
  vtkSlicerDicomReaderBase();
  ~vtkSlicerDicomReaderBase() override;

private:
  vtkSlicerDicomReaderBase(const vtkSlicerDicomReaderBase&) = delete;
  void operator=(const vtkSlicerDicomReaderBase&) = delete;
};

#include "vtkSlicerDicomReaderBase.txx"

#endif
