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
  this->DoseVolumeNodeId = NULL;
  this->ColorTableNodeId = NULL;
  this->IsodoseSurfaceModelsParentHierarchyNodeId = NULL;
  this->ShowIsodoseLines = true;
  this->ShowIsodoseSurfaces = true;
  this->ShowScalarBar = false;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::~vtkMRMLIsodoseNode()
{
  this->SetDoseVolumeNodeId(NULL);
  this->SetColorTableNodeId(NULL);
  this->SetIsodoseSurfaceModelsParentHierarchyNodeId(NULL);
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
    std::stringstream ss;
    if ( this->ColorTableNodeId )
      {
      ss << this->ColorTableNodeId;
      of << indent << " ColorTableNodeId=\"" << ss.str() << "\"";
      }
  }

  {
    std::stringstream ss;
    if ( this->IsodoseSurfaceModelsParentHierarchyNodeId )
      {
      ss << this->IsodoseSurfaceModelsParentHierarchyNodeId;
      of << indent << " IsodoseSurfaceModelsParentHierarchyNodeId=\"" << ss.str() << "\"";
      }
  }

  of << indent << " ShowIsodoseLines=\"" << (this->ShowIsodoseLines ? "true" : "false") << "\"";

  of << indent << " ShowIsodoseSurfaces=\"" << (this->ShowIsodoseSurfaces ? "true" : "false") << "\"";

  of << indent << " ShowScalarBar=\"" << (this->ShowScalarBar ? "true" : "false") << "\"";
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
    else if (!strcmp(attName, "ColorTableNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveColorTableNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "IsodoseSurfaceModelsParentHierarchyNodeId")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetAndObserveIsodoseSurfaceModelsParentHierarchyNodeId(ss.str().c_str());
      }
    else if (!strcmp(attName, "ShowIsodoseLines")) 
      {
      this->ShowIsodoseLines = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ShowIsodoseSurfaces")) 
      {
      this->ShowIsodoseSurfaces = 
        (strcmp(attValue,"true") ? false : true);
      }
    else if (!strcmp(attName, "ShowScalarBar")) 
      {
      this->ShowScalarBar = 
        (strcmp(attValue,"true") ? false : true);
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
  this->SetAndObserveColorTableNodeId(node->ColorTableNodeId);
  this->SetAndObserveIsodoseSurfaceModelsParentHierarchyNodeId(node->IsodoseSurfaceModelsParentHierarchyNodeId);

  this->ShowIsodoseLines = node->ShowIsodoseLines;
  this->ShowIsodoseSurfaces = node->ShowIsodoseSurfaces;
  this->ShowScalarBar = node->ShowScalarBar;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "DoseVolumeNodeId:   " << (this->DoseVolumeNodeId ? this->DoseVolumeNodeId : "NULL") << "\n";
  os << indent << "ColorTableNodeId:   " << (this->ColorTableNodeId ? this->ColorTableNodeId : "NULL") << "\n";
  os << indent << "IsodoseSurfaceModelsParentHierarchyNodeId:   " << (this->IsodoseSurfaceModelsParentHierarchyNodeId ? this->IsodoseSurfaceModelsParentHierarchyNodeId : "NULL") << "\n";
  os << indent << "ShowIsodoseLines:   " << (this->ShowIsodoseLines ? "true" : "false") << "\n";
  os << indent << "ShowIsodoseSurfaces:   " << (this->ShowIsodoseSurfaces ? "true" : "false") << "\n";
  os << indent << "ShowScalarBar:   " << (this->ShowScalarBar ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->DoseVolumeNodeId && !strcmp(oldID, this->DoseVolumeNodeId))
    {
    this->SetAndObserveDoseVolumeNodeId(newID);
    }
  if (this->ColorTableNodeId && !strcmp(oldID, this->ColorTableNodeId))
    {
    this->SetAndObserveColorTableNodeId(newID);
    }
  if (this->IsodoseSurfaceModelsParentHierarchyNodeId && !strcmp(oldID, this->IsodoseSurfaceModelsParentHierarchyNodeId))
    {
    this->SetAndObserveIsodoseSurfaceModelsParentHierarchyNodeId(newID);
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
void vtkMRMLIsodoseNode::SetAndObserveColorTableNodeId(const char* id)
{
  if (this->ColorTableNodeId)
    {
    this->Scene->RemoveReferencedNodeID(this->ColorTableNodeId, this);
    }

  this->SetColorTableNodeId(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->ColorTableNodeId, this);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveIsodoseSurfaceModelsParentHierarchyNodeId(const char* id)
{
  if (this->IsodoseSurfaceModelsParentHierarchyNodeId)
    {
    this->Scene->RemoveReferencedNodeID(this->IsodoseSurfaceModelsParentHierarchyNodeId, this);
    }

  this->SetIsodoseSurfaceModelsParentHierarchyNodeId(id);

  if (id)
    {
    this->Scene->AddReferencedNodeID(this->IsodoseSurfaceModelsParentHierarchyNodeId, this);
    }
}
