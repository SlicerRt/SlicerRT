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

// SlicerRtCommon includes
#include "vtkMRMLContourNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourNode);

//----------------------------------------------------------------------------
vtkMRMLContourNode::vtkMRMLContourNode()
{
  this->RibbonModelNode = NULL;
  this->RibbonModelNodeId = NULL;

  this->IndexedLabelmapVolumeNode = NULL;
  this->IndexedLabelmapVolumeNodeId = NULL;

  this->ClosedSurfaceModelNode = NULL;
  this->ClosedSurfaceModelNodeId = NULL;

  this->BitfieldLabelmapVolumeNode = NULL;
  this->BitfieldLabelmapVolumeNodeId = NULL;

  this->HideFromEditorsOff();
}

//----------------------------------------------------------------------------
vtkMRMLContourNode::~vtkMRMLContourNode()
{
  this->SetAndObserveRibbonModelNodeId(NULL);
  this->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
  this->SetAndObserveClosedSurfaceModelNodeId(NULL);
  this->SetAndObserveBitfieldLabelmapVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->RibbonModelNodeId != NULL) 
    {
    of << indent << " RibbonModelNodeId=\"" << this->RibbonModelNodeId << "\"";
    }
  if (this->IndexedLabelmapVolumeNodeId != NULL) 
    {
    of << indent << " IndexedLabelmapVolumeNodeId=\"" << this->IndexedLabelmapVolumeNodeId << "\"";
    }
  if (this->ClosedSurfaceModelNodeId != NULL) 
    {
    of << indent << " ClosedSurfaceModelNodeId=\"" << this->ClosedSurfaceModelNodeId << "\"";
    }
  if (this->BitfieldLabelmapVolumeNodeId != NULL) 
    {
    of << indent << " BitfieldLabelmapVolumeNodeId=\"" << this->BitfieldLabelmapVolumeNodeId << "\"";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "RibbonModelNodeId")) 
      {
      this->SetAndObserveRibbonModelNodeId(NULL); // clear any previous observers
      // Do not add observers yet because updates may be wrong before reading all the xml attributes
      // Observers will be added when all the attributes are read and UpdateScene is called
      this->SetRibbonModelNodeId(attValue);
      }
    else if (!strcmp(attName, "IndexedLabelmapVolumeNodeId")) 
      {
      this->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
      this->SetIndexedLabelmapVolumeNodeId(attValue);
      }
    else if (!strcmp(attName, "ClosedSurfaceModelNodeId")) 
      {
      this->SetAndObserveClosedSurfaceModelNodeId(NULL);
      this->SetClosedSurfaceModelNodeId(attValue);
      }
    else if (!strcmp(attName, "BitfieldLabelmapVolumeNodeId")) 
      {
      this->SetAndObserveBitfieldLabelmapVolumeNodeId(NULL);
      this->SetBitfieldLabelmapVolumeNodeId(attValue);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLContourNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLContourNode *node = (vtkMRMLContourNode *) anode;

  // Observers must be removed here, otherwise MRML updates would activate nodes on the undo stack
  this->SetAndObserveRibbonModelNodeId( NULL );
  this->SetRibbonModelNodeId( node->RibbonModelNodeId );

  this->SetAndObserveIndexedLabelmapVolumeNodeId( NULL );
  this->SetIndexedLabelmapVolumeNodeId( node->IndexedLabelmapVolumeNodeId );

  this->SetAndObserveClosedSurfaceModelNodeId( NULL );
  this->SetClosedSurfaceModelNodeId( node->ClosedSurfaceModelNodeId );

  this->SetAndObserveBitfieldLabelmapVolumeNodeId( NULL );
  this->SetBitfieldLabelmapVolumeNodeId( node->BitfieldLabelmapVolumeNodeId );

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::UpdateReferences()
{
  Superclass::UpdateReferences();

  if (this->RibbonModelNodeId != NULL && this->Scene->GetNodeByID(this->RibbonModelNodeId) == NULL)
    {
    this->SetRibbonModelNodeId(NULL);
    }
  if (this->IndexedLabelmapVolumeNodeId != NULL && this->Scene->GetNodeByID(this->IndexedLabelmapVolumeNodeId) == NULL)
    {
    this->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
    }
  if (this->ClosedSurfaceModelNodeId != NULL && this->Scene->GetNodeByID(this->ClosedSurfaceModelNodeId) == NULL)
    {
    this->SetAndObserveClosedSurfaceModelNodeId(NULL);
    }
  if (this->BitfieldLabelmapVolumeNodeId != NULL && this->Scene->GetNodeByID(this->BitfieldLabelmapVolumeNodeId) == NULL)
    {
    this->SetAndObserveBitfieldLabelmapVolumeNodeId(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);

  if (this->RibbonModelNodeId && !strcmp(oldID, this->RibbonModelNodeId))
    {
    this->SetAndObserveRibbonModelNodeId(newID);
    }
  if (this->IndexedLabelmapVolumeNodeId && !strcmp(oldID, this->IndexedLabelmapVolumeNodeId))
    {
    this->SetAndObserveIndexedLabelmapVolumeNodeId(newID);
    }
  if (this->ClosedSurfaceModelNodeId && !strcmp(oldID, this->ClosedSurfaceModelNodeId))
    {
    this->SetAndObserveClosedSurfaceModelNodeId(newID);
    }
  if (this->BitfieldLabelmapVolumeNodeId && !strcmp(oldID, this->BitfieldLabelmapVolumeNodeId))
    {
    this->SetAndObserveBitfieldLabelmapVolumeNodeId(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

  this->SetAndObserveRibbonModelNodeId(this->RibbonModelNodeId);
  this->SetAndObserveIndexedLabelmapVolumeNodeId(this->IndexedLabelmapVolumeNodeId);
  this->SetAndObserveClosedSurfaceModelNodeId(this->ClosedSurfaceModelNodeId);
  this->SetAndObserveBitfieldLabelmapVolumeNodeId(this->BitfieldLabelmapVolumeNodeId);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "RibbonModelNodeId:   " << this->RibbonModelNodeId << "\n";
  os << indent << "IndexedLabelmapVolumeNodeId:   " << this->IndexedLabelmapVolumeNodeId << "\n";
  os << indent << "ClosedSurfaceModelNodeId:   " << this->ClosedSurfaceModelNodeId << "\n";
  os << indent << "BitfieldLabelmapVolumeNodeId:   " << this->BitfieldLabelmapVolumeNodeId << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRibbonModelNodeId(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->RibbonModelNode, NULL);
  this->SetRibbonModelNodeId(nodeID);
  vtkMRMLModelNode *tnode = this->GetRibbonModelNode();

  if (tnode)
    {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->RibbonModelNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set RibbonModel node ID!");
    this->SetRibbonModelNodeId(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLContourNode::GetRibbonModelNode()
{
  vtkMRMLModelNode* node = NULL;
  if (this->GetScene() && this->RibbonModelNodeId != NULL )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->RibbonModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
    }

  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveIndexedLabelmapVolumeNodeId(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->IndexedLabelmapVolumeNode, NULL);
  this->SetIndexedLabelmapVolumeNodeId(nodeID);
  vtkMRMLScalarVolumeNode *tnode = this->GetIndexedLabelmapVolumeNode();

  if (tnode)
    {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->IndexedLabelmapVolumeNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set IndexedLabelmap node ID!");
    this->SetIndexedLabelmapVolumeNodeId(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourNode::GetIndexedLabelmapVolumeNode()
{
  vtkMRMLScalarVolumeNode* node = NULL;
  if (this->GetScene() && this->IndexedLabelmapVolumeNodeId != NULL )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->IndexedLabelmapVolumeNodeId);
    node = vtkMRMLScalarVolumeNode::SafeDownCast(snode);
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveClosedSurfaceModelNodeId(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->ClosedSurfaceModelNode, NULL);
  this->SetClosedSurfaceModelNodeId(nodeID);
  vtkMRMLModelNode *tnode = this->GetClosedSurfaceModelNode();

  if (tnode)
    {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->ClosedSurfaceModelNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set ClosedSurface node ID!");
    this->SetClosedSurfaceModelNodeId(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLContourNode::GetClosedSurfaceModelNode()
{
  vtkMRMLModelNode* node = NULL;
  if (this->GetScene() && this->ClosedSurfaceModelNodeId != NULL )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->ClosedSurfaceModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveBitfieldLabelmapVolumeNodeId(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->BitfieldLabelmapVolumeNode, NULL);
  this->SetBitfieldLabelmapVolumeNodeId(nodeID);
  vtkMRMLScalarVolumeNode *tnode = this->GetBitfieldLabelmapVolumeNode();

  if (tnode)
    {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->BitfieldLabelmapVolumeNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set BitfieldLabelmap node ID!");
    this->SetBitfieldLabelmapVolumeNodeId(NULL);
    }
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourNode::GetBitfieldLabelmapVolumeNode()
{
  vtkMRMLScalarVolumeNode* node = NULL;
  if (this->GetScene() && this->BitfieldLabelmapVolumeNodeId != NULL )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->BitfieldLabelmapVolumeNodeId);
    node = vtkMRMLScalarVolumeNode::SafeDownCast(snode);
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetDefaultRepresentationByObject(vtkMRMLDisplayableNode *node)
{
  vtkMRMLScene* mrmlScene = this->GetScene();
  if (!node || !mrmlScene)
    {
    return;
    }

  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  vtkMRMLModelNode* ribbonModelNode = this->GetRibbonModelNode();
  if (ribbonModelNode)
    {
    ribbonModelNode->HideFromEditorsOn();
    }
  vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode = this->GetIndexedLabelmapVolumeNode();
  if (indexedLabelmapVolumeNode)
    {
    indexedLabelmapVolumeNode->HideFromEditorsOn();
    }
  vtkMRMLModelNode* closedSurfaceModelNode = this->GetClosedSurfaceModelNode();
  if (closedSurfaceModelNode)
    {
    closedSurfaceModelNode->HideFromEditorsOn();
    }
  vtkMRMLScalarVolumeNode* bitfieldLabelmapVolumeNode = this->GetBitfieldLabelmapVolumeNode();
  if (bitfieldLabelmapVolumeNode)
    {
    bitfieldLabelmapVolumeNode->HideFromEditorsOn();
    }

  if (this->RibbonModelNodeId
      && !strcmp(this->RibbonModelNodeId, node->GetID()))
    {
    ribbonModelNode->HideFromEditorsOff();
    }
  else if (this->IndexedLabelmapVolumeNodeId
      && !strcmp(this->IndexedLabelmapVolumeNodeId, node->GetID()))
    {
    indexedLabelmapVolumeNode->HideFromEditorsOff();
    }
  else if (this->ClosedSurfaceModelNodeId
      && !strcmp(this->ClosedSurfaceModelNodeId, node->GetID()))
    {
    closedSurfaceModelNode->HideFromEditorsOff();
    }
  else if (this->BitfieldLabelmapVolumeNodeId
      && !strcmp(this->BitfieldLabelmapVolumeNodeId, node->GetID()))
    {
    bitfieldLabelmapVolumeNode->HideFromEditorsOff();
    }
  else
    {
      vtkWarningMacro("Failed to set default representation: given node is not one of the referenced representations!");
    }

  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);
}
