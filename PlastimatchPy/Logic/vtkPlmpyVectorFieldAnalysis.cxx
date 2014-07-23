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
// PlastimatchPy Logic includes
#include "vtkPlmpyVectorFieldAnalysis.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkImageExport.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

// Slicer includes
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTransformNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// Plastimatch includes
#include "bspline_interpolate.h"
#include "plm_config.h"
#include "plm_image_header.h"
#include "volume.h"

#include "vf_jacobian.h"
#include "itk_image_load.h"
#include "itk_image_type.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlmpyVectorFieldAnalysis);

//----------------------------------------------------------------------------
vtkPlmpyVectorFieldAnalysis::vtkPlmpyVectorFieldAnalysis()
{
  this->FixedImageID = NULL;
  this->OutputVolumeID = NULL;

  this->MovingImageToFixedImageVectorField = NULL;

  this->jacobian_min = 0;
  this->jacobian_max = 0;

  this->JacobianMinString = (char *)malloc(32);
  this->JacobianMaxString = (char *)malloc(32);

  this->SetFixedImageID(NULL);
  this->SetOutputVolumeID(NULL);

  this->MovingImageToFixedImageVectorField = NULL;

}

vtkPlmpyVectorFieldAnalysis::~vtkPlmpyVectorFieldAnalysis()
{
}

//----------------------------------------------------------------------------
void vtkPlmpyVectorFieldAnalysis::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//---------------------------------------------------------------------------
void vtkPlmpyVectorFieldAnalysis::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//---------------------------------------------------------------------------
void vtkPlmpyVectorFieldAnalysis::RunJacobian()
{

cout << "started RunJacobian from C++ module\n";
if ( this->VFImageID == NULL ) { cout << "ERRROR: NULL input VF"; } 

// vtk to itk transform derived from SlicerRtCommon.txx

vtkMRMLVectorVolumeNode* VFImage = vtkMRMLVectorVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->VFImageID));


DeformationFieldType::Pointer vol = itk::Image<itk::Vector<float,3> ,3>::New();

vtkImageData* inVolume = VFImage->GetImageData();
  if ( inVolume == NULL )
  {
    vtkErrorWithObjectMacro(VFImage, "ConvertVolumeNodeToItkImage: Failed to convert volume node to itk image - image in input MRML volume node is NULL!");
    return;
  }

cout << "got image data";



vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New();
#if (VTK_MAJOR_VERSION <= 5)
  imageExport->SetInput(inVolume);
#else
  imageExport->SetInputData(inVolume);
#endif

cout << "gone past set input";


imageExport->Update();

cout << "gone past Update";

