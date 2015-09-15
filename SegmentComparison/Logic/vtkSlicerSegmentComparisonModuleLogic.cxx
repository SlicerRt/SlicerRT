/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// SegmentComparison includes
#include "vtkSlicerSegmentComparisonModuleLogic.h"
#include "vtkMRMLSegmentComparisonNode.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// SegmentationCore includes
#include "vtkOrientedImageDataResample.h"

// SlicerRT includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"

// Plastimatch includes
#include "dice_statistics.h"
#include "hausdorff_distance.h"
#if OPENMP_FOUND
  #include <omp.h> //TODO: #227
#endif

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkTimerLog.h>
#include <vtkObjectFactory.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_SegmentComparison
class vtkSlicerSegmentComparisonModuleLogicPrivate : public vtkObject
{
public:
  static vtkSlicerSegmentComparisonModuleLogicPrivate *New();
  vtkTypeMacro(vtkSlicerSegmentComparisonModuleLogicPrivate,vtkObject);

  /// Get input segments as labelmaps, then convert them to Plm_image volumes
  /// \return Error message, empty string if no error
  std::string GetInputSegmentsAsPlmVolumes(
    Plm_image::Pointer& plmRefSegmentLabelmap,
    Plm_image::Pointer& plmCmpSegmentLabelmap,
    double &checkpointItkConvertStart);

  void SetLogic(vtkSlicerSegmentComparisonModuleLogic* logic) { this->Logic = logic; };

protected:
  vtkSlicerSegmentComparisonModuleLogicPrivate();
  ~vtkSlicerSegmentComparisonModuleLogicPrivate();

  vtkSlicerSegmentComparisonModuleLogic* Logic;
};

//-----------------------------------------------------------------------------
// vtkSlicerSegmentComparisonModuleLogicPrivate methods

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSegmentComparisonModuleLogicPrivate);

//-----------------------------------------------------------------------------
vtkSlicerSegmentComparisonModuleLogicPrivate::vtkSlicerSegmentComparisonModuleLogicPrivate()
: Logic(NULL)
{
}

//-----------------------------------------------------------------------------
vtkSlicerSegmentComparisonModuleLogicPrivate::~vtkSlicerSegmentComparisonModuleLogicPrivate()
{
  this->SetLogic(NULL);
}

