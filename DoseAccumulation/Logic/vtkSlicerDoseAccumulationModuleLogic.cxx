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

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageMathematics.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkImageReslice.h>
#include <vtkImageChangeInformation.h>
#include <vtkGeneralTransform.h>

// STD includes
#include <cassert>

#define THRESHOLD 0.001

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
  assert(this->GetMRMLScene() != 0);

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
      const char* doseUnitName = volumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());
      if (doseUnitName != NULL || !this->DoseAccumulationNode->GetShowDoseVolumesOnly())
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

  vtkMRMLVolumeNode* referenceDoseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseAccumulationNode->GetReferenceDoseVolumeNodeId()));

  const char* doseUnitName = referenceDoseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  if (doseUnitName != NULL)
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationModuleLogic::AccumulateDoseVolumes(std::string &errorMessage)
{
  // Make sure inputs are initialized
  if (this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->empty())
  {
    errorMessage = "Dose accumulation: No dose volume selected";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  int size = this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->size();
  vtkSmartPointer<vtkImageData> baseImageData = NULL;

  std::set<std::string>::iterator iterIds = this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->begin();
  std::string Id = *iterIds;
  vtkSmartPointer<vtkMRMLScalarVolumeNode> inputVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(Id));
  if (!inputVolumeNode->GetImageData())
  {
    errorMessage = "No image data found in input volume";
    vtkErrorMacro(<<errorMessage);
    return;
  }
  std::map<std::string,double> *VolumeNodeIdsToWeightsMap = this->GetDoseAccumulationNode()->GetVolumeNodeIdsToWeightsMap();
  double weight = (*VolumeNodeIdsToWeightsMap)[Id];

  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetDoseAccumulationNode()->GetAccumulatedDoseVolumeNodeId()));
  // A volume node needs a display node
  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> outputVolumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  this->GetMRMLScene()->AddNode(outputVolumeDisplayNode);

  vtkSmartPointer<vtkMRMLScalarVolumeNode> referenceDoseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetDoseAccumulationNode()->GetReferenceDoseVolumeNodeId()));
  if (referenceDoseVolumeNode.GetPointer() == NULL)
  {
    errorMessage = "No reference dose volume set";
    vtkErrorMacro(<<errorMessage);
    return;
  }

  // get reference image info
  double originX, originY, originZ;
  double spacingX, spacingY, spacingZ;
  int dimensions[3] = {0, 0, 0};
  referenceDoseVolumeNode->GetOrigin(originX, originY, originZ);
  referenceDoseVolumeNode->GetSpacing(spacingX, spacingY, spacingZ);
  referenceDoseVolumeNode->GetImageData()->GetDimensions(dimensions);

  // set output image info
  outputVolumeNode->CopyOrientation(inputVolumeNode);
  outputVolumeNode->SetOrigin(originX, originY, originZ);
  outputVolumeNode->SetSpacing(spacingX, spacingY, spacingZ);
  outputVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), referenceDoseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str()));
  outputVolumeNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str(), referenceDoseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_VALUE_ATTRIBUTE_NAME.c_str()));

  // test if it is LPS orientation
  // TODO: right now wait for response from slicer developer ...

  vtkSmartPointer<vtkGeneralTransform> inputVolumeRASToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode = inputVolumeNode->GetParentTransformNode();
  if (inputVolumeNodeTransformNode!=NULL)
  {
    inputVolumeNodeTransformNode->GetTransformToWorld(inputVolumeRASToWorldTransform);    
    inputVolumeRASToWorldTransform->Inverse();
  }

  // change image info before reslice
  double origin[3];
  double spacing[3];
  inputVolumeNode->GetOrigin(origin);
  inputVolumeNode->GetSpacing(spacing);
  vtkSmartPointer<vtkImageChangeInformation> changeInfo = vtkSmartPointer<vtkImageChangeInformation>::New();
  changeInfo->SetInput(inputVolumeNode->GetImageData());
  changeInfo->SetOutputOrigin(origin);
  changeInfo->SetOutputSpacing(-spacing[0], -spacing[1], spacing[2]);
  changeInfo->Update();
  vtkSmartPointer<vtkImageData> tempImage = changeInfo->GetOutput();

  // reslice according to reference image
  vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInputConnection(changeInfo->GetOutputPort());
  reslice->SetOutputOrigin(originX, originY, originZ);
  reslice->SetOutputSpacing(-spacingX, -spacingY, spacingZ);
  reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  if (inputVolumeNodeTransformNode!=NULL)
  {
    reslice->SetResliceTransform(inputVolumeRASToWorldTransform);
  }
  reslice->Update();
  tempImage = reslice->GetOutput();

  vtkSmartPointer<vtkImageMathematics> MultiplyFilter1 = vtkSmartPointer<vtkImageMathematics>::New();
  MultiplyFilter1->SetInput(tempImage);
  MultiplyFilter1->SetConstantK(weight);
  MultiplyFilter1->SetOperationToMultiplyByK();
  MultiplyFilter1->Update();
  baseImageData = MultiplyFilter1->GetOutput();

  if (size >=2)
  {
    for (int i = 1; i < size; i++)
    {
      vtkSmartPointer<vtkImageMathematics> MultiplyFilter2 = vtkSmartPointer<vtkImageMathematics>::New();
      vtkSmartPointer<vtkImageMathematics> AddFilter = vtkSmartPointer<vtkImageMathematics>::New();
      iterIds++;
      Id = *iterIds;
      vtkSmartPointer<vtkMRMLScalarVolumeNode> inputVolumeNode2 = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(Id));
      if (!inputVolumeNode2->GetImageData())
      {
        errorMessage = "No image data found in input volume"; 
        vtkErrorMacro(<<errorMessage);
        continue;
      }
      weight = (*VolumeNodeIdsToWeightsMap)[Id];

      vtkSmartPointer<vtkGeneralTransform> inputVolumeNodeToWorldTransform2 = vtkSmartPointer<vtkGeneralTransform>::New();
      inputVolumeNodeToWorldTransform2->Identity();
      vtkSmartPointer<vtkMRMLTransformNode> inputVolumeNodeTransformNode2 = inputVolumeNode2->GetParentTransformNode();
      if (inputVolumeNodeTransformNode2!=NULL)
      {
        // toNode is transformed
        inputVolumeNodeTransformNode2->GetTransformToWorld(inputVolumeNodeToWorldTransform2);    
        inputVolumeNodeToWorldTransform2->Inverse();
      }

      // change image info before reslice
      double origin[3];
      double spacing[3];
      inputVolumeNode2->GetOrigin(origin);
      inputVolumeNode2->GetSpacing(spacing);
      vtkSmartPointer<vtkImageChangeInformation> changeInfo2 = vtkSmartPointer<vtkImageChangeInformation>::New();
      changeInfo2->SetInput(inputVolumeNode2->GetImageData());
      changeInfo2->SetOutputOrigin(origin);
      changeInfo2->SetOutputSpacing(-spacing[0], -spacing[1], spacing[2]);
      changeInfo2->Update();

      // reslice according to reference image
      vtkSmartPointer<vtkImageReslice> reslice2 = vtkSmartPointer<vtkImageReslice>::New();
      reslice2->SetInput(changeInfo2->GetOutput());
      reslice2->SetOutputOrigin(originX, originY, originZ);
      reslice2->SetOutputSpacing(-spacingX, -spacingY, spacingZ);
      reslice2->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
      reslice2->SetResliceTransform(inputVolumeNodeToWorldTransform2);
      reslice2->Update();
      
      MultiplyFilter2->SetInput(reslice2->GetOutput());
      MultiplyFilter2->SetConstantK(weight);
      MultiplyFilter2->SetOperationToMultiplyByK();
      MultiplyFilter2->Update();

      AddFilter->SetInput1(baseImageData);
      AddFilter->SetInput2(MultiplyFilter2->GetOutput());
      AddFilter->SetOperationToAdd();
      AddFilter->Update();
       
      baseImageData = AddFilter->GetOutput();
    }
  }

  // change image info back so it will work for slicer ???
  vtkSmartPointer<vtkImageChangeInformation> changeInfo3 = vtkSmartPointer<vtkImageChangeInformation>::New();
  changeInfo3->SetInput(baseImageData);
  changeInfo3->SetOutputOrigin(0,0,0);
  changeInfo3->SetOutputSpacing(1,1,1);
  changeInfo3->Update();

  outputVolumeNode->SetAndObserveImageData(changeInfo3->GetOutput());
  outputVolumeNode->SetAndObserveDisplayNodeID( outputVolumeDisplayNode->GetID() );

  // Set default colormap to rainbow
  outputVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");

  // Set threshold values so that the background is black
  //outputVolumeDisplayNode->AutoThresholdOff();
  //outputVolumeDisplayNode->SetLowerThreshold(0.0001);
  //outputVolumeDisplayNode->SetApplyThreshold(1);

  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(outputVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  outputVolumeDisplayNode->SetVisibility(1);
}
