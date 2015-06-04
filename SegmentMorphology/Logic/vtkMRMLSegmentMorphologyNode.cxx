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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// MRMLDoseAccumulation includes
#include "vtkMRMLSegmentMorphologyNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* SEGMENTATION_A_REFERENCE_ROLE = "segmentationARef";
static const char* SEGMENTATION_B_REFERENCE_ROLE = "segmentationBRef";
static const char* OUTPUT_SEGMENTATION_REFERENCE_ROLE = "outputSegmentationRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentMorphologyNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentMorphologyNode::vtkMRMLSegmentMorphologyNode()
{
  this->SegmentAID = NULL;
  this->SegmentBID = NULL;

  this->Operation = vtkMRMLSegmentMorphologyNode::Expand;
  this->XSize = 1;
  this->YSize = 1;
  this->ZSize = 1;

  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLSegmentMorphologyNode::~vtkMRMLSegmentMorphologyNode()
{
  this->SetSegmentAID(NULL);
  this->SetSegmentBID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " SegmentAID=\"" << (this->SegmentAID ? this->SegmentAID : "NULL") << "\"";
  of << indent << " SegmentBID=\"" << (this->SegmentBID ? this->SegmentBID : "NULL") << "\"";

  of << indent << " Operation=\"" << (this->Operation) << "\"";
  of << indent << " XSize=\"" << (this->XSize) << "\"";
  of << indent << " YSize=\"" << (this->YSize) << "\"";
  of << indent << " ZSize=\"" << (this->ZSize) << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "SegmentAID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetSegmentAID(ss.str().c_str());
      }
    else if (!strcmp(attName, "SegmentBID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetSegmentBID(ss.str().c_str());
      }
    else if (!strcmp(attName, "Operation")) 
      {
      std::stringstream ss;
      ss << attValue;
      int operation;
      ss >> operation;
      this->Operation = operation;
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
void vtkMRMLSegmentMorphologyNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLSegmentMorphologyNode *node = (vtkMRMLSegmentMorphologyNode *)anode;

  this->SetSegmentAID(node->SegmentAID);
  this->SetSegmentBID(node->SegmentBID);
  this->Operation = node->Operation;
  this->XSize = node->XSize;
  this->YSize = node->YSize;
  this->ZSize = node->ZSize;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " SegmentAID:   " << (this->SegmentAID ? this->SegmentAID : "NULL") << "\n";
  os << indent << " SegmentBID:   " << (this->SegmentBID ? this->SegmentBID : "NULL") << "\n";

  os << indent << " Operation:   " << (this->Operation) << "\n";
  os << indent << " XSize:   " << (this->XSize) << "\n";
  os << indent << " YSize:   " << (this->YSize) << "\n";
  os << indent << " ZSize:   " << (this->ZSize) << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentMorphologyNode::GetSegmentationANode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(SEGMENTATION_A_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::SetAndObserveSegmentationANode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(SEGMENTATION_A_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentMorphologyNode::GetSegmentationBNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(SEGMENTATION_B_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::SetAndObserveSegmentationBNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(SEGMENTATION_B_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentMorphologyNode::GetOutputSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(OUTPUT_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::SetAndObserveOutputSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(OUTPUT_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentMorphologyNode::SetOperation(int operation)
{
  if (this->Operation != operation)
  {
    this->Operation = operation;
  }
}
