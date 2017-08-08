/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationModuleLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerIsodoseModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>

// Slicer includes
#include <vtkSlicerVolumesLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageMathematics.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkImageReslice.h>
#include <vtkGeneralTransform.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
const std::string vtkSlicerDoseAccumulationModuleLogic::DOSEACCUMULATION_ATTRIBUTE_PREFIX = "DoseAccumulation.";
const std::string vtkSlicerDoseAccumulationModuleLogic::DOSEACCUMULATION_DOSE_VOLUME_NODE_NAME_ATTRIBUTE_NAME = vtkSlicerDoseAccumulationModuleLogic::DOSEACCUMULATION_ATTRIBUTE_PREFIX + "DoseVolumeNodeName";
const std::string vtkSlicerDoseAccumulationModuleLogic::DOSEACCUMULATION_OUTPUT_BASE_NAME_PREFIX = "Accumulated_";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseAccumulationModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationModuleLogic::vtkSlicerDoseAccumulationModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationModuleLogic::~vtkSlicerDoseAccumulationModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
    return;
  }

  // Remove volume node from parameter set nodes
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (volumeNode)
  {
    std::vector<vtkMRMLNode*> nodes;
    this->GetMRMLScene()->GetNodesByClass("vtkMRMLDoseAccumulationNode", nodes);
    for (std::vector<vtkMRMLNode*>::iterator nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
    {
      vtkMRMLDoseAccumulationNode* doseAccumulationNode = vtkMRMLDoseAccumulationNode::SafeDownCast(*nodeIt);
      doseAccumulationNode->RemoveSelectedInputVolumeNode(volumeNode);
      doseAccumulationNode->GetVolumeNodeIdsToWeightsMap()->erase(volumeNode->GetID());
    }
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
const char* vtkSlicerDoseAccumulationModuleLogic::AccumulateDoseVolumes(vtkMRMLDoseAccumulationNode* parameterNode)
{
  if (!parameterNode)
  {
    const char* errorMessage = "No parameter set currentNode";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }

  // Make sure inputs are initialized
  int numberOfInputDoseVolumes = parameterNode->GetNumberOfSelectedInputVolumeNodes();
  if (numberOfInputDoseVolumes == 0)
  {
    const char* errorMessage = "No dose volume selected";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }

  // Get reference and output dose volumes
  vtkMRMLScalarVolumeNode* outputAccumulatedDoseVolumeNode = parameterNode->GetAccumulatedDoseVolumeNode();
  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = parameterNode->GetReferenceDoseVolumeNode();
  if (referenceDoseVolumeNode == NULL)
  {
    const char* errorMessage = "Reference volume not specified";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }
  if (outputAccumulatedDoseVolumeNode == NULL)
  {
    const char* errorMessage = "Output volume not specified";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }

  // Get reference image info
  int referenceDimensions[3] = {0, 0, 0};
  referenceDoseVolumeNode->GetImageData()->GetDimensions(referenceDimensions);

  // Apply weight and accumulate input dose volumes
  vtkSmartPointer<vtkImageData> accumulatedImageData = vtkSmartPointer<vtkImageData>::New();
  for (int inputVolumeIndex = 0; inputVolumeIndex<numberOfInputDoseVolumes; inputVolumeIndex++)
  {
    vtkMRMLScalarVolumeNode* currentInputDoseVolumeNode = parameterNode->GetNthSelectedInputVolumeNode(inputVolumeIndex);
    if (!currentInputDoseVolumeNode->GetImageData())
    {
      std::stringstream errorMessage;
      errorMessage << "No image data in input volume #" << inputVolumeIndex;
      vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage.str());
      return errorMessage.str().c_str();
    }
    std::map<std::string,double>* volumeNodeIdsToWeightsMap = parameterNode->GetVolumeNodeIdsToWeightsMap();
    double currentWeight = (*volumeNodeIdsToWeightsMap)[currentInputDoseVolumeNode->GetID()];

    vtkMRMLScalarVolumeNode* resampledInputDoseVolumeNode = 
      vtkSlicerVolumesLogic::ResampleVolumeToReferenceVolume(currentInputDoseVolumeNode, referenceDoseVolumeNode);

    // Apply weight
    vtkSmartPointer<vtkImageMathematics> multiplyFilter = vtkSmartPointer<vtkImageMathematics>::New();
    multiplyFilter->SetInputConnection(resampledInputDoseVolumeNode->GetImageDataConnection());
    multiplyFilter->SetConstantK(currentWeight);
    multiplyFilter->SetOperationToMultiplyByK();
    multiplyFilter->Update();

    // Add (accumulate) current input volume to the intermediate accumulated volume
    if (inputVolumeIndex > 0)
    {
      vtkSmartPointer<vtkImageMathematics> addFilter = vtkSmartPointer<vtkImageMathematics>::New(); 
      addFilter->SetInput1Data(accumulatedImageData);
      addFilter->SetInput2Data(multiplyFilter->GetOutput());
      addFilter->SetOperationToAdd();
      addFilter->Update();

      accumulatedImageData->DeepCopy(addFilter->GetOutput());
    }
    // If intermediate accumulated volume is empty (first iteration)
    // then just copy the weighted input dose volume in it
    else
    {
      accumulatedImageData->DeepCopy(multiplyFilter->GetOutput());
    }

    // Remove the resample dose currentNode from scene and release the memory
    this->GetMRMLScene()->RemoveNode(resampledInputDoseVolumeNode);
  }

  // Create display currentNode for the accumulated volume
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> outputAccumulatedDoseVolumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  this->GetMRMLScene()->AddNode(outputAccumulatedDoseVolumeDisplayNode); 

  // Set colormap to dose
  vtkMRMLColorTableNode* defaultDoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(this->GetMRMLScene());
  if (defaultDoseColorTable)
  {
    outputAccumulatedDoseVolumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
  }
  else
  {
    outputAccumulatedDoseVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
    vtkErrorMacro("AccumulateDoseVolumes: Failed to get default dose color table");
  }

  // Set output accumulated dose image info
  outputAccumulatedDoseVolumeNode->CopyOrientation(referenceDoseVolumeNode);
  outputAccumulatedDoseVolumeNode->SetAndObserveImageData(accumulatedImageData);
  outputAccumulatedDoseVolumeNode->SetAndObserveDisplayNodeID( outputAccumulatedDoseVolumeDisplayNode->GetID() );
  outputAccumulatedDoseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Select as active volume
  if (this->GetApplicationLogic())
  {
    if (this->GetApplicationLogic()->GetSelectionNode())
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(outputAccumulatedDoseVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  // Put accumulated dose volume under the same study as the reference dose volume
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    const char* errorMessage = "Failed to access subject hierarchy node";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }
  vtkIdType referenceDoseVolumeShItemID = shNode->GetItemByDataNode(referenceDoseVolumeNode);
  if (referenceDoseVolumeShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    const char* errorMessage = "No subject hierarchy currentNode found for reference dose";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }
  vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(referenceDoseVolumeShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  if (studyItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    const char* errorMessage = "No study currentNode found for reference dose";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return errorMessage;
  }

  // Setup subject hierarchy item for the accumulated dose volume
  shNode->CreateItem(studyItemID, outputAccumulatedDoseVolumeNode);

  // Set threshold values so that the background is black
  double doseUnitScaling = 1.0;
  std::string doseUnitScalingStr = shNode->GetItemAttribute(studyItemID, SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME);
  if (!doseUnitScalingStr.empty())
  {
    doseUnitScaling = vtkVariant(doseUnitScalingStr).ToDouble();
  }
  outputAccumulatedDoseVolumeDisplayNode->AutoThresholdOff();
  outputAccumulatedDoseVolumeDisplayNode->SetLowerThreshold(0.5 * doseUnitScaling);
  outputAccumulatedDoseVolumeDisplayNode->SetApplyThreshold(1);

  return NULL;
}
