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

  this->SelectedInputVolumeIds = NULL;
  vtkSmartPointer<vtkStringArray> selectedInputVolumeIds = vtkSmartPointer<vtkStringArray>::New();
  this->SetSelectedInputVolumeIds(selectedInputVolumeIds);

  this->SelectedInputVolumeWeights = NULL;
  vtkSmartPointer<vtkDoubleArray> selectedInputVolumeWeights = vtkSmartPointer<vtkDoubleArray>::New();
  this->SetSelectedInputVolumeWeights(selectedInputVolumeWeights);

  this->AccumulatedDoseVolumeId = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLDoseAccumulationNode::~vtkMRMLDoseAccumulationNode()
{
  this->SetSelectedInputVolumeIds(NULL);
  this->SetSelectedInputVolumeWeights(NULL);
  this->SetAccumulatedDoseVolumeId(NULL);
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
    for (int i=0; i<this->SelectedInputVolumeIds->GetNumberOfValues(); ++i)
      {
      std::stringstream ss;
      ss << this->SelectedInputVolumeIds->GetValue(i);
      of << indent << ss.str() << "|";
      }
    of << "\"";
  }

  {
    of << indent << " SelectedInputVolumeWeights=\"";
    for (int i=0; i<this->SelectedInputVolumeWeights->GetNumberOfTuples(); ++i)
      {
      std::stringstream ss;
      ss << this->SelectedInputVolumeIds->GetValue(i);
      of << indent << ss.str() << "|";
      }
    of << "\"";
  }

  {
    std::stringstream ss;
    if ( this->AccumulatedDoseVolumeId )
      {
      ss << this->AccumulatedDoseVolumeId;
      of << indent << " AccumulatedDoseVolumeId=\"" << ss.str() << "\"";
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

      this->SelectedInputVolumeIds->Initialize();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        this->SelectedInputVolumeIds->InsertNextValue( valueStr.substr(0, separatorPosition) );
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        this->SelectedInputVolumeIds->InsertNextValue( valueStr );
        }
      }
    else if (!strcmp(attName, "SelectedInputVolumeWeights")) 
      {
      std::stringstream ss;
      ss << attValue;
      std::string valueStr = ss.str();
      std::string separatorCharacter("|");

      this->SelectedInputVolumeWeights->Initialize();
      size_t separatorPosition = valueStr.find( separatorCharacter );
      while (separatorPosition != std::string::npos)
        {
        double weight;
        std::stringstream vss;
        vss << valueStr.substr(0, separatorPosition);
        vss >> weight;
        this->SelectedInputVolumeWeights->InsertNextValue( weight );
        valueStr = valueStr.substr( separatorPosition+1 );
        separatorPosition = valueStr.find( separatorCharacter );
        }
      if (! valueStr.empty() )
        {
        double weight;
        std::stringstream vss;
        vss << valueStr.substr(0, separatorPosition);
        vss >> weight;
        this->SelectedInputVolumeWeights->InsertNextValue( weight );
        }
      }
    else if (!strcmp(attName, "AccumulatedDoseVolumeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->AccumulatedDoseVolumeId;
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
  this->SetSelectedInputVolumeIds(node->SelectedInputVolumeIds);
  this->SetSelectedInputVolumeWeights(node->SelectedInputVolumeWeights);
  this->SetAccumulatedDoseVolumeId(node->AccumulatedDoseVolumeId);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ShowDoseVolumesOnly:   " << (this->ShowDoseVolumesOnly ? "true" : "false") << "\n";
  os << indent << "SelectedInputVolumeIds:   " << this->SelectedInputVolumeIds << "\n";
  os << indent << "SelectedInputVolumeWeights:   " << this->SelectedInputVolumeWeights << "\n";
  os << indent << "AccumulatedDoseVolumeId:   " << this->AccumulatedDoseVolumeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseAccumulationNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  for (int i=0; i<this->SelectedInputVolumeIds->GetNumberOfValues(); ++i)
    {
    if (!strcmp(oldID, this->SelectedInputVolumeIds->GetValue(i)))
      {
      this->SelectedInputVolumeIds->SetValue(i,newID);
      }
    }
  if (this->AccumulatedDoseVolumeId && !strcmp(oldID, this->AccumulatedDoseVolumeId))
    {
    this->SetAccumulatedDoseVolumeId(newID);
    }
}
