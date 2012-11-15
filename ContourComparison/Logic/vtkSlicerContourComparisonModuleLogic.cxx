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
#include "itk_image.h"
#include "dice_statistics.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkTimerLog.h>

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
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
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
void vtkSlicerContourComparisonModuleLogic::ComputeDiceStatistics(std::string &errorMessage)
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

  if (referenceContourNode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    referenceContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourComparisonNode->GetRasterizationReferenceVolumeNodeId() );
  }
  if (compareContourNode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    compareContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourComparisonNode->GetRasterizationReferenceVolumeNodeId() );
  }

  vtkMRMLScalarVolumeNode* referenceContourLabelmapVolumeNode
    = referenceContourNode->GetIndexedLabelmapVolumeNode();
  vtkMRMLScalarVolumeNode* compareContourLabelmapVolumeNode
    = compareContourNode->GetIndexedLabelmapVolumeNode();
  if (!referenceContourLabelmapVolumeNode || !compareContourLabelmapVolumeNode)
  {
    errorMessage = "Failed to get indexed labelmap representation from selected contours";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // Convert inputs to ITK images
  double checkpointItkConvertStart = timer->GetUniversalTime();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(referenceContourLabelmapVolumeNode, referenceContourLabelmapVolumeItk, true);
  SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(compareContourLabelmapVolumeNode, compareContourLabelmapVolumeItk, true);

  // Get voxel volume and number of voxels (the itk_image_header_compare check made sure the spacings match)
  itk::Image<unsigned char, 3>::SpacingType spacing = referenceContourLabelmapVolumeItk->GetSpacing();
  double voxelVolumeCc = spacing[0] * spacing[1] * spacing[2];
  itk::Image<unsigned char, 3>::RegionType region = referenceContourLabelmapVolumeItk->GetLargestPossibleRegion();
  unsigned long numberOfVoxels = region.GetNumberOfPixels();

  // Compute gamma dose volume
  double checkpointDiceStart = timer->GetUniversalTime();
  Dice_statistics dice;
  dice.set_reference_image(referenceContourLabelmapVolumeItk);
  dice.set_compare_image(compareContourLabelmapVolumeItk);

  dice.run();

  this->ContourComparisonNode->SetDiceCoefficient(dice.get_dice());
  this->ContourComparisonNode->SetTruePositivesPercent(dice.get_true_positives() * 100.0 / (double)numberOfVoxels);
  this->ContourComparisonNode->SetTrueNegativesPercent(dice.get_true_negatives() * 100.0 / (double)numberOfVoxels);
  this->ContourComparisonNode->SetFalsePositivesPercent(dice.get_false_positives() * 100.0 / (double)numberOfVoxels);
  this->ContourComparisonNode->SetFalseNegativesPercent(dice.get_false_negatives() * 100.0 / (double)numberOfVoxels);

  itk::Vector<double, 3> referenceCenterItk = dice.get_reference_center();
  double referenceCenterArray[3]
    = { referenceCenterItk[0], referenceCenterItk[1], referenceCenterItk[2] };
  this->ContourComparisonNode->SetReferenceCenter(referenceCenterArray);

  itk::Vector<double, 3> compareCenterItk = dice.get_compare_center();
  double compareCenterArray[3]
  = { compareCenterItk[0], compareCenterItk[1], compareCenterItk[2] };
  this->ContourComparisonNode->SetCompareCenter(compareCenterArray);

  this->ContourComparisonNode->SetReferenceVolumeCc(dice.get_reference_volume() * voxelVolumeCc);
  this->ContourComparisonNode->SetCompareVolumeCc(dice.get_compare_volume() * voxelVolumeCc);
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