//--
  vtkSmartPointer<vtkMatrix4x4> rasToWorldTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMRMLTransformNode* inTransformNode=VFImage->GetParentTransformNode();
  if (inTransformNode!=NULL)
  {
    if (inTransformNode->IsTransformToWorldLinear() == 0)
    {
      vtkErrorWithObjectMacro(VFImage, "ConvertVolumeNodeToItkImage: There is a non-linear transform assigned to an input dose volume. Only linear transforms are supported!");
      return;
    }
    inTransformNode->GetMatrixTransformToWorld(rasToWorldTransformMatrix);
  }

  vtkSmartPointer<vtkMatrix4x4> inVolumeToRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  VFImage->GetIJKToRASMatrix(inVolumeToRasTransformMatrix);

  vtkSmartPointer<vtkTransform> inVolumeToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldTransform->Identity();
  inVolumeToWorldTransform->PostMultiply();
  inVolumeToWorldTransform->Concatenate(inVolumeToRasTransformMatrix);
  inVolumeToWorldTransform->Concatenate(rasToWorldTransformMatrix);

  // Set ITK image properties
  double outputSpacing[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetScale(outputSpacing);
  vol->SetSpacing(outputSpacing);

  double outputOrigin[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetPosition(outputOrigin);
  vol->SetOrigin(outputOrigin);

  double outputOrienationAngles[3] = {0.0, 0.0, 0.0};
  inVolumeToWorldTransform->GetOrientation(outputOrienationAngles);
  vtkSmartPointer<vtkTransform> inVolumeToWorldOrientationTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldOrientationTransform->Identity();
  inVolumeToWorldOrientationTransform->RotateX(outputOrienationAngles[0]);
  inVolumeToWorldOrientationTransform->RotateY(outputOrienationAngles[1]);
  inVolumeToWorldOrientationTransform->RotateZ(outputOrienationAngles[2]);
  vtkSmartPointer<vtkMatrix4x4> inVolumeToWorldOrientationTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeToWorldOrientationTransform->GetMatrix(inVolumeToWorldOrientationTransformMatrix);
  itk::Matrix<double,3,3> outputDirectionMatrix;
  for(int i=0; i<3; i++)
  {
    for(int j=0; j<3; j++)
    {
      outputDirectionMatrix[i][j] = inVolumeToWorldOrientationTransformMatrix->GetElement(i,j);
    }
  }
  vol->SetDirection(outputDirectionMatrix);

//---


  int inputExtent[6]={0,0,0,0,0,0};
  inVolume->GetExtent(inputExtent);
  itk::Image<float, 3>::SizeType inputSize;
  inputSize[0] = inputExtent[1] - inputExtent[0] + 1;
  inputSize[1] = inputExtent[3] - inputExtent[2] + 1;
  inputSize[2] = inputExtent[5] - inputExtent[4] + 1;

  itk::Image<float, 3>::IndexType start;
  start[0]=start[1]=start[2]=0.0;

  itk::Image<float, 3>::RegionType region;
  region.SetSize(inputSize);
  region.SetIndex(start);   
  vol->SetRegions(region);


  try
  {  
    vol->Allocate();
  }
  catch(itk::ExceptionObject & err)
  {
    vtkErrorWithObjectMacro(VFImage, "ConvertVolumeNodeToItkImage: Failed to allocate memory for the image conversion: " << err.GetDescription())
    return;
  }

imageExport->Export( vol->GetBufferPointer() );


// modeled after pcmd_jacobian
    FloatImageType::Pointer jacimage;

    std::cout << "...loaded xf!" << std::endl;

    /* Make jacobian */
    Jacobian jacobian;
    jacobian.set_input_vf (vol);    
    jacimage=jacobian.make_jacobian();

    printf("min/max jacobian values to output: %lf %lf\n", jacobian.jacobian_min, jacobian.jacobian_max); 

    // The Python code can only access strings via GetJacobianMainString(), GetJacobianMaxString() macros
    sprintf(this->JacobianMinString, "%f", jacobian.jacobian_min);
    sprintf(this->JacobianMaxString, "%f", jacobian.jacobian_max);

    Plm_image::Pointer jacobianImage = Plm_image::New();
  jacobianImage->set_itk(jacimage);
  jacobianImage->convert_to_itk();
  this->SetImageIntoVolumeNode(jacobianImage); // this also needs FixedImageID set to add geometry

}

void vtkPlmpyVectorFieldAnalysis::SetImageIntoVolumeNode(Plm_image::Pointer& plastimatchImage)
{
  if (!plastimatchImage || !plastimatchImage->itk_float())
  {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Invalid warped image!");
    return;
  }

  itk::Image<float, 3>::Pointer outputImageItk = plastimatchImage->itk_float();    

  vtkSmartPointer<vtkImageData> outputImageVtk = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(outputImageItk, outputImageVtk, VTK_FLOAT);
  
  // Read fixed image to get the geometrical information
  vtkMRMLScalarVolumeNode* fixedVolumeNode 
    = vtkMRMLScalarVolumeNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(this->FixedImageID));
  if (!fixedVolumeNode)
  {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Node containing the fixed image cannot be retrieved!");
    return;
  }

  // Create new image node
  vtkMRMLScalarVolumeNode* warpedImageNode 
    = vtkMRMLScalarVolumeNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(this->OutputVolumeID));
  if (!warpedImageNode)
  {
    vtkErrorMacro("SetWarpedImageInVolumeNode: Node containing the warped image cannot be retrieved!");
    return;
  }

  // Set warped image to a Slicer node
  warpedImageNode->CopyOrientation(fixedVolumeNode);
  warpedImageNode->SetAndObserveImageData(outputImageVtk);
}

