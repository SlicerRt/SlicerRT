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
#include "vtkMRMLDoseComparisonNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLDoseComparisonNode);

//----------------------------------------------------------------------------
vtkMRMLDoseComparisonNode::vtkMRMLDoseComparisonNode()
{
  this->ReferenceDoseVolumeNodeId = NULL;
  this->CompareDoseVolumeNodeId = NULL;
  this->GammaVolumeNodeId = NULL;

  this->DtaDistanceToleranceMm = 3.0;
  this->DoseDifferenceTolerancePercent = 3.0;
  this->ReferenceDoseGy = 0.0;
  this->AnalysisThresholdPercent = 0.0;
  this->MaximumGamma = 2.0;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLDoseComparisonNode::~vtkMRMLDoseComparisonNode()
{
  this->SetReferenceDoseVolumeNodeId(NULL);
  this->SetCompareDoseVolumeNodeId(NULL);
  this->SetGammaVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

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
    if ( this->CompareDoseVolumeNodeId )
      {
      ss << this->CompareDoseVolumeNodeId;
      of << indent << " CompareDoseVolumeNodeId=\"" << ss.str() << "\"";
     }
  }

  {
    std::stringstream ss;
    if ( this->GammaVolumeNodeId )
      {
      ss << this->GammaVolumeNodeId;
      of << indent << " GammaVolumeNodeId=\"" << ss.str() << "\"";
     }
  }

  of << indent << " DtaDistanceToleranceMm=\"" << this->DtaDistanceToleranceMm << "\"";
  of << indent << " DoseDifferenceTolerancePercent=\"" << this->DoseDifferenceTolerancePercent << "\"";
  of << indent << " ReferenceDoseGy=\"" << this->ReferenceDoseGy << "\"";
  of << indent << " AnalysisThresholdPercent=\"" << this->AnalysisThresholdPercent << "\"";
  of << indent << " MaximumGamma=\"" << this->MaximumGamma << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "ReferenceDoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveReferenceDoseVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "CompareDoseVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveCompareDoseVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "GammaVolumeNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveGammaVolumeNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "DtaDistanceToleranceMm")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->DtaDistanceToleranceMm = atof(ss.str().c_str());
      }
    else if (!strcmp(attName, "DoseDifferenceTolerancePercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->DoseDifferenceTolerancePercent = atof(ss.str().c_str());
      }
    else if (!strcmp(attName, "ReferenceDoseGy")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->ReferenceDoseGy = atof(ss.str().c_str());
      }
    else if (!strcmp(attName, "AnalysisThresholdPercent")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->AnalysisThresholdPercent = atof(ss.str().c_str());
      }
    else if (!strcmp(attName, "MaximumGamma")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->MaximumGamma = atof(ss.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLDoseComparisonNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLDoseComparisonNode *node = (vtkMRMLDoseComparisonNode *) anode;

  this->SetAndObserveReferenceDoseVolumeNodeId(node->ReferenceDoseVolumeNodeId);
  this->SetAndObserveCompareDoseVolumeNodeId(node->CompareDoseVolumeNodeId);
  this->SetAndObserveGammaVolumeNodeId(node->GammaVolumeNodeId);

  this->DtaDistanceToleranceMm = node->DtaDistanceToleranceMm;
  this->DoseDifferenceTolerancePercent = node->DoseDifferenceTolerancePercent;
  this->ReferenceDoseGy = node->ReferenceDoseGy;
  this->AnalysisThresholdPercent = node->AnalysisThresholdPercent;
  this->MaximumGamma = node->MaximumGamma;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "ReferenceDoseVolumeNodeId:   " << this->ReferenceDoseVolumeNodeId << "\n";
  os << indent << "CompareDoseVolumeNodeId:   " << this->CompareDoseVolumeNodeId << "\n";
  os << indent << "GammaVolumeNodeId:   " << this->GammaVolumeNodeId << "\n";

  os << indent << "DtaDistanceToleranceMm:   " << this->DtaDistanceToleranceMm << "\n";
  os << indent << "DoseDifferenceTolerancePercent:   " << this->DoseDifferenceTolerancePercent << "\n";
  os << indent << "ReferenceDoseGy:   " << this->ReferenceDoseGy << "\n";
  os << indent << "AnalysisThresholdPercent:   " << this->AnalysisThresholdPercent << "\n";
  os << indent << "MaximumGamma:   " << this->MaximumGamma << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveReferenceDoseVolumeNodeId(const char* id)
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
void vtkMRMLDoseComparisonNode::SetAndObserveCompareDoseVolumeNodeId(const char* id)
{
  if (this->CompareDoseVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->CompareDoseVolumeNodeId, this);
  }

  this->SetCompareDoseVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->CompareDoseVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::SetAndObserveGammaVolumeNodeId(const char* id)
{
  if (this->GammaVolumeNodeId)
  {
    this->Scene->RemoveReferencedNodeID(this->GammaVolumeNodeId, this);
  }

  this->SetGammaVolumeNodeId(id);

  if (id)
  {
    this->Scene->AddReferencedNodeID(this->GammaVolumeNodeId, this);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLDoseComparisonNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->ReferenceDoseVolumeNodeId && !strcmp(oldID, this->ReferenceDoseVolumeNodeId))
    {
    this->SetAndObserveReferenceDoseVolumeNodeId(newID);
    }
  if (this->CompareDoseVolumeNodeId && !strcmp(oldID, this->CompareDoseVolumeNodeId))
    {
    this->SetAndObserveCompareDoseVolumeNodeId(newID);
    }
  if (this->GammaVolumeNodeId && !strcmp(oldID, this->GammaVolumeNodeId))
    {
    this->SetAndObserveGammaVolumeNodeId(newID);
    }
}
