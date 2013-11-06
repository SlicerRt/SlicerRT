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
#include "vtkMRMLContourMorphologyNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLContourNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* CONTOUR_A_REFERENCE_ROLE = "contourARef";
static const char* CONTOUR_B_REFERENCE_ROLE = "contourBRef";
static const char* REFERENCE_VOLUME_REFERENCE_ROLE = "referenceVolumeRef";
static const char* OUTPUT_CONTOUR_REFERENCE_ROLE = "outputContourRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourMorphologyNode);

//----------------------------------------------------------------------------
vtkMRMLContourMorphologyNode::vtkMRMLContourMorphologyNode()
{
  this->Operation = Expand;
  this->XSize = 1;
  this->YSize = 1;
  this->ZSize = 1;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLContourMorphologyNode::~vtkMRMLContourMorphologyNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " Operation=\"" << (this->Operation) << "\"";
  of << indent << " XSize=\"" << (this->XSize) << "\"";
  of << indent << " YSize=\"" << (this->YSize) << "\"";
  of << indent << " ZSize=\"" << (this->ZSize) << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "Operation")) 
      {
      std::stringstream ss;
      ss << attValue;
      int operation;
      ss >> operation;
      this->Operation = (ContourMorphologyOperationType)operation;
      }
    else if (!strcmp(attName, "XSize")) 
      {
      std::stringstream ss;
      ss << attValue;
      double xSize;
      ss >> xSize;
      this->XSize = xSize;
      }
    else if (!strcmp(attName, "YSize")) 
      {
      std::stringstream ss;
      ss << attValue;
      double ySize;
      ss >> ySize;
      this->YSize = ySize;
      }
    else if (!strcmp(attName, "ZSize")) 
      {
      std::stringstream ss;
      ss << attValue;
      double zSize;
      ss >> zSize;
      this->ZSize = zSize;
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLContourMorphologyNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLContourMorphologyNode *node = (vtkMRMLContourMorphologyNode *)anode;

  this->Operation = node->Operation;
  this->XSize = node->XSize;
  this->YSize = node->YSize;
  this->ZSize = node->ZSize;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Operation:   " << (int)(this->Operation) << "\n";
  os << indent << "XSize:   " << (this->XSize) << "\n";
  os << indent << "YSize:   " << (this->YSize) << "\n";
  os << indent << "ZSize:   " << (this->ZSize) << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLContourMorphologyNode::GetContourANode()
{
  return vtkMRMLContourNode::SafeDownCast( this->GetNodeReference(CONTOUR_A_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourANode(vtkMRMLContourNode* node)
{
  this->SetNodeReferenceID(CONTOUR_A_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLContourMorphologyNode::GetContourBNode()
{
  return vtkMRMLContourNode::SafeDownCast( this->GetNodeReference(CONTOUR_B_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveContourBNode(vtkMRMLContourNode* node)
{
  this->SetNodeReferenceID(CONTOUR_B_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourMorphologyNode::GetReferenceVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(REFERENCE_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveReferenceVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(REFERENCE_VOLUME_REFERENCE_ROLE, node->GetID());
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLContourMorphologyNode::GetOutputContourNode()
{
  return vtkMRMLContourNode::SafeDownCast( this->GetNodeReference(OUTPUT_CONTOUR_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLContourMorphologyNode::SetAndObserveOutputContourNode(vtkMRMLContourNode* node)
{
  this->SetNodeReferenceID(OUTPUT_CONTOUR_REFERENCE_ROLE, node->GetID());
}
