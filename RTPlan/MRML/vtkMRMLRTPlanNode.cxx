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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// SlicerRtCommon includes
#include "SlicerRtCommon.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkMRMLContourNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTPlanHierarchyNode.h"
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageResample.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>

// STD includes
#include <algorithm> //TODO: workaround for issue #179

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTPlanNode);

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode::vtkMRMLRTPlanNode()
{
  this->RTPlanName = NULL;
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

  strcpy(this->RTPlanName, node->RTPlanName);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::UpdateReferences()
{
  Superclass::UpdateReferences();

}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);

}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "RTPlanName:   " << (this->RTPlanName) << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (this->Scene == NULL)
    {
    return;
    }

  // Representation internal data changed
  if (eventID == vtkMRMLModelNode::PolyDataModifiedEvent || eventID == vtkMRMLVolumeNode::ImageDataModifiedEvent)
    {
    vtkMRMLModelNode* callerModelNode = vtkMRMLModelNode::SafeDownCast(caller);
    vtkMRMLVolumeNode* callerVolumeNode = vtkMRMLVolumeNode::SafeDownCast(caller);
    if (!callerModelNode && !callerVolumeNode)
      {
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::GetRTBeamNodes(vtkCollection *beams)
{
  if (beams == NULL)
    {
    return;
    }
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phrootnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phparentnode = NULL;
  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLRTPlanHierarchyNode"))
      {
      phnode = vtkMRMLRTPlanHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTPlanNode* pnode = vtkMRMLRTPlanNode::SafeDownCast(this->GetScene()->GetNodeByID(phnode->GetAssociatedNodeID()));
      if (pnode == this) 
        {
        phrootnode = phnode;
        break;
        }
      }// end if
    }// end for

  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLRTPlanHierarchyNode"))
      {
      phnode = vtkMRMLRTPlanHierarchyNode::SafeDownCast(mnode);
      phparentnode = vtkMRMLRTPlanHierarchyNode::SafeDownCast(phnode->GetParentNode());
      if (phparentnode == phrootnode) 
        {
        vtkMRMLRTBeamNode* bnode = vtkMRMLRTBeamNode::SafeDownCast(this->GetScene()->GetNodeByID(phnode->GetAssociatedNodeID()));
        beams->AddItem(bnode);
        }
      }// end if
    }// end for
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::AddRTBeamNode(vtkMRMLRTBeamNode *beamnode)
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phrootnode = NULL;
  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLRTPlanHierarchyNode"))
      {
      phnode = vtkMRMLRTPlanHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTPlanNode* pnode = vtkMRMLRTPlanNode::SafeDownCast(this->GetScene()->GetNodeByID(phnode->GetAssociatedNodeID()));
      if (pnode == this) 
        {
        phrootnode = phnode;
        break;
        }
      }// end if
    }// end for
  
  // Create root contour hierarchy node for the series, if it has not been created yet
  if (phrootnode == NULL)
  {
    phrootnode = vtkMRMLRTPlanHierarchyNode::New();
    //phrootnode->SetName(RTPlanHierarchyRootNodeName.c_str());
    phrootnode->AllowMultipleChildrenOn();
    phrootnode->HideFromEditorsOff();
    scene->AddNode(phrootnode);
    phrootnode->SetAssociatedNodeID( this->GetID() );
    phrootnode->Delete();
  }

  // Put the RTBeam node in the hierarchy
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyNode = vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New();
  //std::string RTPlanHierarchyNode(contourNodeName);
  //RTPlanHierarchyNodeName.append(SlicerRtCommon::DICOMRTIMPORT_PATIENT_HIERARCHY_NODE_NAME_POSTFIX);
  //RTPlanHierarchyNode = this->GetMRMLScene()->GenerateUniqueName(phContourNodeName);
  //RTPlanHierarchyNode->SetName(phContourNodeName.c_str());
  RTPlanHierarchyNode->SetParentNodeID( phrootnode->GetID() );
  RTPlanHierarchyNode->SetAssociatedNodeID( beamnode->GetID() );
  RTPlanHierarchyNode->HideFromEditorsOff();
  scene->AddNode(RTPlanHierarchyNode);
}

//---------------------------------------------------------------------------
void vtkMRMLRTPlanNode::RemoveRTBeamNode(vtkMRMLRTBeamNode *beamnode)
{
  vtkMRMLScene *scene = this->GetScene();
  vtkMRMLNode *mnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phnode = NULL;
  vtkMRMLRTPlanHierarchyNode *phrootnode = NULL;
  for (int n=0; n < scene->GetNumberOfNodes(); n++) 
    {
    mnode = scene->GetNthNode(n);
    if (mnode->IsA("vtkMRMLRTPlanHierarchyNode"))
      {
      phnode = vtkMRMLRTPlanHierarchyNode::SafeDownCast(mnode);
      vtkMRMLRTBeamNode* pnode = vtkMRMLRTBeamNode::SafeDownCast(this->GetScene()->GetNodeByID(phnode->GetAssociatedNodeID()));
      if (pnode == beamnode) 
        {
        // remove all nodes
        scene->RemoveNode(phnode);
        scene->RemoveNode(beamnode);
        }
      }// end if
    }// end for

}