//---------------------------------------------------------------------------
std::string vtkSlicerSegmentComparisonModuleLogicPrivate::GetInputSegmentsAsPlmVolumes(
  Plm_image::Pointer& plmRefSegmentLabelmap,
  Plm_image::Pointer& plmCmpSegmentLabelmap,
  double &checkpointItkConvertStart )
{
  if (!this->Logic->GetSegmentComparisonNode() || !this->Logic->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }

  // Get selection
  vtkMRMLSegmentationNode* referenceSegmentationNode = this->Logic->GetSegmentComparisonNode()->GetReferenceSegmentationNode();
  const char* referenceSegmentID = this->Logic->GetSegmentComparisonNode()->GetReferenceSegmentID();
  vtkMRMLSegmentationNode* compareSegmentationNode = this->Logic->GetSegmentComparisonNode()->GetCompareSegmentationNode();
  const char* compareSegmentID = this->Logic->GetSegmentComparisonNode()->GetCompareSegmentID();

  if (!referenceSegmentationNode || !referenceSegmentID)
  {
    std::string errorMessage("Invalid reference segment selection");
    vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }
  if (!compareSegmentationNode || !compareSegmentID)
  {
    std::string errorMessage("Invalid compare segment selection");
    vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }

  // Get segments
  vtkSegmentation* referenceSegmentation = referenceSegmentationNode->GetSegmentation();
  vtkSegment* referenceSegment = referenceSegmentation->GetSegment(referenceSegmentID);
  vtkSegmentation* compareSegmentation = compareSegmentationNode->GetSegmentation();
  vtkSegment* compareSegment = compareSegmentation->GetSegment(compareSegmentID);
  if (!referenceSegment || !compareSegment)
  {
    std::string errorMessage("Failed to get selected segments");
    vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }

  // Get binary labelmap representations of the reference segment
  vtkSmartPointer<vtkOrientedImageData> referenceSegmentLabelmap;
  if ( referenceSegmentation->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) )
  {
    // Temporarily duplicate segment, as it may be transformed
    referenceSegmentLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    referenceSegmentLabelmap->DeepCopy( vtkOrientedImageData::SafeDownCast(
      referenceSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
  }
  else // Need to convert
  {
    // Temporarily duplicate selected segment to only convert them, not the whole segmentation (to save time)
    referenceSegmentLabelmap = vtkSmartPointer<vtkOrientedImageData>::Take( vtkOrientedImageData::SafeDownCast(
      vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment( referenceSegmentation, referenceSegmentID,
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) ) );
    if (!referenceSegmentLabelmap.GetPointer())
    {
      std::string errorMessage("Failed to convert reference segment into binary labelmap\nPlease convert it in Segmentations module using Advanced conversion");
      vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
      return errorMessage;
    }
  }

  // Get binary labelmap representations of the compare segment
  vtkSmartPointer<vtkOrientedImageData> compareSegmentLabelmap;
  if ( compareSegmentation->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) )
  {
    // Temporarily duplicate segment, as it may be transformed
    compareSegmentLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    compareSegmentLabelmap->DeepCopy( vtkOrientedImageData::SafeDownCast(
      compareSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
  }
  else // Need to convert
  {
    // Temporarily duplicate selected segment to only convert them, not the whole segmentation (to save time)
    compareSegmentLabelmap = vtkSmartPointer<vtkOrientedImageData>::Take( vtkOrientedImageData::SafeDownCast(
      vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment( compareSegmentation, compareSegmentID,
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) ) );
    if (!referenceSegmentLabelmap.GetPointer())
    {
      std::string errorMessage("Failed to convert compare segment into binary labelmap\nPlease convert it in Segmentations module using Advanced conversion");
      vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
      return errorMessage;
    }
  }

  // Apply parent transformation nodes if necessary
  if ( referenceSegmentationNode != compareSegmentationNode
    && referenceSegmentationNode->GetParentTransformNode() != compareSegmentationNode->GetParentTransformNode() )
  {
    if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(referenceSegmentationNode, referenceSegmentLabelmap))
    {
      std::string errorMessage("Failed to apply parent transformation to compare segment!");
      vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
      return errorMessage;
    }
    if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(compareSegmentationNode, compareSegmentLabelmap))
    {
      std::string errorMessage("Failed to apply parent transformation to reference segment!");
      vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
      return errorMessage;
    }
  }

  // Convert inputs to ITK images
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  checkpointItkConvertStart = timer->GetUniversalTime();

  plmRefSegmentLabelmap = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(referenceSegmentLabelmap);
  if (!plmRefSegmentLabelmap)
  {
    std::string errorMessage("Failed to convert reference segment labelmap into Plm_image");
    vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }

  plmCmpSegmentLabelmap = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(compareSegmentLabelmap);
  if (!plmCmpSegmentLabelmap)
  {
    std::string errorMessage("Failed to convert compare segment labelmap into Plm_image");
    vtkErrorMacro("GetInputSegmentsAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }

  return "";
}

//-----------------------------------------------------------------------------
// vtkSlicerSegmentComparisonModuleLogic methods

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSegmentComparisonModuleLogic);
vtkCxxSetObjectMacro(vtkSlicerSegmentComparisonModuleLogic, LogicPrivate, vtkSlicerSegmentComparisonModuleLogicPrivate);

//----------------------------------------------------------------------------
vtkSlicerSegmentComparisonModuleLogic::vtkSlicerSegmentComparisonModuleLogic()
{
  this->SegmentComparisonNode = NULL;

  this->LogicPrivate = NULL;
  vtkSmartPointer<vtkSlicerSegmentComparisonModuleLogicPrivate> logicPrivate =
    vtkSmartPointer<vtkSlicerSegmentComparisonModuleLogicPrivate>::New();
  logicPrivate->SetLogic(this);
  this->SetLogicPrivate(logicPrivate);

  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkSlicerSegmentComparisonModuleLogic::~vtkSlicerSegmentComparisonModuleLogic()
{
  this->SetAndObserveSegmentComparisonNode(NULL);
  this->SetLogicPrivate(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::SetAndObserveSegmentComparisonNode(vtkMRMLSegmentComparisonNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->SegmentComparisonNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerSegmentComparisonModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLSegmentComparisonNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLSegmentComparisonNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLSegmentComparisonNode");
  if (node)
  {
    paramNode = vtkMRMLSegmentComparisonNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->SegmentComparisonNode, paramNode);
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentComparisonModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSlicerSegmentComparisonModuleLogic::ComputeDiceStatistics()
{
  if (!this->SegmentComparisonNode || !this->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeDiceStatistics: " << errorMessage);
    return errorMessage;
  }

  this->SegmentComparisonNode->DiceResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed
  double checkpointItkConvertStart = 0.0;

  // Convert input images to the format Plastimatch can use
  Plm_image::Pointer plmRefSegmentLabelmap;
  Plm_image::Pointer plmCmpSegmentLabelmap;
  std::string inputToPlmResult = this->LogicPrivate->GetInputSegmentsAsPlmVolumes(plmRefSegmentLabelmap, plmCmpSegmentLabelmap, checkpointItkConvertStart);
  if (!inputToPlmResult.empty())
  {
    std::string errorMessage("Error occurred during ITK conversion");
    vtkErrorMacro("ComputeDiceStatistics: " << errorMessage);
    return errorMessage;
  }

  // Compute Dice similarity metrics
  double checkpointDiceStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointDiceStart); // Although it is used later, a warning is logged so needs to be suppressed
  Dice_statistics dice;
  dice.set_reference_image(plmRefSegmentLabelmap->itk_uchar());
  dice.set_compare_image(plmCmpSegmentLabelmap->itk_uchar());

  dice.run();

  unsigned long numberOfVoxels = dice.get_true_positives() 
    + dice.get_true_negatives() + dice.get_false_positives()
    + dice.get_false_negatives();

  this->SegmentComparisonNode->SetDiceCoefficient(dice.get_dice());
  this->SegmentComparisonNode->SetTruePositivesPercent(dice.get_true_positives() * 100.0 / (double)numberOfVoxels);
  this->SegmentComparisonNode->SetTrueNegativesPercent(dice.get_true_negatives() * 100.0 / (double)numberOfVoxels);
  this->SegmentComparisonNode->SetFalsePositivesPercent(dice.get_false_positives() * 100.0 / (double)numberOfVoxels);
  this->SegmentComparisonNode->SetFalseNegativesPercent(dice.get_false_negatives() * 100.0 / (double)numberOfVoxels);

  itk::Vector<double, 3> referenceCenterItk = dice.get_reference_center();
  double referenceCenterArray[3] =
    { - referenceCenterItk[0], - referenceCenterItk[1], referenceCenterItk[2] };
  this->SegmentComparisonNode->SetReferenceCenter(referenceCenterArray);

  itk::Vector<double, 3> compareCenterItk = dice.get_compare_center();
  double compareCenterArray[3] =
    { - compareCenterItk[0], - compareCenterItk[1], compareCenterItk[2] };
  this->SegmentComparisonNode->SetCompareCenter(compareCenterArray);

  this->SegmentComparisonNode->SetReferenceVolumeCc(dice.get_reference_volume() / 1000.0);
  this->SegmentComparisonNode->SetCompareVolumeCc(dice.get_compare_volume() / 1000.0);
  this->SegmentComparisonNode->DiceResultsValidOn();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
    vtkDebugMacro("ComputeDiceStatistics: Total Dice computation time: " << checkpointEnd-checkpointStart << " s\n"
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s\n"
      << "\tConverting from VTK to ITK: " << checkpointDiceStart-checkpointItkConvertStart << " s\n"
      << "\tDice computation: " << checkpointEnd-checkpointDiceStart << " s");
  }

  return "";
}

//---------------------------------------------------------------------------
std::string vtkSlicerSegmentComparisonModuleLogic::ComputeHausdorffDistances()
{
  if (!this->SegmentComparisonNode || !this->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeHausdorffDistances: " << errorMessage);
    return errorMessage;
  }

  this->SegmentComparisonNode->HausdorffResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed
  double checkpointItkConvertStart = 0.0;

  // Convert input images to the format Plastimatch can use
  Plm_image::Pointer plmRefSegmentLabelmap;
  Plm_image::Pointer plmCmpSegmentLabelmap;
  std::string inputToPlmResult = this->LogicPrivate->GetInputSegmentsAsPlmVolumes(plmRefSegmentLabelmap, plmCmpSegmentLabelmap, checkpointItkConvertStart);
  if (!inputToPlmResult.empty())
  {
    std::string errorMessage("Error occurred during ITK conversion");
    vtkErrorMacro("ComputeHausdorffDistances: " << errorMessage);
    return errorMessage;
  }

  // Compute Hausdorff distances
  double checkpointHausdorffStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointHausdorffStart); // Although it is used later, a warning is logged so needs to be suppressed
  Hausdorff_distance hausdorff;
  hausdorff.set_reference_image(plmRefSegmentLabelmap->itk_uchar());
  hausdorff.set_compare_image(plmCmpSegmentLabelmap->itk_uchar());

  hausdorff.run();

  this->SegmentComparisonNode->SetMaximumHausdorffDistanceForVolumeMm(hausdorff.get_boundary_hausdorff());
  this->SegmentComparisonNode->SetMaximumHausdorffDistanceForBoundaryMm(hausdorff.get_hausdorff());
  this->SegmentComparisonNode->SetAverageHausdorffDistanceForVolumeMm(hausdorff.get_avg_average_hausdorff());
  this->SegmentComparisonNode->SetAverageHausdorffDistanceForBoundaryMm(hausdorff.get_avg_average_boundary_hausdorff());
  this->SegmentComparisonNode->SetPercent95HausdorffDistanceForVolumeMm(hausdorff.get_percent_hausdorff());
  this->SegmentComparisonNode->SetPercent95HausdorffDistanceForBoundaryMm(hausdorff.get_percent_boundary_hausdorff());
  this->SegmentComparisonNode->HausdorffResultsValidOn();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
    vtkDebugMacro("ComputeHausdorffDistances: Total Hausdorff computation time: " << checkpointEnd-checkpointStart << " s\n"
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s\n"
      << "\tConverting from VTK to ITK: " << checkpointHausdorffStart-checkpointItkConvertStart << " s\n"
      << "\tHausdorff computation: " << checkpointEnd-checkpointHausdorffStart << " s");
  }

  return "";
}
