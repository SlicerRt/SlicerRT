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

// ProtonDose includes
#include "vtkSlicerProtonDoseModuleLogic.h"
#include "vtkMRMLProtonDoseNode.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkNew.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerProtonDoseModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerProtonDoseModuleLogic::vtkSlicerProtonDoseModuleLogic()
{
  this->ProtonDoseNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerProtonDoseModuleLogic::~vtkSlicerProtonDoseModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->ProtonDoseNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::SetAndObserveProtonDoseNode(vtkMRMLProtonDoseNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ProtonDoseNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerProtonDoseModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLProtonDoseNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLProtonDoseNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLProtonDoseNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLProtonDoseNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLProtonDoseNode");
  if (node)
  {
    paramNode = vtkMRMLProtonDoseNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ProtonDoseNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerProtonDoseModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int vtkSlicerProtonDoseModuleLogic::ComputeProtonDose()
{
  // Make sure inputs are initialized
  if (!this->GetMRMLScene())
  {
    return 1;
  }

  return 0;
}
