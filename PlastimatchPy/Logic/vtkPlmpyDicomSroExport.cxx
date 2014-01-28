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
#include "PlmCommon.h"
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPoints.h"

// ITK includes
#include <itkImage.h>

// Plastimatch include
#include "dicom_sro_save.h"
#include "plm_image.h"

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
  Plm_image::Pointer fixedImage = PlmCommon::ConvertVolumeNodeToPlmImage (fixedNode);

  vtkMRMLScalarVolumeNode* movingNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->MovingImageID));
  Plm_image::Pointer movingImage = PlmCommon::ConvertVolumeNodeToPlmImage (movingNode);

  // Convert xform into a form that Plastimatch can use
  vtkMRMLLinearTransformNode *xformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->XformID));
  if (!xformNode) {
    vtkErrorMacro("Sorry, vtkPlmpyDicomSroExport::DoExport () could not digest the transform node");
    return;
  }

  // Make a copy of vtk affine matrix to perform some operation before export
  const vtkMatrix4x4* vtkAffMatrix = xformNode->GetMatrixTransformToParent ();
  vtkSmartPointer<vtkMatrix4x4> vtkAff = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkAff->DeepCopy(vtkAffMatrix);

  // Change from RAS system to ITK/DICOM LPS system
  vtkSmartPointer<vtkMatrix4x4> invMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  invMatrix->Identity();
  invMatrix->SetElement(0,0,-1);
  invMatrix->SetElement(1,1,-1);
  vtkSmartPointer<vtkMatrix4x4> forMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  forMatrix->Identity();
  forMatrix->SetElement(0,0,-1);
  forMatrix->SetElement(1,1,-1);
  vtkMatrix4x4::Multiply4x4(invMatrix, vtkAff, vtkAff);
  vtkMatrix4x4::Multiply4x4(vtkAff, forMatrix, vtkAff);

  // Set up the parameters for itk affine transform
  itk::Array<double> parms(12);
  unsigned int par = 0;
  // First set up the matrix part
  for (int i = 0; i < 3; i++) 
  {
    for (int j = 0; j < 3; j++) 
	{
      //printf ("Setting affine [%d,%d] %g\n", i, j, 
      //        vtkAff->GetElement (i,j));
      parms[par] = vtkAff->GetElement (i, j);
	  par++;
    }
  }
  // Next set up the offset part
  for (int i = 0; i < 3; i++) 
  {
	  parms[par] = vtkAff->GetElement (i, 3);
	  par++;
  }
  Xform::Pointer xform = Xform::New ();
  xform->set_aff (parms);

  // Run exporter
  Dicom_sro_save dss;
  dss.set_fixed_image (fixedImage);
  dss.set_moving_image (movingImage);
  dss.set_xform (xform);
  dss.set_output_dir (this->OutputDirectory);
  dss.run ();
}
