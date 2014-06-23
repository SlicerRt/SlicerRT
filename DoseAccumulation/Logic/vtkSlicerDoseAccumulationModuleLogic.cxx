/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageMathematics.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkImageReslice.h>
#include <vtkGeneralTransform.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseAccumulationModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationModuleLogic::vtkSlicerDoseAccumulationModuleLogic()
{
  this->DoseAccumulationNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationModuleLogic::~vtkSlicerDoseAccumulationModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::SetAndObserveDoseAccumulationNode(vtkMRMLDoseAccumulationNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerDoseAccumulationModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
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
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (this->DoseAccumulationNode && volumeNode)
  {
    this->DoseAccumulationNode->RemoveSelectedInputVolumeNode(volumeNode);
    this->DoseAccumulationNode->GetVolumeNodeIdsToWeightsMap()->erase(node->GetID());
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLDoseAccumulationNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
  if (node)
  {
    paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseAccumulationModuleLogic::GetVolumeNodesFromScene()
{
  vtkCollection* volumeNodes = vtkCollection::New();
  volumeNodes->InitTraversal();

  if (this->GetMRMLScene() == NULL || this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLScalarVolumeNode") < 1
    || this->DoseAccumulationNode == NULL)
  {
    return volumeNodes;
  }

  this->GetMRMLScene()->InitTraversal();
  vtkMRMLNode *node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLScalarVolumeNode");
  while (node != NULL)
  {
    vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
    if (volumeNode)
    {
      const char* doseVolumeIdentifier = volumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (doseVolumeIdentifier != NULL || !this->DoseAccumulationNode->GetShowDoseVolumesOnly())
      {
        volumeNodes->AddItem(volumeNode);
      }
    }

    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLScalarVolumeNode");
  }

  return volumeNodes;
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseAccumulationModuleLogic::ReferenceDoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->DoseAccumulationNode)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene or parameter set node!");
    return false;
  }

  vtkMRMLNode* referenceDoseVolumeNode = this->DoseAccumulationNode->GetReferenceDoseVolumeNode();
  return SlicerRtCommon::IsDoseVolumeNode(referenceDoseVolumeNode);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::AccumulateDoseVolumes(std::string &errorMessage)
{
  vtkMRMLDoseAccumulationNode* paramNode = this->GetDoseAccumulationNode();
  if (!paramNode)
  {
    errorMessage = "No parameter set node";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  // Make sure inputs are initialized
  int numberOfInputDoseVolumes = paramNode->GetNumberOfSelectedInputVolumeNodes();
  if (numberOfInputDoseVolumes == 0)
  {
    errorMessage = "No dose volume selected";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  // Get reference and output dose volumes
  vtkMRMLScalarVolumeNode* outputAccumulatedDoseVolumeNode = paramNode->GetAccumulatedDoseVolumeNode();
  vtkMRMLScalarVolumeNode* referenceDoseVolumeNode = paramNode->GetReferenceDoseVolumeNode();
  if (referenceDoseVolumeNode == NULL || outputAccumulatedDoseVolumeNode == NULL)
  {
    errorMessage = "reference and/or output volume not specified!";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  // Get reference image info
  int referenceDimensions[3] = {0, 0, 0};
  referenceDoseVolumeNode->GetImageData()->GetDimensions(referenceDimensions);

  vtkSmartPointer<vtkMatrix4x4> referenceDoseVolumeRasToIjkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceDoseVolumeNode->GetRASToIJKMatrix(referenceDoseVolumeRasToIjkMatrix);

  // Apply weight and accumulate input dose volumes
  vtkSmartPointer<vtkImageData> accumulatedImageData = vtkSmartPointer<vtkImageData>::New();
  for (int inputVolumeIndex = 0; inputVolumeIndex<numberOfInputDoseVolumes; inputVolumeIndex++)
  {
    vtkMRMLScalarVolumeNode* currentInputDoseVolumeNode = paramNode->GetNthSelectedInputVolumeNode(inputVolumeIndex);
    if (!currentInputDoseVolumeNode->GetImageData())
    {
      errorMessage = "No image data in input volume #" + inputVolumeIndex;
      vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
      return;
    }
    std::map<std::string,double>* volumeNodeIdsToWeightsMap = paramNode->GetVolumeNodeIdsToWeightsMap();
    double currentWeight = (*volumeNodeIdsToWeightsMap)[currentInputDoseVolumeNode->GetID()];

    // Get current input IJK to RAS transform
    vtkSmartPointer<vtkMatrix4x4> currentInputDoseVolumeIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    currentInputDoseVolumeNode->GetIJKToRASMatrix(currentInputDoseVolumeIjkToRasMatrix);

    // Create transform from reference IJK to current input IJK for resampling
    vtkSmartPointer<vtkTransform> referenceVolumeIjkToCurrentInputVolumeIjkTransform = vtkSmartPointer<vtkTransform>::New();
    referenceVolumeIjkToCurrentInputVolumeIjkTransform->Identity();
    referenceVolumeIjkToCurrentInputVolumeIjkTransform->PostMultiply();
    referenceVolumeIjkToCurrentInputVolumeIjkTransform->SetMatrix(currentInputDoseVolumeIjkToRasMatrix);

    vtkMRMLTransformNode* currentInputDoseVolumeTransformNode = currentInputDoseVolumeNode->GetParentTransformNode();
    vtkSmartPointer<vtkMatrix4x4> currentInputDoseVolumeRasToWorldRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    if (currentInputDoseVolumeTransformNode)
    {
      currentInputDoseVolumeTransformNode->GetMatrixTransformToWorld(currentInputDoseVolumeRasToWorldRasMatrix);  
      referenceVolumeIjkToCurrentInputVolumeIjkTransform->Concatenate(currentInputDoseVolumeRasToWorldRasMatrix);
    }
    referenceVolumeIjkToCurrentInputVolumeIjkTransform->Concatenate(referenceDoseVolumeRasToIjkMatrix);
    referenceVolumeIjkToCurrentInputVolumeIjkTransform->Inverse();

    // Resample current input to reference volume geometry
    vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
    resliceFilter->SetInputData(currentInputDoseVolumeNode->GetImageData());
    resliceFilter->SetOutputOrigin(0, 0, 0);
    resliceFilter->SetOutputSpacing(1, 1, 1);
    resliceFilter->SetOutputExtent(0, referenceDimensions[0]-1, 0, referenceDimensions[1]-1, 0, referenceDimensions[2]-1);
    resliceFilter->SetResliceTransform(referenceVolumeIjkToCurrentInputVolumeIjkTransform);

    // Apply weight
    vtkSmartPointer<vtkImageMathematics> multiplyFilter = vtkSmartPointer<vtkImageMathematics>::New();
    multiplyFilter->SetInputConnection(resliceFilter->GetOutputPort());
    multiplyFilter->SetConstantK(currentWeight);
    multiplyFilter->SetOperationToMultiplyByK();

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
      multiplyFilter->Update();
      accumulatedImageData->DeepCopy(multiplyFilter->GetOutput());
    }
  }

  // Create display node for the accumulated volume
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> outputAccumulatedDoseVolumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  this->GetMRMLScene()->AddNode(outputAccumulatedDoseVolumeDisplayNode); 

  // Set default colormap to the dose color table
  vtkSmartPointer<vtkCollection> defaultDoseColorTableNodes = vtkSmartPointer<vtkCollection>::Take(
    this->GetMRMLScene()->GetNodesByName(SlicerRtCommon::DICOMRTIMPORT_DEFAULT_DOSE_COLOR_TABLE_NAME) );
  vtkMRMLColorTableNode* defaultDoseColorTable = vtkMRMLColorTableNode::SafeDownCast(defaultDoseColorTableNodes->GetItemAsObject(0));
  if (defaultDoseColorTable)
  {
    outputAccumulatedDoseVolumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
  }
  else
  {
    outputAccumulatedDoseVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
  }

  // Set visibility 
  outputAccumulatedDoseVolumeDisplayNode->SetVisibility(1);

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
  vtkMRMLSubjectHierarchyNode* referenceDoseVolumeSubjectHierarchyNode =
    vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(referenceDoseVolumeNode);
  if (!referenceDoseVolumeSubjectHierarchyNode)
  {
    errorMessage = "No subject hierarchy node found for reference dose!"; 
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }
  vtkMRMLSubjectHierarchyNode* studyNode = referenceDoseVolumeSubjectHierarchyNode->GetAncestorAtLevel(
    vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY );
  if (!studyNode)
  {
    errorMessage = "No study node found for reference dose!"; 
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  // Create subject hierarchy node for the accumulated dose volume if it doesn't exist
  vtkMRMLSubjectHierarchyNode* subjectHierarchySeriesNode =
    vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(outputAccumulatedDoseVolumeNode);
  if (!subjectHierarchySeriesNode)
  {
    vtkMRMLSubjectHierarchyNode* childSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetMRMLScene(), studyNode, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SERIES, outputAccumulatedDoseVolumeNode->GetName(), outputAccumulatedDoseVolumeNode);
    if (!childSubjectHierarchyNode)
    {
      errorMessage = "Failed to create subject hierarchy node!"; 
      vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
      return;
    }
  }
  else
  {
    subjectHierarchySeriesNode->SetParentNodeID(studyNode->GetID());
  }

  // Set threshold values so that the background is black
  const char* doseUnitScalingChars = studyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str());
  double doseUnitScaling = 0.0;
  std::stringstream doseUnitScalingSs;
  doseUnitScalingSs << doseUnitScalingChars;
  doseUnitScalingSs >> doseUnitScaling;
  outputAccumulatedDoseVolumeDisplayNode->AutoThresholdOff();
  outputAccumulatedDoseVolumeDisplayNode->SetLowerThreshold(0.5 * doseUnitScaling);
  outputAccumulatedDoseVolumeDisplayNode->SetApplyThreshold(1);
}
