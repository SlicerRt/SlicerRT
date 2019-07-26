/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// XXX These undefs are needed to avoid issue
// when building vtkSlicerDicomRtImportExportModuleLogicPythonD

// XXX Avoid  warning: "HAVE_XXXX" redefined
#undef HAVE_STAT
#undef HAVE_FTIME
#undef HAVE_GETPID
#undef HAVE_IO_H
#undef HAVE_STRERROR
#undef HAVE_SYS_UTIME_H
#undef HAVE_TEMPNAM
#undef HAVE_TMPNAM
#undef HAVE_LONG_LONG
// XXX Fix windows build error
#undef HAVE_INT64_T

#include <dcmtk/ofstd/ofstring.h>

//----------------------------------------------------------------------------
template<typename T> void vtkSlicerDicomReaderBase::GetAndStoreRtHierarchyInformation(T* dcmtkRtIodObject)
{
  this->GetAndStorePatientInformation(dcmtkRtIodObject);
  this->GetAndStoreStudyInformation(dcmtkRtIodObject);
  this->GetAndStoreSeriesInformation(dcmtkRtIodObject);

  // Patient comments is a field only supported by RT IODs
  OFString patientComments;
  if (dcmtkRtIodObject->getPatientComments(patientComments).good())
  {
    this->SetPatientComments(patientComments.c_str());
  }
  else
  {
    this->SetPatientComments(nullptr);
  }
}

//----------------------------------------------------------------------------
template<typename T> void vtkSlicerDicomReaderBase::GetAndStorePatientInformation(T* dcmtkIodObject)
{
  OFString patientName;
  if (dcmtkIodObject->getPatientName(patientName).good())
  {
    this->SetPatientName(patientName.c_str());
  }
  else
  {
    this->SetPatientName(nullptr);
  }

  OFString patientId;
  if (dcmtkIodObject->getPatientID(patientId).good())
  {
    this->SetPatientId(patientId.c_str());
  }
  else
  {
    this->SetPatientId(nullptr);
  }

  OFString patientSex;
  if (dcmtkIodObject->getPatientSex(patientSex).good())
  {
    this->SetPatientSex(patientSex.c_str());
  }
  else
  {
    this->SetPatientSex(nullptr);
  }

  OFString patientBirthDate;
  if (dcmtkIodObject->getPatientBirthDate(patientBirthDate).good())
  {
    this->SetPatientBirthDate(patientBirthDate.c_str());
  }
  else
  {
    this->SetPatientBirthDate(nullptr);
  }
}

//----------------------------------------------------------------------------
template<typename T> void vtkSlicerDicomReaderBase::GetAndStoreStudyInformation(T* dcmtkIodObject)
{
  OFString studyInstanceUid;
  if (dcmtkIodObject->getStudyInstanceUID(studyInstanceUid).good())
  {
    this->SetStudyInstanceUid(studyInstanceUid.c_str());
  }
  else
  {
    this->SetStudyInstanceUid(nullptr);
  }

  OFString studyId;
  if (dcmtkIodObject->getStudyID(studyId).good())
  {
    this->SetStudyId(studyId.c_str());
  }
  else
  {
    this->SetStudyId(nullptr);
  }

  OFString studyDescription;
  if (dcmtkIodObject->getStudyDescription(studyDescription).good())
  {
    this->SetStudyDescription(studyDescription.c_str());
  }
  else
  {
    this->SetStudyDescription(nullptr);
  }

  OFString studyDate;
  if (dcmtkIodObject->getStudyDate(studyDate).good())
  {
    this->SetStudyDate(studyDate.c_str());
  }
  else
  {
    this->SetStudyDate(nullptr);
  }

  OFString studyTime;
  if (dcmtkIodObject->getStudyTime(studyTime).good())
  {
    this->SetStudyTime(studyTime.c_str());
  }
  else
  {
    this->SetStudyTime(nullptr);
  }
}

//----------------------------------------------------------------------------
template<typename T> void vtkSlicerDicomReaderBase::GetAndStoreSeriesInformation(T* dcmtkIodObject)
{
  OFString seriesInstanceUid;
  if (dcmtkIodObject->getSeriesInstanceUID(seriesInstanceUid).good())
  {
    this->SetSeriesInstanceUid(seriesInstanceUid.c_str());
  }
  else
  {
    this->SetSeriesInstanceUid(nullptr);
  }

  OFString seriesDescription;
  if (dcmtkIodObject->getSeriesDescription(seriesDescription).good())
  {
    this->SetSeriesDescription(seriesDescription.c_str());
  }
  else
  {
    this->SetSeriesDescription(nullptr);
  }

  OFString seriesModality;
  if (dcmtkIodObject->getModality(seriesModality).good())
  {
    this->SetSeriesModality(seriesModality.c_str());
  }
  else
  {
    this->SetSeriesModality(nullptr);
  }

  OFString seriesNumber;
  if (dcmtkIodObject->getSeriesNumber(seriesNumber).good())
  {
    this->SetSeriesNumber(seriesNumber.c_str());
  }
  else
  {
    this->SetSeriesNumber(nullptr);
  }
}
