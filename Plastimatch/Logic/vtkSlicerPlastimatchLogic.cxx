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
  this->regp=new Registration_parms();
  this->regd=new Registration_data();
  this->XfOut=NULL;
  this->FixedId=NULL;
  this->MovingId=NULL;
  this->WarpedImg=new Plm_image();
  this->FixedLandmarksFn=NULL;
  this->FixedLandmarks=NULL;
  this->MovingLandmarksFn=NULL;
  this->MovingLandmarks=NULL;
  this->OutputImageName=NULL;
}

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::~vtkSlicerPlastimatchLogic()
{
  this->regp=NULL;
  this->regd=NULL;
  this->XfOut=NULL;
  this->SetFixedId(NULL);
  this->SetMovingId(NULL);
  this->WarpedImg=NULL;
  this->SetFixedLandmarksFn(NULL);
  this->FixedLandmarks=NULL;
  this->SetMovingLandmarksFn(NULL);
  this->MovingLandmarks=NULL;
  this->SetOutputImageName(NULL);
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
:: AddStage()
{
  this->regp->append_stage();
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::SetPar(char* key, char* val)
{    
  this->regp->set_key_val(key, val, 1);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::RunRegistration()
{
  
  // Set input images
  vtkMRMLVolumeNode* FixedVtkImg = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(GetFixedId()));
  itk::Image<float, 3>::Pointer FixedItkImg = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(FixedVtkImg, FixedItkImg);

  vtkMRMLVolumeNode* MovingVtkImg = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(GetMovingId()));
  itk::Image<float, 3>::Pointer MovingItkImg = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(MovingVtkImg, MovingItkImg);

  this->regd->fixed_image = new Plm_image (FixedItkImg);
  this->regd->moving_image = new Plm_image (MovingItkImg);

  // Set landmarks
  if (GetFixedLandmarksFn() && GetFixedLandmarksFn()) {
    FixedLandmarks = new Labeled_pointset();
    FixedLandmarks->load(GetFixedLandmarksFn());
    regd->fixed_landmarks = this->FixedLandmarks;
    
    MovingLandmarks = new Labeled_pointset();
    MovingLandmarks->load(GetMovingLandmarksFn());
    regd->moving_landmarks = this->MovingLandmarks;
  } 
  
  // Run registration and warp image
  do_registration_pure (&this->XfOut, this->regd ,this->regp);
  ApplyWarp(this->WarpedImg, this->XfOut, this->regd->fixed_image, this->regd->moving_image,
    -1200, 0, 1);
  GetOutputImg(GetOutputImageName());
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::ApplyWarp(Plm_image* WarpedImg, Xform* XfIn, Plm_image* FixedImg, Plm_image* InputImg,
    float DefaultVal, int UseItk, int InterpLin )
{
  Plm_image_header* pih = new Plm_image_header(FixedImg);
  plm_warp(WarpedImg, 0, XfIn, pih, InputImg, DefaultVal, UseItk, InterpLin);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::GetOutputImg (char* PublicOutputImageName)
{
  itk::Image<float, 3>::Pointer OutputImgItk = this->WarpedImg->itk_float();    
  
  vtkSmartPointer<vtkImageData> OutputImgVtk = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::RegionType region = OutputImgItk->GetBufferedRegion();
  itk::Image<float, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, (int) imageSize[0]-1, 0, (int) imageSize[1]-1, 0, (int) imageSize[2]-1};
  OutputImgVtk->SetExtent(extent);
  OutputImgVtk->SetScalarType(VTK_FLOAT);
  OutputImgVtk->SetNumberOfScalarComponents(1);
  OutputImgVtk->AllocateScalars();
  
  float* OutputImgPtr = (float*)OutputImgVtk->GetScalarPointer();
  itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > ItOutputImgItk(
  OutputImgItk, OutputImgItk->GetLargestPossibleRegion() );
  
  for ( ItOutputImgItk.GoToBegin(); !ItOutputImgItk.IsAtEnd(); ++ItOutputImgItk)
  {
    itk::Image<float, 3>::IndexType i = ItOutputImgItk.GetIndex();
    (*OutputImgPtr) = OutputImgItk->GetPixel(i);
    OutputImgPtr++;
  }
  
  // Read fixed image to get the geometrical information
  vtkMRMLVolumeNode* FixedVtkImg = vtkMRMLVolumeNode::SafeDownCast(
  this->GetMRMLScene()->GetNodeByID(GetFixedId()));
  
  // Create new image node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> WarpedImgNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  WarpedImgNode->SetAndObserveImageData (OutputImgVtk);
  WarpedImgNode->SetSpacing (
    OutputImgItk->GetSpacing()[0],
    OutputImgItk->GetSpacing()[1],
    OutputImgItk->GetSpacing()[2]);
  WarpedImgNode->SetOrigin (
    OutputImgItk->GetOrigin()[0],
    OutputImgItk->GetOrigin()[1],
    OutputImgItk->GetOrigin()[2]);
  std::string WarpedImgName = this->GetMRMLScene()->GenerateUniqueName(PublicOutputImageName);
  WarpedImgNode->SetName(WarpedImgName.c_str());
  
  WarpedImgNode->SetScene(this->GetMRMLScene());
  WarpedImgNode->CopyOrientation(FixedVtkImg);
  this->GetMRMLScene()->AddNode(WarpedImgNode);
}

