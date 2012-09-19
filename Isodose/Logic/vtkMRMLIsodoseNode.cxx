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
#include "vtkMRMLIsodoseNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIsodoseNode);

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::vtkMRMLIsodoseNode()
{
  this->IsodoseLevelVector.clear();
  this->DoseVolumeNodeId = NULL;
  this->OutputHierarchyNodeId = NULL;
  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::~vtkMRMLIsodoseNode()
{
  this->IsodoseLevelVector.clear();
  this->SetDoseVolumeNodeId(NULL);
  this->SetOutputHierarchyNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::WriteXML(ostream& of, int nIndent)
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
    of << indent << " IsodoseLevelVector=\"";
    for (std::vector<double>::iterator it = this->IsodoseLevelVector.begin(); it != this->IsodoseLevelVector.end(); ++it)
      {
      of << (*it) << "|";
      }
    of << "\"";
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
void vtkMRMLIsodoseNode::ReadXMLAttributes(const char** atts)
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
    else if (!strcmp(attName, "IsodoseLevelVector")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->IsodoseLevelVector.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        std::string mapPairStr = valueStr.substr(0, separatorPosition);
        size_t colonPosition = mapPairStr.find( ":" );
        if (colonPosition == std::string::npos)
          {
          continue;
          }
        std::string doseLevelName = mapPairStr.substr(0, colonPosition);

        double doseLevelValue;
        std::stringstream vss;
        vss << mapPairStr.substr( colonPosition+1 );
        vss >> doseLevelValue;
        
        double tempLevel;
        tempLevel = doseLevelValue;
        this->IsodoseLevelVector.push_back(tempLevel);
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        std::string mapPairStr = valueStr.substr(0, separatorPosition);
        size_t colonPosition = mapPairStr.find( ":" );
        if (colonPosition != std::string::npos)
          {
          std::string doseLevelName = mapPairStr.substr(0, colonPosition);

          double doseLevelValue;
          std::stringstream vss;
          vss << mapPairStr.substr( colonPosition+1 );
          vss >> doseLevelValue;

          double tempLevel;
          tempLevel = doseLevelValue;
          this->IsodoseLevelVector.push_back(tempLevel);
          }
        }
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
void vtkMRMLIsodoseNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLIsodoseNode *node = (vtkMRMLIsodoseNode *) anode;

  this->SetAndObserveDoseVolumeNodeId(node->DoseVolumeNodeId);
  this->SetAndObserveOutputHierarchyNodeId(node->OutputHierarchyNodeId);
  this->IsodoseLevelVector = node->IsodoseLevelVector;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "DoseVolumeNodeId:   " << this->DoseVolumeNodeId << "\n";
  os << indent << "OutputHierarchyNodeId:   " << this->OutputHierarchyNodeId << "\n";
  //TODO: Add isodose level vector contents
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::UpdateReferenceID(const char *oldID, const char *newID)
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
void vtkMRMLIsodoseNode::SetAndObserveDoseVolumeNodeId(const char* id)
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
void vtkMRMLIsodoseNode::SetAndObserveOutputHierarchyNodeId(const char* id)
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
