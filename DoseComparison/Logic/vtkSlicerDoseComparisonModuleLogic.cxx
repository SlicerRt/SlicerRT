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
#include "vtkSlicerRtCommon.h"
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
const std::string vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_REFERENCE_DOSE_VOLUME_REFERENCE_ROLE = "referenceDoseVolumeRef"; // Reference
const std::string vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_COMPARE_DOSE_VOLUME_REFERENCE_ROLE = "compareDoseVolumeRef"; // Reference

//---------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic* LogicInstance = nullptr;
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
  this->DefaultGammaColorTableNodeId = nullptr;
  this->Progress = 0.0;

  this->LogSpeedMeasurementsOff();

  LogicInstance = this;
}

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::~vtkSlicerDoseComparisonModuleLogic()
{
  this->SetDefaultGammaColorTableNodeId(nullptr);

  LogicInstance = nullptr;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
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
  if (!scene->IsNodeClassRegistered("vtkMRMLDoseComparisonNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseComparisonNode>::New());
  }
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

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseComparisonNode"))
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

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseComparisonNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::GammaProgressUpdated(float progress)
{
  double progressDouble = (double)progress;
  this->Progress = progressDouble;
  this->InvokeEvent(vtkSlicerRtCommon::ProgressUpdated, (void*)&progressDouble);
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseComparisonModuleLogic::ComputeGammaDoseDifference(vtkMRMLDoseComparisonNode* parameterNode)
{
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  parameterNode->ResultsValidOff();

  double checkpointConvertStart = timer->GetUniversalTime();
  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = parameterNode->GetReferenceDoseVolumeNode();
  Plm_image::Pointer referenceDose = PlmCommon::ConvertVolumeNodeToPlmImage(referenceDoseVolumeNode);
  Plm_image::Pointer compareDose = PlmCommon::ConvertVolumeNodeToPlmImage(parameterNode->GetCompareDoseVolumeNode());

  Plm_image::Pointer maskVolume;
  vtkMRMLSegmentationNode* maskSegmentationNode = parameterNode->GetMaskSegmentationNode();
  const char* maskSegmentID = parameterNode->GetMaskSegmentID();
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
    vtkNew<vtkOrientedImageData>  maskSegmentLabelmap;
    maskSegmentationNode->GetBinaryLabelmapRepresentation(maskSegmentID, maskSegmentLabelmap);

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
  if (maskSegmentationNode && maskSegmentID)
  {
    gamma.set_mask_image(maskVolume->itk_uchar());
  }
  gamma.set_spatial_tolerance(parameterNode->GetDtaDistanceToleranceMm());
  gamma.set_dose_difference_tolerance(parameterNode->GetDoseDifferenceTolerancePercent() / 100.0);
  gamma.set_resample_nn(false); // Note: This used to be driven by the interpolation checkbox
  gamma.set_interp_search(parameterNode->GetUseGeometricGammaCalculation());
  gamma.set_local_gamma(parameterNode->GetLocalDoseDifference());
  if (!parameterNode->GetUseMaximumDose())
  {
    gamma.set_reference_dose(parameterNode->GetReferenceDoseGy());
  }
  gamma.set_analysis_threshold(parameterNode->GetAnalysisThresholdPercent() / 100.0 );
  gamma.set_gamma_max(parameterNode->GetMaximumGamma());
  gamma.set_ref_only_threshold(parameterNode->GetDoseThresholdOnReferenceOnly());
  gamma.set_progress_callback(&GammaProgressCallback);

  gamma.run();

  itk::Image<float, 3>::Pointer gammaVolumeItk = gamma.get_gamma_image_itk();
  parameterNode->SetPassFractionPercent( gamma.get_pass_fraction() * 100.0 );
  parameterNode->SetReportString(gamma.get_report_string().c_str());

  // Convert output to VTK
  double checkpointVtkConvertStart = timer->GetUniversalTime();

  vtkMRMLScalarVolumeNode* gammaVolumeNode = parameterNode->GetGammaVolumeNode();
  if (gammaVolumeNode == nullptr)
  {
    std::string errorMessage("Invalid gamma volume node in parameter set node");
    vtkErrorMacro("ComputeGammaDoseDifference: " << errorMessage);
    return errorMessage;
  }

  vtkSlicerRtCommon::ConvertItkImageToVolumeNode<float>(gammaVolumeItk, gammaVolumeNode, VTK_FLOAT);
  gammaVolumeNode->SetAttribute(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME, "1");

  // Set default colormap to red
  if (gammaVolumeNode->GetVolumeDisplayNode() == nullptr)
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
    gammaScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, parameterNode->GetMaximumGamma());

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

  // Get common ancestor of the two input dose volumes in subject hierarchy
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    std::string errorMessage("");
    vtkErrorMacro("ComputeGammaDoseDifference: Failed to access subject hierarchy node" << errorMessage);
    return errorMessage;
  }
  vtkIdType commonAncestorItemID = vtkSlicerSubjectHierarchyModuleLogic::AreNodesInSameBranch(
    parameterNode->GetReferenceDoseVolumeNode(), parameterNode->GetCompareDoseVolumeNode(),
    vtkMRMLSubjectHierarchyConstants::GetDICOMLevelPatient() );
  if (commonAncestorItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    commonAncestorItemID = shNode->GetSceneItemID();
  }

  // Setup gamma volume subject hierarchy item
  shNode->CreateItem(commonAncestorItemID, gammaVolumeNode);

  // Add connection attribute to input dose volume nodes
  gammaVolumeNode->AddNodeReferenceID( vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_REFERENCE_DOSE_VOLUME_REFERENCE_ROLE.c_str(),
    parameterNode->GetReferenceDoseVolumeNode()->GetID() );
  gammaVolumeNode->AddNodeReferenceID( vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_COMPARE_DOSE_VOLUME_REFERENCE_ROLE.c_str(),
    parameterNode->GetCompareDoseVolumeNode()->GetID() );

  // Select as active volume
  if (this->GetApplicationLogic()!=nullptr)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=nullptr)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(gammaVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  parameterNode->ResultsValidOn();

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
  gammaColorTable->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
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
  vtkMRMLColorTableNode* colorTableNode = nullptr;
  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile( colorTableFilePath.c_str(),
      vtksys::SystemTools::GetFilenameWithoutExtension(vtkSlicerDoseComparisonModuleLogic::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME).c_str() );
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
    colorTableNode->SetSingletonTag(colorTableNode->GetName());
    colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
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
    // If file is not found, then create it programmatically
    this->CreateDefaultGammaColorTable();
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->DefaultGammaColorTableNodeId));
  }

  if (colorTableNode)
  {
    colorTableNode->SetAttribute("Category", vtkSlicerRtCommon::SLICERRT_EXTENSION_NAME);
    this->SetDefaultGammaColorTableNodeId(colorTableNode->GetID());
  }
  else
  {
    vtkErrorMacro("LoadDefaultGammaColorTable: Failed to load or create default gamma color table!");
  }
}
