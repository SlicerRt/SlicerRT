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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// ContourComparison includes
#include "vtkSlicerContourComparisonModuleLogic.h"
#include "vtkMRMLContourComparisonNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

// Plastimatch includes
#include "dice_statistics.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageExport.h>
#include <vtkTimerLog.h>
#include <vtkTransform.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContourComparisonModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogic::vtkSlicerContourComparisonModuleLogic()
{
  this->ContourComparisonNode = NULL;

  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogic::~vtkSlicerContourComparisonModuleLogic()
{
  SetAndObserveContourComparisonNode(NULL); // release the node object to avoid memory leaks
}

//----------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::SetAndObserveContourComparisonNode(vtkMRMLContourComparisonNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ContourComparisonNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourComparisonNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLContourComparisonNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLContourComparisonNode");
  if (node)
  {
    paramNode = vtkMRMLContourComparisonNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ContourComparisonNode, paramNode);
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic
::ConvertVolumeNodeToItkImage(vtkMRMLVolumeNode* inVolumeNode, itk::Image<unsigned char, 3>::Pointer outItkVolume)
{
  if ( inVolumeNode == NULL )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - input MRML volume node is NULL!"); 
    return; 
  }

  vtkImageData* inVolume = inVolumeNode->GetImageData();
  if ( inVolume == NULL )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - image in input MRML volume node is NULL!"); 
    return; 
  }

  if ( outItkVolume.IsNull() )
  {
    vtkErrorMacro("Failed to convert vtk image to itk image - output image is NULL!"); 
    return; 
  }

  // convert vtkImageData to itkImage 
  vtkSmartPointer<vtkImageExport> imageExport = vtkSmartPointer<vtkImageExport>::New(); 
  imageExport->SetInput(inVolume);
  imageExport->Update(); 

  // Determine input volume to world transform
  vtkSmartPointer<vtkMatrix4x4> rasToWorldTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  vtkMRMLTransformNode* inTransformNode=inVolumeNode->GetParentTransformNode();
  if (inTransformNode!=NULL)
  {
    if (inTransformNode->IsTransformToWorldLinear() == 0)
    {
      vtkErrorMacro("There is a non-linear transform assigned to an input dose volume. Only linear transforms are supported!");
      return;
    }
    inTransformNode->GetMatrixTransformToWorld(rasToWorldTransformMatrix);
  }

  vtkSmartPointer<vtkMatrix4x4> inVolumeToRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inVolumeNode->GetIJKToRASMatrix(inVolumeToRasTransformMatrix);

  vtkSmartPointer<vtkTransform> inVolumeToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  inVolumeToWorldTransform->Identity();
  inVolumeToWorldTransform->PostMultiply();
  inVolumeToWorldTransform->Concatenate(inVolumeToRasTransformMatrix);
  inVolumeToWorldTransform->Concatenate(rasToWorldTransformMatrix);

  // Set ITK image properties
  double outputSpacing[3];
  inVolumeToWorldTransform->GetScale(outputSpacing);
  outItkVolume->SetSpacing(outputSpacing);

  double outputOrigin[3];
  inVolumeToWorldTransform->GetPosition(outputOrigin);
  outItkVolume->SetOrigin(outputOrigin);

  double outputOrienationAngles[3];
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
  outItkVolume->SetDirection(outputDirectionMatrix);

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
  outItkVolume->SetRegions(region);

  try
  {
    outItkVolume->Allocate();
  }
  catch(itk::ExceptionObject & err)
  {
    vtkErrorMacro("Failed to allocate memory for the image conversion: " << err.GetDescription() );
    return;
  }

  imageExport->Export( outItkVolume->GetBufferPointer() );
}

