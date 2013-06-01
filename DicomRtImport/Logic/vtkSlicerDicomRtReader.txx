/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

#include <dcmtk/ofstd/ofstring.h>

//----------------------------------------------------------------------------
template<typename T> void vtkSlicerDicomRtReader::GetAndStoreHierarchyInformation(T* dcmtkIodObject)
{
  OFString patientName;
  if (dcmtkIodObject->getPatientName(patientName).good())
  {
    this->SetPatientName(patientName.c_str());
  }
  else
  {
    this->SetPatientName(NULL);
  }

  OFString patientId;
  if (dcmtkIodObject->getPatientID(patientId).good())
  {
    this->SetPatientId(patientId.c_str());
  }
  else
  {
    this->SetPatientId(NULL);
  }

  OFString patientSex;
  if (dcmtkIodObject->getPatientSex(patientSex).good())
  {
    this->SetPatientSex(patientSex.c_str());
  }
  else
  {
    this->SetPatientSex(NULL);
  }

  OFString patientBirthDate;
  if (dcmtkIodObject->getPatientBirthDate(patientBirthDate).good())
  {
    this->SetPatientBirthDate(patientBirthDate.c_str());
  }
  else
  {
    this->SetPatientBirthDate(NULL);
  }

  OFString studyInstanceUid;
  if (dcmtkIodObject->getStudyInstanceUID(studyInstanceUid).good())
  {
    this->SetStudyInstanceUid(studyInstanceUid.c_str());
  }
  else
  {
    this->SetStudyInstanceUid(NULL);
  }

  OFString studyDescription;
  if (dcmtkIodObject->getStudyDescription(studyDescription).good())
  {
    this->SetStudyDescription(studyDescription.c_str());
  }
  else
  {
    this->SetStudyDescription(NULL);
  }

  OFString studyDate;
  if (dcmtkIodObject->getStudyDate(studyDate).good())
  {
    this->SetStudyDate(studyDate.c_str());
  }
  else
  {
    this->SetStudyDate(NULL);
  }

  OFString studyTime;
  if (dcmtkIodObject->getStudyTime(studyTime).good())
  {
    this->SetStudyTime(studyTime.c_str());
  }
  else
  {
    this->SetStudyTime(NULL);
  }

  OFString seriesInstanceUid;
  if (dcmtkIodObject->getSeriesInstanceUID(seriesInstanceUid).good())
  {
    this->SetSeriesInstanceUid(seriesInstanceUid.c_str());
  }
  else
  {
    this->SetSeriesInstanceUid(NULL);
  }

  OFString seriesDescription;
  if (dcmtkIodObject->getSeriesDescription(seriesDescription).good())
  {
    this->SetSeriesDescription(seriesDescription.c_str());
  }
  else
  {
    this->SetSeriesDescription(NULL);
  }

  OFString seriesModality;
  if (dcmtkIodObject->getModality(seriesModality).good())
  {
    this->SetSeriesModality(seriesModality.c_str());
  }
  else
  {
    this->SetSeriesModality(NULL);
  }
}
