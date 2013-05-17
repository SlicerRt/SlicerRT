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
#include "vtkSlicerDicomRtExportModuleLogic.h"
#include "vtkSlicerDicomRtWriter.h"

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkNew.h>
#include "vtkPolyData.h"
#include <vtkSmartPointer.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDicomRtExportLogic);

//----------------------------------------------------------------------------
vtkSlicerDicomRtExportLogic::vtkSlicerDicomRtExportLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerDicomRtExportLogic::~vtkSlicerDicomRtExportLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtExportLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDicomRtExportLogic::SaveDicomRT(char *name)
{
  // to do ...

}