//-----------------------------------------------------------------------------
bool vtkSlicerContourComparisonModuleLogic::IsReferenceVolumeNeeded()
{
  if (!this->ContourComparisonNode || !this->GetMRMLScene())
  {
    return false;
  }

  vtkMRMLContourNode* referenceContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourComparisonNode->GetReferenceContourNodeId()));
  vtkMRMLContourNode* compareContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourComparisonNode->GetCompareContourNodeId()));

  if (!referenceContourNode || !compareContourNode)
  {
    return false;
  }

  return !( referenceContourNode->GetIndexedLabelmapVolumeNodeId()
        && compareContourNode->GetIndexedLabelmapVolumeNodeId() );
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::ComputeDiceStatistics()
{
  if (!this->ContourComparisonNode || !this->GetMRMLScene())
  {
    return;
  }

  this->ContourComparisonNode->ResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  // Convert input images to the format Plastimatch can use
  vtkMRMLContourNode* referenceContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourComparisonNode->GetReferenceContourNodeId()));
  itk::Image<unsigned char, 3>::Pointer referenceContourLabelmapVolumeItk
    = itk::Image<unsigned char, 3>::New();

  vtkMRMLContourNode* compareContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourComparisonNode->GetCompareContourNodeId()));
  itk::Image<unsigned char, 3>::Pointer compareContourLabelmapVolumeItk
    = itk::Image<unsigned char, 3>::New();

  if (this->IsReferenceVolumeNeeded())
  {
    referenceContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourComparisonNode->GetRasterizationReferenceVolumeNodeId() );
    compareContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourComparisonNode->GetRasterizationReferenceVolumeNodeId() );
  }

  vtkMRMLScalarVolumeNode* referenceContourLabelmapVolumeNode
    = referenceContourNode->GetIndexedLabelmapVolumeNode();
  vtkMRMLScalarVolumeNode* compareContourLabelmapVolumeNode
    = compareContourNode->GetIndexedLabelmapVolumeNode();
  if (!referenceContourLabelmapVolumeNode || !compareContourLabelmapVolumeNode)
  {
    vtkErrorMacro("Failed to get indexed labelmap representation from selected input contours!");
    return;
  }

  // Convert inputs to ITK images
  double checkpointItkConvertStart = timer->GetUniversalTime();
  this->ConvertVolumeNodeToItkImage(referenceContourLabelmapVolumeNode, referenceContourLabelmapVolumeItk);
  this->ConvertVolumeNodeToItkImage(compareContourLabelmapVolumeNode, compareContourLabelmapVolumeItk);

  // Compute gamma dose volume
  double checkpointDiceStart = timer->GetUniversalTime();
  Dice_statistics dice;
  dice.set_reference_image(referenceContourLabelmapVolumeItk);
  dice.set_compare_image(compareContourLabelmapVolumeItk);

  dice.run();

  this->ContourComparisonNode->SetDiceCoefficient(dice.get_dice());
  this->ContourComparisonNode->SetTruePositives(dice.get_true_positives());
  this->ContourComparisonNode->SetTrueNegatives(dice.get_true_negatives());
  this->ContourComparisonNode->SetFalsePositives(dice.get_false_positives());
  this->ContourComparisonNode->SetFalseNegatives(dice.get_false_negatives());

  itk::Vector<double, 3> referenceCenterItk = dice.get_reference_center();
  double referenceCenterArray[3]
    = { referenceCenterItk[0], referenceCenterItk[1], referenceCenterItk[2] };
  this->ContourComparisonNode->SetReferenceCenter(referenceCenterArray);

  itk::Vector<double, 3> compareCenterItk = dice.get_compare_center();
  double compareCenterArray[3]
  = { compareCenterItk[0], compareCenterItk[1], compareCenterItk[2] };
  this->ContourComparisonNode->SetCompareCenter(compareCenterArray);

  this->ContourComparisonNode->SetReferenceVolumeCc(dice.get_reference_volume());
  this->ContourComparisonNode->SetCompareVolumeCc(dice.get_compare_volume());
  this->ContourComparisonNode->ResultsValidOn();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    std::cout << "Total gamma computation time: " << checkpointEnd-checkpointStart << " s" << std::endl
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s" << std::endl
      << "\tConverting from VTK to ITK: " << checkpointDiceStart-checkpointItkConvertStart << " s" << std::endl
      << "\tDice computation: " << checkpointEnd-checkpointDiceStart << " s" << std::endl;
  }
}
