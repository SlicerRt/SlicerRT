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
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkLabelmapToModelFilter.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageReslice.h>
#include <vtkGeneralTransform.h>
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
  if ( !representations[this->ActiveRepresentationType] )
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
  this->SetActiveRepresentationByType(this->ActiveRepresentationType);
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

  if (this->Scene == NULL)
    {
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

        this->Scene->RemoveNode(node);
        }
      }
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
      if ( ( this->TransformNodeID || newTransformNodeId )
        && ( this->TransformNodeID != newTransformNodeId || STRCASECMP(this->TransformNodeID, newTransformNodeId) ) )
        {
        this->SetAndObserveTransformNodeID(newTransformNodeId);
        }

      // Set the parent transform to the representations
      std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();
      for (int i=0; i<NumberOfRepresentationTypes; ++i)
        {
        if ( representations[i] && (representations[i]->GetTransformNodeID() || newTransformNodeId)
          && ( representations[i]->GetTransformNodeID() != newTransformNodeId
            || STRCASECMP(representations[i]->GetTransformNodeID(), newTransformNodeId) ) )
          {
          representations[i]->SetAndObserveTransformNodeID(newTransformNodeId);
          }
        }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::SetAndObserveRibbonModelNodeId(const char *nodeID)
{
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
void vtkMRMLContourNode::SetAndObserveIndexedLabelmapVolumeNodeId(const char *nodeID)
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
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourNode::GetIndexedLabelmapVolumeNode()
{
  vtkMRMLScalarVolumeNode* node = NULL;
  if (!this->Scene)
    {
    return node;
    }
  else if (this->IndexedLabelmapVolumeNodeId != NULL )
    {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->IndexedLabelmapVolumeNodeId);
    node = vtkMRMLScalarVolumeNode::SafeDownCast(snode);
    }
  else if (this->RasterizationReferenceVolumeNodeId != NULL)
    {
    if (this->ConvertToRepresentation(IndexedLabelmap))
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
void vtkMRMLContourNode::SetAndObserveClosedSurfaceModelNodeId(const char *nodeID)
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
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLContourNode::GetClosedSurfaceModelNode()
{
  vtkMRMLModelNode* node = NULL;
  if (!this->Scene)
    {
    return node;
    }
  else if (this->ClosedSurfaceModelNodeId != NULL )
    {
    vtkMRMLNode* snode = this->Scene->GetNodeByID(this->ClosedSurfaceModelNodeId);
    node = vtkMRMLModelNode::SafeDownCast(snode);
    }
  else
    {
    if (this->ConvertToRepresentation(ClosedSurfaceModel))
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

  // Make sure the original representation type is shown on failure to set the new one
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
  
  vtkMRMLDisplayNode* displayNode;
  if ( (displayNode = representation->GetDisplayNode()) != NULL )
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
bool vtkMRMLContourNode::ConvertToRepresentation(ContourRepresentationType type)
{
  if (type == this->ActiveRepresentationType)
  {
    return true;
  }

  // Set default parameters if none specified
  this->SetDefaultConversionParametersForRepresentation(type);

  // Active representation is a model of any kind and we want an indexed labelmap
  if ( ( this->ActiveRepresentationType == RibbonModel
      || this->ActiveRepresentationType == ClosedSurfaceModel )
    && type == IndexedLabelmap )
    {
    if (!this->RasterizationReferenceVolumeNodeId)
      {
      vtkErrorMacro("Unable to convert to indexed labelmap without a reference volume node!");
      return false;
      }

    vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode = this->ConvertFromModelToIndexedLabelmap(
      (this->RibbonModelNode ? this->RibbonModelNode : this->ClosedSurfaceModelNode) );

    return (indexedLabelmapVolumeNode != NULL);
    }
  // Active representation is an indexed labelmap and we want a closed surface model
  else if ( this->ActiveRepresentationType == IndexedLabelmap && type == ClosedSurfaceModel )
    {
    vtkMRMLModelNode* closedSurfaceVolumeNode
      = this->ConvertFromIndexedLabelmapToClosedSurfaceModel(this->IndexedLabelmapVolumeNode);

    return (closedSurfaceVolumeNode != NULL);
    }
  // Active representation is a ribbon model and we want a closed surface model
  else if (this->ActiveRepresentationType == RibbonModel && type == ClosedSurfaceModel)
    {
    // If the indexed labelmap is not created yet then we convert to it first
    if (!this->IndexedLabelmapVolumeNode)
      {
      if (!this->RasterizationReferenceVolumeNodeId)
        {
        vtkErrorMacro("Unable to convert to indexed labelmap without a reference volume node (it is needed to convert into closed surface model)!");
        return false;
        }
      if (this->ConvertFromModelToIndexedLabelmap(this->RibbonModelNode) == NULL)
        {
        vtkErrorMacro("Conversion to indexed labelmap failed (it is needed to convert into closed surface model)!");
        return false;
        }
      }

    vtkMRMLModelNode* closedSurfaceVolumeNode
      = this->ConvertFromIndexedLabelmapToClosedSurfaceModel(this->IndexedLabelmapVolumeNode);

    return (closedSurfaceVolumeNode != NULL);
    }
  else
    {
    vtkWarningMacro("Requested conversion not implemented yet!");
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::ReconvertRepresentation(ContourRepresentationType type)
{
  if (!this->Scene)
    {
    return;
    }
  if (type == None)
    {
    vtkWarningMacro("Cannot convert to 'None' representation type!");
    return;
    }

  // Not implemented yet, cannot be re-converted
  if (type == RibbonModel)
    {
    vtkWarningMacro("Convert to 'RibbonMode' representation type is not implemented yet!");
    return;
    }

  std::vector<vtkMRMLDisplayableNode*> representations = this->CreateTemporaryRepresentationsVector();

  // Delete current representation if it exists
  if (this->RepresentationExists(type))
    {
    vtkMRMLDisplayableNode* node = representations[(unsigned int)type];
    switch (type)
      {
      case IndexedLabelmap:
        this->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
        break;
      case ClosedSurfaceModel:
        this->SetAndObserveClosedSurfaceModelNodeId(NULL);
        break;
      default:
        break;
      }

    this->Scene->RemoveNode(node);
    }

  // Set active representation type to the one that can be the source of the new conversion
  switch (type)
    {
    case IndexedLabelmap:
      if (this->RibbonModelNode)
        {
        this->SetActiveRepresentationByType(RibbonModel);
        }
      else if (this->ClosedSurfaceModelNode)
        {
        this->SetActiveRepresentationByType(ClosedSurfaceModel);
        }
      break;
    case ClosedSurfaceModel:
      if (this->RibbonModelNode)
        {
        this->SetActiveRepresentationByType(RibbonModel);
        }
      else if (this->IndexedLabelmapVolumeNode)
        {
        this->SetActiveRepresentationByType(IndexedLabelmap);
        }
      break;
    default:
      break;
  }

  // Do the conversion as normally
  bool success = this->ConvertToRepresentation(type);
  if (!success)
    {
    vtkErrorMacro("Failed to re-convert representation to type #" << (unsigned int)type);
    }
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkMRMLContourNode::ConvertFromModelToIndexedLabelmap(vtkMRMLModelNode* modelNode)
{
  vtkMRMLScene* mrmlScene = this->Scene;
  if (!mrmlScene || !modelNode)
    {
    return NULL;
    }

  // Sanity check
  if ( this->RasterizationOversamplingFactor < 0.01
    || this->RasterizationOversamplingFactor > 100.0 )
    {
    vtkErrorMacro("Unreasonable rasterization oversampling factor is given: " << this->RasterizationOversamplingFactor);
    return NULL;
    }

  // Get reference volume node
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(this->RasterizationReferenceVolumeNodeId));
  if (!referenceVolumeNode)
    {
    vtkErrorMacro("Error: No reference volume node!");
    return NULL;
    }

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  this->GetColorIndex(structureColorIndex, colorNode, modelNode);

  // Create model to referenceIjk transform
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  this->GetTransformFromModelToVolumeIjk(modelNode, referenceVolumeNode, modelToReferenceVolumeIjkTransform);

  // Transform the model polydata to referenceIjk coordinate frame (the labelmap image coordinate frame is referenceIjk)
  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataModelToReferenceVolumeIjkFilter
    = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInput( modelNode->GetPolyData() );
  transformPolyDataModelToReferenceVolumeIjkFilter->SetTransform(modelToReferenceVolumeIjkTransform.GetPointer());

  // Convert model to labelmap
  vtkSmartPointer<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter = vtkSmartPointer<vtkPolyDataToLabelmapFilter>::New();
  transformPolyDataModelToReferenceVolumeIjkFilter->Update();
  polyDataToLabelmapFilter->SetBackgroundValue(0);
  polyDataToLabelmapFilter->SetLabelValue(structureColorIndex);
  polyDataToLabelmapFilter->UseReferenceValuesOff();
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToReferenceVolumeIjkFilter->GetOutput() );

  if (this->RasterizationOversamplingFactor != 1.0)
    {
    int outputExtent[6];
    double outputSpacing[3];
    SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor(referenceVolumeNode, this->RasterizationOversamplingFactor, outputExtent, outputSpacing);

    vtkSmartPointer<vtkImageReslice> reslicer = vtkSmartPointer<vtkImageReslice>::New();
    reslicer->SetInput(referenceVolumeNode->GetImageData());
    reslicer->SetInterpolationMode(VTK_RESLICE_LINEAR);
    reslicer->SetOutputExtent(outputExtent);
    reslicer->SetOutputSpacing(outputSpacing);
    reslicer->Update();

    polyDataToLabelmapFilter->SetReferenceImage( reslicer->GetOutput() );
    }
  else
    {
    polyDataToLabelmapFilter->SetReferenceImage( referenceVolumeNode->GetImageData() );
    }
  polyDataToLabelmapFilter->Update();    

  // Create indexed labelmap volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> indexedLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  indexedLabelmapVolumeNode->CopyOrientation( referenceVolumeNode );
  if (this->RasterizationOversamplingFactor != 1.0)
    {
    double* referenceSpacing = referenceVolumeNode->GetSpacing();
    indexedLabelmapVolumeNode->SetSpacing(
      referenceSpacing[0]/this->RasterizationOversamplingFactor,
      referenceSpacing[1]/this->RasterizationOversamplingFactor,
      referenceSpacing[2]/this->RasterizationOversamplingFactor );

    vtkImageData* indexedLabelmapVolumeImageData = polyDataToLabelmapFilter->GetOutput();
    indexedLabelmapVolumeImageData->SetSpacing(1.0, 1.0, 1.0); // The spacing is set to the MRML node
    }

  std::string indexedLabelmapVolumeNodeName = std::string(this->Name) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  indexedLabelmapVolumeNodeName = mrmlScene->GenerateUniqueName(indexedLabelmapVolumeNodeName);

  indexedLabelmapVolumeNode->SetAndObserveTransformNodeID( indexedLabelmapVolumeNode->GetTransformNodeID() );
  indexedLabelmapVolumeNode->SetName( indexedLabelmapVolumeNodeName.c_str() );
  indexedLabelmapVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  indexedLabelmapVolumeNode->LabelMapOn();
  mrmlScene->AddNode(indexedLabelmapVolumeNode);

  // Create display node
  vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode>::New();
  displayNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(mrmlScene->AddNode(displayNode));
  if (colorNode)
    {
    displayNode->SetAndObserveColorNodeID(colorNode->GetID());
    }
  else
    {
    displayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeLabels");
    }

  indexedLabelmapVolumeNode->SetAndObserveDisplayNodeID( displayNode->GetID() );

  // Set parent transform node
  if (this->GetTransformNodeID())
  {
    indexedLabelmapVolumeNode->SetAndObserveTransformNodeID(this->GetTransformNodeID());
  }

  this->SetAndObserveIndexedLabelmapVolumeNodeId(indexedLabelmapVolumeNode->GetID());

  return indexedLabelmapVolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLContourNode::ConvertFromIndexedLabelmapToClosedSurfaceModel(vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode)
{
  vtkMRMLScene* mrmlScene = this->Scene;
  if (!mrmlScene || !indexedLabelmapVolumeNode)
    {
    return NULL;
    }

  // TODO: Workaround for the the issue that slice intersections are not visible
  // of newly converted models
  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  this->GetColorIndex(structureColorIndex, colorNode);

  // Convert labelmap to model
  vtkSmartPointer<vtkLabelmapToModelFilter> labelmapToModelFilter = vtkSmartPointer<vtkLabelmapToModelFilter>::New();
  labelmapToModelFilter->SetInputLabelmap( indexedLabelmapVolumeNode->GetImageData() );
  labelmapToModelFilter->SetDecimateTargetReduction( this->DecimationTargetReductionFactor );
  labelmapToModelFilter->Update();    

  // Create display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mrmlScene->AddNode(displayNode));
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn();
  displayNode->SetBackfaceCulling(0);

  // Set visibility and opacity to match the existing ribbon model visualization
  if (this->RibbonModelNode && this->RibbonModelNode->GetModelDisplayNode())
    {
    displayNode->SetVisibility(this->RibbonModelNode->GetModelDisplayNode()->GetVisibility());
    displayNode->SetOpacity(this->RibbonModelNode->GetModelDisplayNode()->GetOpacity());
    }
  // Set color
  double color[4];
  if (colorNode)
    {
    colorNode->GetColor(structureColorIndex, color);
    displayNode->SetColor(color);
    }

  // Create closed surface model node
  vtkSmartPointer<vtkMRMLModelNode> closedSurfaceModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  closedSurfaceModelNode = vtkMRMLModelNode::SafeDownCast(mrmlScene->AddNode(closedSurfaceModelNode));

  std::string closedSurfaceModelNodeName = std::string(this->Name) + SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX;
  closedSurfaceModelNodeName = mrmlScene->GenerateUniqueName(closedSurfaceModelNodeName);

  closedSurfaceModelNode->SetName( closedSurfaceModelNodeName.c_str() );

  closedSurfaceModelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
  closedSurfaceModelNode->SetAndObserveTransformNodeID( indexedLabelmapVolumeNode->GetTransformNodeID() );

  // Create model to referenceIjk transform
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  this->GetTransformFromModelToVolumeIjk(closedSurfaceModelNode, indexedLabelmapVolumeNode, modelToReferenceVolumeIjkTransform);
  
  vtkSmartPointer<vtkGeneralTransform> referenceVolumeIjkToModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  referenceVolumeIjkToModelTransform->Concatenate(modelToReferenceVolumeIjkTransform);
  referenceVolumeIjkToModelTransform->Inverse();

  // Transform the model polydata to referenceIjk coordinate frame (the labelmap image coordinate frame is referenceIjk)
  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataModelToReferenceVolumeIjkFilter
    = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInput( labelmapToModelFilter->GetOutput() );
  transformPolyDataModelToReferenceVolumeIjkFilter->SetTransform(referenceVolumeIjkToModelTransform.GetPointer());
  transformPolyDataModelToReferenceVolumeIjkFilter->Update();

  closedSurfaceModelNode->SetAndObservePolyData( transformPolyDataModelToReferenceVolumeIjkFilter->GetOutput() );

  // Set parent transform node
  if (this->GetTransformNodeID())
    {
    closedSurfaceModelNode->SetAndObserveTransformNodeID(this->GetTransformNodeID());
    }

  // Put new model in the same model hierarchy as the ribbons
  if (this->RibbonModelNode && this->RibbonModelNodeId)
    {
    vtkMRMLModelHierarchyNode* ribbonModelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
      vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->Scene, this->RibbonModelNodeId));
    if (ribbonModelHierarchyNode)
      {
      vtkMRMLModelHierarchyNode* parentModelHierarchyNode
        = vtkMRMLModelHierarchyNode::SafeDownCast(ribbonModelHierarchyNode->GetParentNode());

      vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      this->Scene->AddNode(modelHierarchyNode);
      modelHierarchyNode->SetParentNodeID( parentModelHierarchyNode->GetID() );
      modelHierarchyNode->SetModelNodeID( closedSurfaceModelNode->GetID() );
      }
    else
      {
      std::cerr << "Error: No hierarchy node found for ribbon '" << this->RibbonModelNode->GetName() << "'" << std::endl;
      }
    }

  // Set new model node as closed surface representation of this contour
  this->SetAndObserveClosedSurfaceModelNodeId(closedSurfaceModelNode->GetID());

  // TODO: Workaround (see above)
  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);

  return closedSurfaceModelNode;
}

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetColorIndex(int &colorIndex, vtkMRMLColorTableNode* &colorNode, vtkMRMLModelNode* referenceModelNode/*=NULL*/)
{
  // Initialize output color index with Gray 'invalid' color
  colorIndex = 1;

  // Get hierarchy node
  vtkMRMLContourHierarchyNode* contourHierarchyNode = vtkMRMLContourHierarchyNode::SafeDownCast(
    vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->Scene, this->ID));
  if (!contourHierarchyNode)
    {
    std::cerr << "Error: No hierarchy node found for structure '" << this->Name << "'" << std::endl;
    return;
    }

  // Get color node created for the structure set
  vtkMRMLContourHierarchyNode* parentContourHierarchyNode = vtkMRMLContourHierarchyNode::SafeDownCast(contourHierarchyNode->GetParentNode());

  std::string seriesName = parentContourHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME.c_str());
  std::string colorNodeName = seriesName + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;
  vtkSmartPointer<vtkCollection> colorNodes = vtkSmartPointer<vtkCollection>::Take( this->Scene->GetNodesByName(colorNodeName.c_str()) );
  if (colorNodes->GetNumberOfItems() == 0)
    {
    std::cerr << "Error: No color table found for structure set '" << parentContourHierarchyNode->GetName() << "'" << std::endl;
    }
  colorNodes->InitTraversal();
  colorNode = vtkMRMLColorTableNode::SafeDownCast(colorNodes->GetNextItemAsObject());
  int structureColorIndex = -1;
  while (colorNode)
    {
    int currentColorIndex = -1;

    //TODO: workaround for issue #179, restore when Slicer mantis issue http://www.na-mic.org/Bug/view.php?id=2783 is fixed
    // Try to search for the structure name with the spaces replaced with underscores in case it was loaded from scene file
    std::string structureNameWithUnderscores(this->StructureName);
    std::replace(structureNameWithUnderscores.begin(), structureNameWithUnderscores.end(), ' ', '_');
    
    if ( (currentColorIndex = colorNode->GetColorIndexByName(this->StructureName)) != -1
      || (currentColorIndex = colorNode->GetColorIndexByName(structureNameWithUnderscores.c_str())) != -1 )
    //if ((currentColorIndex = colorNode->GetColorIndexByName(this->StructureName)) != -1)
      {
      if (referenceModelNode)
        {
        double modelColor[3];
        double foundColor[4];
        referenceModelNode->GetDisplayNode()->GetColor(modelColor);
        colorNode->GetColor(currentColorIndex, foundColor);
        if ((fabs(modelColor[0]-foundColor[0]) < EPSILON) && (fabs(modelColor[1]-foundColor[1]) < EPSILON) && (fabs(modelColor[2]-foundColor[2])) < EPSILON)
          {
          structureColorIndex = currentColorIndex;
          break;
          }
        }
      else
        {
        structureColorIndex = currentColorIndex;
        break;
        }
      }
    colorNode = vtkMRMLColorTableNode::SafeDownCast(colorNodes->GetNextItemAsObject());
    }

  if (structureColorIndex == -1)
    {
    std::cout << "No matching entry found in the color tables for structure '" << this->StructureName << "'" << std::endl;
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

//----------------------------------------------------------------------------
void vtkMRMLContourNode::GetTransformFromModelToVolumeIjk(vtkMRMLModelNode* fromModelNode, vtkMRMLScalarVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToToVolumeIjkTransform)
{
  if (!fromModelToToVolumeIjkTransform)
    {
    return;
    }
  if (!fromModelNode || !toVolumeNode)
    {
    return;
    }

  vtkSmartPointer<vtkGeneralTransform> fromModelToToVolumeRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();

  // Determine the 'from' node (the transform that is applied to the model node and also this contour node is ignored)
  vtkMRMLTransformableNode* fromNode = vtkMRMLTransformableNode::SafeDownCast(fromModelNode);
  if (fromModelNode->GetTransformNodeID())
    {
    if ( !this->TransformNodeID || STRCASECMP(this->TransformNodeID, fromModelNode->GetTransformNodeID()) )
      {
      vtkErrorMacro("Parent transform nodes of the contour and its model representation do not match!");
      return;
      }

    fromNode = vtkMRMLTransformableNode::SafeDownCast( this->Scene->GetNodeByID(fromModelNode->GetTransformNodeID()) );  
    }

  // Determine the 'to' node (the transform that is applied to the labelmap node and also this contour node is ignored)
  vtkMRMLTransformableNode* toNode = vtkMRMLTransformableNode::SafeDownCast(toVolumeNode);
  if (toVolumeNode->GetTransformNodeID())
    {
    if ( !this->TransformNodeID || STRCASECMP(this->TransformNodeID, toVolumeNode->GetTransformNodeID()) )
      {
      vtkErrorMacro("Parent transform nodes of the contour and its model representation do not match!");
      return;
      }

    toNode = vtkMRMLTransformableNode::SafeDownCast( this->Scene->GetNodeByID(toVolumeNode->GetTransformNodeID()) );  
    }

  // Get transform between the source node and the volume
  SlicerRtCommon::GetTransformBetweenTransformables(fromNode, toNode, fromModelToToVolumeRasTransform);

  // Create volumeRas to volumeIjk transform
  vtkSmartPointer<vtkMatrix4x4> toVolumeRasToToVolumeIjkTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  toVolumeNode->GetRASToIJKMatrix( toVolumeRasToToVolumeIjkTransformMatrix );  

  // Create model to volumeIjk transform
  fromModelToToVolumeIjkTransform->Identity();
  fromModelToToVolumeIjkTransform->Concatenate(toVolumeRasToToVolumeIjkTransformMatrix);
  fromModelToToVolumeIjkTransform->Concatenate(fromModelToToVolumeRasTransform);
}
