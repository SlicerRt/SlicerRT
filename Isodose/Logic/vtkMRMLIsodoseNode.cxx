/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
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
static const char* COLOR_TABLE_REFERENCE_ROLE = "colorTableRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIsodoseNode);

//----------------------------------------------------------------------------
vtkMRMLIsodoseNode::vtkMRMLIsodoseNode()
{
  this->ShowIsodoseLines = true;
  this->ShowIsodoseSurfaces = true;
  this->ShowScalarBar = false;
  this->ShowDoseVolumesOnly = true;

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
  of << " ShowIsodoseLines=\"" << (this->ShowIsodoseLines ? "true" : "false") << "\"";
  of << " ShowIsodoseSurfaces=\"" << (this->ShowIsodoseSurfaces ? "true" : "false") << "\"";
  of << " ShowScalarBar=\"" << (this->ShowScalarBar ? "true" : "false") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

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
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(DOSE_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLIsodoseNode::GetColorTableNode()
{
  return vtkMRMLColorTableNode::SafeDownCast( this->GetNodeReference(COLOR_TABLE_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLIsodoseNode::SetAndObserveColorTableNode(vtkMRMLColorTableNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(COLOR_TABLE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}
