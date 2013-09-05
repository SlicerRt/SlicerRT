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

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageMathematics.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkImageReslice.h>
#include <vtkImageChangeInformation.h>
#include <vtkGeneralTransform.h>

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
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
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
void vtkSlicerDoseAccumulationModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (this->DoseAccumulationNode && node->IsA("vtkMRMLVolumeNode"))
  {
    this->DoseAccumulationNode->GetSelectedInputVolumeIds()->erase(node->GetID());
    this->DoseAccumulationNode->GetVolumeNodeIdsToWeightsMap()->erase(node->GetID());
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoseAccumulationNode"))
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
  this->Modified();
}

//---------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseAccumulationModuleLogic::GetVolumeNodesFromScene()
{
  vtkCollection* volumeNodes = vtkCollection::New();
  volumeNodes->InitTraversal();

  if (this->GetMRMLScene() == NULL || this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLVolumeNode") < 1
    || this->DoseAccumulationNode == NULL)
  {
    return volumeNodes;
  }

  this->GetMRMLScene()->InitTraversal();
  vtkMRMLNode *node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLVolumeNode");
  while (node != NULL)
  {
    vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(node);
    if (volumeNode)
    {
      const char* doseVolumeIdentifier = volumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (doseVolumeIdentifier != NULL || !this->DoseAccumulationNode->GetShowDoseVolumesOnly())
      {
        volumeNodes->AddItem(volumeNode);
      }
    }

    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLVolumeNode");
  }

  return volumeNodes;
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseAccumulationModuleLogic::ReferenceDoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->DoseAccumulationNode)
  {
    return false;
  }

  vtkMRMLNode* referenceDoseVolumeNode = this->GetMRMLScene()->GetNodeByID(this->DoseAccumulationNode->GetReferenceDoseVolumeNodeId());
  return SlicerRtCommon::IsDoseVolumeNode(referenceDoseVolumeNode);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::AccumulateDoseVolumes(std::string &errorMessage)
{
  vtkSmartPointer<vtkImageData> baseImageData = NULL;

  // Make sure inputs are initialized
  if (this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->empty())
  {
    errorMessage = "No dose volume selected";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  int numberOfInputDoseVolumes = this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->size();

  std::set<std::string>::iterator iterIds = this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->begin();
  std::string Id = *iterIds;
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputDoseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(Id));
  if (!inputDoseVolumeNode->GetImageData())
  {
    errorMessage = "No image data found in input volume";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }
  std::map<std::string,double> *VolumeNodeIdsToWeightsMap = this->GetDoseAccumulationNode()->GetVolumeNodeIdsToWeightsMap();
  double weight = (*VolumeNodeIdsToWeightsMap)[Id];

  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputAccumulatedDoseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetDoseAccumulationNode()->GetAccumulatedDoseVolumeNodeId()));
  vtkSmartPointer<vtkMRMLScalarVolumeNode> referenceDoseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetDoseAccumulationNode()->GetReferenceDoseVolumeNodeId()));
  // make sure the reference and output volume nodes are initialized
  if (referenceDoseVolumeNode.GetPointer() == NULL || outputAccumulatedDoseVolumeNode.GetPointer() == NULL)
  {
    errorMessage = "reference and/or output volume not specified!";
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  // get reference image info
  int dimensions[3] = {0, 0, 0};
  referenceDoseVolumeNode->GetImageData()->GetDimensions(dimensions);

  vtkSmartPointer<vtkMatrix4x4> inputDoseVolumeIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inputDoseVolumeNode->GetIJKToRASMatrix(inputDoseVolumeIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> referenceDoseVolumeRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceDoseVolumeNode->GetRASToIJKMatrix(referenceDoseVolumeRAS2IJKMatrix);

  vtkSmartPointer<vtkTransform> outputIJK2IJKResliceTransform = vtkSmartPointer<vtkTransform>::New();
  outputIJK2IJKResliceTransform->Identity();
  outputIJK2IJKResliceTransform->PostMultiply();
  outputIJK2IJKResliceTransform->SetMatrix(inputDoseVolumeIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputDoseVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(inputDoseVolumeNode->GetTransformNodeID()));
  vtkSmartPointer<vtkMatrix4x4> inputDoseVolumeRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputDoseVolumeNodeTransformNode != NULL)
  {
    inputDoseVolumeNodeTransformNode->GetMatrixTransformToWorld(inputDoseVolumeRAS2RASMatrix);  
    outputIJK2IJKResliceTransform->Concatenate(inputDoseVolumeRAS2RASMatrix);
  }
  outputIJK2IJKResliceTransform->Concatenate(referenceDoseVolumeRAS2IJKMatrix);
  outputIJK2IJKResliceTransform->Inverse();

  vtkSmartPointer<vtkImageReslice> resliceFilter = vtkSmartPointer<vtkImageReslice>::New();
  resliceFilter->SetInput(inputDoseVolumeNode->GetImageData());
  resliceFilter->SetOutputOrigin(0, 0, 0);
  resliceFilter->SetOutputSpacing(1, 1, 1);
  resliceFilter->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  resliceFilter->SetResliceTransform(outputIJK2IJKResliceTransform);
  resliceFilter->Update();

  vtkSmartPointer<vtkImageData> reslicedDoseVolumeImage = NULL;
  reslicedDoseVolumeImage = resliceFilter->GetOutput();

  vtkSmartPointer<vtkImageMathematics> multiplyFilter = vtkSmartPointer<vtkImageMathematics>::New();
  multiplyFilter->SetInput(reslicedDoseVolumeImage);
  multiplyFilter->SetConstantK(weight);
  multiplyFilter->SetOperationToMultiplyByK();
  multiplyFilter->Update();
  baseImageData = multiplyFilter->GetOutput();

  vtkSmartPointer<vtkImageMathematics> addFilter = vtkSmartPointer<vtkImageMathematics>::New(); //TODO: local variables camel case starting with small (e.g. addFilter)
  if (numberOfInputDoseVolumes >=2) //TODO: size bad variable name
  {
    for (int i = 1; i < numberOfInputDoseVolumes; i++)
    {
      iterIds++;
      Id = *iterIds;
      inputDoseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(Id));
      if (!inputDoseVolumeNode->GetImageData())
      {
        errorMessage = "No image data found in input volume"; 
        vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
        continue;
      }
      weight = (*VolumeNodeIdsToWeightsMap)[Id];

      vtkSmartPointer<vtkMatrix4x4> inputDoseVolumeIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      inputDoseVolumeNode->GetIJKToRASMatrix(inputDoseVolumeIJK2RASMatrix);

      outputIJK2IJKResliceTransform->Identity();
      outputIJK2IJKResliceTransform->PostMultiply();
      outputIJK2IJKResliceTransform->SetMatrix(inputDoseVolumeIJK2RASMatrix);

      inputDoseVolumeNodeTransformNode = vtkMRMLTransformNode::SafeDownCast(
        this->GetMRMLScene()->GetNodeByID(inputDoseVolumeNode->GetTransformNodeID()));
      inputDoseVolumeRAS2RASMatrix->Identity();
      if (inputDoseVolumeNodeTransformNode != NULL)
      {
        inputDoseVolumeNodeTransformNode->GetMatrixTransformToWorld(inputDoseVolumeRAS2RASMatrix);  
        outputIJK2IJKResliceTransform->Concatenate(inputDoseVolumeRAS2RASMatrix);
      }
      outputIJK2IJKResliceTransform->Concatenate(referenceDoseVolumeRAS2IJKMatrix);
      outputIJK2IJKResliceTransform->Inverse();

      resliceFilter->SetInput(inputDoseVolumeNode->GetImageData());
      resliceFilter->SetOutputOrigin(0, 0, 0);
      resliceFilter->SetOutputSpacing(1, 1, 1);
      resliceFilter->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
      resliceFilter->SetResliceTransform(outputIJK2IJKResliceTransform);
      resliceFilter->Update();

      multiplyFilter->SetInput(resliceFilter->GetOutput());
      multiplyFilter->SetConstantK(weight);
      multiplyFilter->SetOperationToMultiplyByK();
      multiplyFilter->Update();

      addFilter->SetInput1(baseImageData);
      addFilter->SetInput2(multiplyFilter->GetOutput());
      addFilter->SetOperationToAdd();
      addFilter->Update();
       
      baseImageData = addFilter->GetOutput();
    }
  }

  // A volume node needs a display node
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

  // set visibility 
  outputAccumulatedDoseVolumeDisplayNode->SetVisibility(1);

  // Set output accumulated dose image info
  outputAccumulatedDoseVolumeNode->CopyOrientation(inputDoseVolumeNode);
  outputAccumulatedDoseVolumeNode->SetAndObserveImageData(baseImageData);
  outputAccumulatedDoseVolumeNode->SetAndObserveDisplayNodeID( outputAccumulatedDoseVolumeDisplayNode->GetID() );
  outputAccumulatedDoseVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(outputAccumulatedDoseVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  // Put accumulated dose volume under the same study as the reference dose volume
  vtkMRMLHierarchyNode* studyNode = vtkSlicerPatientHierarchyModuleLogic::GetAncestorAtLevel(referenceDoseVolumeNode, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_STUDY);
  if (!studyNode)
  {
    errorMessage = "No study node found for reference dose!"; 
    vtkErrorMacro("AccumulateDoseVolumes: " << errorMessage);
    return;
  }

  std::string phSeriesNodeName(outputAccumulatedDoseVolumeNode->GetName());
  phSeriesNodeName.append(SlicerRtCommon::PATIENTHIERARCHY_NODE_NAME_POSTFIX);
  phSeriesNodeName = this->GetMRMLScene()->GenerateUniqueName(phSeriesNodeName);
  vtkSmartPointer<vtkMRMLHierarchyNode> patientHierarchySeriesNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  patientHierarchySeriesNode->HideFromEditorsOff();
  patientHierarchySeriesNode->SetAssociatedNodeID(outputAccumulatedDoseVolumeNode->GetID());
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME,
    SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME,
    vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  patientHierarchySeriesNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMUID_ATTRIBUTE_NAME,
    "");
  patientHierarchySeriesNode->SetName(phSeriesNodeName.c_str());
  this->GetMRMLScene()->AddNode(patientHierarchySeriesNode);

  patientHierarchySeriesNode->SetParentNodeID(studyNode->GetID());

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
