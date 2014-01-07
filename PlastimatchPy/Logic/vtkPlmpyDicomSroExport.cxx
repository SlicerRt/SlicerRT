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
#include "vtkPlmpyDicomSroExport.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPoints.h"

// ITK includes
#include <itkImage.h>

//------------------------------------------------------------------------------
//vtkCxxSetObjectMacro(vtkPlmpyDicomSroExport,Points,vtkPoints);

vtkStandardNewMacro(vtkPlmpyDicomSroExport);

//----------------------------------------------------------------------------
vtkPlmpyDicomSroExport::vtkPlmpyDicomSroExport()
{
  this->FixedImageID = NULL;
  this->MovingImageID = NULL;
  this->XformID = NULL;
  this->OutputDirectory = NULL;
}

//----------------------------------------------------------------------------
vtkPlmpyDicomSroExport::~vtkPlmpyDicomSroExport()
{
  this->SetFixedImageID(NULL);
  this->SetMovingImageID(NULL);
  this->SetXformID(NULL);
  this->SetOutputDirectory(NULL);
}

//----------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::PrintSelf(ostream& os, vtkIndent indent)
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

void vtkPlmpyDicomSroExport::set_mrml_scene_hack(vtkMRMLScene * newScene)
{
  this->SetMRMLScene (newScene);
}

//---------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::RegisterNodes()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RegisterNodes: Invalid MRML Scene!");
    return;
    }
}

//---------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML Scene!");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkPlmpyDicomSroExport::DoExport ()
{
  if (this->FixedImageID == NULL
    || this->MovingImageID == NULL
    || this->XformID == NULL
    || this->OutputDirectory == NULL)
  {
    vtkErrorMacro("Sorry, vtkPlmpyDicomSroExport::DoExport () is missing some inputs");
    return;
  }

  if (this->GetMRMLScene() == NULL) {
    vtkErrorMacro("Sorry, vtkPlmpyDicomSroExport::DoExport () found that it has no mrml scene");
    return;
  }

  // Convert input CT/MR image to the format Plastimatch can use
  vtkMRMLScalarVolumeNode* fixedNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  itk::Image<float, 3>::Pointer itkImage = itk::Image<float, 3>::New();
  if (SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(fixedNode, itkImage, true) == false)
  {
    printf ("Dicom SRO Export: Failed to convert image volumeNode to ITK volume!");
    vtkErrorMacro("Dicom SRO Export: Failed to convert image volumeNode to ITK volume!");
    return;
  }
#if defined (commentout)
#endif

  printf ("Doing export in C++\n");
}
