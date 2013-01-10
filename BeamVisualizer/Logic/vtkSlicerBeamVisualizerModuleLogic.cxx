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

// BeamVisualizer includes
#include "vtkSlicerBeamVisualizerModuleLogic.h"
#include "vtkMRMLBeamVisualizerNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerBeamVisualizerModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerBeamVisualizerModuleLogic::vtkSlicerBeamVisualizerModuleLogic()
{
  this->BeamVisualizerNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerBeamVisualizerModuleLogic::~vtkSlicerBeamVisualizerModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamVisualizerNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::SetAndObserveBeamVisualizerNode(vtkMRMLBeamVisualizerNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamVisualizerNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerBeamVisualizerModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLBeamVisualizerNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLAnnotationFiducialNode") || node->IsA("vtkMRMLBeamVisualizerNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLAnnotationFiducialNode") || node->IsA("vtkMRMLBeamVisualizerNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLBeamVisualizerNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLBeamVisualizerNode");
  if (node)
  {
    paramNode = vtkMRMLBeamVisualizerNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->BeamVisualizerNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::ComputeSourceFiducialPosition(std::string &errorMessage)
{
  if (!this->BeamVisualizerNode || !this->GetMRMLScene())
  {
    return;
  }
  if ( !this->BeamVisualizerNode->GetIsocenterFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetIsocenterFiducialNodeId(), "") )
  {
    errorMessage = "Empty isocenter fiducial node ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  vtkMRMLAnnotationFiducialNode* isocenterNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->BeamVisualizerNode->GetIsocenterFiducialNodeId()) );
  if (!isocenterNode)
  {
    errorMessage = "Unable to retrieve isocenter fiducial node according its ID!";
    vtkErrorMacro(<<errorMessage); 
    return;
  }

  double collimatorAngle;
}

//---------------------------------------------------------------------------
void vtkSlicerBeamVisualizerModuleLogic::CreateBeamModel(std::string &errorMessage)
{
  if (!this->BeamVisualizerNode || !this->GetMRMLScene())
  {
    return;
  }
  if ( !this->BeamVisualizerNode->GetIsocenterFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetIsocenterFiducialNodeId(), "")
    || this->BeamVisualizerNode->GetSourceFiducialNodeId()
    || !strcmp(this->BeamVisualizerNode->GetSourceFiducialNodeId(), "") )
  {
    errorMessage = "Insufficient input (isocenter and.or source fiducial is empty)!";
    vtkErrorMacro(<<errorMessage);
    return;
  }
}
