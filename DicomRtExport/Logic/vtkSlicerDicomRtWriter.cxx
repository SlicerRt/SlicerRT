/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

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
void vtkSlicerDicomRtWriter::AddContour(UCharImageType::Pointer itk_structure, const char *contourName, double *contourColor)
{
  std::string colorString("");
  std::ostringstream strs;
  strs << contourColor[0];
  // TODO: rename variable str, strs
  std::string str = strs.str();
  colorString = colorString + str;
  strs << contourColor[1];
  str = strs.str();
  colorString = colorString + "\\" + str;
  strs << contourColor[2];
  str = strs.str();
  colorString = colorString + "\\" + str;

  RtStudy.add_structure(itk_structure, contourName, colorString.c_str());
}
  
//----------------------------------------------------------------------------
void vtkSlicerDicomRtWriter::Write()
{
  RtStudy.save_dicom(this->FileName);
}
  

