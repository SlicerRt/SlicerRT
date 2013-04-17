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
  this->ContourANodeID = NULL;
  this->ContourBNodeID = NULL;
  this->ReferenceVolumeNodeID = NULL;
  this->OutputContourNodeID = NULL;
  this->Operation = SLICERRT_EXPAND;
  this->XSize = 1;
  this->YSize = 1;
  this->ZSize = 1;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLContourMorphologyNode::~vtkMRMLContourMorphologyNode()
{
  this->SetContourANodeID(NULL);
  this->SetContourBNodeID(NULL);
  this->SetReferenceVolumeNodeID(NULL);
  this->SetOutputContourNodeID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->ContourANodeID )
      {
      ss << this->ContourANodeID;
      of << indent << " ContourANodeID=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->ContourBNodeID )
      {
      ss << this->ContourBNodeID;
      of << indent << " ContourBNodeID=\"" << ss.str() << "\"";
      }
  }

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
    if ( this->OutputContourNodeID )
      {
      ss << this->OutputContourNodeID;
      of << indent << " OutputContourNodeID=\"" << ss.str() << "\"";
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

    if (!strcmp(attName, "ContourANodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveContourANodeID(ss.str().c_str());
      }
    if (!strcmp(attName, "ContourBNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveContourBNodeID(ss.str().c_str());
      }
    if (!strcmp(attName, "ReferenceVolumeNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceVolumeNodeID(ss.str().c_str());
      }
    else if (!strcmp(attName, "OutputContourNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveOutputContourNodeID(ss.str().c_str());
      }
    else if (!strcmp(attName, "Operation")) 
      {
      this->Operation = atoi(attValue);
      }
    else if (!strcmp(attName, "XSize")) 
      {
      this->XSize = atof(attValue);
      }
    else if (!strcmp(attName, "YSize")) 
      {
      this->YSize = atof(attValue);
      }
    else if (!strcmp(attName, "ZSize")) 
      {
      this->ZSize = atof(attValue);
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

  this->SetAndObserveContourANodeID(node->ContourANodeID);
  this->SetAndObserveContourBNodeID(node->ContourBNodeID);
  this->SetAndObserveReferenceVolumeNodeID(node->ReferenceVolumeNodeID);
  this->SetAndObserveOutputContourNodeID(node->OutputContourNodeID);

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

  os << indent << "ContourANodeID:   " << this->ContourANodeID << "\n";
  os << indent << "ContourBNodeID:   " << this->ContourBNodeID << "\n";
  os << indent << "ReferenceVolumeNodeID:   " << this->ReferenceVolumeNodeID << "\n";
  os << indent << "OutputContourNodeID:   " << this->OutputContourNodeID << "\n";
  os << indent << "Operation:   " << (this->Operation) << "\n";
  os << indent << "XSize:   " << (this->XSize) << "\n";
  os << indent << "YSize:   " << (this->YSize) << "\n";
  os << indent << "ZSize:   " << (this->ZSize) << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ContourANodeID && !strcmp(oldID, this->ContourANodeID))
    {
    this->SetAndObserveContourANodeID(newID);
    }
  if (this->ContourBNodeID && !strcmp(oldID, this->ContourBNodeID))
    {
    this->SetAndObserveContourBNodeID(newID);
    }
  if (this->ReferenceVolumeNodeID && !strcmp(oldID, this->ReferenceVolumeNodeID))
    {
    this->SetAndObserveReferenceVolumeNodeID(newID);
    }
  if (this->OutputContourNodeID && !strcmp(oldID, this->OutputContourNodeID))
    {
    this->SetAndObserveOutputContourNodeID(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourANodeID(const char* id)
{
  if (this->ContourANodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ContourANodeID, this);
    }

  this->SetContourANodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ContourANodeID, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourBNodeID(const char* id)
{
  if (this->ContourBNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ContourBNodeID, this);
    }

  this->SetContourBNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ContourBNodeID, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveReferenceVolumeNodeID(const char* id)
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
void vtkMRMLContourMorphologyNode::SetAndObserveOutputContourNodeID(const char* id)
{
  if (this->OutputContourNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->OutputContourNodeID, this);
    }

  this->SetOutputContourNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->OutputContourNodeID, this);
    }
}
