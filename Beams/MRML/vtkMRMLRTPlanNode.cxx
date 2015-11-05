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
#include "SlicerRtCommon.h"

// RTPlan includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>

//------------------------------------------------------------------------------
static const char* RTPLAN_DOSEVOLUME_REFERENCE_ROLE = "rtPlanDoseVolumeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTPlanNode);

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode::vtkMRMLRTPlanNode()
{
  this->RTPlanName = NULL;
  this->SetRTPlanName("RTPlan");

  this->RTPlanDoseEngine = vtkMRMLRTPlanNode::Plastimatch;

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

  of << indent << " RTPlanName=\"" << (this->RTPlanName) << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "RTPlanName")) 
      {
      strcpy(this->RTPlanName, attValue);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTPlanNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLRTPlanNode *node = vtkMRMLRTPlanNode::SafeDownCast(anode);

  this->SetRTPlanName(node->RTPlanName);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "RTPlanName:   " << (this->RTPlanName ? this->RTPlanName : "NULL") << "\n";
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
vtkMRMLRTPlanNode* vtkMRMLRTPlanNode::CreateRTPlanNodeInSubjectHierarchy(vtkMRMLSubjectHierarchyNode* parentSHNode, 
                                    const char* nodeName,
                                    vtkMRMLRTPlanNode* rtPlanNode/*=NULL*/)
{
  vtkMRMLScene *scene = parentSHNode->GetScene();
  // Create RTPlan node
  vtkMRMLRTPlanNode* planNode = NULL;
  planNode = rtPlanNode;
  if(planNode == NULL)
  {
    planNode = vtkMRMLRTPlanNode::New();
    std::string planNodeName = nodeName ;
    planNode->SetName(planNodeName.c_str());
    scene->AddNode(planNode);
    planNode->Delete(); // Return ownership to the scene only
  }  

  // Create subject hierarchy node
  vtkMRMLSubjectHierarchyNode *planSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::New();
  // Default level is series
  planSubjectHierarchyNode->SetLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSeries());
  std::string shNodeName = nodeName + vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyNodeNamePostfix();
  planSubjectHierarchyNode->SetName(shNodeName.c_str());
  scene->AddNode(planSubjectHierarchyNode);
  planSubjectHierarchyNode->Delete(); // Return ownership to the scene only

  planSubjectHierarchyNode->SetAssociatedNodeID(planNode->GetID());

  if (parentSHNode)
  {
    planSubjectHierarchyNode->SetParentNodeID(parentSHNode->GetID());
  }

  return planNode;
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
    vtkErrorMacro("GetRTBeamNodes: Invalid input collection!");
    return;
  }
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLSubjectHierarchyNode *shnode = NULL;
  vtkMRMLSubjectHierarchyNode *shparentnode = NULL;

  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
  {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLSubjectHierarchyNode"))
    {
      shnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(mnode);
      shparentnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shnode->GetParentNode());
      vtkMRMLRTPlanNode *pnode = vtkMRMLRTPlanNode::SafeDownCast(shparentnode->GetAssociatedNode());
      if (pnode && pnode == this) 
      {
        vtkMRMLRTBeamNode* bnode = vtkMRMLRTBeamNode::SafeDownCast(shnode->GetAssociatedNode());
        beams->AddItem(bnode);
      }
    }// end if
  }// end for
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLRTPlanNode::GetRTBeamNode(const std::string& beamName)
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLSubjectHierarchyNode *shnode = NULL;
  vtkMRMLSubjectHierarchyNode *shparentnode = NULL;

  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
  {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLSubjectHierarchyNode"))
    {
      shnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(mnode);
      shparentnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shnode->GetParentNode());
      vtkMRMLRTPlanNode *pnode = vtkMRMLRTPlanNode::SafeDownCast(shparentnode->GetAssociatedNode());
      if (pnode && pnode == this) 
      {
        vtkMRMLRTBeamNode* bnode = vtkMRMLRTBeamNode::SafeDownCast(shnode->GetAssociatedNode());
        if (bnode && bnode->BeamNameIs (beamName)) {
          return bnode;
        }
      }
    }// end if
  }// end for
  return NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::AddRTBeamNode(vtkMRMLRTBeamNode *beamnode)
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLSubjectHierarchyNode *shnode = NULL;
  vtkMRMLSubjectHierarchyNode *shrootnode = NULL;

  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLSubjectHierarchyNode"))
      {
      shnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTPlanNode* pnode = vtkMRMLRTPlanNode::SafeDownCast(shnode->GetAssociatedNode());
      if (pnode == this) 
        {
        shrootnode = shnode;
        break;
        }
      }// end if
    }// end for
  
  // Create root subject hierarchy node for the RT Plan, if it has not been created yet
  if (shrootnode == NULL)
  {
    shrootnode = vtkMRMLSubjectHierarchyNode::New();
    shrootnode->AllowMultipleChildrenOn();
    shrootnode->HideFromEditorsOff();
    scene->AddNode(shrootnode);
    shrootnode->SetAssociatedNodeID( this->GetID() );
    shrootnode->Delete();
  }

  // Put the RTBeam node in the hierarchy
  vtkSmartPointer<vtkMRMLSubjectHierarchyNode> RTPlanSubjectHierarchyNode = vtkSmartPointer<vtkMRMLSubjectHierarchyNode>::New();
  RTPlanSubjectHierarchyNode->SetParentNodeID( shrootnode->GetID() );
  RTPlanSubjectHierarchyNode->SetAssociatedNodeID( beamnode->GetID() );
  RTPlanSubjectHierarchyNode->HideFromEditorsOff();
  scene->AddNode(RTPlanSubjectHierarchyNode);
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::RemoveRTBeamNode(vtkMRMLRTBeamNode *beamnode)
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLSubjectHierarchyNode *shnode = NULL;

  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLSubjectHierarchyNode"))
      {
      shnode = vtkMRMLSubjectHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTBeamNode* bnode = vtkMRMLRTBeamNode::SafeDownCast(shnode->GetAssociatedNode());
      if (bnode && bnode == beamnode) 
        {
        // remove all nodes
        scene->RemoveNode(shnode);
        scene->RemoveNode(beamnode);
        }
      }// end if
    }// end for
}
