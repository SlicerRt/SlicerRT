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

// ContourComparison includes
#include "vtkSlicerContourComparisonModuleLogic.h"
#include "vtkMRMLContourComparisonNode.h"

// SlicerRT includes
#include "PlmCommon.h"
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"

// Plastimatch includes
#include "dice_statistics.h"
#include "hausdorff_distance.h"
#if OPENMP_FOUND
  #include <omp.h> //TODO: #227
#endif

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkSlicerContoursModuleLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkTimerLog.h>
#include <vtkObjectFactory.h>

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_ContourComparison
class vtkSlicerContourComparisonModuleLogicPrivate : public vtkObject
{
public:
  static vtkSlicerContourComparisonModuleLogicPrivate *New();
  vtkTypeMacro(vtkSlicerContourComparisonModuleLogicPrivate,vtkObject);

  /// Get input contours as labelmaps, then convert them to Plm_image volumes
  /// \return Error message, empty string if no error
  std::string GetInputContoursAsPlmVolumes(
    Plm_image::Pointer& plmRefContourLabelmap,
    Plm_image::Pointer& plmCmpContourLabelmap,
    double &checkpointItkConvertStart);

  void SetLogic(vtkSlicerContourComparisonModuleLogic* logic) { this->Logic = logic; };

protected:
  vtkSlicerContourComparisonModuleLogicPrivate();
  ~vtkSlicerContourComparisonModuleLogicPrivate();

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

//-----------------------------------------------------------------------------
vtkSlicerContourComparisonModuleLogicPrivate::~vtkSlicerContourComparisonModuleLogicPrivate()
{
  this->SetLogic(NULL);
}

//---------------------------------------------------------------------------
std::string vtkSlicerContourComparisonModuleLogicPrivate::GetInputContoursAsPlmVolumes(
  Plm_image::Pointer& plmRefContourLabelmap,
  Plm_image::Pointer& plmCmpContourLabelmap,
  double &checkpointItkConvertStart )
{
  if (!this->Logic->GetContourComparisonNode() || !this->Logic->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("GetInputContoursAsItkVolumes: " << errorMessage);
    return errorMessage;
  }

  vtkMRMLContourNode* referenceContourNode = this->Logic->GetContourComparisonNode()->GetReferenceContourNode();
  vtkMRMLContourNode* compareContourNode = this->Logic->GetContourComparisonNode()->GetCompareContourNode();

  if (!referenceContourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
  {
    referenceContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->Logic->GetContourComparisonNode()->GetRasterizationReferenceVolumeNode()->GetID() );
  }
  if (!compareContourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap))
  {
    compareContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->Logic->GetContourComparisonNode()->GetRasterizationReferenceVolumeNode()->GetID() );
  }

  if (!referenceContourNode->GetLabelmapImageData() || !compareContourNode->GetLabelmapImageData())
  {
    std::string errorMessage("Failed to get indexed labelmap representation from selected contours");
    vtkErrorMacro("GetInputContoursAsItkVolumes: " << errorMessage);
    return errorMessage;
  }

  // Convert inputs to ITK images
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  checkpointItkConvertStart = timer->GetUniversalTime();

  vtkMRMLScalarVolumeNode* refVolumeNode = vtkSlicerContoursModuleLogic::ExtractLabelmapFromContour(referenceContourNode);
  plmRefContourLabelmap = 
    PlmCommon::ConvertVolumeNodeToPlmImage (refVolumeNode);
  if (!plmRefContourLabelmap)
  {
    std::string errorMessage("Failed to convert contour labelmaps into Plm_image");
    vtkErrorMacro("GetInputContoursAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }
  this->Logic->GetMRMLScene()->RemoveNode(refVolumeNode);

  vtkMRMLScalarVolumeNode* compareVolumeNode = vtkSlicerContoursModuleLogic::ExtractLabelmapFromContour(compareContourNode);
  plmCmpContourLabelmap = 
    PlmCommon::ConvertVolumeNodeToPlmImage (compareVolumeNode);
  if (!plmCmpContourLabelmap)
  {
    std::string errorMessage("Failed to convert contour labelmaps into Plm_image");
    vtkErrorMacro("GetInputContoursAsPlmVolumes: " << errorMessage);
    return errorMessage;
  }
  this->Logic->GetMRMLScene()->RemoveNode(compareVolumeNode);

  return "";
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
  vtkSmartPointer<vtkSlicerContourComparisonModuleLogicPrivate> logicPrivate =
    vtkSmartPointer<vtkSlicerContourComparisonModuleLogicPrivate>::New();
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
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourComparisonNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContourComparisonModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
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
void vtkSlicerContourComparisonModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
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
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSlicerContourComparisonModuleLogic::ComputeDiceStatistics()
{
  if (!this->ContourComparisonNode || !this->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeDiceStatistics: " << errorMessage);
    return errorMessage;
  }

  this->ContourComparisonNode->DiceResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed
  double checkpointItkConvertStart = 0.0;

  // Convert input images to the format Plastimatch can use
  Plm_image::Pointer plmRefContourLabelmap;
  Plm_image::Pointer plmCmpContourLabelmap;
  std::string inputToPlmResult = this->LogicPrivate->GetInputContoursAsPlmVolumes(plmRefContourLabelmap, plmCmpContourLabelmap, checkpointItkConvertStart);
  if (!inputToPlmResult.empty())
  {
    std::string errorMessage("Error occurred during ITK conversion");
    vtkErrorMacro("ComputeDiceStatistics: " << errorMessage);
    return errorMessage;
  }

  // Get voxel volume and number of voxels (the itk_image_header_compare check made sure the spacings match)
  double voxelVolumeCc = plmRefContourLabelmap->spacing(0) * plmRefContourLabelmap->spacing(1) * plmRefContourLabelmap->spacing(2);
  unsigned long numberOfVoxels = plmRefContourLabelmap->dim(0) * plmRefContourLabelmap->dim(1) * plmRefContourLabelmap->dim(2);

  //// Compute Dice similarity metrics
  double checkpointDiceStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointDiceStart); // Although it is used later, a warning is logged so needs to be suppressed
  Dice_statistics dice;
  dice.set_reference_image(plmRefContourLabelmap->itk_uchar());
  dice.set_compare_image(plmCmpContourLabelmap->itk_uchar());

  dice.run();

  this->ContourComparisonNode->SetDiceCoefficient(dice.get_dice());
  this->ContourComparisonNode->SetTruePositivesPercent(dice.get_true_positives() * 100.0 / (double)numberOfVoxels);
  this->ContourComparisonNode->SetTrueNegativesPercent(dice.get_true_negatives() * 100.0 / (double)numberOfVoxels);
  this->ContourComparisonNode->SetFalsePositivesPercent(dice.get_false_positives() * 100.0 / (double)numberOfVoxels);
  this->ContourComparisonNode->SetFalseNegativesPercent(dice.get_false_negatives() * 100.0 / (double)numberOfVoxels);

  itk::Vector<double, 3> referenceCenterItk = dice.get_reference_center();
  double referenceCenterArray[3] =
    { referenceCenterItk[0], referenceCenterItk[1], referenceCenterItk[2] };
  this->ContourComparisonNode->SetReferenceCenter(referenceCenterArray);

  itk::Vector<double, 3> compareCenterItk = dice.get_compare_center();
  double compareCenterArray[3] =
    { compareCenterItk[0], compareCenterItk[1], compareCenterItk[2] };
  this->ContourComparisonNode->SetCompareCenter(compareCenterArray);

  this->ContourComparisonNode->SetReferenceVolumeCc(dice.get_reference_volume() * voxelVolumeCc);
  this->ContourComparisonNode->SetCompareVolumeCc(dice.get_compare_volume() * voxelVolumeCc);
  this->ContourComparisonNode->DiceResultsValidOn();

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
std::string vtkSlicerContourComparisonModuleLogic::ComputeHausdorffDistances()
{
  if (!this->ContourComparisonNode || !this->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeHausdorffDistances: " << errorMessage);
    return errorMessage;
  }

  this->ContourComparisonNode->HausdorffResultsValidOff();

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed
  double checkpointItkConvertStart = 0.0;

  // Convert input images to the format Plastimatch can use
  Plm_image::Pointer plmRefContourLabelmap;
  Plm_image::Pointer plmCmpContourLabelmap;
  std::string inputToPlmResult = this->LogicPrivate->GetInputContoursAsPlmVolumes(plmRefContourLabelmap, plmCmpContourLabelmap, checkpointItkConvertStart);
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
  hausdorff.set_reference_image(plmRefContourLabelmap->itk_uchar());
  hausdorff.set_compare_image(plmCmpContourLabelmap->itk_uchar());

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
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
    vtkDebugMacro("ComputeHausdorffDistances: Total Hausdorff computation time: " << checkpointEnd-checkpointStart << " s\n"
      << "\tApplying transforms: " << checkpointItkConvertStart-checkpointStart << " s\n"
      << "\tConverting from VTK to ITK: " << checkpointHausdorffStart-checkpointItkConvertStart << " s\n"
      << "\tHausdorff computation: " << checkpointEnd-checkpointHausdorffStart << " s");
  }

  return "";
}
