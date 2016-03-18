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

#include <string>
#include <vector>

// DicomRtExport includes
#include "vtkSlicerDicomRtWriter.h"

// VTK includes
#include <vtkNew.h>
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

// ITK includes
#include "itkImage.h"

// DCMTK includes
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmrt/drtdose.h"
#include "dcmtk/dcmrt/drtimage.h"
#include "dcmtk/dcmrt/drtplan.h"
#include "dcmtk/dcmrt/drtstrct.h"
#include "dcmtk/dcmrt/drttreat.h"
#include "dcmtk/dcmrt/drtionpl.h"
#include "dcmtk/dcmrt/drtiontr.h"

// Plastimatch includes
#include "rt_study.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtWriter);

//----------------------------------------------------------------------------
vtkSlicerDicomRtWriter::vtkSlicerDicomRtWriter()
{
  this->PatientName = NULL;
  this->PatientID = NULL;
  this->PatientSex = NULL;
  this->StudyDate = NULL;
  this->StudyTime = NULL;
  this->StudyDescription = NULL;
  this->StudyInstanceUid = NULL;
  this->StudyID = NULL;

  this->ImageSeriesDescription = NULL;
  this->ImageSeriesNumber = 1;

  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtWriter::~vtkSlicerDicomRtWriter()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::SetImage(const Plm_image::Pointer& img)
{
  RtStudy.set_image(img);
}
  
//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::SetDose(const Plm_image::Pointer& img)
{
  RtStudy.set_dose(img);
}
  
//----------------------------------------------------------------------------
std::string vtkSlicerDicomRtWriter::formatColorString (const double *color)
{
  std::string colorString = "";
  std::ostringstream strs;
  int value[3];

  for (int i = 0; i < 3; i++) {
    value[i] = (int) (color[i] * 255 + 0.5);
    if (value[i] > 255) value[i] = 255;
    if (value[i] < 0) value[i] = 0;
  }
  
  strs << value[0] << " " << value[1] << " " << value[2];
  colorString = strs.str();
  return colorString;
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::AddStructure(UCharImageType::Pointer itk_structure, const char *name, double *color)
{
  std::string colorString = this->formatColorString(color);
  RtStudy.add_structure(itk_structure, name, colorString.c_str());
}
  
//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::Write()
{
  /* Set study metadata */
  Rt_study_metadata::Pointer& rt_metadata = RtStudy.get_rt_study_metadata ();
  if (this->PatientName && this->PatientName[0] != 0)
  {
    rt_metadata->set_study_metadata (0x0010, 0x0010, this->PatientName);
  }
  if (this->PatientID && this->PatientID[0] != 0)
  {
    rt_metadata->set_study_metadata (0x0010, 0x0020, this->PatientID);
  }
  if (this->PatientSex && this->PatientSex[0] != 0)
  {
    rt_metadata->set_study_metadata (0x0010, 0x0040, this->PatientSex);
  }
  if (this->StudyDescription && this->StudyDescription[0] != 0)
  {
    rt_metadata->set_study_metadata (0x0008, 0x1030, this->StudyDescription);
  }
  if (this->StudyDate && this->StudyDate[0] != 0)
  {
    rt_metadata->set_study_date (this->StudyDate);
  }
  if (this->StudyTime && this->StudyTime[0] != 0)
  {
    rt_metadata->set_study_time (this->StudyTime);
  }
  if (this->StudyInstanceUid && this->StudyInstanceUid[0] != 0)
  {
    rt_metadata->set_study_uid (this->StudyInstanceUid);
  }
  if (this->StudyID && this->StudyID[0] != 0)
  {
    rt_metadata->set_study_metadata (0x0020, 0x0010, this->StudyID);
  }
  
  /* Set image, dose, rtstruct metadata */

  /* Write output to files */
  RtStudy.save_dicom(this->FileName);
}
