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

// MRMLDoseAccumulation includes
#include "vtkMRMLExternalBeamPlanningNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLExternalBeamPlanningNode);

//----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode::vtkMRMLExternalBeamPlanningNode()
{
  this->ReferenceVolumeNodeID = NULL;
  this->RTPlanNodeID = NULL;
  this->IsocenterNodeID = NULL;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode::~vtkMRMLExternalBeamPlanningNode()
{
  this->SetReferenceVolumeNodeID(NULL);
  this->SetRTPlanNodeID(NULL);
  this->SetIsocenterNodeID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->ReferenceVolumeNodeID )
      {
      ss << this->ReferenceVolumeNodeID;
      of << indent << " ReferenceVolumeNodeID=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->RTPlanNodeID )
      {
      ss << this->RTPlanNodeID;
      of << indent << " RTPlanNodeID=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->IsocenterNodeID )
      {
      ss << this->IsocenterNodeID;
      of << indent << " IsocenterNodeID=\"" << ss.str() << "\"";
      }
  }

}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ReferenceVolumeNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceVolumeNodeID(ss.str().c_str());
      }
    else if (!strcmp(attName, "RTPlanNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveRTPlanNodeID(ss.str().c_str());
      }
    else if (!strcmp(attName, "IsocenterNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveIsocenterNodeID(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLExternalBeamPlanningNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLExternalBeamPlanningNode *node = (vtkMRMLExternalBeamPlanningNode *)anode;

  this->SetAndObserveReferenceVolumeNodeID(node->ReferenceVolumeNodeID);
  this->SetAndObserveRTPlanNodeID(node->RTPlanNodeID);
  this->SetAndObserveIsocenterNodeID(node->RTPlanNodeID);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ReferenceVolumeNodeID:   " << this->ReferenceVolumeNodeID << "\n";
  os << indent << "RTPlanNodeID:   " << this->RTPlanNodeID << "\n";
  os << indent << "IsocenterNodeID:   " << this->IsocenterNodeID << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ReferenceVolumeNodeID && !strcmp(oldID, this->ReferenceVolumeNodeID))
    {
    this->SetAndObserveReferenceVolumeNodeID(newID);
    }
  if (this->RTPlanNodeID && !strcmp(oldID, this->RTPlanNodeID))
    {
    this->SetAndObserveRTPlanNodeID(newID);
    }
  if (this->IsocenterNodeID && !strcmp(oldID, this->IsocenterNodeID))
    {
    this->SetAndObserveIsocenterNodeID(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveReferenceVolumeNodeID(const char* id)
{
  if (this->ReferenceVolumeNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ReferenceVolumeNodeID, this);
    }

  this->SetReferenceVolumeNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ReferenceVolumeNodeID, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveRTPlanNodeID(const char* id)
{
  if (this->RTPlanNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->RTPlanNodeID, this);
    }

  this->SetRTPlanNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->RTPlanNodeID, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveIsocenterNodeID(const char* id)
{
  if (this->IsocenterNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->IsocenterNodeID, this);
    }

  this->SetIsocenterNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->IsocenterNodeID, this);
    }
}

