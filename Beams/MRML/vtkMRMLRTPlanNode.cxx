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

// SlicerRt includes
#include "vtkMRMLSegmentationNode.h"

// RTPlan includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLMarkupsFiducialNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkDataArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVariant.h>

//------------------------------------------------------------------------------
static const char* RTPLAN_MARKUPS_REFERENCE_ROLE = "rtPlanMarkupsRef";
static const char* RTPLAN_DOSEVOLUME_REFERENCE_ROLE = "rtPlanDoseVolumeRef";
static const char* RTPLAN_REFERENCE_VOLUME_REFERENCE_ROLE = "rtPlanReferenceVolumeRef";
static const char* RTPLAN_SEGMENTATION_REFERENCE_ROLE = "rtPlanSegmentationRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTPlanNode);

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode::vtkMRMLRTPlanNode()
{
  this->NextBeamNumber = 0;

  this->RTPlanDoseEngine = vtkMRMLRTPlanNode::Plastimatch;
  this->RxDose = 1.0;

  this->RTPlanDoseGrid[0] = 0;
  this->RTPlanDoseGrid[1] = 0;
  this->RTPlanDoseGrid[2] = 0;

  this->HideFromEditorsOff();

  // Register parent transform modified event so that the representations
  //   can be put under the same transform node
  //vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  //events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  //vtkObserveMRMLObjectEventsMacro(this, events);
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode::~vtkMRMLRTPlanNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " NextBeamNumber=\"" << (this->NextBeamNumber) << "\"";
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
      //TODO: Great idea to use vtkVariant, should be used everywhere in the code instead of the lengthy stringstream way!
      this->NextBeamNumber = vtkVariant(attValue).ToDouble();
    }
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTPlanNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
    {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene!");
    return;
    }
  if (this->Scene->IsBatchProcessing())
    {
    return;
    }

  // Representation internal data changed
  if (eventID == vtkMRMLModelNode::PolyDataModifiedEvent || eventID == vtkMRMLVolumeNode::ImageDataModifiedEvent)
    {
    vtkMRMLModelNode* callerModelNode = vtkMRMLModelNode::SafeDownCast(caller);
    vtkMRMLScalarVolumeNode* callerVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(caller);
    if (!callerModelNode && !callerVolumeNode)
      {
      return;
      }
      //TODO: Implement or delete
    }
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTPlanNode::GetRTPlanReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetNodeReference(RTPLAN_REFERENCE_VOLUME_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveRTPlanReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(RTPLAN_REFERENCE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLRTPlanNode::GetRTPlanSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(RTPLAN_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveRTPlanSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(RTPLAN_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLRTPlanNode::GetMarkupsFiducialNode()
{
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(this->GetNodeReference(RTPLAN_MARKUPS_REFERENCE_ROLE));
  if (!markupsNode)
  {
    markupsNode = this->CreateMarkupsFiducialNode();
  }
  if (!markupsNode)
  {
    vtkErrorMacro("vtkMRMLRTPlanNode: Could not create Markups node for RTPlan");
    return NULL;
  }

  // Ensure that Markups has an isocenter fiducial at index 0
  if (markupsNode->GetNumberOfFiducials() < 1) {
    markupsNode->AddFiducial(0,0,0,"Isocenter");
  }
  else if (markupsNode->GetNthFiducialLabel(0) != "Isocenter") {
    markupsNode->SetNthFiducialLabel(0,"Isocenter");
    markupsNode->SetNthFiducialPosition(0,0,0,0);
  }

  return markupsNode;
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveMarkupsFiducialNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNodeReferenceID(RTPLAN_MARKUPS_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLRTPlanNode::CreateMarkupsFiducialNode()
{
  // Create name
  std::string markupsName = std::string(this->GetName()) + "_POI";
  
  // Create markups node
  vtkNew<vtkMRMLMarkupsFiducialNode> markupsNode;
  markupsNode->SetName (markupsName.c_str());
  this->GetScene()->AddNode(markupsNode.GetPointer());
  this->SetAndObserveMarkupsFiducialNode(markupsNode.GetPointer());

  // Subject hierarchy node is created automatically
  
  // If plan belongs to a study, set the markups node as belonging to the same study
  vtkMRMLSubjectHierarchyNode* planSHNode = this->GetPlanSubjectHierarchyNode();
  if (!planSHNode)
  {
    vtkErrorMacro("CreateMarkupsFiducialNode: Subject hierarchy node should always exist for RTPlan");
    return markupsNode.GetPointer();
  }
  vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    this->GetScene(), planSHNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), 
    markupsName.c_str(), markupsNode.GetPointer() );

  return markupsNode.GetPointer();
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLRTPlanNode::GetRTPlanDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(RTPLAN_DOSEVOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::SetAndObserveRTPlanDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(RTPLAN_DOSEVOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::GetRTBeamNodes(vtkCollection *beams)
{
  if (!beams)
  {
    vtkErrorMacro("GetRTBeamNodes: Invalid input beam collection!");
    return;
  }
  beams->RemoveAllItems();

  vtkMRMLSubjectHierarchyNode* rtPlanShNode = this->GetPlanSubjectHierarchyNode();
  if (!rtPlanShNode)
  {
    vtkErrorMacro("GetRTBeamNodes: Failed to acctess RT plan subject hierarchy node, although it should always be available!");
    return;
  }

  rtPlanShNode->GetAssociatedChildrenNodes(beams, "vtkMRMLRTBeamNode");
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::GetRTBeamNodes(std::vector<vtkMRMLRTBeamNode*>& beams)
{
  beams.clear();

  // Get unsorted list from hierarchy
  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  this->GetRTBeamNodes(beamCollection);

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
vtkMRMLRTBeamNode* vtkMRMLRTPlanNode::GetRTBeamNodeByName(const std::string& beamName)
{
  vtkMRMLSubjectHierarchyNode* rtPlanShNode = this->GetPlanSubjectHierarchyNode();
  if (!rtPlanShNode)
  {
    vtkErrorMacro("GetRTBeamNodes: Failed to acctess RT plan subject hierarchy node, although it should always be available!");
    return NULL;
  }

  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  rtPlanShNode->GetAssociatedChildrenNodes(beamCollection, "vtkMRMLRTBeamNode");

  beamCollection->InitTraversal();
  for (int i=0; i<beamCollection->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beamCollection->GetItemAsObject(i));
    if (beamNode && beamNode->BeamNameIs(beamName))
    {
      return beamNode;
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLRTPlanNode::GetRTBeamNodeByNumber(int beamNumber)
{
  vtkMRMLSubjectHierarchyNode* rtPlanShNode = this->GetPlanSubjectHierarchyNode();
  if (!rtPlanShNode)
  {
    vtkErrorMacro("GetRTBeamNodeByNumber: Failed to acctess RT plan subject hierarchy node, although it should always be available!");
    return NULL;
  }
  
  vtkSmartPointer<vtkCollection> beamCollection = vtkSmartPointer<vtkCollection>::New();
  rtPlanShNode->GetAssociatedChildrenNodes(beamCollection, "vtkMRMLRTBeamNode");

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
void vtkMRMLRTPlanNode::AddRTBeamNode(vtkMRMLRTBeamNode *beamnode)
{
  vtkMRMLScene *scene = this->GetScene();

  // Get subject hierarchy node for the RT Plan
  vtkMRMLSubjectHierarchyNode* planSHNode = this->GetPlanSubjectHierarchyNode();

  // Set the beam number
  beamnode->SetBeamNumber(this->NextBeamNumber);
  this->NextBeamNumber++;

  // Copy the plan markups node reference into the beam
  beamnode->SetAndObserveIsocenterFiducialNode(
    this->GetMarkupsFiducialNode());

  // Copy the segmentation node reference into the beam
  beamnode->SetAndObserveTargetSegmentationNode(
    this->GetRTPlanSegmentationNode());

  // Put the RTBeam node in the subject hierarchy
  vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode (
    scene, planSHNode, 
    vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), 
    beamnode->GetName(), beamnode);
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::RemoveRTBeamNode(vtkMRMLRTBeamNode *beamNode)
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(beamNode, scene);
  if (!shNode)
  {
    vtkWarningMacro("RemoveRTBeamNodes tried to remove a beam without a SubjectHierarchyNode\n");
    return;
  }
  scene->RemoveNode(shNode);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode* vtkMRMLRTPlanNode::GetPlanSubjectHierarchyNode()
{
  vtkMRMLSubjectHierarchyNode* planSHNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this, this->GetScene());

  // If none found, create new subject hierarchy node for the RT Plan
  if (planSHNode == NULL)
  {
    planSHNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      this->GetScene(), NULL,
      vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries(), this->Name, this );
  }
  if (planSHNode == NULL)
  {
    vtkErrorMacro("vtkMRMLRTPlanNode::GetSHNode: Could not create subject hierarchy node.");
  }

  return planSHNode;
}
