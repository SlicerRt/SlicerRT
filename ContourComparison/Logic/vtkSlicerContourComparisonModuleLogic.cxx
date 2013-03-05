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
#include "hausdorff_distance.h"
#if OPENMP_FOUND
#include <omp.h>
#endif


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

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ContourComparison
class vtkSlicerContourComparisonModuleLogicPrivate : public vtkObject
{
public:
  static vtkSlicerContourComparisonModuleLogicPrivate *New();
  vtkTypeMacro(vtkSlicerContourComparisonModuleLogicPrivate,vtkObject);

  /// Get input contours as labelmaps, then convert them to ITK volumes
  void GetInputContoursAsItkVolumes( itk::Image<unsigned char, 3>::Pointer referenceContourLabelmapVolumeItk,
    itk::Image<unsigned char, 3>::Pointer compareContourLabelmapVolumeItk, double &checkpointItkConvertStart, std::string & errorMessage );

  vtkSetObjectMacro(Logic, vtkSlicerContourComparisonModuleLogic);

protected:
  vtkSlicerContourComparisonModuleLogicPrivate();

  vtkSlicerContourComparisonModuleLogic* Logic;
};

//-----------------------------------------------------------------------------
// vtkSlicerContourComparisonModuleLogicPrivate methods

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContourComparisonModuleLogicPrivate);

//-----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogicPrivate::vtkSlicerContourComparisonModuleLogicPrivate()
: Logic(NULL)
{
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogicPrivate::GetInputContoursAsItkVolumes(
  itk::Image<unsigned char, 3>::Pointer referenceContourLabelmapVolumeItk,
  itk::Image<unsigned char, 3>::Pointer compareContourLabelmapVolumeItk,
  double &checkpointItkConvertStart, std::string & errorMessage )
{
  if (!this->Logic->GetContourComparisonNode() || !this->Logic->GetMRMLScene())
  {
    return;
  }

  vtkMRMLContourNode* referenceContourNode = vtkMRMLContourNode::SafeDownCast(
    this->Logic->GetMRMLScene()->GetNodeByID(this->Logic->GetContourComparisonNode()->GetReferenceContourNodeId()));
  vtkMRMLContourNode* compareContourNode = vtkMRMLContourNode::SafeDownCast(
    this->Logic->GetMRMLScene()->GetNodeByID(this->Logic->GetContourComparisonNode()->GetCompareContourNodeId()));

  if (referenceContourNode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    referenceContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->Logic->GetContourComparisonNode()->GetRasterizationReferenceVolumeNodeId() );
  }
  if (compareContourNode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    compareContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->Logic->GetContourComparisonNode()->GetRasterizationReferenceVolumeNodeId() );
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
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  checkpointItkConvertStart = timer->GetUniversalTime();

  if ( SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(referenceContourLabelmapVolumeNode, referenceContourLabelmapVolumeItk, true) == false
    || SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(compareContourLabelmapVolumeNode, compareContourLabelmapVolumeItk, true) == false )
  {
    errorMessage = "Failed to convert contour labelmaps to ITK volumes!";
    vtkErrorMacro(<<errorMessage);
    return;
  }
}

//-----------------------------------------------------------------------------
// vtkSlicerContourComparisonModuleLogic methods

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContourComparisonModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerContourComparisonModuleLogic, LogicPrivate, vtkSlicerContourComparisonModuleLogicPrivate);

