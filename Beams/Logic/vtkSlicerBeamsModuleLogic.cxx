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
#include "vtkSlicerMLCPositionLogic.h"

// SlicerRT includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTIonRangeShifterNode.h"

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
  : MLCPositionLogic(vtkSlicerMLCPositionLogic::New())
  , IECLogic(vtkIECTransformLogic::New())
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
  if (this->IECLogic)
  {
    this->IECLogic->Delete();
    this->IECLogic = nullptr;
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
void vtkSlicerBeamsModuleLogic::SetIECLogic(vtkIECTransformLogic* iecLogic)
{
  if (iecLogic == nullptr)
  {
    return; // Do not set invalid IED logic because one is needed at all times
  }

  // Delete existing logic to prevent memory leak
  this->IECLogic->Delete();
  this->IECLogic = nullptr;

  vtkSetObjectBodyMacro(IECLogic, vtkIECTransformLogic, iecLogic);
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
      vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(beamNode);
      if (ionBeamNode)
      {
        vtkMRMLRTIonRangeShifterNode* rsNode = ionBeamNode->GetRangeShifterNode();
        if (rsNode)
        {
          // Update range shifter model
          rsNode->UpdateGeometry();
        }
      }
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
      vtkIntArray* array = static_cast< vtkIntArray* >(callData);

      // Iterate through all beam nodes
      std::vector<vtkMRMLNode*> beamNodes;
      mrmlScene->GetNodesByClass("vtkMRMLRTBeamNode", beamNodes);
      for (std::vector<vtkMRMLNode*>::iterator beamIterator = beamNodes.begin(); 
        beamIterator != beamNodes.end(); ++beamIterator)
      {
        // if caller node and referenced table node is the same
        // update beam polydata
        vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(*beamIterator);
        vtkMRMLRTIonBeamNode* ionBeamNode = vtkMRMLRTIonBeamNode::SafeDownCast(beamNode);
        vtkMRMLTableNode* beamMLCTableNode = beamNode->GetMultiLeafCollimatorTableNode();
        vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(caller);
        if (beamMLCTableNode == tableNode) // update MLC for RTBeam and RTIonBeam
        {
          // Update beam model
          beamNode->UpdateGeometry();
          return;
        }
        if (ionBeamNode) // update ScanSpot RTIonBeam
        {
          vtkMRMLTableNode* beamScanSpotTableNode = ionBeamNode->GetScanSpotTableNode();
          if ((beamScanSpotTableNode == tableNode) && array) // ion beam with scan spot rows
          {
            ionBeamNode->UpdateScanSpotGeometry(array);
          }
        }
      }
    }
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

  this->UpdateBeamTransform(beamNode);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateTransformForBeam(vtkMRMLScene* beamSequenceScene, 
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
  //TODO: beamSequenceScene is not used at all
  if (!beamSequenceScene)
  {
    vtkErrorMacro("UpdateTransformForBeam: Invalid MRML scene");
    return;
  }

  this->UpdateBeamTransform(beamNode, beamTransformNode, isocenter);
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateBeamTransform(vtkMRMLRTBeamNode* beamNode)
{
  //TODO: Observe beam node's geometry modified event (vtkMRMLRTBeamNode::BeamGeometryModified)
  // and its parent plan's POI markups fiducial's point modified event (vtkMRMLMarkupsNode::PointModifiedEvent)
  // so that UpdateTransformsFromBeamGeometry is called. It may be needed to change the signature of the
  // update function. It may be also needed to store a reference to the beam node (see defined nodes in SlicerRT)

  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid beam node");
    return;
  }

  // Make sure transform node exists
  beamNode->CreateDefaultTransformNode();

  // Update transform for beam
  vtkMRMLLinearTransformNode* beamTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetParentTransformNode());
  if (!beamTransformNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Failed to access transform node of beam " << beamNode->GetName());
    return;
  }

  this->UpdateBeamTransform(beamNode, beamTransformNode);
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateBeamTransform(vtkMRMLRTBeamNode* beamNode, vtkMRMLLinearTransformNode* beamTransformNode, double* isocenter/*=nullptr*/)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid beam node");
    return;
  }
  if (!beamTransformNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid beam transform node");
    return;
  }

  // Update transforms in IEC logic from beam node parameters
  this->UpdateIECTransformsFromBeam(beamNode, isocenter);

  vtkNew<vtkGeneralTransform> beamGeneralTransform;
  this->IECLogic->GetTransformBetween(vtkIECTransformLogic::Collimator, vtkIECTransformLogic::RAS, beamGeneralTransform, true);

  // Convert general transform to linear
  // This call also makes hard copy of the transform so that it doesn't change when other beam transforms change
  vtkNew<vtkTransform> beamLinearTransform;
  if (!vtkMRMLTransformNode::IsGeneralTransformLinear(beamGeneralTransform, beamLinearTransform))
  {
    vtkErrorMacro("UpdateBeamTransform: Unable to set transform with non-linear components to beam " << beamNode->GetName());
    return;
  }

  // Set transform to beam node
  beamTransformNode->SetAndObserveTransformToParent(beamLinearTransform);
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateIECTransformsFromBeam(vtkMRMLRTBeamNode* beamNode, double* isocenter)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateIECTransformsFromBeam: Invalid beam node");
    return;
  }
  if (!isocenter)
  {
    vtkMRMLScene* scene = beamNode->GetScene();
    if (!scene || this->GetMRMLScene() != scene)
    {
      vtkErrorMacro("UpdateIECTransformsFromBeam: Invalid MRML scene");
      return;
    }
  }

  // Set beam angles to IEC logic
  this->IECLogic->UpdateGantryToFixedReferenceTransform(beamNode->GetGantryAngle());
  this->IECLogic->UpdateCollimatorToGantryTransform(beamNode->GetCollimatorAngle());
  // Set the inverse of the couch angle as now what moves is the room (treatment machine) around the patient,
  // so the patient support table top (couch) always stays stationary in RAS.
  this->IECLogic->UpdatePatientSupportRotationToFixedReferenceTransform(-1. * beamNode->GetCouchAngle());

  // Update fixed reference to RAS transform as well
  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  this->UpdateRASRelatedTransforms(nullptr, parentPlanNode, isocenter, true);
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateRASRelatedTransforms(
  vtkIECTransformLogic* iecLogic/*=nullptr*/, vtkMRMLRTPlanNode* planNode/*=nullptr*/, double* isocenter/*=nullptr*/, bool transformForBeam/*=false*/)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateRASRelatedTransforms: Invalid MRML scene");
    return;
  }
  if (iecLogic == nullptr)
  {
    iecLogic = this->IECLogic;
  }

  // Update IEC FixedReference to RAS transform based on the isocenter defined in the beam's parent plan.
  // Do the same for the RAS to Patient transform as well.
  vtkNew<vtkTransform> fixedReferenceToRASTransformBeamComponent;
  vtkTransform* rasToPatientReferenceTransform = iecLogic->GetElementaryTransformBetween(
    vtkIECTransformLogic::RAS, vtkIECTransformLogic::Patient);
  if (rasToPatientReferenceTransform == nullptr)
  {
    vtkErrorMacro("UpdateRASRelatedTransforms: Failed to find RAS related transforms in the IEC logic");
    return;
  }

  // Reset transforms before applying translation and rotations
  fixedReferenceToRASTransformBeamComponent->Identity();
  rasToPatientReferenceTransform->Identity();

  // Apply isocenter translation if requested for both transforms
  if (planNode)
  {
    if (isocenter)
    {
      // Once again the dirty hack for dynamic beams, the actual translation 
      // will be in vtkSlicerDicomRtImportExportModuleLogic::vtkInternal::LoadDynamicBeamSequence method
      fixedReferenceToRASTransformBeamComponent->Translate(isocenter[0], isocenter[1], isocenter[2]); //TODO: This was always 0 before, confirm this change (to use isocenter if given as argument)
      rasToPatientReferenceTransform->Translate(isocenter[0], isocenter[1], isocenter[2]);
    }
    else
    {
      // translation for a static beam
      std::array<double, 3> isocenterPosition = { 0.0, 0.0, 0.0 };
      if (planNode->GetIsocenterPosition(isocenterPosition.data()))
      {
        fixedReferenceToRASTransformBeamComponent->Translate(isocenterPosition[0], isocenterPosition[1], isocenterPosition[2]);
        rasToPatientReferenceTransform->Translate(isocenterPosition[0], isocenterPosition[1], isocenterPosition[2]);
      }
      else
      {
        vtkErrorMacro("UpdateRASRelatedTransforms: Failed to get isocenter position for plan " << planNode->GetName());
      }
    }
  }

  // Set up RAS to Patient transform
  rasToPatientReferenceTransform->RotateX(-90.0);
  rasToPatientReferenceTransform->RotateZ(180.0);
  rasToPatientReferenceTransform->Modified();

  //
  // Set up fixed reference to RAS transform. It keeps the table top stationary so that it is fixed relative to the patient.
  //

  // The "S" direction in RAS is the "A" direction in FixedReference
  fixedReferenceToRASTransformBeamComponent->RotateX(-90.0);
  // The "S" direction to be toward the gantry (head first position) by default
  fixedReferenceToRASTransformBeamComponent->RotateZ(180.0);
  fixedReferenceToRASTransformBeamComponent->Modified();

  // Set up concatenation for final fixed reference to RAS transform
  vtkNew<vtkGeneralTransform> tableTopToTableTopEccentricRotationGeneralTransform;
  iecLogic->GetTransformBetween(
    vtkIECTransformLogic::TableTop, vtkIECTransformLogic::TableTopEccentricRotation, tableTopToTableTopEccentricRotationGeneralTransform, transformForBeam);
  vtkNew<vtkTransform> tableTopToTableTopEccentricRotationLinearTransform;
  if (!vtkMRMLTransformNode::IsGeneralTransformLinear(tableTopToTableTopEccentricRotationGeneralTransform, tableTopToTableTopEccentricRotationLinearTransform))
  {
    vtkErrorMacro("UpdateRASRelatedTransforms: IEC transform TableTop to TableTopEccentricRotation contains non-linear components");
    return;
  }

  vtkNew<vtkGeneralTransform> patientSupportRotationToFixedReferenceGeneralTransform;
  iecLogic->GetTransformBetween(
    vtkIECTransformLogic::PatientSupportRotation, vtkIECTransformLogic::FixedReference, patientSupportRotationToFixedReferenceGeneralTransform, transformForBeam);
  vtkNew<vtkTransform> patientSupportRotationToFixedReferenceLinearTransform;
  if (!vtkMRMLTransformNode::IsGeneralTransformLinear(patientSupportRotationToFixedReferenceGeneralTransform, patientSupportRotationToFixedReferenceLinearTransform))
  {
    vtkErrorMacro("UpdateRASRelatedTransforms: IEC transform PatientSupportRotation to FixedReference contains non-linear components");
    return;
  }

  vtkNew<vtkTransform> fixedReferenceToRASTransformNew;
  fixedReferenceToRASTransformNew->Concatenate(fixedReferenceToRASTransformBeamComponent);
  tableTopToTableTopEccentricRotationLinearTransform->Inverse();
  fixedReferenceToRASTransformNew->Concatenate(tableTopToTableTopEccentricRotationLinearTransform);
  patientSupportRotationToFixedReferenceLinearTransform->Inverse();
  fixedReferenceToRASTransformNew->Concatenate(patientSupportRotationToFixedReferenceLinearTransform);

  // Update fixed reference to RAS transform in IEC logic
  vtkTransform* fixedReferenceToRASTransform = iecLogic->GetElementaryTransformBetween(
    vtkIECTransformLogic::FixedReference, vtkIECTransformLogic::RAS);
  fixedReferenceToRASTransform->DeepCopy(fixedReferenceToRASTransformNew);
}
