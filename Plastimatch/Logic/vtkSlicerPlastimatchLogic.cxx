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

#include "SlicerRtCommon.h"

// Plastimatch Logic includes
#include "vtkSlicerPlastimatchLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

//STD includes
#include <string.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>

// Plastimatch includes
#include "plm_config.h"
#include "plm_image_header.h"
#include "plm_warp.h"
#include "plmregister.h"
#include "xform.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlastimatchLogic);

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::vtkSlicerPlastimatchLogic()
{
regp = new Registration_parms();
regd = new Registration_data();
xf_out = 0;
fixed_id = new char [256];
moving_id = new char [256];
warped_img = new Plm_image();
fixed_landmarks = 0;
moving_landmarks = 0;
}

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::~vtkSlicerPlastimatchLogic()
{
delete regp;
delete regd;
if (xf_out) delete xf_out;
delete &fixed_id;
delete &moving_id;
delete warped_img;
if (fixed_landmarks) delete fixed_landmarks;
if (moving_landmarks) delete moving_landmarks;
}

//----------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::set_input_images(char* fixed_id, char* moving_id)
{
  strcpy(this->fixed_id, fixed_id);
  vtkMRMLVolumeNode* fixed_vtk_img = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->fixed_id));
  itk::Image<float, 3>::Pointer fixed_itk_img = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(fixed_vtk_img, fixed_itk_img);
  
  strcpy(this->moving_id, moving_id);
  vtkMRMLVolumeNode* moving_vtk_img = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->moving_id));
  itk::Image<float, 3>::Pointer moving_itk_img = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(moving_vtk_img, moving_itk_img);
  
  this->regd->fixed_image = new Plm_image (fixed_itk_img);
  this->regd->moving_image = new Plm_image (moving_itk_img);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::set_input_landmarks(char* fixed_landmark_fn, char* moving_landmark_fn)
{
  fixed_landmarks = new Labeled_pointset();
  fixed_landmarks->load(fixed_landmark_fn);
  regd->fixed_landmarks = this->fixed_landmarks;  
  
  moving_landmarks = new Labeled_pointset();
  moving_landmarks->load(moving_landmark_fn);
  regd->moving_landmarks = this->moving_landmarks;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
:: add_stage()
{
  this->regp->append_stage();
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::set_par(char* key, char* val)
{    
  this->regp->set_key_val(key, val, 1);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::run_registration (char* output_image_name)
{
  do_registration_pure (&this->xf_out, this->regd ,this->regp);
  apply_warp(this->warped_img, this->xf_out, this->regd->fixed_image, this->regd->moving_image,
    -1200, 0, 1);
  get_output_img(output_image_name);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::apply_warp(Plm_image *im_warped, Xform * xf_in, Plm_image * fixed_img, Plm_image * im_in,
    float default_val, int use_itk, int interp_lin )
{
  Plm_image_header * pih = new Plm_image_header(fixed_img);
  plm_warp(im_warped, 0, xf_in, pih, im_in, default_val, use_itk, interp_lin);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::get_output_img (char* output_image_name)
{
  itk::Image<float, 3>::Pointer output_img_itk = this->warped_img->itk_float();    
  
  vtkSmartPointer<vtkImageData> output_img_vtk = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::RegionType region = output_img_itk->GetBufferedRegion();
  itk::Image<float, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, (int) imageSize[0]-1, 0, (int) imageSize[1]-1, 0, (int) imageSize[2]-1};
  output_img_vtk->SetExtent(extent);
  output_img_vtk->SetScalarType(VTK_FLOAT);
  output_img_vtk->SetNumberOfScalarComponents(1);
  output_img_vtk->AllocateScalars();
  
  float* output_img_Ptr = (float*)output_img_vtk->GetScalarPointer();
  itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > it_output_img_itk(
  output_img_itk, output_img_itk->GetLargestPossibleRegion() );
  
  for ( it_output_img_itk.GoToBegin(); !it_output_img_itk.IsAtEnd(); ++it_output_img_itk)
  {
    itk::Image<float, 3>::IndexType i = it_output_img_itk.GetIndex();
    (*output_img_Ptr) = output_img_itk->GetPixel(i);
    output_img_Ptr++;
  }
  
  // Read fixed image to get the geometrical information
  vtkMRMLVolumeNode* fixed_vtk_img = vtkMRMLVolumeNode::SafeDownCast(
  this->GetMRMLScene()->GetNodeByID(this->fixed_id));
  
  // Create new image node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> warped_img_node = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  warped_img_node->SetAndObserveImageData (output_img_vtk);
  warped_img_node->SetSpacing (
    output_img_itk->GetSpacing()[0],
    output_img_itk->GetSpacing()[1],
    output_img_itk->GetSpacing()[2]);
  warped_img_node->SetOrigin (
    output_img_itk->GetOrigin()[0],
    output_img_itk->GetOrigin()[1],
    output_img_itk->GetOrigin()[2]);
  std::string warped_img_name = this->GetMRMLScene()->GenerateUniqueName(output_image_name);
  warped_img_node->SetName(warped_img_name.c_str());
  
  warped_img_node->SetScene(this->GetMRMLScene());
  warped_img_node->CopyOrientation(fixed_vtk_img);
  this->GetMRMLScene()->AddNode(warped_img_node);
}

