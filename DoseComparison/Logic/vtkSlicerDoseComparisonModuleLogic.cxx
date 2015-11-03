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

// DoseComparison includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "PlmCommon.h"

// Plastimatch includes
#include "gamma_dose_comparison.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkOrientedImageData.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// MRMLLogic includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLApplicationLogic.h>
#include <vtkSlicerSubjectHierarchyModuleLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkTimerLog.h>
#include <vtkLookupTable.h>
#include <vtkImageConstantPad.h>
#include <vtkObjectFactory.h>
#include "vtksys/SystemTools.hxx"

// SlicerBase includes
#include "vtkSlicerApplicationLogic.h"

//---------------------------------------------------------------------------
const char* vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME = "DoseComparison.GammaVolume"; // Identifier
const char* vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME = "Gamma_ColorTable.ctbl";
const std::string vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_OUTPUT_BASE_NAME_PREFIX = "GammaVolume_";
const std::string vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_REFERENCE_DOSE_VOLUME_REFERENCE_ROLE = "referenceDoseVolume" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_COMPARE_DOSE_VOLUME_REFERENCE_ROLE = "compareDoseVolume" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference

//---------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic* LogicInstance = NULL;
void GammaProgressCallback(float progress)
{
  if (LogicInstance)
  {
    LogicInstance->GammaProgressUpdated(progress);
  }
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseComparisonModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::vtkSlicerDoseComparisonModuleLogic()
{
  this->DoseComparisonNode = NULL;
  this->DefaultGammaColorTableNodeId = NULL;
  this->Progress = 0.0;

  this->LogSpeedMeasurementsOff();

  LogicInstance = this;
}

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::~vtkSlicerDoseComparisonModuleLogic()
{
  this->SetAndObserveDoseComparisonNode(NULL);
  this->SetDefaultGammaColorTableNodeId(NULL);

  LogicInstance = NULL;
}

//----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetAndObserveDoseComparisonNode(vtkMRMLDoseComparisonNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseComparisonNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());

  // Load default gamma color table
  this->LoadDefaultGammaColorTable();
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseComparisonNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
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
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
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
void vtkSlicerDoseComparisonModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLDoseComparisonNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseComparisonNode");
  if (node)
  {
    paramNode = vtkMRMLDoseComparisonNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseComparisonNode, paramNode);
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::GammaProgressUpdated(float progress)
{
  double progressDouble = (double)progress;
  this->Progress = progressDouble;
  this->InvokeEvent(SlicerRtCommon::ProgressUpdated, (void*)&progressDouble);
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseComparisonModuleLogic::ComputeGammaDoseDifference()
{
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  this->DoseComparisonNode->ResultsValidOff();

  double checkpointConvertStart = timer->GetUniversalTime();
  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = this->DoseComparisonNode->GetReferenceDoseVolumeNode();
  Plm_image::Pointer referenceDose = PlmCommon::ConvertVolumeNodeToPlmImage(referenceDoseVolumeNode);
  Plm_image::Pointer compareDose = PlmCommon::ConvertVolumeNodeToPlmImage(this->DoseComparisonNode->GetCompareDoseVolumeNode());

  Plm_image::Pointer maskVolume;
  vtkMRMLSegmentationNode* maskSegmentationNode = this->DoseComparisonNode->GetMaskSegmentationNode();
  const char* maskSegmentID = this->DoseComparisonNode->GetMaskSegmentID();
  if (maskSegmentationNode && maskSegmentID)
  {
    // Extract a labelmap for the dose comparison to use it as a mask
    vtkSegmentation* maskSegmentation = maskSegmentationNode->GetSegmentation();
    vtkSegment* maskSegment = maskSegmentation->GetSegment(maskSegmentID);
    if (!maskSegment)
    {
      std::string errorMessage("Failed to get mask segment");
      vtkErrorMacro("ComputeGammaDoseDifference: " << errorMessage);
      return errorMessage;
    }

    // Temporarily duplicate selected segments to contain binary labelmap of a different geometry (tied to dose volume)
    vtkSmartPointer<vtkSegmentation> segmentationCopy = vtkSmartPointer<vtkSegmentation>::New();
    segmentationCopy->SetMasterRepresentationName(maskSegmentation->GetMasterRepresentationName());
    segmentationCopy->CopyConversionParameters(maskSegmentation);
    segmentationCopy->CopySegmentFromSegmentation(maskSegmentation, maskSegmentID);
    if (!segmentationCopy->CreateRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      std::string errorMessage("Failed to create binary labelmap representation for mask segment");
      vtkErrorMacro("ComputeGammaDoseDifference: " << errorMessage);
      return errorMessage;
    }
    // Get segment binary labelmap
    vtkOrientedImageData* maskSegmentLabelmap = vtkOrientedImageData::SafeDownCast( segmentationCopy->GetSegment(maskSegmentID)->GetRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) );

    // Apply parent transformation nodes if necessary
    if ( maskSegmentationNode->GetParentTransformNode()
      && (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(maskSegmentationNode, maskSegmentLabelmap)) )
    {
      std::string errorMessage("Failed to apply parent transform on mask segment");
      vtkErrorMacro("ComputeGammaDoseDifference: " << errorMessage);
      return errorMessage;
    }

    // Convert mask to Plm image
    maskVolume = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(maskSegmentLabelmap);
    if (!maskVolume)
    {
      std::string errorMessage("Failed to convert mask segment labelmap into Plm_image");
      vtkErrorMacro("ComputeGammaDoseDifference: " << errorMessage);
      return errorMessage;
    }
  }

  // Compute gamma dose volume
  double checkpointGammaStart = timer->GetUniversalTime();
  Gamma_dose_comparison gamma;
  gamma.set_reference_image(referenceDose->itk_float());
  gamma.set_compare_image(compareDose->itk_float());
  if (maskSegmentationNode)
  {
    gamma.set_mask_image(maskVolume->itk_uchar());
  }
  gamma.set_spatial_tolerance(this->DoseComparisonNode->GetDtaDistanceToleranceMm());
  gamma.set_dose_difference_tolerance(this->DoseComparisonNode->GetDoseDifferenceTolerancePercent() / 100.0);
  gamma.set_resample_nn(!this->DoseComparisonNode->GetUseLinearInterpolation());
  if (!this->DoseComparisonNode->GetUseMaximumDose())
  {
    gamma.set_reference_dose(this->DoseComparisonNode->GetReferenceDoseGy());
  }
  gamma.set_analysis_threshold(this->DoseComparisonNode->GetAnalysisThresholdPercent() / 100.0 );
  gamma.set_gamma_max(this->DoseComparisonNode->GetMaximumGamma());
  gamma.set_ref_only_threshold(this->DoseComparisonNode->GetDoseThresholdOnReferenceOnly());
  gamma.set_progress_callback(&GammaProgressCallback);

  gamma.run();

  itk::Image<float, 3>::Pointer gammaVolumeItk = gamma.get_gamma_image_itk();
  this->DoseComparisonNode->SetPassFractionPercent( gamma.get_pass_fraction() * 100.0 );
  this->DoseComparisonNode->SetReportString(gamma.get_report_string().c_str());

  // Convert output to VTK
  double checkpointVtkConvertStart = timer->GetUniversalTime();

  vtkMRMLScalarVolumeNode* gammaVolumeNode = this->DoseComparisonNode->GetGammaVolumeNode();
  if (gammaVolumeNode == NULL)
  {
    std::string errorMessage("Invalid gamma volume node in parameter set node");
    vtkErrorMacro("ComputeGammaDoseDifference: " << errorMessage);
    return errorMessage;
  }

  SlicerRtCommon::ConvertItkImageToVolumeNode<float>(gammaVolumeItk, gammaVolumeNode, VTK_FLOAT);
  gammaVolumeNode->SetAttribute(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME, "1");

  // Set default colormap to red
  if (gammaVolumeNode->GetVolumeDisplayNode() == NULL)
  {
    vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
    displayNode->SetScene(this->GetMRMLScene());
    this->GetMRMLScene()->AddNode(displayNode);
    gammaVolumeNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  }
  if (gammaVolumeNode->GetVolumeDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* gammaScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(gammaVolumeNode->GetVolumeDisplayNode());
    gammaScalarVolumeDisplayNode->SetAutoWindowLevel(0);
    gammaScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, this->DoseComparisonNode->GetMaximumGamma());

    if (this->DefaultGammaColorTableNodeId)
    {
      gammaScalarVolumeDisplayNode->SetAndObserveColorNodeID(this->DefaultGammaColorTableNodeId);
    }
    else
    {
      vtkWarningMacro("ComputeGammaDoseDifference: Loading gamma color table failed, stock color table is used!");
      gammaScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    }
  }
  else
  {
    vtkWarningMacro("ComputeGammaDoseDifference: Display node is not available for gamma volume node. The default color table will be used.");
  }

  // Determine if the input dose volumes are in subject hierarchy. Only perform related tasks if they are.
  bool areInputsInSubjectHierarchy =
    ( vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this->DoseComparisonNode->GetReferenceDoseVolumeNode())
    && vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this->DoseComparisonNode->GetCompareDoseVolumeNode()) );
  if (areInputsInSubjectHierarchy)
  {
    // Get common ancestor of the two input dose volumes
    vtkMRMLSubjectHierarchyNode* commonAncestor = vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch(
      this->DoseComparisonNode->GetReferenceDoseVolumeNode(), this->DoseComparisonNode->GetCompareDoseVolumeNode(),
      vtkMRMLSubjectHierarchyConstants::GetDICOMLevelPatient() );

    // Add gamma volume to subject hierarchy
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetMRMLScene(), commonAncestor, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
      gammaVolumeNode->GetName(), gammaVolumeNode);
  }

  // Add connection attribute to input dose volume nodes
  gammaVolumeNode->AddNodeReferenceID( vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_REFERENCE_DOSE_VOLUME_REFERENCE_ROLE.c_str(), 
    this->DoseComparisonNode->GetReferenceDoseVolumeNode()->GetID() );
  gammaVolumeNode->AddNodeReferenceID( vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_COMPARE_DOSE_VOLUME_REFERENCE_ROLE.c_str(),
    this->DoseComparisonNode->GetCompareDoseVolumeNode()->GetID() );

  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(gammaVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  this->DoseComparisonNode->ResultsValidOn();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    std::cout << "Total gamma computation time: " << checkpointEnd-checkpointStart << " s" << std::endl
              << "\tApplying transforms: " << checkpointConvertStart-checkpointStart << " s" << std::endl
              << "\tConverting from VTK to ITK: " << checkpointGammaStart-checkpointConvertStart << " s" << std::endl
              << "\tGamma computation: " << checkpointVtkConvertStart-checkpointGammaStart << " s" << std::endl
              << "\tConverting back from ITK to VTK: " << checkpointEnd-checkpointVtkConvertStart << " s" << std::endl;
  }

  return "";
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::CreateDefaultGammaColorTable()
{
  vtkSmartPointer<vtkMRMLColorTableNode> gammaColorTable = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string nodeName = vtksys::SystemTools::GetFilenameWithoutExtension(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME);
  nodeName = this->GetMRMLScene()->GenerateUniqueName(nodeName);
  gammaColorTable->SetName(nodeName.c_str());
  gammaColorTable->SetTypeToUser();
  gammaColorTable->SetSingletonTag(nodeName.c_str());
  gammaColorTable->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
  gammaColorTable->HideFromEditorsOn();
  gammaColorTable->SetNumberOfColors(256);
  gammaColorTable->GetLookupTable()->SetNumberOfTableValues(256);
  gammaColorTable->GetLookupTable()->SetHueRange(0.28, 0.0);
  gammaColorTable->GetLookupTable()->SetSaturationRange(1,1);
  gammaColorTable->GetLookupTable()->SetValueRange(1,1);
  gammaColorTable->GetLookupTable()->SetRampToLinear();
  gammaColorTable->GetLookupTable()->ForceBuild();
  gammaColorTable->SetNamesFromColors();
  gammaColorTable->SaveWithSceneOff();
  gammaColorTable->SetDescription("Goes from green to red, passing through the colors of the rainbow in between in reverse. Useful for a colorful display of a difference volume");

  this->GetMRMLScene()->AddNode(gammaColorTable);
  this->SetDefaultGammaColorTableNodeId(gammaColorTable->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::LoadDefaultGammaColorTable()
{
  // Load default color table file
  std::string moduleShareDirectory = this->GetModuleShareDirectory();
  std::string colorTableFilePath = moduleShareDirectory + "/" + vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME;
  vtkMRMLColorTableNode* colorTableNode = NULL;
  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile( colorTableFilePath.c_str(),
      vtksys::SystemTools::GetFilenameWithoutExtension(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME).c_str() );
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
    colorTableNode->SetSingletonTag(colorTableNode->GetName());
    colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
    colorTableNode->HideFromEditorsOn();
    colorTableNode->SaveWithSceneOff();
  }
  else
  {
    if (!moduleShareDirectory.empty())
    {
      // Only log warning if the application exists (no warning when running automatic tests)
      vtkWarningMacro("LoadDefaultGammaColorTable: Default gamma color table file '" << colorTableFilePath << "' cannot be found!");
    }
    // If file is not found, then create it programatically
    this->CreateDefaultGammaColorTable();
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DefaultGammaColorTableNodeId));
  }

  if (colorTableNode)
  {
    colorTableNode->SetAttribute("Category", SlicerRtCommon::SLICERRT_EXTENSION_NAME);
    this->SetDefaultGammaColorTableNodeId(colorTableNode->GetID());
  }
  else
  {
    vtkErrorMacro("LoadDefaultGammaColorTable: Failed to load or create default gamma color table!");
  }
}
