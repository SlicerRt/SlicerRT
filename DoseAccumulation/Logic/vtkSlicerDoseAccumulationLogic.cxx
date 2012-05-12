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

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include "vtkMRMLDoseAccumulationNode.h"

// VTK includes
#include <vtkNew.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseAccumulationLogic);

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

  this->SceneChangedOn();
  this->Modified(); //TODO: Why does it have to be called explicitly?
  this->OnMRMLNodeModified(node);
}

//---------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseAccumulationLogic::GetVolumeNodesFromScene()
{
  if (this->GetMRMLScene() == NULL || this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLVolumeNode") < 1
    || this->DoseAccumulationNode == NULL)
  {
    return NULL;
  }

  vtkCollection* volumeNodes = vtkCollection::New();
  volumeNodes->InitTraversal();

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
  vtkMRMLDoseAccumulationNode *tnode = 0;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseAccumulationNode");
  if (node)
  {
    tnode = vtkMRMLDoseAccumulationNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseAccumulationNode, tnode);
  }
  this->InvokeEvent(vtkMRMLScene::EndImportEvent);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic
::AccumulateDoseVolumes()
{
  //TODO: Kevin's code goes here
  // The selected volume node IDs and their weights can be found in the two arrays
  //  this->GetDoseAccumulationNode()->GetSelectedInputVolumeIds()
  //  and this->GetDoseAccumulationNode()->GetSelectedInputVolumeWeights()
  // Output volume node ID is this->GetDoseAccumulationNode()->GetAccumulatedDoseVolumeId()
}
