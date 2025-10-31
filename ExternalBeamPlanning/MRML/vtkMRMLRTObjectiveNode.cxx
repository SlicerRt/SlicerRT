/*==============================================================================

  Copyright (c) German Cancer Research Center (DKFZ),
  Heidelberg, Germany. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Lina Bucher, German Cancer
  Research Center (DKFZ) and Institute for Biomedical Engineering (IBT),
  Karlsruhe Institute of Technology (KIT).

==============================================================================*/

// ExternalBeamPlanning includes
#include "vtkMRMLRTObjectiveNode.h"

// MRML includes
#include <vtkMRMLSegmentationNode.h>

// Qt includes
#include <QDebug>

//----------------------------------------------------------------------------
static const char* SEGMENTATION_REFERENCE_ROLE = "segmentationRef";
//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRTObjectiveNode);

//----------------------------------------------------------------------------
vtkMRMLRTObjectiveNode::vtkMRMLRTObjectiveNode()
{
  this->AddNodeReferenceRole(SEGMENTATION_REFERENCE_ROLE, "Segmentation");
}

//----------------------------------------------------------------------------
vtkMRMLRTObjectiveNode::~vtkMRMLRTObjectiveNode()
{
  this->SetName(nullptr);
  this->SetSegmentID(nullptr);
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStringMacro(Name, Name);
  vtkMRMLWriteXMLStringMacro(SegmentID, SegmentID);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // read all MRML node attributes from two arrays of names and values
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStringMacro(Name, Name);
  vtkMRMLReadXMLStringMacro(SegmentID, SegmentID);
  vtkMRMLReadXMLEndMacro();
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRTObjectiveNode::Copy(vtkMRMLNode* anode)
{
  Superclass::Copy(anode);

  vtkMRMLRTObjectiveNode* node = vtkMRMLRTObjectiveNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  this->DisableModifiedEventOn();

  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyStringMacro(Name);
  vtkMRMLCopyStringMacro(SegmentID);
  vtkMRMLCopyEndMacro();

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::CopyContent(vtkMRMLNode* anode, bool deepCopy/*=true*/)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::CopyContent(anode, deepCopy);

  vtkMRMLRTObjectiveNode* node = vtkMRMLRTObjectiveNode::SafeDownCast(anode);
  if (!node)
  {
    return;
  }

  vtkMRMLCopyBeginMacro(node);
  vtkMRMLCopyStringMacro(Name);
  vtkMRMLCopyStringMacro(SegmentID);
  vtkMRMLCopyEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStringMacro(Name);
  vtkMRMLPrintStringMacro(SegmentID);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLRTObjectiveNode::GetSegmentationNode()
{
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(this->GetNodeReference(SEGMENTATION_REFERENCE_ROLE));
  if (!segmentationNode)
  {
    qWarning() << Q_FUNC_INFO << ": Segmentation node reference is null.";
  }
  return segmentationNode;
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::SetSegmentationNode(vtkMRMLSegmentationNode* segmentationNode)
{
  if (!this->GetScene())
  {
    vtkErrorMacro("Cannot set reference: referencing node has no scene.");
    return;
  }
  if (segmentationNode && !segmentationNode->GetScene())
  {
    vtkErrorMacro("Cannot set reference: referenced node has no scene.");
    return;
  }
  if (segmentationNode && this->GetScene() != segmentationNode->GetScene())
  {
    vtkErrorMacro("Cannot set reference: referenced and referencing nodes are in different scenes.");
    return;
  }

  if (!this->IsSegmentIDValid(segmentationNode, this->SegmentID))
  {
    qWarning() << Q_FUNC_INFO << ": Segment with ID" << this->SegmentID << "not found in the new segmentation.";
    qWarning() << "Setting segmentation node to null.";
    this->SegmentID = nullptr;
  }
  this->SetNodeReferenceID(SEGMENTATION_REFERENCE_ROLE, (segmentationNode ? segmentationNode->GetID() : nullptr));
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::SetSegmentID(const char* segmentID)
{
  if (segmentID == nullptr)
  {
    // Setting to null is always valid
    delete[] this->SegmentID;
    this->SegmentID = nullptr;
    this->Modified();
    return;
  }

  vtkMRMLSegmentationNode* segmentationNode = this->GetSegmentationNode();
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Segmentation node is null.";
    return;
  }
  if (!this->IsSegmentIDValid(segmentationNode, segmentID))
  {
    qCritical() << Q_FUNC_INFO << ": Segment with ID" << segmentID << "not found in segmentation.";
    return;
  }
  // Avoid unnecessary updates
  if (this->SegmentID && segmentID && !strcmp(this->SegmentID, segmentID))
  {
    return;
  }

  // Set new segment ID (free old value & allocate new space)
  delete [] this->SegmentID;
  if (segmentID)
  {
    size_t n = strlen(segmentID) + 1;
    this->SegmentID = new char[n];
    strcpy(this->SegmentID, segmentID);
  }
  else
  {
    this->SegmentID = nullptr;
  }

  // Invoke modified event
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLRTObjectiveNode::SetSegmentationAndSegmentID(vtkMRMLSegmentationNode* segmentationNode, const char* segmentID)
{
  if (!this->GetScene())
  {
    vtkErrorMacro("Cannot set reference: referencing node has no scene.");
    return;
  }
  if (segmentationNode && !segmentationNode->GetScene())
  {
    vtkErrorMacro("Cannot set reference: referenced node has no scene.");
    return;
  }
  if (segmentationNode && this->GetScene() != segmentationNode->GetScene())
  {
    vtkErrorMacro("Cannot set reference: referenced and referencing nodes are in different scenes.");
    return;
  }

  if (!this->IsSegmentIDValid(segmentationNode, segmentID))
  {
    qCritical() << Q_FUNC_INFO << ": Segment with ID" << segmentID << "not found in segmentation.";
    return;
  }

  this->SetNodeReferenceID(SEGMENTATION_REFERENCE_ROLE, (segmentationNode ? segmentationNode->GetID() : nullptr));
  this->SetSegmentID(segmentID);
}

//----------------------------------------------------------------------------
bool vtkMRMLRTObjectiveNode::IsSegmentIDValid(vtkMRMLSegmentationNode* segmentationNode, const char* segmentID)
{
  if (!segmentationNode || !segmentID)
  {
    return false;
  }

  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
  {
    return false;
  }

  return segmentation->GetSegment(segmentID) != nullptr;
}
