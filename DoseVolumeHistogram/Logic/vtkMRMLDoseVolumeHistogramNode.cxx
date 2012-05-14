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
#include "vtkMRMLDoseVolumeHistogramNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDoseVolumeHistogramNode);

//----------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode::vtkMRMLDoseVolumeHistogramNode()
{
  this->ShowDoseVolumesOnly = true;
  this->SelectedInputVolumeIds.clear();
  this->VolumeNodeIdsToWeightsMap.clear();
  this->AccumulatedDoseVolumeNodeId = NULL;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode::~vtkMRMLDoseVolumeHistogramNode()
{
  this->SelectedInputVolumeIds.clear();
  this->VolumeNodeIdsToWeightsMap.clear();
  this->SetAccumulatedDoseVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::WriteXML(ostream& of, int nIndent)
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
    if ( this->AccumulatedDoseVolumeNodeId )
      {
      ss << this->AccumulatedDoseVolumeNodeId;
      of << indent << " AccumulatedDoseVolumeNodeId=\"" << ss.str() << "\"";
     }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::ReadXMLAttributes(const char** atts)
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
      if (!strcmp(attValue,"true")) 
        {
        this->ShowDoseVolumesOnly = true;
        }
      else
        {
        this->ShowDoseVolumesOnly = false;
        }
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
    else if (!strcmp(attName, "AccumulatedDoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAccumulatedDoseVolumeNodeId(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDoseVolumeHistogramNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLDoseVolumeHistogramNode *node = (vtkMRMLDoseVolumeHistogramNode *) anode;

  this->SetShowDoseVolumesOnly(node->ShowDoseVolumesOnly);
  this->SetAccumulatedDoseVolumeNodeId(node->AccumulatedDoseVolumeNodeId);

  this->SelectedInputVolumeIds = node->SelectedInputVolumeIds;
  this->VolumeNodeIdsToWeightsMap = node->VolumeNodeIdsToWeightsMap;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ShowDoseVolumesOnly:   " << (this->ShowDoseVolumesOnly ? "true" : "false") << "\n";

  {
    os << indent << "SelectedInputVolumeIds:   ";
    for (std::set<std::string>::iterator it = this->SelectedInputVolumeIds.begin(); it != this->SelectedInputVolumeIds.end(); ++it)
      {
      os << indent << (*it) << "|";
      }
    os << "\n";
  }

  {
    os << indent << "VolumeNodeIdsToWeightsMap:   ";
    for (std::map<std::string,double>::iterator it = this->VolumeNodeIdsToWeightsMap.begin(); it != this->VolumeNodeIdsToWeightsMap.end(); ++it)
      {
      os << indent << it->first << ":" << it->second << "|";
      }
    os << "\n";
  }

  os << indent << "AccumulatedDoseVolumeNodeId:   " << this->AccumulatedDoseVolumeNodeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::UpdateReferenceID(const char *oldID, const char *newID)
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
  if (this->AccumulatedDoseVolumeNodeId && !strcmp(oldID, this->AccumulatedDoseVolumeNodeId))
    {
    this->SetAccumulatedDoseVolumeNodeId(newID);
    }
}
