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
#include "vtkMRMLDoseAccumulationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDoseAccumulationNode);

//----------------------------------------------------------------------------
vtkMRMLDoseAccumulationNode::vtkMRMLDoseAccumulationNode()
{
  this->ShowDoseVolumesOnly = true;
  this->SelectedInputVolumeIds.clear();
  this->VolumeNodeIdsToWeightsMap.clear();
  this->ReferenceDoseVolumeNodeId = NULL;
  this->AccumulatedDoseVolumeNodeId = NULL;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseAccumulationNode::~vtkMRMLDoseAccumulationNode()
{
  this->SelectedInputVolumeIds.clear();
  this->VolumeNodeIdsToWeightsMap.clear();
  this->SetReferenceDoseVolumeNodeId(NULL);
  this->SetAccumulatedDoseVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " ShowDoseVolumesOnly=\"" << (this->ShowDoseVolumesOnly ? "true" : "false") << "\"";

  {
    of << indent << " SelectedInputVolumeIds=\"";
    for (std::set<std::string>::iterator it = this->SelectedInputVolumeIds.begin(); it != this->SelectedInputVolumeIds.end(); ++it)
      {
      of << (*it) << "|";
      }
    of << "\"";
  }

  {
    of << indent << " VolumeNodeIdsToWeightsMap=\"";
    for (std::map<std::string,double>::iterator it = this->VolumeNodeIdsToWeightsMap.begin(); it != this->VolumeNodeIdsToWeightsMap.end(); ++it)
      {
      of << it->first << ":" << it->second << "|";
      }
    of << "\"";
  }

  {
    std::stringstream ss;
    if ( this->ReferenceDoseVolumeNodeId )
      {
      ss << this->ReferenceDoseVolumeNodeId;
      of << indent << " ReferenceDoseVolumeNodeId=\"" << ss.str() << "\"";
     }
  }

  {
    std::stringstream ss;
    if ( this->AccumulatedDoseVolumeNodeId )
      {
      ss << this->AccumulatedDoseVolumeNodeId;
      of << indent << " AccumulatedDoseVolumeNodeId=\"" << ss.str() << "\"";
     }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "ShowDoseVolumesOnly")) 
      {
      this->ShowDoseVolumesOnly = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "SelectedInputVolumeIds")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->SelectedInputVolumeIds.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        this->SelectedInputVolumeIds.insert( valueStr.substr(0, separatorPosition) );
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        this->SelectedInputVolumeIds.insert( valueStr );
        }
      }
    else if (!strcmp(attName, "VolumeNodeIdsToWeightsMap")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->VolumeNodeIdsToWeightsMap.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        std::string mapPairStr = valueStr.substr(0, separatorPosition);
        size_t colonPosition = mapPairStr.find( ":" );
        if (colonPosition == std::string::npos)
          {
          continue;
          }
        std::string volumeNodeId = mapPairStr.substr(0, colonPosition);

        double weight;
        std::stringstream vss;
        vss << mapPairStr.substr( colonPosition+1 );
        vss >> weight;

        this->VolumeNodeIdsToWeightsMap[volumeNodeId] = weight;
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        std::string mapPairStr = valueStr.substr(0, separatorPosition);
        size_t colonPosition = mapPairStr.find( ":" );
        if (colonPosition != std::string::npos)
          {
          std::string volumeNodeId = mapPairStr.substr(0, colonPosition);

          double weight;
          std::stringstream vss;
          vss << mapPairStr.substr( colonPosition+1 );
          vss >> weight;

          this->VolumeNodeIdsToWeightsMap[volumeNodeId] = weight;
          }
        }
      }
    else if (!strcmp(attName, "ReferenceDoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceDoseVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "AccumulatedDoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveAccumulatedDoseVolumeNodeId(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDoseAccumulationNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLDoseAccumulationNode *node = (vtkMRMLDoseAccumulationNode *) anode;

  this->SetShowDoseVolumesOnly(node->ShowDoseVolumesOnly);
  this->SetAndObserveReferenceDoseVolumeNodeId(node->ReferenceDoseVolumeNodeId);
  this->SetAndObserveAccumulatedDoseVolumeNodeId(node->AccumulatedDoseVolumeNodeId);

  this->SelectedInputVolumeIds = node->SelectedInputVolumeIds;
  this->VolumeNodeIdsToWeightsMap = node->VolumeNodeIdsToWeightsMap;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ShowDoseVolumesOnly:   " << (this->ShowDoseVolumesOnly ? "true" : "false") << "\n";

  {
    os << indent << "SelectedInputVolumeIds:   ";
    for (std::set<std::string>::iterator it = this->SelectedInputVolumeIds.begin(); it != this->SelectedInputVolumeIds.end(); ++it)
      {
      os << (*it) << "|";
      }
    os << "\n";
  }

  {
    os << indent << "VolumeNodeIdsToWeightsMap:   ";
    for (std::map<std::string,double>::iterator it = this->VolumeNodeIdsToWeightsMap.begin(); it != this->VolumeNodeIdsToWeightsMap.end(); ++it)
      {
      os << it->first << ":" << it->second << "|";
      }
    os << "\n";
  }

  os << indent << "ReferenceDoseVolumeNodeId:   " << (this->ReferenceDoseVolumeNodeId ? this->ReferenceDoseVolumeNodeId : "NULL") << "\n";
  os << indent << "AccumulatedDoseVolumeNodeId:   " << (this->AccumulatedDoseVolumeNodeId ? this->AccumulatedDoseVolumeNodeId : "NULL") << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::SetAndObserveReferenceDoseVolumeNodeId(const char* id)
{
  if (this->ReferenceDoseVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->ReferenceDoseVolumeNodeId, this);
  }

  this->SetReferenceDoseVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->ReferenceDoseVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::SetAndObserveAccumulatedDoseVolumeNodeId(const char* id)
{
  if (this->AccumulatedDoseVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->AccumulatedDoseVolumeNodeId, this);
  }

  this->SetAccumulatedDoseVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->AccumulatedDoseVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->SelectedInputVolumeIds.find(oldID) != this->SelectedInputVolumeIds.end())
    {
    this->SelectedInputVolumeIds.erase(oldID);
    this->SelectedInputVolumeIds.insert(newID);
    }
  std::map<std::string,double>::iterator it;
  if ((it = this->VolumeNodeIdsToWeightsMap.find(oldID)) != this->VolumeNodeIdsToWeightsMap.end())
    {
      double weight = it->second;
      this->VolumeNodeIdsToWeightsMap.erase(oldID);
      this->VolumeNodeIdsToWeightsMap[newID] = weight;
    }
  if (this->ReferenceDoseVolumeNodeId && !strcmp(oldID, this->ReferenceDoseVolumeNodeId))
    {
    this->SetAndObserveReferenceDoseVolumeNodeId(newID);
    }
  if (this->AccumulatedDoseVolumeNodeId && !strcmp(oldID, this->AccumulatedDoseVolumeNodeId))
    {
    this->SetAndObserveAccumulatedDoseVolumeNodeId(newID);
    }
}
