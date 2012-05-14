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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationLogic.h"
#include "vtkMRMLDoseAccumulationNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageMathematics.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkImageCast.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseAccumulationLogic);

//vtkCxxSetObjectMacro(vtkSlicerDoseAccumulationLogic, VolumesLogic, vtkSlicerVolumesLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationLogic::vtkSlicerDoseAccumulationLogic()
{
  this->DoseAccumulationNode = NULL;
  this->SceneChangedOff();
}

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationLogic::~vtkSlicerDoseAccumulationLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::SetAndObserveDoseAccumulationNode(vtkMRMLDoseAccumulationNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseAccumulationNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->SceneChangedOn();
  this->Modified(); //TODO: Why does it have to be called explicitly?
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  this->SceneChangedOn();
  this->Modified(); //TODO: Why does it have to be called explicitly?
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (this->DoseAccumulationNode)
  {
    this->DoseAccumulationNode->GetSelectedInputVolumeIds()->erase(node->GetID());
    this->DoseAccumulationNode->GetVolumeNodeIdsToWeightsMap()->erase(node->GetID());
  }

  this->SceneChangedOn();
  this->Modified(); //TODO: Why does it have to be called explicitly?
}

//---------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseAccumulationLogic::GetVolumeNodesFromScene()
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
      const char* doseUnitName = volumeNode->GetAttribute("DoseUnitName");
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
void vtkSlicerDoseAccumulationLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLDoseAccumulationNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
  if (node)
  {
    paramNode = vtkMRMLDoseAccumulationNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, paramNode);
  }
  this->InvokeEvent(vtkMRMLScene::EndImportEvent);
}

//---------------------------------------------------------------------------
int vtkSlicerDoseAccumulationLogic
::AccumulateDoseVolumes()
{
  // Make sure inputs are initialized
  if(this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->empty())
  {
    std::cerr << "Dose accumulation: No dose volume selected" << std::endl;
    return -1;
  }

  int size = this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->size();
  vtkSmartPointer<vtkImageData> baseImageData = NULL;

  std::set<std::string>::iterator iterIds = this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()->begin();
  std::string Id = *iterIds;
  vtkSmartPointer<vtkMRMLVolumeNode> inputVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(Id));
  std::map<std::string,double> *VolumeNodeIdsToWeightsMap = this->GetDoseAccumulationNode()->GetVolumeNodeIdsToWeightsMap();
  double weight = (*VolumeNodeIdsToWeightsMap)[Id];

  vtkSmartPointer<vtkMRMLVolumeNode> outputVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(
    this->GetDoseAccumulationNode()->GetAccumulatedDoseVolumeNodeId()));

  vtkSmartPointer<vtkImageMathematics> MultiplyFilter1 = vtkSmartPointer<vtkImageMathematics>::New();
  MultiplyFilter1->SetInput(inputVolumeNode->GetImageData());
  MultiplyFilter1->SetConstantK(weight);
  MultiplyFilter1->SetOperationToMultiplyByK();
  MultiplyFilter1->Update();
  baseImageData = MultiplyFilter1->GetOutput();

  if (size >=2)
  {
    vtkSmartPointer<vtkImageMathematics> MultiplyFilter2 = vtkSmartPointer<vtkImageMathematics>::New();
    vtkSmartPointer<vtkImageMathematics> AddFilter = vtkSmartPointer<vtkImageMathematics>::New();
    for (int i = 1; i < size; i++)
    {
      Id = *iterIds++;
      inputVolumeNode = vtkMRMLVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(Id));
      weight = (*VolumeNodeIdsToWeightsMap)[Id];

      MultiplyFilter2->SetInput(inputVolumeNode->GetImageData());
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
  double originX, originY, originZ;
  baseImageData->GetOrigin(originX, originY, originZ);
  outputVolumeNode->SetOrigin(originX, originY, originZ);
  outputVolumeNode->SetAndObserveImageData(baseImageData);
  outputVolumeNode->SetModifiedSinceRead(1); 

  // Set default colormap to rainbow
  if (outputVolumeNode->GetVolumeDisplayNode()!=NULL)
  {
    outputVolumeNode->GetVolumeDisplayNode()->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
  }
  // Select as active volume
  if (this->GetApplicationLogic()!=NULL)
  {
    if (this->GetApplicationLogic()->GetSelectionNode()!=NULL)
    {
      this->GetApplicationLogic()->GetSelectionNode()->SetReferenceActiveVolumeID(outputVolumeNode->GetID());
      this->GetApplicationLogic()->PropagateVolumeSelection();
    }
  }

  return 0;
}
