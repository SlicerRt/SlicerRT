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

// MRMLDoseAccumulation includes
#include "vtkMRMLProtonDoseNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLProtonDoseNode);

//----------------------------------------------------------------------------
vtkMRMLProtonDoseNode::vtkMRMLProtonDoseNode()
{
  this->GantryAngle = 0.f;
  this->CollimatorAngle = 0.f;
  this->DoseVolumeNodeId = NULL;
  this->OutputHierarchyNodeId = NULL;
  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLProtonDoseNode::~vtkMRMLProtonDoseNode()
{
  this->SetDoseVolumeNodeId(NULL);
  this->SetOutputHierarchyNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLProtonDoseNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->DoseVolumeNodeId )
      {
      ss << this->DoseVolumeNodeId;
      of << indent << " DoseVolumeNodeId=\"" << ss.str() << "\"";
     }
  }

  {
    std::stringstream ss;
    if ( this->OutputHierarchyNodeId )
      {
      ss << this->OutputHierarchyNodeId;
      of << indent << " OutputHierarchyNodeId=\"" << ss.str() << "\"";
     }
  }

}

//----------------------------------------------------------------------------
void vtkMRMLProtonDoseNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "DoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveDoseVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "OutputHierarchyNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveOutputHierarchyNodeId(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLProtonDoseNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLProtonDoseNode *node = (vtkMRMLProtonDoseNode *) anode;

  this->SetAndObserveDoseVolumeNodeId(node->DoseVolumeNodeId);
  this->SetAndObserveOutputHierarchyNodeId(node->OutputHierarchyNodeId);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLProtonDoseNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "DoseVolumeNodeId:   " << this->DoseVolumeNodeId << "\n";
  os << indent << "OutputHierarchyNodeId:   " << this->OutputHierarchyNodeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLProtonDoseNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->DoseVolumeNodeId && !strcmp(oldID, this->DoseVolumeNodeId))
    {
    this->SetAndObserveDoseVolumeNodeId(newID);
    }
  if (this->OutputHierarchyNodeId && !strcmp(oldID, this->OutputHierarchyNodeId))
    {
    this->SetAndObserveOutputHierarchyNodeId(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLProtonDoseNode::SetAndObserveDoseVolumeNodeId(const char* id)
{
  if (this->DoseVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->DoseVolumeNodeId, this);
  }

  this->SetDoseVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->DoseVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLProtonDoseNode::SetAndObserveOutputHierarchyNodeId(const char* id)
{
  if (this->DoseVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->OutputHierarchyNodeId, this);
  }

  this->SetOutputHierarchyNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->OutputHierarchyNodeId, this);
  }
}
