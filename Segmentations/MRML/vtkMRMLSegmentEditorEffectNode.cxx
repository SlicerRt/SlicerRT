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

// Segmentations MRML includes
#include "vtkMRMLSegmentEditorEffectNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentEditorEffectNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentEditorEffectNode::vtkMRMLSegmentEditorEffectNode()
{
  this->EffectName = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLSegmentEditorEffectNode::~vtkMRMLSegmentEditorEffectNode()
{
  this->SetEffectName(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorEffectNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->EffectName )
      {
      ss << this->EffectName;
      of << indent << " EffectName=\"" << ss.str() << "\"";
      }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorEffectNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
    {
    if (!strcmp(attName, "EffectName")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetEffectName(ss.str().c_str());
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSegmentEditorEffectNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();
  
  vtkMRMLSegmentEditorEffectNode* otherNode = vtkMRMLSegmentEditorEffectNode::SafeDownCast(anode);

  this->SetEffectName(otherNode->EffectName);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorEffectNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "EffectName:   " << (this->EffectName ? this->EffectName : "") << "\n";
}
