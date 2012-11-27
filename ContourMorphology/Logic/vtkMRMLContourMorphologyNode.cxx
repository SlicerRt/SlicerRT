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
  this->ContourNodeID = NULL;
  this->SecondaryContourNodeID = NULL;
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
  this->SetContourNodeID(NULL);
  this->SetSecondaryContourNodeID(NULL);
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
    if ( this->ContourNodeID )
      {
      ss << this->ContourNodeID;
      of << indent << " ContourNodeID=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->SecondaryContourNodeID )
      {
      ss << this->SecondaryContourNodeID;
      of << indent << " SecondaryContourNodeID=\"" << ss.str() << "\"";
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

    if (!strcmp(attName, "ContourNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveContourNodeID(ss.str().c_str());
      }
    if (!strcmp(attName, "SecondaryContourNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveSecondaryContourNodeID(ss.str().c_str());
      }
    else if (!strcmp(attName, "OutputContourNodeID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveOutputContourNodeID(ss.str().c_str());
      }
    else if (!strcmp(attName, "Operation")) 
      {
      this->Operation = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "XSize")) 
      {
      this->XSize = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "YSize")) 
      {
      this->YSize = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ZSize")) 
      {
      this->ZSize = 
        (strcmp(attValue,"true") ? false : true);
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

  this->SetAndObserveContourNodeID(node->ContourNodeID);
  this->SetAndObserveSecondaryContourNodeID(node->ContourNodeID);
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

  os << indent << "ContourNodeID:   " << this->ContourNodeID << "\n";
  os << indent << "SecondaryContourNodeID:   " << this->SecondaryContourNodeID << "\n";
  os << indent << "OutputContourNodeID:   " << this->OutputContourNodeID << "\n";
  os << indent << "Operation:   " << (this->Operation) << "\n";
  os << indent << "XSize:   " << (this->XSize) << "\n";
  os << indent << "YSize:   " << (this->YSize) << "\n";
  os << indent << "ZSize:   " << (this->ZSize) << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ContourNodeID && !strcmp(oldID, this->ContourNodeID))
    {
    this->SetAndObserveContourNodeID(newID);
    }
  if (this->SecondaryContourNodeID && !strcmp(oldID, this->SecondaryContourNodeID))
    {
    this->SetAndObserveContourNodeID(newID);
    }
  if (this->OutputContourNodeID && !strcmp(oldID, this->OutputContourNodeID))
    {
    this->SetAndObserveOutputContourNodeID(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourNodeID(const char* id)
{
  if (this->ContourNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->ContourNodeID, this);
    }

  this->SetContourNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ContourNodeID, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveSecondaryContourNodeID(const char* id)
{
  if (this->SecondaryContourNodeID != NULL)
    {
    this->Scene->RemoveReferencedNodeID(this->SecondaryContourNodeID, this);
    }

  this->SetSecondaryContourNodeID(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->SecondaryContourNodeID, this);
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
