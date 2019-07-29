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

// SlicerRtCommon includes
#include "vtkSlicerDicomReaderBase.h"
#include "vtkSlicerRtCommon.h"

// STD includes
#include <array>
#include <vector>
#include <map>

// DCMTK includes
#include <dcmtk/config/osconfig.h>    /* make sure OS specific configuration is included first */

#include <dcmtk/ofstd/ofconapp.h>

//----------------------------------------------------------------------------
const std::string vtkSlicerDicomReaderBase::DICOMREADER_DICOM_DATABASE_FILENAME = "/ctkDICOM.sql";
const std::string vtkSlicerDicomReaderBase::DICOMREADER_DICOM_CONNECTION_NAME = "SlicerRt";

vtkStandardNewMacro(vtkSlicerDicomReaderBase);

//----------------------------------------------------------------------------
// vtkSlicerDicomReaderBase methods

//----------------------------------------------------------------------------
vtkSlicerDicomReaderBase::vtkSlicerDicomReaderBase()
{
  this->FileName = nullptr;

  this->PatientName = nullptr;
  this->PatientId = nullptr;
  this->PatientSex = nullptr;
  this->PatientBirthDate = nullptr;
  this->PatientComments = nullptr;
  this->StudyInstanceUid = nullptr;
  this->StudyId = nullptr;
  this->StudyDescription = nullptr;
  this->StudyDate = nullptr;
  this->StudyTime = nullptr;
  this->SeriesInstanceUid = nullptr;
  this->SeriesDescription = nullptr;
  this->SeriesModality = nullptr;
  this->SeriesNumber = nullptr;
  this->SOPInstanceUID = nullptr;

  this->DatabaseFile = nullptr;
}

//----------------------------------------------------------------------------
vtkSlicerDicomReaderBase::~vtkSlicerDicomReaderBase()
= default;

//----------------------------------------------------------------------------
void vtkSlicerDicomReaderBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
