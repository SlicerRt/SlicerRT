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
  this->DoseVolumeNodeId = NULL;
  this->StructureSetModelNodeId = NULL;
  this->ChartNodeId = NULL;
  this->DvhDoubleArrayNodeIds.clear();
  this->ShowHideAll = 0;
  this->ShowInChartCheckStates.clear();
  this->VDoseValues = NULL;
  this->ShowVMetricsCc = false;
  this->ShowVMetricsPercent = false;
  this->DVolumeValues = NULL;
  this->ShowDMetrics = false;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseVolumeHistogramNode::~vtkMRMLDoseVolumeHistogramNode()
{
  this->SetDoseVolumeNodeId(NULL);
  this->SetStructureSetModelNodeId(NULL);
  this->SetChartNodeId(NULL);
  this->DvhDoubleArrayNodeIds.clear();
  this->ShowInChartCheckStates.clear();
  this->SetVDoseValues(NULL);
  this->SetDVolumeValues(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::WriteXML(ostream& of, int nIndent)
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
    if ( this->StructureSetModelNodeId )
      {
      ss << this->StructureSetModelNodeId;
      of << indent << " StructureSetModelNodeId=\"" << ss.str() << "\"";
     }
  }

  {
    std::stringstream ss;
    if ( this->ChartNodeId )
      {
      ss << this->ChartNodeId;
      of << indent << " ChartNodeId=\"" << ss.str() << "\"";
     }
  }

  {
    of << indent << " DvhDoubleArrayNodeIds=\"";
    for (std::set<std::string>::iterator it = this->DvhDoubleArrayNodeIds.begin(); it != this->DvhDoubleArrayNodeIds.end(); ++it)
      {
      of << (*it) << "|";
      }
    of << "\"";
  }

  {
    std::stringstream ss;
    ss << this->ShowHideAll;
    of << indent << " ShowHideAll=\"" << ss.str() << "\"";
  }

  {
    of << indent << " ShowInChartCheckStates=\"";
    for (std::vector<bool>::iterator it = this->ShowInChartCheckStates.begin(); it != this->ShowInChartCheckStates.end(); ++it)
      {
      of << ((*it) ? "true" : "false") << "|";
      }
    of << "\"";
  }

  {
    std::stringstream ss;
    if ( this->VDoseValues )
      {
      ss << this->VDoseValues;
      of << indent << " VDoseValues=\"" << ss.str() << "\"";
     }
  }

  of << indent << " ShowVMetricsCc=\"" << (this->ShowVMetricsCc ? "true" : "false") << "\"";

  of << indent << " ShowVMetricsPercent=\"" << (this->ShowVMetricsPercent ? "true" : "false") << "\"";

  {
    std::stringstream ss;
    if ( this->DVolumeValues )
      {
      ss << this->DVolumeValues;
      of << indent << " DVolumeValues=\"" << ss.str() << "\"";
     }
  }

  of << indent << " ShowDMetrics=\"" << (this->ShowDMetrics ? "true" : "false") << "\"";
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

    if (!strcmp(attName, "DoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetDoseVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "StructureSetModelNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetStructureSetModelNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "ChartNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetChartNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "DvhDoubleArrayNodeIds")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->DvhDoubleArrayNodeIds.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        this->DvhDoubleArrayNodeIds.insert( valueStr.substr(0, separatorPosition) );
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        this->DvhDoubleArrayNodeIds.insert( valueStr );
        }
      }
    else if (!strcmp(attName, "ShowHideAll")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->ShowHideAll = atoi(ss.str().c_str());
      }
    else if (!strcmp(attName, "ShowInChartCheckStates")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->ShowInChartCheckStates.clear();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        this->ShowInChartCheckStates.push_back(
          (valueStr.substr(0, separatorPosition).compare("true") ? false : true));
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        this->ShowInChartCheckStates.push_back(
          (valueStr.compare("true") ? false : true));
        }
      }
    else if (!strcmp(attName, "VDoseValues")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetVDoseValues(ss.str().c_str());
      }
    else if (!strcmp(attName, "ShowVMetricsCc")) 
      {
      this->ShowVMetricsCc = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ShowVMetricsPercent")) 
      {
      this->ShowVMetricsPercent = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "DVolumeValues")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetDVolumeValues(ss.str().c_str());
      }
    else if (!strcmp(attName, "ShowDMetrics")) 
      {
      this->ShowDMetrics = 
        (strcmp(attValue,"true") ? false : true);
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

  this->SetDoseVolumeNodeId(node->DoseVolumeNodeId);
  this->SetStructureSetModelNodeId(node->StructureSetModelNodeId);
  this->SetChartNodeId(node->ChartNodeId);

  this->DvhDoubleArrayNodeIds = node->DvhDoubleArrayNodeIds;
  this->ShowHideAll = node->ShowHideAll;
  this->ShowInChartCheckStates = node->ShowInChartCheckStates;

  this->SetVDoseValues(node->VDoseValues);
  this->ShowVMetricsCc = node->ShowVMetricsCc;
  this->ShowVMetricsPercent = node->ShowVMetricsPercent;

  this->SetDVolumeValues(node->DVolumeValues);
  this->ShowDMetrics = node->ShowDMetrics;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "DoseVolumeNodeId:   " << this->DoseVolumeNodeId << "\n";
  os << indent << "StructureSetModelNodeId:   " << this->StructureSetModelNodeId << "\n";
  os << indent << "ChartNodeId:   " << this->ChartNodeId << "\n";

  {
    os << indent << "DvhDoubleArrayNodeIds:   ";
    for (std::set<std::string>::iterator it = this->DvhDoubleArrayNodeIds.begin(); it != this->DvhDoubleArrayNodeIds.end(); ++it)
      {
      os << (*it) << "|";
      }
    os << "\n";
  }

  os << indent << "ShowHideAll:   " << this->ShowHideAll << "\n";

  {
    os << indent << "ShowInChartCheckStates:   ";
    for (std::vector<bool>::iterator it = this->ShowInChartCheckStates.begin(); it != this->ShowInChartCheckStates.end(); ++it)
      {
      os << indent << ((*it) ? "true" : "false") << "|";
      }
    os << "\n";
  }

  os << indent << "VDoseValues:   " << this->VDoseValues << "\n";
  os << indent << "ShowVMetricsCc:   " << (this->ShowVMetricsCc ? "true" : "false") << "\n";
  os << indent << "ShowVMetricsPercent:   " << (this->ShowVMetricsPercent ? "true" : "false") << "\n";

  os << indent << "DVolumeValues:   " << this->DVolumeValues << "\n";
  os << indent << "ShowDMetrics:   " << (this->ShowDMetrics ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseVolumeHistogramNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->DoseVolumeNodeId && !strcmp(oldID, this->DoseVolumeNodeId))
    {
    this->SetDoseVolumeNodeId(newID);
    }
  if (this->StructureSetModelNodeId && !strcmp(oldID, this->StructureSetModelNodeId))
    {
    this->SetStructureSetModelNodeId(newID);
    }
  if (this->ChartNodeId && !strcmp(oldID, this->ChartNodeId))
    {
    this->SetChartNodeId(newID);
    }
  if (this->DvhDoubleArrayNodeIds.find(oldID) != this->DvhDoubleArrayNodeIds.end())
    {
    this->DvhDoubleArrayNodeIds.erase(oldID);
    this->DvhDoubleArrayNodeIds.insert(newID);
    }
}