//----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogic::vtkSlicerContourComparisonModuleLogic()
{
  this->ContourComparisonNode = NULL;

  this->LogicPrivate = NULL;
  vtkSmartPointer<vtkSlicerContourComparisonModuleLogicPrivate> logicPrivate
    = vtkSmartPointer<vtkSlicerContourComparisonModuleLogicPrivate>::New();
  logicPrivate->SetLogic(this);
  this->SetLogicPrivate(logicPrivate);

  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogic::~vtkSlicerContourComparisonModuleLogic()
{
  this->SetAndObserveContourComparisonNode(NULL);
  this->SetLogicPrivate(NULL);
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

  this->ContourComparisonNode->DiceResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  double checkpointItkConvertStart;

  // Convert input images to the format Plastimatch can use
  itk::Image<unsigned char, 3>::Pointer referenceContourLabelmapVolumeItk
    = itk::Image<unsigned char, 3>::New();
  itk::Image<unsigned char, 3>::Pointer compareContourLabelmapVolumeItk
    = itk::Image<unsigned char, 3>::New();
  this->LogicPrivate->GetInputContoursAsItkVolumes(referenceContourLabelmapVolumeItk, compareContourLabelmapVolumeItk, checkpointItkConvertStart, errorMessage);
  if (!errorMessage.empty())
  {
    return;
  }

  // Get voxel volume and number of voxels (the itk_image_header_compare check made sure the spacings match)
  itk::Image<unsigned char, 3>::SpacingType spacing = referenceContourLabelmapVolumeItk->GetSpacing();
  double voxelVolumeCc = spacing[0] * spacing[1] * spacing[2];
  itk::Image<unsigned char, 3>::RegionType region = referenceContourLabelmapVolumeItk->GetLargestPossibleRegion();
  unsigned long numberOfVoxels = region.GetNumberOfPixels();

  //// Compute Dice similarity metrics
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
  this->ContourComparisonNode->DiceResultsValidOn();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    std::cout << "Total Dice computation time: " << checkpointEnd-checkpointStart << " s" << std::endl
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s" << std::endl
      << "\tConverting from VTK to ITK: " << checkpointDiceStart-checkpointItkConvertStart << " s" << std::endl
      << "\tDice computation: " << checkpointEnd-checkpointDiceStart << " s" << std::endl;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::ComputeHausdorffDistances(std::string &errorMessage)
{
  if (!this->ContourComparisonNode || !this->GetMRMLScene())
  {
    return;
  }

  this->ContourComparisonNode->HausdorffResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  double checkpointItkConvertStart;

  // Convert input images to the format Plastimatch can use
  itk::Image<unsigned char, 3>::Pointer referenceContourLabelmapVolumeItk
    = itk::Image<unsigned char, 3>::New();
  itk::Image<unsigned char, 3>::Pointer compareContourLabelmapVolumeItk
    = itk::Image<unsigned char, 3>::New();
  this->LogicPrivate->GetInputContoursAsItkVolumes(referenceContourLabelmapVolumeItk, compareContourLabelmapVolumeItk, checkpointItkConvertStart, errorMessage);
  if (!errorMessage.empty())
  {
    return;
  }

  // Compute Hausdorff distances
  double checkpointHausdorffStart = timer->GetUniversalTime();
  Hausdorff_distance hausdorff;
  hausdorff.set_reference_image(referenceContourLabelmapVolumeItk);
  hausdorff.set_compare_image(compareContourLabelmapVolumeItk);

  hausdorff.run();

  this->ContourComparisonNode->SetMaximumHausdorffDistanceForVolumeMm(hausdorff.get_boundary_hausdorff());
  this->ContourComparisonNode->SetMaximumHausdorffDistanceForBoundaryMm(hausdorff.get_hausdorff());
  this->ContourComparisonNode->SetAverageHausdorffDistanceForVolumeMm(hausdorff.get_average_hausdorff());
  this->ContourComparisonNode->SetAverageHausdorffDistanceForBoundaryMm(hausdorff.get_average_boundary_hausdorff());
  this->ContourComparisonNode->SetPercent95HausdorffDistanceForVolumeMm(hausdorff.get_percent_hausdorff());
  this->ContourComparisonNode->SetPercent95HausdorffDistanceForBoundaryMm(hausdorff.get_percent_boundary_hausdorff());
  this->ContourComparisonNode->HausdorffResultsValidOn();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    std::cout << "Total Hausdorff computation time: " << checkpointEnd-checkpointStart << " s" << std::endl
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s" << std::endl
      << "\tConverting from VTK to ITK: " << checkpointHausdorffStart-checkpointItkConvertStart << " s" << std::endl
      << "\tHausdorff computation: " << checkpointEnd-checkpointHausdorffStart << " s" << std::endl;
  }
}
