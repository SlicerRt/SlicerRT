/*==========================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, RMP, Princess Margaret Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==========================================================================*/

#include "vtkVolumesOrientedResampleUtility.h"

// MRML nodes includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkImageReslice.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>

//----------------------------------------------------------------------------
// vtkVolumesOrientedResampleUtility methods

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumesOrientedResampleUtility);

//----------------------------------------------------------------------------
vtkVolumesOrientedResampleUtility::vtkVolumesOrientedResampleUtility()
{
}

//----------------------------------------------------------------------------
vtkVolumesOrientedResampleUtility::~vtkVolumesOrientedResampleUtility()
{
}

//----------------------------------------------------------------------------
bool vtkVolumesOrientedResampleUtility
::ResampleInputVolumeNodeToReferenceVolumeNode(vtkMRMLScalarVolumeNode* inVolumeNode, 
                                               vtkMRMLScalarVolumeNode* refVolumeNode, 
                                               vtkMRMLScalarVolumeNode* outVolumeNode)
{
  int dimensions[3] = {0, 0, 0};

  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(refVolumeNode);
  vtkMRMLScalarVolumeNode* inputVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(inVolumeNode);
  vtkMRMLScalarVolumeNode* outputVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(outVolumeNode);
  // Make sure inputs are initialized
  if (!inputVolumeNode || !referenceVolumeNode || !outputVolumeNode)
  {
    vtkGenericWarningMacro("vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode: Inputs are not specified!");
    return false;
  }

  // Make sure input volume node is in the scene
  if (!inputVolumeNode->GetScene())
  {
    vtkErrorWithObjectMacro(inputVolumeNode, "vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode: Input volume node is not in a MRML scene!");
    return false;
  }

  vtkSmartPointer<vtkMatrix4x4> inputVolumeIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inputVolumeNode->GetIJKToRASMatrix(inputVolumeIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> referenceVolumeRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceVolumeNode->GetRASToIJKMatrix(referenceVolumeRAS2IJKMatrix);
  referenceVolumeNode->GetImageData()->GetDimensions(dimensions);

  vtkSmartPointer<vtkTransform> outputVolumeResliceTransform = vtkSmartPointer<vtkTransform>::New();
  outputVolumeResliceTransform->Identity();
  outputVolumeResliceTransform->PostMultiply();
  outputVolumeResliceTransform->SetMatrix(inputVolumeIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
    inputVolumeNode->GetScene()->GetNodeByID(inputVolumeNode->GetTransformNodeID()));
  vtkSmartPointer<vtkMatrix4x4> inputVolumeRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(inputVolumeRAS2RASMatrix);
    outputVolumeResliceTransform->Concatenate(inputVolumeRAS2RASMatrix);
  }
  vtkSmartPointer<vtkMRMLTransformNode> referenceVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
    referenceVolumeNode->GetScene()->GetNodeByID(referenceVolumeNode->GetTransformNodeID()));
  vtkSmartPointer<vtkMatrix4x4> referenceVolumeRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (referenceVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetMatrixTransformToWorld(referenceVolumeRAS2RASMatrix);
    referenceVolumeRAS2RASMatrix->Invert();
    outputVolumeResliceTransform->Concatenate(referenceVolumeRAS2RASMatrix);
  }
  outputVolumeResliceTransform->Concatenate(referenceVolumeRAS2IJKMatrix);
  outputVolumeResliceTransform->Inverse();

  vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
#if (VTK_MAJOR_VERSION <= 5)
  resliceFilter->SetInput(inputVolumeNode->GetImageData());
#else
  resliceFilter->SetInputData(inputVolumeNode->GetImageData());
#endif
  resliceFilter->SetOutputOrigin(0, 0, 0);
  resliceFilter->SetOutputSpacing(1, 1, 1);
  resliceFilter->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  resliceFilter->SetResliceTransform(outputVolumeResliceTransform);
  resliceFilter->Update();

  outputVolumeNode->CopyOrientation(referenceVolumeNode);
  outputVolumeNode->SetAndObserveImageData(resliceFilter->GetOutput());
  
  return true;
}

//----------------------------------------------------------------------------
void vtkVolumesOrientedResampleUtility::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


