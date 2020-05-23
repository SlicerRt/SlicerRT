/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// Beams includes
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkSlicerIECTransformLogic.h"
#include "vtkSlicerMLCPositionLogic.h"

// SlicerRT includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkGeneralTransform.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::vtkSlicerBeamsModuleLogic()
 :
 MLCPositionLogic(vtkSlicerMLCPositionLogic::New())
{
}

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::~vtkSlicerBeamsModuleLogic()
{
  if (this->MLCPositionLogic)
  {
    this->MLCPositionLogic->Delete();
    this->MLCPositionLogic = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLRTPlanNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  }
  if (!scene->IsNodeClassRegistered("vtkMRMLRTBeamNode"))
  {
    scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());

  if (this->MLCPositionLogic)
  {
    this->MLCPositionLogic->SetMRMLScene(newScene);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node");
    return;
  }

  if (node->IsA("vtkMRMLRTBeamNode"))
  {
    // Observe beam events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTBeamNode::BeamGeometryModified);
    events->InsertNextValue(vtkMRMLRTBeamNode::BeamTransformModified);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
  else if (node->IsA("vtkMRMLRTPlanNode"))
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTPlanNode::BeamAdded);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
  else if (node->IsA("vtkMRMLTableNode"))
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneEndImport()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("OnMRMLSceneEndImport: Invalid MRML scene");
    return;
  }

  // Observe beam events of all beam nodes
  std::vector<vtkMRMLNode*> beamNodes;
  scene->GetNodesByClass("vtkMRMLRTBeamNode", beamNodes);
  for (std::vector<vtkMRMLNode*>::iterator beamIt=beamNodes.begin(); beamIt!=beamNodes.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(*beamIt);

    // Re-observe poly data in beam model node
    // Note: Without this, the beam polydata display is not updated when the beam geometry changes. The
    //   reason for this is possibly that the pipeline is set up with the file reader and on any modified
    //   event that pipeline is used instead of simply using the changed contents of the beam polydata.
    beamNode->SetAndObserveMesh(beamNode->GetMesh());
    
    // Observe beam events
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLRTBeamNode::BeamGeometryModified);
    events->InsertNextValue(vtkMRMLRTBeamNode::BeamTransformModified);
    vtkObserveMRMLNodeEventsMacro(beamNode, events);

    // Make sure geometry and transforms are up-to-date
    beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamGeometryModified);
    beamNode->InvokeCustomModifiedEvent(vtkMRMLRTBeamNode::BeamTransformModified);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateTransformForBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateTransformForBeam: Invalid beam node");
    return;
  }
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateTransformForBeam: Invalid MRML scene");
    return;
  }

  //TODO: Use one IEC logic in a private scene for all beam transform updates?
  vtkSmartPointer<vtkSlicerIECTransformLogic> iecLogic = vtkSmartPointer<vtkSlicerIECTransformLogic>::New();
  iecLogic->SetMRMLScene(scene);
  iecLogic->UpdateBeamTransform(beamNode);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateTransformForBeam( vtkMRMLScene* beamSequenceScene, 
  vtkMRMLRTBeamNode* beamNode, vtkMRMLLinearTransformNode* beamTransformNode, double* isocenter)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateTransformForBeam: Invalid beam node");
    return;
  }
  if (!beamTransformNode)
  {
    vtkErrorMacro("UpdateTransformForBeam: Invalid beam transform node");
    return;
  }

  if (!beamSequenceScene)
  {
    vtkErrorMacro("UpdateTransformForBeam: Invalid MRML scene");
    return;
  }

  //TODO: Use one IEC logic in a private scene for all beam transform updates?
  vtkSmartPointer<vtkSlicerIECTransformLogic> iecLogic = vtkSmartPointer<vtkSlicerIECTransformLogic>::New();
  iecLogic->SetMRMLScene(beamSequenceScene);
  iecLogic->UpdateBeamTransform( beamNode, beamTransformNode, isocenter);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLNodesEvents(caller, event, callData);

  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("ProcessMRMLNodesEvents: Invalid MRML scene");
    return;
  }
  if (mrmlScene->IsBatchProcessing())
  {
    return;
  }

  if (caller->IsA("vtkMRMLRTBeamNode"))
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(caller);
    if (event == vtkMRMLRTBeamNode::BeamTransformModified)
    {
      // Make sure transform node exists
      beamNode->CreateDefaultTransformNode();
      // Calculate transform from beam parameters and isocenter from plan
      this->UpdateTransformForBeam(beamNode);
    }
    else if (event == vtkMRMLRTBeamNode::BeamGeometryModified)
    {
      // Update beam model
      beamNode->UpdateGeometry();
    }
  }
  else if (caller->IsA("vtkMRMLRTPlanNode"))
  {
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(caller);

    if (event == vtkMRMLRTPlanNode::BeamAdded)
    {
      // Get added beam node
      char* beamNodeID = reinterpret_cast<char*>(callData);
      if (!beamNodeID)
      {
        vtkErrorMacro("ProcessMRMLNodesEvents: No beam node ID for beam added event in plan " << planNode->GetName());
        return;
      }
      vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(mrmlScene->GetNodeByID(beamNodeID));
      if (!beamNode)
      {
        vtkErrorMacro("ProcessMRMLNodesEvents: Failed to get added beam node by ID " << beamNodeID);
        return;
      }

      // Make sure transform node exists
      beamNode->CreateDefaultTransformNode();
      // Calculate transform from beam parameters and isocenter from plan
      this->UpdateTransformForBeam(beamNode);

      // Make sure display is set up
      beamNode->UpdateGeometry();
    }
  }
  else if (caller->IsA("vtkMRMLTableNode"))
  {
    if (event == vtkCommand::ModifiedEvent)
    {
      // Iterate through all beam nodes
      std::vector<vtkMRMLNode*> beamNodes;
      mrmlScene->GetNodesByClass("vtkMRMLRTBeamNode", beamNodes);
      for (std::vector<vtkMRMLNode*>::iterator beamIterator = beamNodes.begin(); 
        beamIterator != beamNodes.end(); ++beamIterator)
      {
        // if caller node and referenced table node is the same
        // update beam polydata
        vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(*beamIterator);
        vtkMRMLTableNode* beamMLCTableNode = beamNode->GetMultiLeafCollimatorTableNode();
        vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(caller);
        if (beamMLCTableNode == tableNode)
        {
          // Update beam model
          beamNode->UpdateGeometry();
        }
      }
    }
  }
}
