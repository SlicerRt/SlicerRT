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

// DoseComparison includes
#include "vtkSlicerDoseComparisonModuleLogic.h"
#include "vtkMRMLDoseComparisonNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "PlmCommon.h"
#include "vtkMRMLContourNode.h"

// Plastimatch includes
#include "gamma_dose_comparison.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"
#include "vtkSlicerContoursModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>

// MRMLLogic includes
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLApplicationLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkTimerLog.h>
#include <vtkLookupTable.h>
#include <vtkObjectFactory.h>
#include "vtksys/SystemTools.hxx"

// SlicerBase includes
#include "vtkSlicerApplicationLogic.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseComparisonModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::vtkSlicerDoseComparisonModuleLogic()
{
  this->DoseComparisonNode = NULL;
  this->DefaultGammaColorTableNodeId = NULL;

  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkSlicerDoseComparisonModuleLogic::~vtkSlicerDoseComparisonModuleLogic()
{
  this->SetAndObserveDoseComparisonNode(NULL);
  this->SetDefaultGammaColorTableNodeId(NULL);
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
void vtkSlicerDoseComparisonModuleLogic::ComputeGammaDoseDifference()
{
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  this->DoseComparisonNode->ResultsValidOff();

  double checkpointConvertStart = timer->GetUniversalTime();
  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = this->DoseComparisonNode->GetReferenceDoseVolumeNode();
  Plm_image::Pointer referenceDose = PlmCommon::ConvertVolumeNodeToPlmImage(
    this->DoseComparisonNode->GetReferenceDoseVolumeNode());
  Plm_image::Pointer compareDose = PlmCommon::ConvertVolumeNodeToPlmImage(
    this->DoseComparisonNode->GetCompareDoseVolumeNode());

  Plm_image::Pointer maskVolume;
  vtkMRMLContourNode* maskContourNode = this->DoseComparisonNode->GetMaskContourNode();
  if (maskContourNode )
  {
    // Extract a labelmap for the dose comparison to use it as a mask
    maskContourNode->SetAndObserveRasterizationReferenceVolumeNodeId(referenceDoseVolumeNode->GetID());
    maskContourNode->SetRasterizationOversamplingFactor(1.0);
    vtkMRMLScalarVolumeNode* maskVolumeNode = vtkSlicerContoursModuleLogic::ExtractLabelmapFromContour(maskContourNode);
    maskVolume = PlmCommon::ConvertVolumeNodeToPlmImage( maskVolumeNode );
    this->GetMRMLScene()->RemoveNode(maskVolumeNode);
  }

  // Compute gamma dose volume
  double checkpointGammaStart = timer->GetUniversalTime();
  Gamma_dose_comparison gamma;
  gamma.set_reference_image(referenceDose->itk_float());
  gamma.set_compare_image(compareDose->itk_float());
  if (maskContourNode)
  {
    gamma.set_mask_image(maskVolume->itk_uchar());
  }
  gamma.set_spatial_tolerance(this->DoseComparisonNode->GetDtaDistanceToleranceMm());
  gamma.set_dose_difference_tolerance(this->DoseComparisonNode->GetDoseDifferenceTolerancePercent() / 100.0);
  if (!this->DoseComparisonNode->GetUseMaximumDose())
  {
    gamma.set_reference_dose(this->DoseComparisonNode->GetReferenceDoseGy());
  }
  gamma.set_analysis_threshold(this->DoseComparisonNode->GetAnalysisThresholdPercent() / 100.0 );
  gamma.set_gamma_max(this->DoseComparisonNode->GetMaximumGamma());

  gamma.run();

  itk::Image<float, 3>::Pointer gammaVolumeItk = gamma.get_gamma_image_itk();
  this->DoseComparisonNode->SetPassFractionPercent( gamma.get_pass_fraction() * 100.0 );
  this->DoseComparisonNode->ResultsValidOn();

  // Convert output to VTK
  double checkpointVtkConvertStart = timer->GetUniversalTime();
  vtkSmartPointer<vtkImageData> gammaVolume = vtkSmartPointer<vtkImageData>::New();
  SlicerRtCommon::ConvertItkImageToVtkImageData<float>(gammaVolumeItk, gammaVolume, VTK_FLOAT);

  // Set properties of output volume node
  vtkMRMLScalarVolumeNode* gammaVolumeNode = this->DoseComparisonNode->GetGammaVolumeNode();

  if (gammaVolumeNode == NULL)
  {
    vtkErrorMacro("ComputeGammaDoseDifference: Invalid gamma volume node in parameter set node!");
    return;
  }

  gammaVolumeNode->CopyOrientation(referenceDoseVolumeNode);
  gammaVolumeNode->SetAndObserveImageData(gammaVolume);
  gammaVolumeNode->SetAttribute(SlicerRtCommon::DOSECOMPARISON_GAMMA_VOLUME_IDENTIFIER_ATTRIBUTE_NAME, "1");

  // Assign gamma volume under the transform node of the reference volume node
  if (referenceDoseVolumeNode->GetParentTransformNode())
  {
    gammaVolumeNode->SetAndObserveTransformNodeID(referenceDoseVolumeNode->GetParentTransformNode()->GetID());
  }

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
      vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_SUBJECT );

    // Add gamma volume to subject hierarchy
    vtkMRMLSubjectHierarchyNode* dvhSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetMRMLScene(), commonAncestor, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES,
      gammaVolumeNode->GetName(), gammaVolumeNode);
  }

  // Add connection attribute to input dose volume nodes
  //TODO:

  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(gammaVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    std::cout << "Total gamma computation time: " << checkpointEnd-checkpointStart << " s" << std::endl
              << "\tApplying transforms: " << checkpointConvertStart-checkpointStart << " s" << std::endl
              << "\tConverting from VTK to ITK: " << checkpointGammaStart-checkpointConvertStart << " s" << std::endl
              << "\tGamma computation: " << checkpointVtkConvertStart-checkpointGammaStart << " s" << std::endl
              << "\tConverting back from ITK to VTK: " << checkpointEnd-checkpointVtkConvertStart << " s" << std::endl;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseComparisonModuleLogic::CreateDefaultGammaColorTable()
{
  vtkSmartPointer<vtkMRMLColorTableNode> gammaColorTable = vtkSmartPointer<vtkMRMLColorTableNode>::New();
  std::string nodeName = vtksys::SystemTools::GetFilenameWithoutExtension(SlicerRtCommon::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME);
  nodeName = this->GetMRMLScene()->GenerateUniqueName(nodeName);
  gammaColorTable->SetName(nodeName.c_str());
  gammaColorTable->SetTypeToUser();
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
  std::string colorTableFilePath = moduleShareDirectory + "/" + SlicerRtCommon::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME;
  vtkMRMLColorTableNode* colorTableNode = NULL;
  if (vtksys::SystemTools::FileExists(colorTableFilePath.c_str()) && this->GetMRMLApplicationLogic() && this->GetMRMLApplicationLogic()->GetColorLogic())
  {
    vtkMRMLColorNode* loadedColorNode = this->GetMRMLApplicationLogic()->GetColorLogic()->LoadColorFile( colorTableFilePath.c_str(),
      vtksys::SystemTools::GetFilenameWithoutExtension(SlicerRtCommon::DOSECOMPARISON_DEFAULT_GAMMA_COLOR_TABLE_FILE_NAME).c_str() );
    colorTableNode = vtkMRMLColorTableNode::SafeDownCast(loadedColorNode);
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
