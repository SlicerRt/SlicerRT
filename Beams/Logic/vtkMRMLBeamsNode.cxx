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

// MRMLBeams includes
#include "vtkMRMLBeamsNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
std::string vtkMRMLBeamsNode::IsocenterMarkupsReferenceRole = std::string("isocenterMarkups") + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;
std::string vtkMRMLBeamsNode::BeamModelReferenceRole = std::string("beamModel") + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX;

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBeamsNode);

//----------------------------------------------------------------------------
vtkMRMLBeamsNode::vtkMRMLBeamsNode()
{
  this->BeamModelOpacity = 0.08;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLBeamsNode::~vtkMRMLBeamsNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->BeamModelOpacity )
      {
      ss << this->BeamModelOpacity;
      of << indent << " BeamModelOpacity=\"" << ss.str() << "\"";
      }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "BeamModelOpacity")) 
      {
      std::stringstream ss;
      ss << attValue;
      double beamModelOpacity;
      ss >> beamModelOpacity;
      this->BeamModelOpacity = beamModelOpacity;
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLBeamsNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLBeamsNode *node = (vtkMRMLBeamsNode *) anode;

  this->SetBeamModelOpacity(node->GetBeamModelOpacity());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "BeamModelOpacity:   " << this->BeamModelOpacity << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLBeamsNode::GetIsocenterMarkupsNode()
{
  return vtkMRMLMarkupsFiducialNode::SafeDownCast(
    this->GetNodeReference(vtkMRMLBeamsNode::IsocenterMarkupsReferenceRole.c_str()) );
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::SetAndObserveIsocenterMarkupsNode(vtkMRMLMarkupsFiducialNode* node)
{
  this->SetNthNodeReferenceID(vtkMRMLBeamsNode::IsocenterMarkupsReferenceRole.c_str(), 0, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLBeamsNode::GetBeamModelNode()
{
  return vtkMRMLModelNode::SafeDownCast(
    this->GetNodeReference(vtkMRMLBeamsNode::BeamModelReferenceRole.c_str()) );
}

//----------------------------------------------------------------------------
void vtkMRMLBeamsNode::SetAndObserveBeamModelNode(vtkMRMLModelNode* node)
{
  this->SetNthNodeReferenceID(vtkMRMLBeamsNode::BeamModelReferenceRole.c_str(), 0, node->GetID());
}
