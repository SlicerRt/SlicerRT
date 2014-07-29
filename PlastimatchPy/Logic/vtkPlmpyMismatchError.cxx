/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Nadya Shusharina, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#include "vtkPlmpyMismatchError.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPoints.h"

//------------------------------------------------------------------------------

vtkStandardNewMacro(vtkPlmpyMismatchError);

//----------------------------------------------------------------------------
vtkPlmpyMismatchError::plmpyMismatchError()
{
  this->FixedImageID = NULL;
  this->MovingImageID = NULL;
  this->FixedLandmarksFileName = NULL;
  this->MovingLandmarksFileName = NULL;

  this->FixedLandmarks = NULL;
  this->MovingLandmarks = NULL;

  this->WarpedLandmarks = NULL;
  vtkSmartPointer<vtkPoints> warpedLandmarks = vtkSmartPointer<vtkPoints>::New();
  this->SetWarpedLandmarks(warpedLandmarks);
}

//----------------------------------------------------------------------------
void vtkPlmpyMismatchError::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//---------------------------------------------------------------------------
void vtkPlmMismatchError::RunMismatchError()
{

/*------This code is intended to use Plastimatch functions-------------
  // Set input images
  vtkMRMLScalarVolumeNode* fixedVtkImage = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  itk::Image<float, 3>::Pointer fixedItkImage = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImageInLPS<float>(fixedVtkImage, fixedItkImage);

  vtkMRMLScalarVolumeNode* movingVtkImage = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->MovingImageID));
  itk::Image<float, 3>::Pointer movingItkImage = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImageInLPS<float>(movingVtkImage, movingItkImage);

  //this->RegistrationData->fixed_image = new Plm_image(fixedItkImage);
  //this->RegistrationData->moving_image = new Plm_image(movingItkImage);
  this->RegistrationData->fixed_image = Plm_image::New (
    new Plm_image(fixedItkImage));
  this->RegistrationData->moving_image = Plm_image::New(
    new Plm_image(movingItkImage));
*/
  
  // Set landmarks 
  if (this->FixedLandmarks && this->MovingLandmarks)
    {
    // From Slicer
    std::cout << "setting landmarks from slicer" << std::endl;
    this->SetLandmarksFromSlicer();
    }
  else if (this->FixedLandmarksFileName && this->FixedLandmarksFileName)
    {
    // From Files
    this->SetLandmarksFromFiles();
    }
  else
    {
    vtkErrorMacro("RunRegistration: Unable to retrieve fixed and moving landmarks!");
    return;
    }

/* ------- If Plastimatch code is used-----------------------
     this->AverageString = (char *)malloc(32);
     this->VarianceString = (char *)malloc(32);
     this->StdevString = (char *)malloc(32);
     
     this->SeparationString = (char *)malloc(32);
	 
     sprintf(this->AverageString, "%f", avg);
     sprintf(this->VarianceString, "%f", var);
     sprintf(this->StdevString, "%f", sqrt(var));

     sprintf(this->SeparationString, "%f",  var  );
*/

}

