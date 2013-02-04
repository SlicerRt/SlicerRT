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

// ModuleTemplate includes
#include "vtkSlicerDicomRtWriter.h"

// MRML includes

// VTK includes
#include <vtkNew.h>
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"

// STD includes
#include <cassert>
#include <vector>

// DCMTK includes

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/ofstd/ofconapp.h"

#include "dcmtk/dcmrt/drtdose.h"
#include "dcmtk/dcmrt/drtimage.h"
#include "dcmtk/dcmrt/drtplan.h"
#include "dcmtk/dcmrt/drtstruct.h"
#include "dcmtk/dcmrt/drttreat.h"
#include "dcmtk/dcmrt/drtionpl.h"
#include "dcmtk/dcmrt/drtiontr.h"

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
void vtkSlicerDicomRtWriter::SetFileName(const char * name)
{
  if ( this->FileName && name && (!strcmp(this->FileName,name)))
    {
    return;
    }
  if (!name && !this->FileName)
    {
    return;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  if (name)
    {
    this->FileName = new char[strlen(name) + 1];
    strcpy(this->FileName, name);
    }

  this->Modified();
}

