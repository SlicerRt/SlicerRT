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
  this->ReferenceContourNodeId = NULL;
  this->CompareContourNodeId = NULL;
  this->RasterizationReferenceVolumeNodeId = NULL;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLContourComparisonNode::~vtkMRMLContourComparisonNode()
{
  this->SetReferenceContourNodeId(NULL);
  this->SetCompareContourNodeId(NULL);
  this->SetRasterizationReferenceVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if ( this->ReferenceContourNodeId )
    {
    of << indent << " ReferenceContourNodeId=\"" << this->ReferenceContourNodeId << "\"";
    }
  if ( this->CompareContourNodeId )
    {
    of << indent << " CompareContourNodeId=\"" << this->CompareContourNodeId << "\"";
    }
  if ( this->RasterizationReferenceVolumeNodeId )
    {
    of << indent << " ReferenceVolumeNodeId=\"" << this->RasterizationReferenceVolumeNodeId << "\"";
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

    if (!strcmp(attName, "ReferenceContourNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceContourNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "CompareContourNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveCompareContourNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "ReferenceVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceVolumeNodeId(ss.str().c_str());
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

  this->SetAndObserveReferenceContourNodeId(node->ReferenceContourNodeId);
  this->SetAndObserveCompareContourNodeId(node->CompareContourNodeId);
  this->SetAndObserveReferenceVolumeNodeId(node->RasterizationReferenceVolumeNodeId);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ReferenceContourNodeId:   " << this->ReferenceContourNodeId << "\n";
  os << indent << "CompareContourNodeId:   " << this->CompareContourNodeId << "\n";
  os << indent << "ReferenceVolumeNodeId:   " << this->RasterizationReferenceVolumeNodeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveReferenceContourNodeId(const char* id)
{
  if (this->ReferenceContourNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->ReferenceContourNodeId, this);
  }

  this->SetReferenceContourNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->ReferenceContourNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveCompareContourNodeId(const char* id)
{
  if (this->CompareContourNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->CompareContourNodeId, this);
  }

  this->SetCompareContourNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->CompareContourNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::SetAndObserveReferenceVolumeNodeId(const char* id)
{
  if (this->RasterizationReferenceVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->RasterizationReferenceVolumeNodeId, this);
  }

  this->SetRasterizationReferenceVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->RasterizationReferenceVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLContourComparisonNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ReferenceContourNodeId && !strcmp(oldID, this->ReferenceContourNodeId))
    {
    this->SetAndObserveReferenceContourNodeId(newID);
    }
  if (this->CompareContourNodeId && !strcmp(oldID, this->CompareContourNodeId))
    {
    this->SetAndObserveCompareContourNodeId(newID);
    }
  if (this->RasterizationReferenceVolumeNodeId && !strcmp(oldID, this->RasterizationReferenceVolumeNodeId))
    {
    this->SetAndObserveReferenceVolumeNodeId(newID);
    }
}
