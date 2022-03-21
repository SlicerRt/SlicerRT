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

#include <vtkSlicerRtCommon.h>

// SubjectHierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// Sequences inludes
#include <vtkMRMLSequenceBrowserNode.h>
#include <vtkMRMLSequenceNode.h>

// MRML includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
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

//----------------------------------------------------------------------------
bool vtkSlicerBeamsModuleLogic::CreateArcBeamDynamicSequence( 
  double initialAngle, double finalAngle, double stepAngle, bool direction,
  vtkMRMLRTPlanNode* planNode, vtkMRMLSequenceBrowserNode* beamSequenceBrowserNode,
  vtkMRMLSequenceNode* beamSequenceNode, vtkMRMLSequenceNode* transformSequenceNode)
{
  std::vector<double> angles;
  if (vtkSlicerRtCommon::AreEqualWithTolerance(finalAngle, initialAngle))
  {
    return false;
  }

  if (finalAngle < 0. && finalAngle > 360. && initialAngle < 0. && initialAngle > 360.)
  {
    return false;
  }

  if (!direction && initialAngle < finalAngle) // CW, ini < fin
  {
    for (double angle = initialAngle; angle <= finalAngle; angle += stepAngle)
    {
      angles.push_back(angle);
    }
    if (angles.back() < finalAngle)
    {
      angles.push_back(finalAngle);
    }
    std::cout << "Angles " << angles.size() << '\n';
    std::for_each( angles.begin(), angles.end(), [](double v){ std::cout << v << ' '; });
    std::cout << std::endl;
  }
  else if (!direction && initialAngle > finalAngle) // CW, ini > fin
  {
    for (double angle = initialAngle; angle <= 360.; angle += stepAngle)
    {
      angles.push_back(angle);
    }
    double startAngle = 1.;
    if (angles.back() < 360.)
    {
      startAngle = angles.back() + stepAngle - 360.;
    }
    else if (angles.back() == 360.)
    {
      startAngle = stepAngle;
    }

    for (double angle = startAngle; angle <= finalAngle; angle += stepAngle)
    {
      angles.push_back(angle);
    }
    if (angles.back() < finalAngle)
    {
      angles.push_back(finalAngle);
    }
    std::cout << "Angles " << angles.size() << '\n';
    std::for_each( angles.begin(), angles.end(), [](double v){ std::cout << v << ' '; });
    std::cout << std::endl;
  }
  else if (direction && initialAngle < finalAngle) // CCW, ini < fin
  {
    for (double angle = initialAngle; angle >= 0.0; angle -= stepAngle)
    {
      angles.push_back(angle);
    }
    double startAngle = 359.;
    if (angles.back() > 0.)
    {
      startAngle = angles.back() - stepAngle + 360.;
    }
    else if (angles.back() == 0.)
    {
      startAngle = 360. - stepAngle;
    }
    for (double angle = startAngle; angle >= finalAngle; angle -= stepAngle)
    {
      angles.push_back(angle);
    }
    if (angles.back() > finalAngle)
    {
      angles.push_back(finalAngle);
    }
    std::cout << "Angles " << angles.size() << '\n';
    std::for_each( angles.begin(), angles.end(), [](double v){ std::cout << v << ' '; });
    std::cout << std::endl;
  }
  else if (direction && initialAngle > finalAngle) // CCW, ini > fin
  {
    for (double angle = initialAngle; angle >= finalAngle; angle -= stepAngle)
    {
      angles.push_back(angle);
    }
    if (angles.back() > finalAngle)
    {
      angles.push_back(finalAngle);
    }
    std::cout << "Angles " << angles.size() << '\n';
    std::for_each( angles.begin(), angles.end(), [](double v){ std::cout << v << ' '; });
    std::cout << std::endl;
  }
/*
  if (finalAngle < 0. && finalAngle > 360. && initialAngle < 0. && initialAngle > 360.)
  {
    return false;
  }

  if (!direction && initialAngle < finalAngle) // CW, ini < fin
  {
    for (double angle = initialAngle; angle <= finalAngle; angle += 1.)
    {
      angles.push_back(angle);
    }
  }
  else if (!direction && initialAngle > finalAngle) // CW, ini > fin
  {
    for (double angle = initialAngle; angle <= 360.; angle += 1.)
    {
      angles.push_back(angle);
    }
    for (double angle = 1.; angle <= finalAngle; angle += 1.)
    {
      angles.push_back(angle);
    }
  }
  else if (direction && initialAngle < finalAngle) // CCW, ini < fin
  {
    for (double angle = initialAngle; angle >= 0.0; angle -= 1.)
    {
      angles.push_back(angle);
    }
    for (double angle = 359.; angle >= finalAngle; angle -= 1.)
    {
      angles.push_back(angle);
    }
  }
  else if (direction && initialAngle > finalAngle) // CCW, ini > fin
  {
    for (double angle = initialAngle; angle >= finalAngle; angle -= 1.)
    {
      angles.push_back(angle);
    }
  }
*/
  if (angles.size() < 2)
  {
    vtkErrorMacro("CreateArcBeamDynamicSequence: Number of angle elements is less than 2");
    return false;
  }

  vtkMRMLScene* scene = planNode->GetScene();

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  if (!shNode)
  {
    vtkErrorMacro("CreateArcBeamDynamicSequence: Failed to access subject hierarchy node");
    return false;
  }
  vtkSmartPointer<vtkMRMLModelHierarchyNode> beamModelHierarchyRootNode;

  const char* beamName = "Beam [ArcDelivery]";
  // Create sequence node for RTBeam, transformation, table

  // beamSequenceNode
  beamSequenceNode->SetName(beamName);
  beamSequenceNode->SetIndexName("Control point");
  beamSequenceNode->SetIndexUnit("index");
  beamSequenceNode->SetIndexType(vtkMRMLSequenceNode::NumericIndex);

  // transformSequenceNode
  std::string name = std::string(beamName) + vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX;
  transformSequenceNode->SetName(name.c_str());
  transformSequenceNode->SetIndexName("Control point");
  transformSequenceNode->SetIndexUnit("index");
  transformSequenceNode->SetIndexType(vtkMRMLSequenceNode::NumericIndex);

  // beamSequenceBrowserNode
  name = std::string(beamName) + "_SequenceBrowser";
  beamSequenceBrowserNode->SetName(name.c_str());

  // Add sequence nodes to the scene
  scene->AddNode(beamSequenceNode);
  scene->AddNode(transformSequenceNode);
  scene->AddNode(beamSequenceBrowserNode);

  for ( auto it = angles.begin(); it != angles.end(); ++it)
  {
    unsigned int controlPointIndex = it - angles.begin();
    // Create the beam node for each control point
    vtkSmartPointer<vtkMRMLRTBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();

    std::ostringstream nameStream;
    nameStream << beamName << " [Dynamic] : CP" << controlPointIndex;

    std::string newBeamName = nameStream.str();
    beamNode->SetName(newBeamName.c_str());

    // Set beam geometry parameters from DICOM
    double jawPositions[2][2] = {{-40.0, 40.0},{-40.0, 40.0}};
    beamNode->SetX1Jaw(jawPositions[0][0]);
    beamNode->SetX2Jaw(jawPositions[0][1]);
    beamNode->SetY1Jaw(jawPositions[1][0]);
    beamNode->SetY2Jaw(jawPositions[1][1]);

    beamNode->SetGantryAngle(*it);

    // SAD for RTPlan, source to beam limiting devices (Jaws, MLC)
    if (beamNode)
    {
//      beamNode->SetSAD(rtReader->GetBeamSourceAxisDistance(dicomBeamNumber));
//      beamNode->SetSourceToJawsDistanceX(rtReader->GetBeamSourceToJawsDistanceX(dicomBeamNumber));
//      beamNode->SetSourceToJawsDistanceY(rtReader->GetBeamSourceToJawsDistanceY(dicomBeamNumber));
//      beamNode->SetSourceToMultiLeafCollimatorDistance(rtReader->GetBeamSourceToMultiLeafCollimatorDistance(dicomBeamNumber));
    }

    // Add beam to beam sequence node
    if (beamNode)
    {
      beamSequenceNode->SetDataNodeAtValue( beamNode, std::to_string(controlPointIndex));
    }

    // Get or create RTBeam transformation from RTPlan without adding beam to the plan
    // Add beam transformation to transform sequence
    vtkMRMLLinearTransformNode* transformNode = beamNode->CreateBeamTransformNode(scene);
    if (transformNode)
    {
      double isocenter[3] = { 0., 0., 0.};
      planNode->GetIsocenterPosition(isocenter);
      {
        // Update beam transform without translation to isocenter
        this->UpdateTransformForBeam( beamSequenceNode->GetSequenceScene(), beamNode, transformNode, isocenter);

        vtkTransform* transform = vtkTransform::SafeDownCast(transformNode->GetTransformToParent());
        if (isocenter)
        {
          // Actual translation to isocenter
          transform->Translate( isocenter[0], isocenter[1], isocenter[2]);
          transformNode->Modified();
        }
    
        transformSequenceNode->SetDataNodeAtValue( transformNode, std::to_string(controlPointIndex));
      }
    }
  } // end of a control point

  // Synchronize beam sequence, beam transform sequence, table sequence nodes
  beamSequenceBrowserNode->SetAndObserveMasterSequenceNodeID(beamSequenceNode->GetID());
  beamSequenceBrowserNode->AddSynchronizedSequenceNode(transformSequenceNode);

  // Get proxy beam node
  vtkMRMLNode* node = beamSequenceBrowserNode->GetProxyNode(beamSequenceNode);
  if (vtkMRMLRTBeamNode::SafeDownCast(node))
  {
    return true;
  }
  else
  {
    return false;
  }
}
