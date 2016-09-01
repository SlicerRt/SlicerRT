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
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkCell.h>
#include <vtkPoints.h>

// ITK includes
#include "itkImage.h"

// DCMTK includes
#include "dcmtk/config/osconfig.h" // make sure OS specific configuration is included first
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
#include "segmentation.h"
#include "rtss_roi.h"
#include "rtss_contour.h"

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
  this->ImageSeriesNumber = NULL;
  this->ImageSeriesModality = NULL;

  this->DoseSeriesDescription = NULL;
  this->DoseSeriesNumber = NULL;

  this->RtssSeriesDescription = NULL;
  this->RtssSeriesNumber = NULL;

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
  this->RtStudy.set_image(img);
}
  
//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::SetDose(const Plm_image::Pointer& img)
{
  this->RtStudy.set_dose(img);
}
  
//----------------------------------------------------------------------------
std::string vtkSlicerDicomRtWriter::formatColorString(const double *color)
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

  this->RtStudy.add_structure(itk_structure, name, colorString.c_str());
}
  
//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::AddStructure(const char *name, double *color,
                                          std::vector<int> sliceNumbers,
                                          std::vector<std::string> sliceUIDs,
                                          std::vector<vtkPolyData*> sliceContours )
{
  if (sliceNumbers.size() != sliceUIDs.size() || sliceNumbers.size() != sliceContours.size())
  {
    vtkErrorMacro("AddStructure: Invalid contours arguments!");
    return;
  }

  std::string colorString = this->formatColorString(color);

  // Make sure there is a segmentation in the RT study
  Segmentation::Pointer segmentation;
  if (this->RtStudy.have_segmentation())
  {
    segmentation = this->RtStudy.get_segmentation();
  }
  else
  {
    segmentation = Segmentation::New();
    this->RtStudy.set_segmentation(segmentation);
  }
  Rtss_roi* roi = segmentation->add_rtss_roi(name, colorString.c_str());

  for (int contourIndex=0; contourIndex<sliceContours.size(); ++contourIndex)
  {
    int sliceNumber = sliceNumbers[contourIndex];
    std::string sliceUID = sliceUIDs[contourIndex];
    vtkPolyData* contourPolyData = sliceContours[contourIndex];
    for (int cellIndex=0; cellIndex<contourPolyData->GetNumberOfCells(); ++cellIndex)
    {
      vtkPoints* points = contourPolyData->GetCell(cellIndex)->GetPoints();
      Rtss_contour* contour = roi->add_polyline(points->GetNumberOfPoints());
      contour->slice_no = sliceNumber;
      contour->ct_slice_uid = sliceUID;

      for (int pointIndex=0; pointIndex<points->GetNumberOfPoints(); ++pointIndex)
      {
        double point[3] = {0.0,0.0,0.0};
        points->GetPoint(pointIndex, point);
        // RAS to LPS conversion
        contour->x[pointIndex] = point[0] * -1.0;
        contour->y[pointIndex] = point[1] * -1.0;
        contour->z[pointIndex] = point[2];
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::Write()
{
  // Set study metadata
  Rt_study_metadata::Pointer& rt_metadata = this->RtStudy.get_rt_study_metadata ();
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
  
  // Set image, dose, structures metadata
  if (this->ImageSeriesDescription && this->ImageSeriesDescription[0] != 0)
  {
    rt_metadata->set_image_metadata (0x0008, 0x103e, this->ImageSeriesDescription);
  }
  if (this->ImageSeriesNumber && this->ImageSeriesNumber[0] != 0)
  {
    rt_metadata->set_image_metadata (0x0020, 0x0011, this->ImageSeriesNumber);
  }
  if (this->ImageSeriesModality && this->ImageSeriesModality[0] != 0)
  {
    rt_metadata->set_image_metadata (0x0008, 0x0060, this->ImageSeriesModality);
  }
  if (this->DoseSeriesDescription && this->DoseSeriesDescription[0] != 0)
  {
    rt_metadata->set_dose_metadata (0x0008, 0x103e, this->DoseSeriesDescription);
  }
  if (this->DoseSeriesNumber && this->DoseSeriesNumber[0] != 0)
  {
    rt_metadata->set_dose_metadata (0x0020, 0x0011, this->DoseSeriesNumber);
  }
  if (this->RtssSeriesDescription && this->RtssSeriesDescription[0] != 0)
  {
    rt_metadata->set_rtstruct_metadata (0x0008, 0x103e, this->RtssSeriesDescription);
  }
  if (this->RtssSeriesNumber && this->RtssSeriesNumber[0] != 0)
  {
    rt_metadata->set_rtstruct_metadata (0x0020, 0x0011, this->RtssSeriesNumber);
  }
  
  // Write output to files
  this->RtStudy.save_dicom(this->FileName);
}
