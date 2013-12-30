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

// MRMLIsodose includes
#include "vtkMRMLIsodoseNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* DOSE_VOLUME_REFERENCE_ROLE = "doseVolumeRef";
static const char* ISODOSE_SURFACE_MODELS_SUBJECT_HIERARCHY_REFERENCE_ROLE = "isodoseSurfaceModelsParentHierarchyRef";
static const char* COLOR_TABLE_REFERENCE_ROLE = "colorTableRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIsodoseNode);

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::vtkMRMLIsodoseNode()
{
  this->ShowIsodoseLines = true;
  this->ShowIsodoseSurfaces = true;
  this->ShowScalarBar = false;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::~vtkMRMLIsodoseNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

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

    if (!strcmp(attName, "ShowIsodoseLines")) 
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

  this->ShowIsodoseLines = node->ShowIsodoseLines;
  this->ShowIsodoseSurfaces = node->ShowIsodoseSurfaces;
  this->ShowScalarBar = node->ShowScalarBar;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "ShowIsodoseLines:   " << (this->ShowIsodoseLines ? "true" : "false") << "\n";
  os << indent << "ShowIsodoseSurfaces:   " << (this->ShowIsodoseSurfaces ? "true" : "false") << "\n";
  os << indent << "ShowScalarBar:   " << (this->ShowScalarBar ? "true" : "false") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLIsodoseNode::GetDoseVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(DOSE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveDoseVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(DOSE_VOLUME_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLModelHierarchyNode* vtkMRMLIsodoseNode::GetIsodoseSurfaceModelsParentHierarchyNode()
{
  return vtkMRMLModelHierarchyNode::SafeDownCast( this->GetNodeReference(ISODOSE_SURFACE_MODELS_SUBJECT_HIERARCHY_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveIsodoseSurfaceModelsParentHierarchyNode(vtkMRMLModelHierarchyNode* node)
{
  this->SetNodeReferenceID(ISODOSE_SURFACE_MODELS_SUBJECT_HIERARCHY_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLIsodoseNode::GetColorTableNode()
{
  return vtkMRMLColorTableNode::SafeDownCast( this->GetNodeReference(COLOR_TABLE_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveColorTableNode(vtkMRMLColorTableNode* node)
{
  this->SetNodeReferenceID(COLOR_TABLE_REFERENCE_ROLE, node->GetID());
}
