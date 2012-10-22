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

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkIntArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageResample.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>

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

  this->RasterizationDownsamplingFactor = 2.0;
  this->DecimationTargetReductionFactor = 0.0;

  this->HideFromEditorsOff();
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

  of << indent << " RasterizationDownsamplingFactor=\"" << this->RasterizationDownsamplingFactor << "\"";
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
    else if (!strcmp(attName, "RasterizationDownsamplingFactor")) 
      {
      std::stringstream ss;
      ss << attValue;
      double doubleAttValue;
      ss >> doubleAttValue;
      this->RasterizationDownsamplingFactor = doubleAttValue;
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

  this->SetRasterizationDownsamplingFactor( node->RasterizationDownsamplingFactor );
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
  os << indent << "RasterizationDownsamplingFactor:   " << this->RasterizationDownsamplingFactor << "\n";
  os << indent << "DecimationTargetReductionFactor:   " << this->DecimationTargetReductionFactor << "\n";
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

  std::vector<vtkMRMLDisplayableNode*> representations
    = this->CreateTemporaryRepresentationsVector();

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

  std::vector<vtkMRMLDisplayableNode*> representations
    = this->CreateTemporaryRepresentationsVector();

  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  // Show only the active representation and set active representation type
  bool success = false;
  for (unsigned int i=0; i<NumberOfRepresentationTypes; ++i)
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

  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);
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
  // Active representation is a model of any kind and we want an indexed labelmap
  else if ( (this->ActiveRepresentationType == RibbonModel
          || this->ActiveRepresentationType == ClosedSurfaceModel)
    && type == IndexedLabelmap)
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
  else if (this->ActiveRepresentationType == IndexedLabelmap
    && type == ClosedSurfaceModel)
    {
    vtkMRMLModelNode* closedSurfaceVolumeNode
      = this->ConvertFromIndexedLabelmapToClosedSurfaceModel(this->IndexedLabelmapVolumeNode);

    return (closedSurfaceVolumeNode != NULL);
    }
  // Active representation is a ribbon model and we want a closed surface model
  else if (this->ActiveRepresentationType == RibbonModel
    && type == ClosedSurfaceModel)
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
vtkMRMLScalarVolumeNode* vtkMRMLContourNode::ConvertFromModelToIndexedLabelmap(vtkMRMLModelNode* modelNode)
{
  vtkMRMLScene* mrmlScene = this->Scene;
  if (!mrmlScene || !modelNode)
  {
    return NULL;
  }

  // Sanity check
  if ( this->RasterizationDownsamplingFactor < 0.01
    || this->RasterizationDownsamplingFactor > 100.0 )
  {
    vtkErrorMacro("Unreasonable rasterization downsampling factor is given: " << this->RasterizationDownsamplingFactor);
    return NULL;
  }

  // Get reference volume node
  vtkMRMLVolumeNode* referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(this->RasterizationReferenceVolumeNodeId));
  if (!referenceVolumeNode)
  {
    vtkErrorMacro("Error: No reference volume node!");
    return NULL;
  }

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  SlicerRtCommon::GetColorIndexForContour(this, mrmlScene, structureColorIndex, colorNode, modelNode);

  // Create model to referenceIjk transform
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  SlicerRtCommon::GetTransformFromModelToVolumeIjk(modelNode, referenceVolumeNode, modelToReferenceVolumeIjkTransform);

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

  if (this->RasterizationDownsamplingFactor != 1.0)
  {
    vtkSmartPointer<vtkImageResample> resampler = vtkSmartPointer<vtkImageResample>::New();
    resampler->SetInput(referenceVolumeNode->GetImageData());
    resampler->SetAxisMagnificationFactor(0, this->RasterizationDownsamplingFactor);
    resampler->SetAxisMagnificationFactor(1, this->RasterizationDownsamplingFactor);
    resampler->SetAxisMagnificationFactor(2, this->RasterizationDownsamplingFactor);
    resampler->Update();

    polyDataToLabelmapFilter->SetReferenceImage( resampler->GetOutput() );
  }
  else
  {
    polyDataToLabelmapFilter->SetReferenceImage( referenceVolumeNode->GetImageData() );
  }
  polyDataToLabelmapFilter->Update();    


  // Create indexed labelmap volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> indexedLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  indexedLabelmapVolumeNode->CopyOrientation( referenceVolumeNode );
  if (this->RasterizationDownsamplingFactor != 1.0)
  {
    double* referenceSpacing = referenceVolumeNode->GetSpacing();
    indexedLabelmapVolumeNode->SetSpacing(
      referenceSpacing[0]/this->RasterizationDownsamplingFactor,
      referenceSpacing[1]/this->RasterizationDownsamplingFactor,
      referenceSpacing[2]/this->RasterizationDownsamplingFactor );

    vtkImageData* indexedLabelmapVolumeImageData = polyDataToLabelmapFilter->GetOutput();
    indexedLabelmapVolumeImageData->SetSpacing(1.0, 1.0, 1.0); // The spacing is set to the MRML node
  }

  std::string indexedLabelmapVolumeNodeName = std::string(this->StructureName) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
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

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  SlicerRtCommon::GetColorIndexForContour(this, mrmlScene, structureColorIndex, colorNode);

  // Convert labelmap to model
  vtkSmartPointer<vtkLabelmapToModelFilter> labelmapToModelFilter = vtkSmartPointer<vtkLabelmapToModelFilter>::New();
  labelmapToModelFilter->SetInputLabelmap( indexedLabelmapVolumeNode->GetImageData() );
  labelmapToModelFilter->SetDecimateTargetReduction( this->DecimationTargetReductionFactor );
  labelmapToModelFilter->Update();    


  // Create closed surface model node
  vtkSmartPointer<vtkMRMLModelNode> closedSurfaceModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();

  std::string closedSurfaceModelNodeName = this->StructureName + SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX;
  closedSurfaceModelNodeName = mrmlScene->GenerateUniqueName(closedSurfaceModelNodeName);

  closedSurfaceModelNode->SetAndObserveTransformNodeID( indexedLabelmapVolumeNode->GetTransformNodeID() );
  closedSurfaceModelNode->SetName( closedSurfaceModelNodeName.c_str() );

  // Create model to referenceIjk transform
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  SlicerRtCommon::GetTransformFromModelToVolumeIjk(closedSurfaceModelNode, indexedLabelmapVolumeNode, modelToReferenceVolumeIjkTransform);
  
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

  mrmlScene->AddNode(closedSurfaceModelNode);

  // Create display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mrmlScene->AddNode(displayNode));

  double color[4];
  if (colorNode)
  {
    colorNode->GetColor(structureColorIndex, color);
    displayNode->SetColor(color);
  }

  closedSurfaceModelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );

  this->SetAndObserveClosedSurfaceModelNodeId(closedSurfaceModelNode->GetID());

  return closedSurfaceModelNode;
}
