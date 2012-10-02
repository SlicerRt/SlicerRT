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
#include "vtkMRMLContourComparisonNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLContourComparisonNode::vtkMRMLContourComparisonNode()
{
  this->ReferenceContourLabelmapVolumeNodeId = NULL;
  this->CompareContourLabelmapVolumeNodeId = NULL;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLContourComparisonNode::~vtkMRMLContourComparisonNode()
{
  this->SetReferenceContourLabelmapVolumeNodeId(NULL);
  this->SetCompareContourLabelmapVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->ReferenceContourLabelmapVolumeNodeId )
      {
      ss << this->ReferenceContourLabelmapVolumeNodeId;
      of << indent << " ReferenceContourLabelmapVolumeNodeId=\"" << ss.str() << "\"";
     }
  }

  {
    std::stringstream ss;
    if ( this->CompareContourLabelmapVolumeNodeId )
      {
      ss << this->CompareContourLabelmapVolumeNodeId;
      of << indent << " CompareContourLabelmapVolumeNodeId=\"" << ss.str() << "\"";
     }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ReferenceContourLabelmapVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceContourLabelmapVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "CompareContourLabelmapVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveCompareContourLabelmapVolumeNodeId(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLContourComparisonNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLContourComparisonNode *node = (vtkMRMLContourComparisonNode *) anode;

  this->SetAndObserveReferenceContourLabelmapVolumeNodeId(node->ReferenceContourLabelmapVolumeNodeId);
  this->SetAndObserveCompareContourLabelmapVolumeNodeId(node->CompareContourLabelmapVolumeNodeId);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ReferenceContourLabelmapVolumeNodeId:   " << this->ReferenceContourLabelmapVolumeNodeId << "\n";
  os << indent << "CompareContourLabelmapVolumeNodeId:   " << this->CompareContourLabelmapVolumeNodeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveReferenceContourLabelmapVolumeNodeId(const char* id)
{
  if (this->ReferenceContourLabelmapVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->ReferenceContourLabelmapVolumeNodeId, this);
  }

  this->SetReferenceContourLabelmapVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->ReferenceContourLabelmapVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveCompareContourLabelmapVolumeNodeId(const char* id)
{
  if (this->CompareContourLabelmapVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->CompareContourLabelmapVolumeNodeId, this);
  }

  this->SetCompareContourLabelmapVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->CompareContourLabelmapVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ReferenceContourLabelmapVolumeNodeId && !strcmp(oldID, this->ReferenceContourLabelmapVolumeNodeId))
    {
    this->SetAndObserveReferenceContourLabelmapVolumeNodeId(newID);
    }
  if (this->CompareContourLabelmapVolumeNodeId && !strcmp(oldID, this->CompareContourLabelmapVolumeNodeId))
    {
    this->SetAndObserveCompareContourLabelmapVolumeNodeId(newID);
    }
}
