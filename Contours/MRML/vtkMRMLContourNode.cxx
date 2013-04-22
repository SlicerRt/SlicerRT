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
#include "SlicerRtCommon.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkLabelmapToModelFilter.h"

// Contours includes
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"
#include "vtkConvertContourRepresentations.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLDisplayNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkCollection.h>

// STD includes
#include <algorithm> //TODO: workaround for issue #179

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLContourNode);

//----------------------------------------------------------------------------
vtkMRMLContourNode::vtkMRMLContourNode()
{
  this->StructureName = NULL;

  this->RibbonModelNode = NULL;
  this->RibbonModelNodeId = NULL;

  this->IndexedLabelmapVolumeNode = NULL;
  this->IndexedLabelmapVolumeNodeId = NULL;

  this->ClosedSurfaceModelNode = NULL;
  this->ClosedSurfaceModelNodeId = NULL;

  this->ActiveRepresentationType = None;

  this->RasterizationReferenceVolumeNodeId = NULL;

  this->RasterizationOversamplingFactor = -1.0;
  this->DecimationTargetReductionFactor = -1.0;

  this->HideFromEditorsOff();

  // Register parent transform modified event so that the representations
  //   can be put under the same transform node
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkObserveMRMLObjectEventsMacro(this, events);
}

//----------------------------------------------------------------------------
vtkMRMLContourNode::~vtkMRMLContourNode()
{
  this->SetAndObserveRibbonModelNodeId(NULL);
  this->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
  this->SetAndObserveClosedSurfaceModelNodeId(NULL);
  this->SetRasterizationReferenceVolumeNodeId(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->StructureName != NULL) 
    {
    of << indent << " StructureName=\"" << this->StructureName << "\"";
    }
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
  if (this->RasterizationReferenceVolumeNodeId != NULL) 
    {
    of << indent << " RasterizationReferenceVolumeNodeId=\"" << this->RasterizationReferenceVolumeNodeId << "\"";
    }
  of << indent << " ActiveRepresentationType=\"" << (int)this->ActiveRepresentationType << "\"";

  of << indent << " RasterizationOversamplingFactor=\"" << this->RasterizationOversamplingFactor << "\"";
  of << indent << " DecimationTargetReductionFactor=\"" << this->DecimationTargetReductionFactor << "\"";
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

    if (!strcmp(attName, "StructureName")) 
      {
      this->SetStructureName(attValue);
      }
    else if (!strcmp(attName, "RibbonModelNodeId")) 
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
    else if (!strcmp(attName, "RasterizationReferenceVolumeNodeId")) 
      {
      this->SetAndObserveRasterizationReferenceVolumeNodeId(attValue);
      }
    else if (!strcmp(attName, "ActiveRepresentationType")) 
      {
      std::stringstream ss;
      ss << attValue;
      int intAttValue;
      ss >> intAttValue;
      this->ActiveRepresentationType = (ContourRepresentationType)intAttValue;
      }
    else if (!strcmp(attName, "RasterizationOversamplingFactor")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->RasterizationOversamplingFactor = doubleAttValue;
      }
    else if (!strcmp(attName, "DecimationTargetReductionFactor")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->DecimationTargetReductionFactor = doubleAttValue;
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

  this->SetStructureName( node->GetStructureName() );

  // Observers must be removed here, otherwise MRML updates would activate nodes on the undo stack
  this->SetAndObserveRibbonModelNodeId( NULL );
  this->SetRibbonModelNodeId( node->RibbonModelNodeId );

  this->SetAndObserveIndexedLabelmapVolumeNodeId( NULL );
  this->SetIndexedLabelmapVolumeNodeId( node->IndexedLabelmapVolumeNodeId );

  this->SetAndObserveClosedSurfaceModelNodeId( NULL );
  this->SetClosedSurfaceModelNodeId( node->ClosedSurfaceModelNodeId );

  this->ActiveRepresentationType = node->GetActiveRepresentationType();

  this->SetAndObserveRasterizationReferenceVolumeNodeId( node->RasterizationReferenceVolumeNodeId );

  this->SetRasterizationOversamplingFactor( node->RasterizationOversamplingFactor );
  this->SetDecimationTargetReductionFactor( node->DecimationTargetReductionFactor );

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
  if (this->RasterizationReferenceVolumeNodeId != NULL && this->Scene->GetNodeByID(this->RasterizationReferenceVolumeNodeId) == NULL)
    {
    this->SetAndObserveRasterizationReferenceVolumeNodeId(NULL);
    }

  // Set a new active representation if the current one was deleted
  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();
  if ( this->ActiveRepresentationType != None && !representations[this->ActiveRepresentationType] )
    {
    for (int i=0; i<NumberOfRepresentationTypes; ++i)
      {
      if ( representations[i] )
        {
        this->SetActiveRepresentationByType( (ContourRepresentationType)i );
        break;
        }
      }
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
  if (this->RasterizationReferenceVolumeNodeId && !strcmp(oldID, this->RasterizationReferenceVolumeNodeId))
    {
    this->SetAndObserveRasterizationReferenceVolumeNodeId(newID);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

  this->SetAndObserveRibbonModelNodeId(this->RibbonModelNodeId);
  this->SetAndObserveIndexedLabelmapVolumeNodeId(this->IndexedLabelmapVolumeNodeId);
  this->SetAndObserveClosedSurfaceModelNodeId(this->ClosedSurfaceModelNodeId);
  this->SetAndObserveRasterizationReferenceVolumeNodeId(this->RasterizationReferenceVolumeNodeId);
  if (this->ActiveRepresentationType != None)
    {
    this->SetActiveRepresentationByType(this->ActiveRepresentationType);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  os << indent << "RibbonModelNodeId:   " << this->RibbonModelNodeId << "\n";
  os << indent << "IndexedLabelmapVolumeNodeId:   " << this->IndexedLabelmapVolumeNodeId << "\n";
  os << indent << "ClosedSurfaceModelNodeId:   " << this->ClosedSurfaceModelNodeId << "\n";
  os << indent << "RasterizationReferenceVolumeNodeId:   " << this->RasterizationReferenceVolumeNodeId << "\n";
  os << indent << "ActiveRepresentationType:   " << this->ActiveRepresentationType << "\n";
  os << indent << "RasterizationOversamplingFactor:   " << this->RasterizationOversamplingFactor << "\n";
  os << indent << "DecimationTargetReductionFactor:   " << this->DecimationTargetReductionFactor << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ProcessMRMLEvents(vtkObject *caller, unsigned long eventID, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, eventID, callData);

  if (!this->Scene)
    {
    vtkErrorMacro("ProcessMRMLEvents: Invalid MRML scene!");
    return;
    }

  // Representation internal data changed
  if (eventID == vtkMRMLModelNode::PolyDataModifiedEvent || eventID == vtkMRMLVolumeNode::ImageDataModifiedEvent)
    {
    vtkMRMLModelNode* callerModelNode = vtkMRMLModelNode::SafeDownCast(caller);
    vtkMRMLVolumeNode* callerVolumeNode = vtkMRMLVolumeNode::SafeDownCast(caller);
    if (!callerModelNode && !callerVolumeNode)
      {
      return;
      }

    // Either the ribbon model or the closed surface model has been modified
    if (eventID == vtkMRMLModelNode::PolyDataModifiedEvent && callerModelNode)
      {
      if (this->ActiveRepresentationType != RibbonModel && this->ActiveRepresentationType != ClosedSurfaceModel)
        {
        vtkErrorMacro("Non-active contour representation is not supposed to be able to be modified!");
        return;
        }
      }
    // The indexed labelmap has been modified
    else if (eventID == vtkMRMLVolumeNode::ImageDataModifiedEvent && callerVolumeNode)
      {
      if (this->ActiveRepresentationType != IndexedLabelmap)
        {
        vtkErrorMacro("Non-active contour representation is not supposed to be able to be modified!");
        return;
        }
      }
    else
      {
      vtkErrorMacro("Caller node - event type mismatch!");
      return;
      }

    // The active representation has been modified, and both the caller object and the event is fine:
    //   Delete all non-active representations 
    this->DeleteNonActiveRepresentations();
    }
  // Parent transform changed (of this contour node or one of the representations)
  else if (eventID == vtkMRMLTransformableNode::TransformModifiedEvent)
    {
    vtkMRMLTransformableNode* callerNode = vtkMRMLTransformableNode::SafeDownCast(caller);
    if ( !callerNode
      || !( caller->IsA("vtkMRMLContourNode") || caller->IsA("vtkMRMLModelNode") || caller->IsA("vtkMRMLScalarVolumeNode")) )
      {
      return;
      }

    const char* newTransformNodeId = callerNode->GetTransformNodeID();

    // Set the parent transform to this contour node
    if ( ( (!this->GetTransformNodeID() || !newTransformNodeId) && (this->GetTransformNodeID() != newTransformNodeId) )
      || ( this->GetTransformNodeID() && newTransformNodeId && STRCASECMP(this->GetTransformNodeID(), newTransformNodeId) ) )
      {
      this->SetAndObserveTransformNodeID(newTransformNodeId);
      }

    // Set the parent transform to the representations
    std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();
    for (int i=0; i<NumberOfRepresentationTypes; ++i)
      {
      if ( representations[i]
        && ( ( (!representations[i]->GetTransformNodeID() || !newTransformNodeId) && (representations[i]->GetTransformNodeID() != newTransformNodeId) )
          || ( representations[i]->GetTransformNodeID() && newTransformNodeId && STRCASECMP(representations[i]->GetTransformNodeID(), newTransformNodeId) ) ) )
        {
        representations[i]->SetAndObserveTransformNodeID(newTransformNodeId);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRibbonModelNodeIdOnly(const char *nodeID)
{
  if (this->RibbonModelContainsEmptyPolydata() && this->Scene)
    {
    this->Scene->RemoveNode(this->RibbonModelNode);
    }
  vtkSetAndObserveMRMLObjectMacro(this->RibbonModelNode, NULL);
  this->SetRibbonModelNodeId(nodeID);
  if (!nodeID)
    {
    return;
    }

  vtkMRMLModelNode *tnode = this->GetRibbonModelNode();
  if (tnode)
    {
    tnode->HideFromEditorsOn();
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLModelNode::PolyDataModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->RibbonModelNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set RibbonModel node ID!");
    this->SetRibbonModelNodeId(NULL);
    }
  this->SetActiveRepresentationByType(RibbonModel);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRibbonModelNodeId(const char *nodeID)
{
  this->SetAndObserveRibbonModelNodeIdOnly(nodeID);
  if (nodeID)
    {
    this->DeleteNonActiveRepresentations();
    }
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLContourNode::GetRibbonModelNode()
{
  vtkMRMLModelNode* node = NULL;
  if (this->Scene && this->RibbonModelNodeId != NULL )
    {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->RibbonModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
    }

  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveIndexedLabelmapVolumeNodeIdOnly(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->IndexedLabelmapVolumeNode, NULL);
  this->SetIndexedLabelmapVolumeNodeId(nodeID);
  if (!nodeID)
    {
    return;
    }

  vtkMRMLScalarVolumeNode *tnode = this->GetIndexedLabelmapVolumeNode();
  if (tnode)
    {
    tnode->HideFromEditorsOn();
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLVolumeNode::ImageDataModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->IndexedLabelmapVolumeNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set IndexedLabelmap node ID!");
    this->SetIndexedLabelmapVolumeNodeId(NULL);
    }
  this->SetActiveRepresentationByType(IndexedLabelmap);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveIndexedLabelmapVolumeNodeId(const char *nodeID)
{
  this->SetAndObserveIndexedLabelmapVolumeNodeIdOnly(nodeID);
  if (nodeID)
    {
    this->DeleteNonActiveRepresentations();
    }
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourNode::GetIndexedLabelmapVolumeNode()
{
  vtkMRMLScalarVolumeNode* node = NULL;
  if (!this->Scene)
    {
    vtkErrorMacro("GetIndexedLabelmapVolumeNode: Invalid MRML scene!");
    return node;
    }
  else if (this->IndexedLabelmapVolumeNodeId != NULL )
    {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->IndexedLabelmapVolumeNodeId);
    node = vtkMRMLScalarVolumeNode::SafeDownCast(snode);
    }
  else if (this->RasterizationReferenceVolumeNodeId != NULL)
    {
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(this);
    if (converter->ConvertToRepresentation(IndexedLabelmap))
      {
      node = this->IndexedLabelmapVolumeNode;
      }
    else
      {
      vtkErrorMacro("Conversion to indexed labelmap failed!");
      }
    }
  else
    {
    vtkErrorMacro("Unable to convert to indexed labelmap representation without a reference volume!");
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveClosedSurfaceModelNodeIdOnly(const char *nodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->ClosedSurfaceModelNode, NULL);
  this->SetClosedSurfaceModelNodeId(nodeID);
  if (!nodeID)
    {
    return;
    }

  vtkMRMLModelNode *tnode = this->GetClosedSurfaceModelNode();
  if (tnode)
    {
    tnode->HideFromEditorsOn();
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    events->InsertNextValue(vtkMRMLModelNode::PolyDataModifiedEvent);
    vtkSetAndObserveMRMLObjectEventsMacro(this->ClosedSurfaceModelNode, tnode, events);
    }
  else
    {
    vtkErrorMacro("Failed to set ClosedSurface node ID!");
    this->SetClosedSurfaceModelNodeId(NULL);
    }
  this->SetActiveRepresentationByType(ClosedSurfaceModel);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveClosedSurfaceModelNodeId(const char *nodeID)
{
  this->SetAndObserveClosedSurfaceModelNodeIdOnly(nodeID);
  if (nodeID)
    {
    this->DeleteNonActiveRepresentations();
    }
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLContourNode::GetClosedSurfaceModelNode()
{
  vtkMRMLModelNode* node = NULL;
  if (!this->Scene)
    {
    vtkErrorMacro("GetClosedSurfaceModelNode: Invalid MRML scene!");
    return node;
    }
  else if (this->ClosedSurfaceModelNodeId != NULL )
    {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->ClosedSurfaceModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
    }
  else
    {
    vtkSmartPointer<vtkConvertContourRepresentations> converter = vtkSmartPointer<vtkConvertContourRepresentations>::New();
    converter->SetContourNode(this);
    if (converter->ConvertToRepresentation(ClosedSurfaceModel))
      {
      node = this->ClosedSurfaceModelNode;
      }
    else
      {
      vtkErrorMacro("Conversion to closed surface model failed!");
      }
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRasterizationReferenceVolumeNodeId(const char* id)
{
  if (this->RasterizationReferenceVolumeNodeId && this->Scene)
    {
    this->Scene->RemoveReferencedNodeID(this->RasterizationReferenceVolumeNodeId, this);
    }

  this->SetRasterizationReferenceVolumeNodeId(id);

  if (id && this->Scene)
    {
    this->Scene->AddReferencedNodeID(this->RasterizationReferenceVolumeNodeId, this);
    }
}


//----------------------------------------------------------------------------
std::vector<vtkMRMLDisplayableNode*> vtkMRMLContourNode::CreateTemporaryRepresentationsVector()
{
  std::vector<vtkMRMLDisplayableNode*> representations(NumberOfRepresentationTypes, NULL);
  if (this->RibbonModelNode)
    {
    representations[RibbonModel] = this->RibbonModelNode;
    }
  if (this->IndexedLabelmapVolumeNode)
    {
    representations[IndexedLabelmap] = this->IndexedLabelmapVolumeNode;
    }
  if (this->ClosedSurfaceModelNode)
    {
    representations[ClosedSurfaceModel] = this->ClosedSurfaceModelNode;
    }
  return representations;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetActiveRepresentationByNode(vtkMRMLDisplayableNode *node)
{
  vtkMRMLScene* mrmlScene = this->Scene;
  if (!node || !mrmlScene)
    {
    vtkErrorMacro("SetActiveRepresentationByNode: Invalid MRML scene or argument node!");
    return;
    }

  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();

  // Check whether the argument node is referenced, because we don't 
  //   want to start hiding nodes until we know we have one to show
  ContourRepresentationType foundType = None;
  for (unsigned int i=0; i<NumberOfRepresentationTypes; ++i)
    {
    if (representations[i] == node)
      {
      foundType = ContourRepresentationType(i);
      break;
      }
    }
  if (foundType == None)
    {
    vtkErrorMacro("Failed to set active representation: given node is not one of the referenced representations!");
    return;
    }

  this->SetActiveRepresentationByType(foundType);
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetActiveRepresentationByType(ContourRepresentationType type)
{
  vtkMRMLScene* mrmlScene = this->Scene;
  if (!mrmlScene)
    {
    vtkErrorMacro("SetActiveRepresentationByType: Invalid MRML scene!");
    return;
    }
  if (type == None)
    {
    vtkWarningMacro("Cannot set 'None' as representation type!");
    return;
    }

  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();

  // Show only the active representation and set active representation type
  bool success = false;
  for (int i=0; i<NumberOfRepresentationTypes; ++i)
    {
    if (i == type)
      {
      if (representations[i])
        {
        this->ShowRepresentation(representations[i], true);
        success = true;
        }
      else
        {
        vtkErrorMacro("Failed to set active representation: given representation type has no referenced node!");
        }
      }
    else if (representations[i])
      {
      this->ShowRepresentation(representations[i], false);
      }
    }

  // Make sure the original representation type is shown if unable to set the new one
  if (!success && representations[this->ActiveRepresentationType])
    {
    this->ShowRepresentation(representations[this->ActiveRepresentationType], true);
    }
  else
    {
    this->ActiveRepresentationType = type; // Set the type when there is no valid pointer (yet)
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ShowRepresentation(vtkMRMLDisplayableNode* representation, bool show)
{
  representation->SetHideFromEditors((!show)?1:0);
  representation->SetDisplayVisibility(show?1:0);
  
  vtkMRMLDisplayNode* displayNode = representation->GetDisplayNode();
  if (displayNode)
  {
    displayNode->SetSliceIntersectionVisibility(show?1:0);
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLContourNode::RepresentationExists( ContourRepresentationType type )
{
  switch (type)
    {
    case RibbonModel:
      return this->RibbonModelNodeId ? true : false;
    case IndexedLabelmap:
      return this->IndexedLabelmapVolumeNodeId ? true : false;
    case ClosedSurfaceModel:
      return this->ClosedSurfaceModelNodeId ? true : false;
    default:
      return false;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetColorIndex(int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLModelNode* referenceModelNode/*=NULL*/)
{
  // Initialize output color index with Gray 'invalid' color
  colorIndex = 1;
  colorNode = NULL;

  // Get hierarchy node
  vtkMRMLDisplayableHierarchyNode* hierarchyNode = 
    vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->Scene, this->ID);
  if (!hierarchyNode)
    {
    vtkErrorMacro("Error: No hierarchy node found for structure '" << this->Name << "'");
    return;
    }

  // Get color node created for the structure set
  vtkMRMLContourHierarchyNode* parentContourHierarchyNode = vtkMRMLContourHierarchyNode::SafeDownCast(hierarchyNode->GetParentNode());

  std::string seriesName = parentContourHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME.c_str());
  std::string colorNodeName = seriesName + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  vtkSmartPointer<vtkCollection> colorNodes = vtkSmartPointer<vtkCollection>::Take( this->Scene->GetNodesByName(colorNodeName.c_str()) );
  if (colorNodes->GetNumberOfItems() == 0)
    {
    vtkErrorMacro("Error: No color table found for structure set '" << parentContourHierarchyNode->GetName() << "'");
    }

  int structureColorIndex = -1;
  if (!this->StructureName)
    {
    vtkErrorMacro("Unable to find color index for contour '" << this->Name << "', because structure name is empty!");
    return;
    }

  vtkObject* nextObject = NULL;
  for (colorNodes->InitTraversal(); (nextObject = colorNodes->GetNextItemAsObject()); )
    {
    vtkMRMLColorTableNode* currentColorNode = vtkMRMLColorTableNode::SafeDownCast(nextObject);
    int currentColorIndex = -1;

    //TODO: workaround for issue #179, restore when Slicer mantis issue http://www.na-mic.org/Bug/view.php?id=2783 is fixed
    // Try to search for the structure name with the spaces replaced with underscores in case it was loaded from scene file
    std::string structureNameWithUnderscores(this->StructureName);
    std::replace(structureNameWithUnderscores.begin(), structureNameWithUnderscores.end(), ' ', '_');
    
    if ( (currentColorIndex = currentColorNode->GetColorIndexByName(this->StructureName)) != -1
      || (currentColorIndex = currentColorNode->GetColorIndexByName(structureNameWithUnderscores.c_str())) != -1 )
      {
      if (referenceModelNode)
        {
        double modelColor[3];
        double foundColor[4];
        referenceModelNode->GetDisplayNode()->GetColor(modelColor);
        currentColorNode->GetColor(currentColorIndex, foundColor);
        if ((fabs(modelColor[0]-foundColor[0]) < EPSILON) && (fabs(modelColor[1]-foundColor[1]) < EPSILON) && (fabs(modelColor[2]-foundColor[2])) < EPSILON)
          {
          structureColorIndex = currentColorIndex;
          colorNode = currentColorNode;
          break;
          }
        }
      else
        {
        structureColorIndex = currentColorIndex;
        colorNode = currentColorNode;
        break;
        }
      }
    }

  if (structureColorIndex == -1)
    {
    vtkErrorMacro("No matching entry found in the color tables for structure '" << this->StructureName << "'");
    return;
    }

  colorIndex = structureColorIndex;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetDefaultConversionParametersForRepresentation(ContourRepresentationType type)
{
  if (type == IndexedLabelmap || type == ClosedSurfaceModel)
    {
    if (this->RasterizationOversamplingFactor == -1.0)
      {
      this->SetRasterizationOversamplingFactor(SlicerRtCommon::DEFAULT_RASTERIZATION_OVERSAMPLING_FACTOR);
      }
    }
  else if (type == ClosedSurfaceModel)
    {
    if (this->DecimationTargetReductionFactor == -1.0)
      {
      this->SetDecimationTargetReductionFactor(SlicerRtCommon::DEFAULT_DECIMATION_TARGET_REDUCTION_FACTOR);
      }
    }
}

//---------------------------------------------------------------------------
int vtkMRMLContourNode::GetDisplayVisibility()
{
  if (this->ActiveRepresentationType == None)
  {
    return 0;
  }

  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();
  vtkMRMLDisplayableNode* activeRepresentation = representations[this->ActiveRepresentationType];
  if (!activeRepresentation)
  {
    vtkErrorMacro("Invalid active contour representation for contour node " << this->Name);
    return 0;
  }

  return activeRepresentation->GetDisplayVisibility();
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetDisplayVisibility(int visible)
{
  if ( (visible != 0 && visible != 1) || this->ActiveRepresentationType == None )
  {
    return;
  }

  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();
  vtkMRMLDisplayableNode* activeRepresentation = representations[this->ActiveRepresentationType];
  if (!activeRepresentation)
  {
    vtkErrorMacro("Invalid active contour representation for contour node " << this->Name);
    return;
  }

  activeRepresentation->SetDisplayVisibility(visible);

  vtkMRMLDisplayNode* displayNode = activeRepresentation->GetDisplayNode();
  if (displayNode)
  {
    displayNode->SetSliceIntersectionVisibility(visible);
  }
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::SetName(const char* newName)
{
  if ( this->Name == NULL && newName == NULL) { return; }
  if ( this->Name && newName && (!strcmp(this->Name,newName))) { return; }
  if (this->Name) { delete [] this->Name; }
  if (newName)
  {
    size_t n = strlen(newName) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (newName);
    this->Name = cp1;
    do { *cp1++ = *cp2++; } while ( --n );

    // Set new name to representations
    if (this->RibbonModelNode)
    {
      std::string newRibbonModelName(newName);
      newRibbonModelName.append(SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX);
      this->RibbonModelNode->SetName(newRibbonModelName.c_str());
    }
    if (this->IndexedLabelmapVolumeNode)
    {
      std::string newIndexedLabelmapName(newName);
      newIndexedLabelmapName.append(SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX);
      this->IndexedLabelmapVolumeNode->SetName(newIndexedLabelmapName.c_str());
    }
    if (this->ClosedSurfaceModelNode)
    {
      std::string newClosedSurfaceModelName(newName);
      newClosedSurfaceModelName.append(SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX);
      this->IndexedLabelmapVolumeNode->SetName(newClosedSurfaceModelName.c_str());
    }
  }
  else
  {
    this->Name = NULL;
  }
  this->Modified();
}

//---------------------------------------------------------------------------
vtkMRMLContourNode* vtkMRMLContourNode::IsNodeAContourRepresentation(vtkMRMLScene* scene, vtkMRMLNode* node)
{
  if (!scene)
  {
    std::cerr << "vtkMRMLContourNode::IsNodeAContourRepresentation: NULL argument given!" << std::endl;
    return NULL;
  }

  if (!node)
  {
    vtkErrorWithObjectMacro(scene, "IsNodeAContourRepresentation: Invalid nodeID argument!");
    return NULL;
  }

  // If the node is neither a model nor a volume, the it cannot be a representation
  if (!node->IsA("vtkMRMLModelNode") && !node->IsA("vtkMRMLVolumeNode"))
  {
    return NULL;
  }

  // Check every contour node if they have the argument node among the representations
  const char* nodeID = node->GetID();
  vtkSmartPointer<vtkCollection> contourNodes = vtkSmartPointer<vtkCollection>::Take( scene->GetNodesByClass("vtkMRMLContourNode") );
  vtkObject* nextObject = NULL;
  for (contourNodes->InitTraversal(); (nextObject = contourNodes->GetNextItemAsObject()); )
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(nextObject);
    if ( (contourNode->GetRibbonModelNodeId() && !STRCASECMP(contourNode->GetRibbonModelNodeId(), nodeID))
      || (contourNode->GetIndexedLabelmapVolumeNodeId() && !STRCASECMP(contourNode->GetIndexedLabelmapVolumeNodeId(), nodeID))
      || (contourNode->GetClosedSurfaceModelNodeId() && !STRCASECMP(contourNode->GetClosedSurfaceModelNodeId(), nodeID)) )
    {
      return contourNode;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
bool vtkMRMLContourNode::RibbonModelContainsEmptyPolydata()
{
  return ( this->RibbonModelNode && this->RibbonModelNode->GetPolyData()
        && this->RibbonModelNode->GetPolyData()->GetNumberOfPoints() == 0 );
}

//---------------------------------------------------------------------------
void vtkMRMLContourNode::DeleteNonActiveRepresentations()
{
  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();
  for (int i=0; i<NumberOfRepresentationTypes; ++i)
  {
    if (i != this->ActiveRepresentationType && representations[i])
    {
      vtkMRMLDisplayableNode* node = representations[i];
      switch ( (ContourRepresentationType)(i) )
      {
      case RibbonModel:
        this->SetAndObserveRibbonModelNodeId(NULL);
        break;
      case IndexedLabelmap:
        this->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
        break;
      case ClosedSurfaceModel:
        this->SetAndObserveClosedSurfaceModelNodeId(NULL);
        break;
      default:
        break;
      }

      if (this->Scene->IsNodePresent(node))
      {
        this->Scene->RemoveNode(node);
      }
    }
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLContourNode::HasBeenCreatedFromIndexedLabelmap()
{
  return ( this->IndexedLabelmapVolumeNode
    && !SlicerRtCommon::IsStringNullOrEmpty(this->RasterizationReferenceVolumeNodeId)
    && !STRCASECMP(this->IndexedLabelmapVolumeNodeId, this->RasterizationReferenceVolumeNodeId) );
}
