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

==============================================================================*/

// DoseAccumulation includes
#include "vtkSlicerDoseAccumulationLogic.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkNew.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseAccumulationLogic);

vtkCxxSetObjectMacro(vtkSlicerDoseAccumulationLogic, AccumulatedDoseVolumeNode, vtkMRMLVolumeNode);

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationLogic::vtkSlicerDoseAccumulationLogic()
{
  this->AccumulatedDoseVolumeNode = NULL;

  this->SceneChangedOff();
}

//----------------------------------------------------------------------------
vtkSlicerDoseAccumulationLogic::~vtkSlicerDoseAccumulationLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->SceneChangedOn();
  this->Modified();
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
  this->Modified();
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
  this->Modified();
}

//---------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseAccumulationLogic
::GetVolumeNodes(bool doseVolumesOnly)
{
  if (this->GetMRMLScene() == NULL || this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLVolumeNode") < 1)
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
      if (doseUnitName != NULL || !doseVolumesOnly)
      {
        volumeNodes->AddItem(volumeNode);
      }
    }

    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLVolumeNode");
  }

  return volumeNodes;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseAccumulationLogic
::AccumulateDoseVolumes(std::vector< std::pair<std::string,double> > volumeIdsAndWeights)
{
  //TODO: Kevin's code goes here
}
