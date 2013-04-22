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
#include "vtkMRMLContourMorphologyNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLContourNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourMorphologyNode);

//----------------------------------------------------------------------------
vtkMRMLContourMorphologyNode::vtkMRMLContourMorphologyNode()
{
  this->ContourANodeId = NULL;
  this->ContourBNodeId = NULL;
  this->ReferenceVolumeNodeId = NULL;
  this->OutputContourNodeId = NULL;
  this->Operation = Expand;
  this->XSize = 1;
  this->YSize = 1;
  this->ZSize = 1;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLContourMorphologyNode::~vtkMRMLContourMorphologyNode()
{
  this->SetContourANodeId(NULL);
  this->SetContourBNodeId(NULL);
  this->SetReferenceVolumeNodeId(NULL);
  this->SetOutputContourNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->ContourANodeId )
      {
      ss << this->ContourANodeId;
      of << indent << " ContourANodeId=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->ContourBNodeId )
      {
      ss << this->ContourBNodeId;
      of << indent << " ContourBNodeId=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->ReferenceVolumeNodeId )
      {
      ss << this->ReferenceVolumeNodeId;
      of << indent << " ReferenceVolumeNodeId=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->OutputContourNodeId )
      {
      ss << this->OutputContourNodeId;
      of << indent << " OutputContourNodeId=\"" << ss.str() << "\"";
      }
  }

  of << indent << " Operation=\"" << (this->Operation) << "\"";

  of << indent << " XSize=\"" << (this->XSize) << "\"";

  of << indent << " YSize=\"" << (this->YSize) << "\"";

  of << indent << " ZSize=\"" << (this->ZSize) << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ContourANodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveContourANodeId(ss.str().c_str());
      }
    if (!strcmp(attName, "ContourBNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveContourBNodeId(ss.str().c_str());
      }
    if (!strcmp(attName, "ReferenceVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "OutputContourNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveOutputContourNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "Operation")) 
      {
      std::stringstream ss;
      ss << attValue;
      int operation;
      ss >> operation;
      this->Operation = (ContourMorphologyOperationType)operation;
      }
    else if (!strcmp(attName, "XSize")) 
      {
      std::stringstream ss;
      ss << attValue;
      double xSize;
      ss >> xSize;
      this->XSize = xSize;
      }
    else if (!strcmp(attName, "YSize")) 
      {
      std::stringstream ss;
      ss << attValue;
      double ySize;
      ss >> ySize;
      this->YSize = ySize;
      }
    else if (!strcmp(attName, "ZSize")) 
      {
      std::stringstream ss;
      ss << attValue;
      double zSize;
      ss >> zSize;
      this->ZSize = zSize;
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLContourMorphologyNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLContourMorphologyNode *node = (vtkMRMLContourMorphologyNode *)anode;

  this->SetAndObserveContourANodeId(node->ContourANodeId);
  this->SetAndObserveContourBNodeId(node->ContourBNodeId);
  this->SetAndObserveReferenceVolumeNodeId(node->ReferenceVolumeNodeId);
  this->SetAndObserveOutputContourNodeId(node->OutputContourNodeId);

  this->Operation = node->Operation;
  this->XSize = node->XSize;
  this->YSize = node->YSize;
  this->ZSize = node->ZSize;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ContourANodeId:   " << this->ContourANodeId << "\n";
  os << indent << "ContourBNodeId:   " << this->ContourBNodeId << "\n";
  os << indent << "ReferenceVolumeNodeId:   " << this->ReferenceVolumeNodeId << "\n";
  os << indent << "OutputContourNodeId:   " << this->OutputContourNodeId << "\n";
  os << indent << "Operation:   " << (this->Operation) << "\n";
  os << indent << "XSize:   " << (this->XSize) << "\n";
  os << indent << "YSize:   " << (this->YSize) << "\n";
  os << indent << "ZSize:   " << (this->ZSize) << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ContourANodeId && !strcmp(oldID, this->ContourANodeId))
    {
    this->SetAndObserveContourANodeId(newID);
    }
  if (this->ContourBNodeId && !strcmp(oldID, this->ContourBNodeId))
    {
    this->SetAndObserveContourBNodeId(newID);
    }
  if (this->ReferenceVolumeNodeId && !strcmp(oldID, this->ReferenceVolumeNodeId))
    {
    this->SetAndObserveReferenceVolumeNodeId(newID);
    }
  if (this->OutputContourNodeId && !strcmp(oldID, this->OutputContourNodeId))
    {
    this->SetAndObserveOutputContourNodeId(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourANodeId(const char* id)
{
  if (this->ContourANodeId != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ContourANodeId, this);
    }

  this->SetContourANodeId(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ContourANodeId, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourBNodeId(const char* id)
{
  if (this->ContourBNodeId != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ContourBNodeId, this);
    }

  this->SetContourBNodeId(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ContourBNodeId, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveReferenceVolumeNodeId(const char* id)
{
  if (this->ReferenceVolumeNodeId != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ReferenceVolumeNodeId, this);
    }

  this->SetReferenceVolumeNodeId(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ReferenceVolumeNodeId, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveOutputContourNodeId(const char* id)
{
  if (this->OutputContourNodeId != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->OutputContourNodeId, this);
    }

  this->SetOutputContourNodeId(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->OutputContourNodeId, this);
    }
}
