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
#include "vtkMRMLSegmentEditorNode.h"

#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* SEGMENTATION_REFERENCE_ROLE = "segmentationRef";
static const char* MASTER_VOLUME_REFERENCE_ROLE = "masterVolumeRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentEditorNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentEditorNode::vtkMRMLSegmentEditorNode()
  : EditedLabelmap(NULL)
  , MaskLabelmap(NULL)
  , SelectedSegmentID(NULL)
  , ActiveEffectName(NULL)
{
  this->EditedLabelmap = vtkOrientedImageData::New();
  this->MaskLabelmap = vtkOrientedImageData::New();
}

//----------------------------------------------------------------------------
vtkMRMLSegmentEditorNode::~vtkMRMLSegmentEditorNode()
{
  this->SetSelectedSegmentID(NULL);
  this->SetActiveEffectName(NULL);

  if (this->EditedLabelmap)
  {
    this->EditedLabelmap->Delete();
    this->EditedLabelmap = NULL;
  }
  if (this->MaskLabelmap)
  {
    this->MaskLabelmap->Delete();
    this->MaskLabelmap = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    std::stringstream ss;
    if ( this->SelectedSegmentID )
      {
      ss << this->SelectedSegmentID;
      of << indent << " SelectedSegmentID=\"" << ss.str() << "\"";
      }
    else if ( this->ActiveEffectName )
      {
      ss << this->ActiveEffectName;
      of << indent << " ActiveEffectName=\"" << ss.str() << "\"";
      }
  }

  // Note: The image data are excluded from storage as they are temporary
  //   data objects that are automatically updated when selection changes
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
    {
    if (!strcmp(attName, "SelectedSegmentID")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetSelectedSegmentID(ss.str().c_str());
      }
    else if (!strcmp(attName, "ActiveEffectName")) 
      {
      std::stringstream ss;
      ss << attValue;
      this->SetActiveEffectName(ss.str().c_str());
      }
    }

  this->EndModify(disabledModify);

  // Note: The image data are excluded from storage as they are temporary
  //   data objects that are automatically updated when selection changes
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSegmentEditorNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();
  
  vtkMRMLSegmentEditorNode* otherNode = vtkMRMLSegmentEditorNode::SafeDownCast(anode);

  this->SetSelectedSegmentID(otherNode->SelectedSegmentID);
  this->SetActiveEffectName(otherNode->ActiveEffectName);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();

  // Note: The image data are excluded from storage as they are temporary
  //   data objects that are automatically updated when selection changes
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "SelectedSegmentID:   " << (this->SelectedSegmentID ? this->SelectedSegmentID : "") << "\n";
  os << indent << "ActiveEffectName:   " << (this->ActiveEffectName ? this->ActiveEffectName : "") << "\n";

  os << indent << "EditedLabelmap:\n";
  this->EditedLabelmap->PrintSelf(os,indent);
  os << indent << "\n";
  os << indent << "MaskLabelmap:\n";
  this->MaskLabelmap->PrintSelf(os,indent);
  os << indent << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLSegmentEditorNode::GetMasterVolumeNode()
{
  return vtkMRMLScalarVolumeNode::SafeDownCast( this->GetNodeReference(MASTER_VOLUME_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorNode::SetAndObserveMasterVolumeNode(vtkMRMLScalarVolumeNode* node)
{
  this->SetNodeReferenceID(MASTER_VOLUME_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentEditorNode::GetSegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentEditorNode::SetAndObserveSegmentationNode(vtkMRMLSegmentationNode* node)
{
  this->SetNodeReferenceID(SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

