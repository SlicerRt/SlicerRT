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
#include "vtkMRMLRTPlanModuleNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTPlanModuleNode);

//----------------------------------------------------------------------------
vtkMRMLRTPlanModuleNode::vtkMRMLRTPlanModuleNode()
{
  this->ReferenceVolumeNodeID = NULL;
  this->RTPlanNodeID = NULL;
  this->ISOCenterNodeID = NULL;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanModuleNode::~vtkMRMLRTPlanModuleNode()
{
  this->SetReferenceVolumeNodeID(NULL);
  this->SetRTPlanNodeID(NULL);
  this->SetISOCenterNodeID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanModuleNode::WriteXML(ostream& of, int nIndent)
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
    if ( this->ISOCenterNodeID )
      {
      ss << this->ISOCenterNodeID;
      of << indent << " ISOCenterNodeID=\"" << ss.str() << "\"";
      }
  }

}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanModuleNode::ReadXMLAttributes(const char** atts)
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
    else if (!strcmp(attName, "ISOCenterNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveISOCenterNodeID(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTPlanModuleNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLRTPlanModuleNode *node = (vtkMRMLRTPlanModuleNode *)anode;

  this->SetAndObserveReferenceVolumeNodeID(node->ReferenceVolumeNodeID);
  this->SetAndObserveRTPlanNodeID(node->RTPlanNodeID);
  this->SetAndObserveISOCenterNodeID(node->RTPlanNodeID);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanModuleNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ReferenceVolumeNodeID:   " << this->ReferenceVolumeNodeID << "\n";
  os << indent << "RTPlanNodeID:   " << this->RTPlanNodeID << "\n";
  os << indent << "ISOCenterNodeID:   " << this->ISOCenterNodeID << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanModuleNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ReferenceVolumeNodeID && !strcmp(oldID, this->ReferenceVolumeNodeID))
    {
    this->SetAndObserveReferenceVolumeNodeID(newID);
    }
  if (this->RTPlanNodeID && !strcmp(oldID, this->RTPlanNodeID))
    {
    this->SetAndObserveRTPlanNodeID(newID);
    }
  if (this->ISOCenterNodeID && !strcmp(oldID, this->ISOCenterNodeID))
    {
    this->SetAndObserveISOCenterNodeID(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLRTPlanModuleNode::SetAndObserveReferenceVolumeNodeID(const char* id)
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
void vtkMRMLRTPlanModuleNode::SetAndObserveRTPlanNodeID(const char* id)
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
void vtkMRMLRTPlanModuleNode::SetAndObserveISOCenterNodeID(const char* id)
{
  if (this->ISOCenterNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ISOCenterNodeID, this);
    }

  this->SetISOCenterNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ISOCenterNodeID, this);
    }
}

