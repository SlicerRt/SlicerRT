/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// Beams includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// SlicerRt includes
#include "PlmCommon.h"

// Plastimatch includes
#include "image_center.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkSlicerSegmentationsModuleLogic.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkDataArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVariant.h>

//------------------------------------------------------------------------------
const char* vtkMRMLRTPlanNode::ISOCENTER_FIDUCIAL_NAME = "Isocenter";
const int vtkMRMLRTPlanNode::ISOCENTER_FIDUCIAL_INDEX = 0;

//------------------------------------------------------------------------------
static const char* REFERENCE_VOLUME_REFERENCE_ROLE = "referenceVolumeRef";
static const char* SEGMENTATION_REFERENCE_ROLE = "segmentationRef";
static const char* POIS_MARKUPS_REFERENCE_ROLE = "posMarkupsRef";
static const char* OUTPUT_TOTAL_DOSE_VOLUME_REFERENCE_ROLE = "outputTotalDoseVolumeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTPlanNode);

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode::vtkMRMLRTPlanNode()
{
  this->RxDose = 1.0;

  this->TargetSegmentID = NULL;

  this->IsocenterSpecification = vtkMRMLRTPlanNode::CenterOfTarget;

  this->NextBeamNumber = 1;

  this->DoseEngineName = NULL;

  this->DoseGrid[0] = 0;
  this->DoseGrid[1] = 0;
  this->DoseGrid[2] = 0;
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode::~vtkMRMLRTPlanNode()
{
  this->SetTargetSegmentID(NULL);
  this->SetDoseEngineName(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  if (this->TargetSegmentID != NULL) 
  {
    of << " TargetSegmentID=\"" << this->TargetSegmentID << "\"";
  }
  if (this->DoseEngineName != NULL) 
  {
    of << " DoseEngineName=\"" << this->DoseEngineName << "\"";
  }

  of << " NextBeamNumber=\"" << (this->NextBeamNumber) << "\"";

  of << " RxDose=\"" << (this->RxDose) << "\"";

  of << " IsocenterSpecification=\"" << (int)(this->IsocenterSpecification) << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "NextBeamNumber")) 
    {
      this->NextBeamNumber = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "TargetSegmentID")) 
    {
      this->SetTargetSegmentID(attValue);
    }
    else if (!strcmp(attName, "DoseEngineName")) 
    {
      this->SetDoseEngineName(attValue);
    }
    else if (!strcmp(attName, "RxDose")) 
    {
      this->RxDose = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "IsocenterSpecification")) 
    {
      this->IsocenterSpecification = (IsocenterSpecificationType)(vtkVariant(attValue).ToInt());
    }
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTPlanNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);

  vtkMRMLRTPlanNode* node = vtkMRMLRTPlanNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  this->DisableModifiedEventOn();

  this->SetTargetSegmentID(node->TargetSegmentID);
  this->SetDoseEngineName(node->DoseEngineName);

  this->SetIsocenterSpecification(node->GetIsocenterSpecification());

  this->NextBeamNumber = node->NextBeamNumber;

  // Copy beams
  this->RemoveAllBeams();

  std::vector<vtkMRMLRTBeamNode*> beams;
  this->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    vtkSmartPointer<vtkMRMLRTBeamNode> beamNodeCopy = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
    this->GetScene()->AddNode(beamNodeCopy);
    beamNodeCopy->Copy(beamNode);
  }

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " TargetSegmentID:   " << (this->TargetSegmentID?this->TargetSegmentID:"NULL") << "\n";
  os << indent << " DoseEngineName:   " << (this->DoseEngineName?this->DoseEngineName:"NULL") << "\n";
  os << indent << " NextBeamNumber:   " << this->NextBeamNumber << "\n";
  os << indent << " RxDose:   " << this->RxDose << "\n";
  os << indent << " IsocenterSpecification:   " << (int)(this->IsocenterSpecification) << "\n";

  // Beams
  std::vector<vtkMRMLRTBeamNode*> beams;
  this->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    beamNode->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetDoseEngineName(const char* engineName)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting DoseEngineName to " << (engineName?engineName:"(null)") );
  if ( this->DoseEngineName == NULL && engineName == NULL) { return;}
  if ( this->DoseEngineName && engineName && (!strcmp(this->DoseEngineName,engineName))) { return;}

  // Set dose engine name
  delete [] this->DoseEngineName;
  if (engineName)
  {
    size_t n = strlen(engineName) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (engineName);
    this->DoseEngineName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
  }
  else
  {
    this->DoseEngineName = NULL;
  }

  // Invoke events
  this->Modified();
  this->InvokeEvent(vtkMRMLRTPlanNode::DoseEngineChanged, this);
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
  {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene");
    return;
  }
  if (this->Scene->IsBatchProcessing())
  {
    return;
  }

  if ( eventID == vtkMRMLMarkupsNode::PointModifiedEvent
    && caller == this->GetPoisMarkupsFiducialNode() )
  {
    // Update the model
    this->InvokeCustomModifiedEvent(vtkMRMLRTPlanNode::IsocenterModifiedEvent);
  }
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTPlanNode::GetReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast(this->GetNodeReference(REFERENCE_VOLUME_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(REFERENCE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLRTPlanNode::GetSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLRTPlanNode::GetPoisMarkupsFiducialNode()
{
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(this->GetNodeReference(POIS_MARKUPS_REFERENCE_ROLE));
  if (!markupsNode)
  {
    markupsNode = this->CreateMarkupsFiducialNode();
  }

  if (!markupsNode)
  {
    vtkErrorMacro("GetPoisMarkupsFiducialNode: Could not create Markups node for RTPlan");
  }
  else
  {
    this->IsPoisMarkupsFiducialNodeValid();
  }

  return markupsNode;
}

//----------------------------------------------------------------------------
bool vtkMRMLRTPlanNode::IsPoisMarkupsFiducialNodeValid()
{
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(this->GetNodeReference(POIS_MARKUPS_REFERENCE_ROLE));
  if (!markupsNode)
  {
    return false;
  }

  // Check if markups node contains default fiducials
  if (markupsNode->GetNthFiducialLabel(ISOCENTER_FIDUCIAL_INDEX) != std::string(ISOCENTER_FIDUCIAL_NAME))
  {
    vtkErrorMacro("IsPoisMarkupsFiducialNodeValid: Unable to access isocenter fiducial in the markups node belonging to plan " << this->Name);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObservePoisMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNodeReferenceID(POIS_MARKUPS_REFERENCE_ROLE, (node ? node->GetID() : NULL));

  if (node)
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLMarkupsNode::PointModifiedEvent);
    vtkObserveMRMLObjectEventsMacro(node, events);

    this->IsPoisMarkupsFiducialNodeValid();
  }
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLRTPlanNode::CreateMarkupsFiducialNode()
{
  // Create name
  std::string markupsName = std::string(this->GetName()) + "_POI";
  
  // Create markups node (subject hierarchy node is created automatically)
  vtkSmartPointer<vtkMRMLMarkupsFiducialNode> markupsNode = vtkSmartPointer<vtkMRMLMarkupsFiducialNode>::New();
  markupsNode->SetName(markupsName.c_str());

  // Populate POI markups with default fiducials
  markupsNode->AddFiducial(0,0,0,ISOCENTER_FIDUCIAL_NAME); // index 0: ISOCENTER_FIDUCIAL_INDEX

  // Lock isocenter fiducial based on isocenter specification setting
  if (this->IsocenterSpecification == vtkMRMLRTPlanNode::CenterOfTarget)
  {
    markupsNode->SetNthMarkupLocked(ISOCENTER_FIDUCIAL_INDEX, true);
  }

  this->GetScene()->AddNode(markupsNode);
  this->SetAndObservePoisMarkupsFiducialNode(markupsNode);

  // If plan belongs to a study, set the markups node as belonging to the same study
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("CreateMarkupsFiducialNode: Failed to access subject hierarchy node");
    return markupsNode.GetPointer();
  }
  vtkIdType planShItemID = this->GetPlanSubjectHierarchyItemID();
  if (!planShItemID)
  {
    vtkErrorMacro("CreateMarkupsFiducialNode: Subject hierarchy item should always exist for RTPlan");
    return markupsNode.GetPointer();
  }
  shNode->CreateItem(planShItemID, markupsNode);

  return markupsNode.GetPointer();
}

//----------------------------------------------------------------------------
bool vtkMRMLRTPlanNode::GetIsocenterPosition(double isocenter[3])
{
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetPoisMarkupsFiducialNode();
  if (!fiducialNode)
  {
    vtkErrorMacro("GetIsocenterPosition: Unable to access fiducial node for plan " << this->Name);
    return false;
  }

  fiducialNode->GetNthFiducialPosition(ISOCENTER_FIDUCIAL_INDEX, isocenter);
  return true;
}

//----------------------------------------------------------------------------
bool vtkMRMLRTPlanNode::SetIsocenterPosition(double isocenter[3])
{
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetPoisMarkupsFiducialNode();
  if (!fiducialNode)
  {
    vtkErrorMacro("SetIsocenterPosition: Unable to access fiducial node for plan " << this->Name);
    return false;
  }

  fiducialNode->SetNthFiducialPositionFromArray(ISOCENTER_FIDUCIAL_INDEX, isocenter);

  return true;
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTPlanNode::GetOutputTotalDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(OUTPUT_TOTAL_DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveOutputTotalDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(OUTPUT_TOTAL_DOSE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::GetBeams(vtkCollection *beams)
{
  if (!beams)
  {
    vtkErrorMacro("GetBeams: Invalid input beam collection");
    return;
  }
  beams->RemoveAllItems();

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("GetBeams: Failed to access subject hierarchy node");
    return;
  }

  vtkIdType planShItemID = this->GetPlanSubjectHierarchyItemID();
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkErrorMacro("GetBeams: Failed to access RT plan subject hierarchy item, although it should always be available");
    return;
  }

  shNode->GetDataNodesInBranch(planShItemID, beams, "vtkMRMLRTBeamNode");
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::GetBeams(std::vector<vtkMRMLRTBeamNode*>& beams)
{
  beams.clear();

  // Get unsorted list from hierarchy
  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  this->GetBeams(beamCollection);

  // Insertion sort puts them into vector sorted by beam number
  beamCollection->InitTraversal();
  for (int i=0; i<beamCollection->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beamCollection->GetItemAsObject(i));
    int newBeamNumber = beamNode->GetBeamNumber();
    std::vector<vtkMRMLRTBeamNode*>::iterator it = beams.begin();
    while (it != beams.end())
    {
      int thisBeamNumber = (*it)->GetBeamNumber();
      if (thisBeamNumber > newBeamNumber)
      {
        break;
      }
      ++it;
    }
    beams.insert(it, beamNode);
  }
}

//---------------------------------------------------------------------------
int vtkMRMLRTPlanNode::GetNumberOfBeams()
{
  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  this->GetBeams(beamCollection);
  return beamCollection->GetNumberOfItems();
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLRTPlanNode::GetBeamByName(const std::string& beamName)
{
  vtkIdType planShItemID = this->GetPlanSubjectHierarchyItemID();
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkErrorMacro("GetBeamByName: Failed to access RT plan subject hierarchy item, although it should always be available");
    return NULL;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("GetBeamByName: Failed to access subject hierarchy node");
    return NULL;
  }

  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  shNode->GetDataNodesInBranch(planShItemID, beamCollection, "vtkMRMLRTBeamNode");

  beamCollection->InitTraversal();
  for (int i=0; i<beamCollection->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beamCollection->GetItemAsObject(i));
    if ( beamNode && beamNode->GetName()
      && !strcmp(beamNode->GetName(), beamName.c_str()) )
    {
      return beamNode;
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLRTPlanNode::GetBeamByNumber(int beamNumber)
{
  vtkIdType planShItemID = this->GetPlanSubjectHierarchyItemID();
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkErrorMacro("GetBeamByName: Failed to access RT plan subject hierarchy item, although it should always be available");
    return NULL;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("GetBeamByName: Failed to access subject hierarchy node");
    return NULL;
  }
  
  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  shNode->GetDataNodesInBranch(planShItemID, beamCollection, "vtkMRMLRTBeamNode");

  beamCollection->InitTraversal();
  for (int i=0; i<beamCollection->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beamCollection->GetItemAsObject(i));
    if (beamNode && beamNode->GetBeamNumber() == beamNumber)
    {
      return beamNode;
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
std::string vtkMRMLRTPlanNode::GenerateNewBeamName()
{
  std::stringstream newBeamNameStream;
  newBeamNameStream << vtkMRMLRTBeamNode::NEW_BEAM_NODE_NAME_PREFIX << this->NextBeamNumber;
  return newBeamNameStream.str();
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::AddBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!this->GetScene())
  {
    vtkErrorMacro("AddBeam: Invalid MRML scene");
    return;
  }
  if (!beamNode)
  {
    return;
  }

  // Get subject hierarchy item for the RT Plan
  vtkIdType planShItemID = this->GetPlanSubjectHierarchyItemID();
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkErrorMacro("AddBeam: Failed to access RT plan subject hierarchy item, although it should always be available");
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(beamNode->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("AddBeam: Failed to access subject hierarchy node");
    return;
  }

  // Set the beam number
  beamNode->SetBeamNumber(this->NextBeamNumber++);

  // Add beam node in the right subject hierarchy branch
  shNode->CreateItem(planShItemID, beamNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries());

  // Calculate transform from beam parameters and isocenter from plan
  beamNode->UpdateTransform();
  // Make sure display is set up
  beamNode->UpdateGeometry();

  // Fire beam added event
  this->InvokeEvent(vtkMRMLRTPlanNode::BeamAdded, (void*)beamNode->GetID());

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::RemoveBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!this->GetScene())
  {
    vtkErrorMacro("RemoveBeam: Invalid MRML scene");
    return;
  }

  // Fire beam added event (do it first so that operations can be performed with beam while exists)
  this->InvokeEvent(vtkMRMLRTPlanNode::BeamRemoved, (void*)beamNode->GetID());

  // Remove beam node from the scene. The subject hierarchy item will automatically be removed
  this->GetScene()->RemoveNode(beamNode);

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::RemoveAllBeams()
{
  this->DisableModifiedEventOn();

  std::vector<vtkMRMLRTBeamNode*> beams;
  this->GetBeams(beams);
  for (std::vector<vtkMRMLRTBeamNode*>::iterator beamIt = beams.begin(); beamIt != beams.end(); ++beamIt)
  {
    vtkMRMLRTBeamNode* beamNode = (*beamIt);
    this->RemoveBeam(beamNode);
  }

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//---------------------------------------------------------------------------
vtkIdType vtkMRMLRTPlanNode::GetPlanSubjectHierarchyItemID()
{
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetScene());
  if (!shNode)
  {
    vtkErrorMacro("GetPlanSubjectHierarchyItemID: Failed to access subject hierarchy node");
    return vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  }

  vtkIdType planShItemID = shNode->GetItemByDataNode(this);

  // If none found, create new subject hierarchy item for the RT Plan
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID && this->GetScene())
  {
    planShItemID = shNode->CreateItem(shNode->GetSceneItemID(), this);
  }
  if (planShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkErrorMacro("GetPlanSubjectHierarchyItemID: Could not create subject hierarchy item");
  }

  return planShItemID;
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetIsocenterSpecification(vtkMRMLRTPlanNode::IsocenterSpecificationType isoSpec)
{
  if (isoSpec == this->GetIsocenterSpecification())
  {
    return;
  }

  // Get POIs markups node
  vtkMRMLMarkupsFiducialNode* fiducialNode = this->GetPoisMarkupsFiducialNode();
  if (!fiducialNode)
  {
    vtkErrorMacro("SetIsocenterSpecification: Unable to access fiducial node for plan " << this->Name);
    return;
  }

  this->IsocenterSpecification = isoSpec;

  // Move isocenter to center of target if specified (and not disabled by the second parameter)
  if (isoSpec == vtkMRMLRTPlanNode::CenterOfTarget)
  {
    // Calculate target center and move isocenter to that point
    this->SetIsocenterToTargetCenter();

    // Lock isocenter fiducial so that user cannot move it
    fiducialNode->SetNthMarkupLocked(ISOCENTER_FIDUCIAL_INDEX, true);
  }
  else // ArbitraryPoint
  {
    // Unlock isocenter fiducial so that user can move it
    fiducialNode->SetNthMarkupLocked(ISOCENTER_FIDUCIAL_INDEX, false);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetIsocenterToTargetCenter()
{
  if (!this->TargetSegmentID)
  {
    vtkErrorMacro("SetIsocenterToTargetCenter: Unable to set isocenter to target segment as no target segment defined in beam " << this->Name);
    return;
  }

  // Compute center of target segment
  double center[3] = {0.0,0.0,0.0};
  if (!this->ComputeTargetVolumeCenter(center))
  {
    vtkErrorMacro("SetIsocenterToTargetCenter: Failed to compute target volume center");
    return;
  }

  // Set isocenter in parent plan
  if (!this->SetIsocenterPosition(center))
  {
    vtkErrorMacro("SetIsocenterToTargetCenter: Failed to set plan isocenter");
    return;
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkOrientedImageData> vtkMRMLRTPlanNode::GetTargetOrientedImageData()
{
  vtkSmartPointer<vtkOrientedImageData> targetOrientedImageData;
  vtkMRMLSegmentationNode* segmentationNode = this->GetSegmentationNode();
  if (!segmentationNode)
  {
    vtkErrorMacro("GetTargetOrientedImageData: Failed to get target segmentation node");
    return targetOrientedImageData;
  }

  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
  {
    vtkErrorMacro("GetTargetOrientedImageData: Failed to get segmentation");
    return targetOrientedImageData;
  }

  if (!this->TargetSegmentID)
  {
    vtkErrorMacro("GetTargetOrientedImageData: No target segment specified");
    return targetOrientedImageData;
  }
  vtkSegment* segment = segmentation->GetSegment(this->TargetSegmentID);
  if (!segment) 
  {
    vtkErrorMacro("GetTargetOrientedImageData: Failed to get segment");
    return targetOrientedImageData;
  }

  if (segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
  {
    targetOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::New();
    targetOrientedImageData->DeepCopy( vtkOrientedImageData::SafeDownCast(
        segment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
  }
  else
  {
    // Need to convert
    targetOrientedImageData = vtkSmartPointer<vtkOrientedImageData>::Take( vtkOrientedImageData::SafeDownCast(
      vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment(
        segmentation, this->GetTargetSegmentID(), vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) ) );
    if (!targetOrientedImageData.GetPointer())
    {
      std::string errorMessage("Failed to convert target segment into binary labelmap");
      vtkErrorMacro("GetTargetOrientedImageData: " << errorMessage);
      return targetOrientedImageData;
    }
  }

  // Apply parent transformation nodes if necessary
  if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(segmentationNode, targetOrientedImageData))
  {
    std::string errorMessage("Failed to apply parent transformation to target segment");
    vtkErrorMacro("GetTargetOrientedImageData: " << errorMessage);
    return targetOrientedImageData;
  }
  return targetOrientedImageData;
}

//----------------------------------------------------------------------------
bool vtkMRMLRTPlanNode::ComputeTargetVolumeCenter(double* center)
{
  if (!this->Scene)
  {
    vtkErrorMacro("ComputeTargetVolumeCenter: Invalid MRML scene");
    return false;
  }

  // Get a labelmap for the target
  vtkSmartPointer<vtkOrientedImageData> targetLabelmap = this->GetTargetOrientedImageData();
  if (targetLabelmap.GetPointer() == NULL)
  {
    return false;
  }

  // Convert inputs to plm image
  Plm_image::Pointer targetPlmVolume 
    = PlmCommon::ConvertVtkOrientedImageDataToPlmImage(targetLabelmap);
  if (!targetPlmVolume)
  {
    std::string errorMessage("Failed to convert reference segment labelmap into Plm_image");
    vtkErrorMacro("ComputeTargetVolumeCenter: " << errorMessage);
    return false;
  }

  // Compute image center
  Image_center imageCenter;
  imageCenter.set_image(targetPlmVolume);
  imageCenter.run();
  itk::Vector<double,3> centerOfMass = imageCenter.get_image_center_of_mass();

  // Copy to output argument, and convert LPS -> RAS
  center[0] = -centerOfMass[0];
  center[1] = -centerOfMass[1];
  center[2] =  centerOfMass[2];

  return true;
}
