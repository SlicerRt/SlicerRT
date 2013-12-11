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
#include "plmpyDicomSroExport.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPoints.h"

//------------------------------------------------------------------------------
//vtkCxxSetObjectMacro(plmpyDicomSroExport,Points,vtkPoints);

vtkStandardNewMacro(plmpyDicomSroExport);

//----------------------------------------------------------------------------
plmpyDicomSroExport::plmpyDicomSroExport()
{
  this->FixedImageID = NULL;
  this->MovingImageID = NULL;
  this->XformID = NULL;
  this->OutputDirectory = NULL;
}

//----------------------------------------------------------------------------
plmpyDicomSroExport::~plmpyDicomSroExport()
{
  this->SetFixedImageID(NULL);
  this->SetMovingImageID(NULL);
  this->SetXformID(NULL);
  this->SetOutputDirectory(NULL);
}

//----------------------------------------------------------------------------
void plmpyDicomSroExport::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

#if defined (commentout)
  os << indent << "Value: "  << this->Value;
  os << indent << "Radius: " << this->Radius;
  os << indent << "Shape: "  << this->Shape;

  // vtkSetObjectMacro
  os << indent << "Points: ";
  if (this->Points)
    {
    this->Points->PrintSelf(os << "\n" ,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
#endif
}

//----------------------------------------------------------------------------
void plmpyDicomSroExport::DoExport ()
{
  if (this->FixedImageID == NULL
    || this->MovingImageID == NULL
    || this->XformID == NULL
    || this->OutputDirectory == NULL)
  {
    printf ("Sorry, plmpyDicomSroExport::DoExport () is missing some inputs\n");
    return;
  }

  printf ("Doing export in C++\n");
}
