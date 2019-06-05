/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Gregory C. Sharp, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#include "vtkPlmpyDicomSroExport.h"

// SlicerRT includes
#include "PlmCommon.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkMatrix4x4.h>

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
  this->FixedImageID = nullptr;
  this->MovingImageID = nullptr;
  this->XformID = nullptr;
  this->OutputDirectory = nullptr;
}

//----------------------------------------------------------------------------
vtkPlmpyDicomSroExport::~vtkPlmpyDicomSroExport()
{
  this->SetFixedImageID(nullptr);
  this->SetMovingImageID(nullptr);
  this->SetXformID(nullptr);
  this->SetOutputDirectory(nullptr);
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
int vtkPlmpyDicomSroExport::DoExport()
{
  if ( this->FixedImageID == nullptr
    || this->MovingImageID == nullptr
    || this->XformID == nullptr
    || this->OutputDirectory == nullptr)
  {
    vtkErrorMacro("DoExport: Invalid inputs");
    return 1;
  }

  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("DoExport: Invalid MRML scene");
    return 1;
  }

  // Convert input CT/MR image to the format Plastimatch can use
  vtkMRMLScalarVolumeNode* fixedNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  Plm_image::Pointer fixedImage = PlmCommon::ConvertVolumeNodeToPlmImage(fixedNode, true);

  vtkMRMLScalarVolumeNode* movingNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->MovingImageID));
  Plm_image::Pointer movingImage = PlmCommon::ConvertVolumeNodeToPlmImage(movingNode, false);

  // Convert xform into a form that Plastimatch can use
  Xform::Pointer xform = Xform::New ();
  vtkMRMLTransformNode *xformNode = vtkMRMLTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->XformID));
  if (!xformNode)
  {
    vtkErrorMacro("DoExport: Transform node was not actually a transform");
    return 1;
  }
  if (xformNode->IsA ("vtkMRMLLinearTransformNode"))
  {
    vtkMRMLLinearTransformNode *xformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->XformID));
    vtkSmartPointer<vtkMatrix4x4> vtkAff = vtkSmartPointer<vtkMatrix4x4>::New();
    xformNode->GetMatrixTransformToParent(vtkAff);
    AffineTransformType itkAff;
    bool rc = vtkITKTransformConverter::SetITKLinearTransformFromVTK(this,itkAff,vtkAff);
    xform->set_aff(itkAff);
  }
  else if (xformNode->IsA ("vtkMRMLGridTransformNode"))
  {
    //vtkMRMLDeformationGridTransformNode *xformNode = vtkMRMLDeformationGridTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->XformID));
    //..
    //itk::Image< itk::Vector< float 3 >, 3 > itk_vf = ....
    //xform->set_itk_vf(itk_vf);
  }
  else
  {
    vtkErrorMacro("DoExport: Only Linear and Grid Transforms are exported");
    return 1;
  }

#if defined (commentout)
  vtkSmartPointer<vtkMatrix4x4> vtkAff = vtkSmartPointer<vtkMatrix4x4>::New();
  xformNode->GetMatrixTransformToParent(vtkAff);

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
#endif
  
  // Run exporter
  Dicom_sro_save dss;
  dss.set_fixed_image (fixedImage);
  dss.set_moving_image (movingImage);
  dss.set_xform (xform);
  dss.set_output_dir (this->OutputDirectory);
  dss.run (); //TODO: Error handling in exporter

  return 0;
}
