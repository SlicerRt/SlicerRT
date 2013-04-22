/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

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

// Contours includes
#include "vtkConvertContourRepresentations.h"
#include "vtkMRMLContourNode.h"
#include "vtkMRMLContourHierarchyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkPolyDataToLabelmapFilter.h"

// MRML includes
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageReslice.h>
#include <vtkGeneralTransform.h>
#include <vtkCollection.h>

// STD includes
#include <cassert>
#include <algorithm> //TODO: workaround for issue #179

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkConvertContourRepresentations);

//----------------------------------------------------------------------------
vtkConvertContourRepresentations::vtkConvertContourRepresentations()
{
  this->ContourNode = NULL;
}

//----------------------------------------------------------------------------
vtkConvertContourRepresentations::~vtkConvertContourRepresentations()
{
  this->SetContourNode(NULL);
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::GetTransformFromModelToVolumeIjk(
  vtkMRMLModelNode* fromModelNode, vtkMRMLScalarVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToToVolumeIjkTransform)
{
  if (!fromModelToToVolumeIjkTransform)
  {
    vtkErrorMacro("GetTransformFromModelToVolumeIjk: Invalid output transform given!");
    return;
  }
  if (!fromModelNode || !toVolumeNode)
  {
    vtkErrorMacro("GetTransformFromModelToVolumeIjk: Invalid input nodes given!");
    return;
  }

  vtkSmartPointer<vtkGeneralTransform> fromModelToToVolumeRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();

  // Determine the 'from' node (the transform that is applied to the model node and also this contour node is ignored)
  vtkMRMLTransformableNode* fromNode = vtkMRMLTransformableNode::SafeDownCast(fromModelNode);
  if (fromModelNode->GetTransformNodeID())
  {
    fromNode = vtkMRMLTransformableNode::SafeDownCast( this->ContourNode->GetScene()->GetNodeByID(fromModelNode->GetTransformNodeID()) );  
  }

  // Determine the 'to' node (the transform that is applied to the labelmap node and also this contour node is ignored)
  vtkMRMLTransformableNode* toNode = vtkMRMLTransformableNode::SafeDownCast(toVolumeNode);
  if (toVolumeNode->GetTransformNodeID())
  {
    toNode = vtkMRMLTransformableNode::SafeDownCast( this->ContourNode->GetScene()->GetNodeByID(toVolumeNode->GetTransformNodeID()) );  
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

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkConvertContourRepresentations::ConvertFromModelToIndexedLabelmap(vtkMRMLModelNode* modelNode)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid ontour node!");
    return NULL;
  }

  vtkMRMLScene* mrmlScene = this->ContourNode->GetScene();
  if (!mrmlScene || !modelNode)
  {
    return NULL;
  }

  // Sanity check
  if ( this->ContourNode->RasterizationOversamplingFactor < 0.01
    || this->ContourNode->RasterizationOversamplingFactor > 100.0 )
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Unreasonable rasterization oversampling factor is given: " << this->ContourNode->RasterizationOversamplingFactor);
    return NULL;
  }

  // Get reference volume node
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(this->ContourNode->RasterizationReferenceVolumeNodeId));
  if (!referenceVolumeNode)
  {
    vtkErrorMacro("Error: No reference volume node!");
    return NULL;
  }

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  this->ContourNode->GetColorIndex(structureColorIndex, colorNode, modelNode);

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

  vtkSmartPointer<vtkMatrix4x4> reslicedImageIjkToIndexedLabelmapRasTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (this->ContourNode->RasterizationOversamplingFactor != 1.0)
  {
    int outputExtent[6] = {0, 0, 0, 0, 0, 0};
    double outputSpacing[3] = {0.0, 0.0, 0.0};
    SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor(referenceVolumeNode,
      this->ContourNode->RasterizationOversamplingFactor, outputExtent, outputSpacing);

    vtkSmartPointer<vtkImageReslice> reslicer = vtkSmartPointer<vtkImageReslice>::New();
    reslicer->SetInput(referenceVolumeNode->GetImageData());
    reslicer->SetInterpolationMode(VTK_RESLICE_LINEAR);
    reslicer->SetOutputExtent(outputExtent);
    reslicer->SetOutputSpacing(outputSpacing);
    reslicer->Update();

    SlicerRtCommon::GetIjkToRasMatrixForResampledVolume(referenceVolumeNode,
      reslicer->GetOutput(), reslicedImageIjkToIndexedLabelmapRasTransformMatrix);

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

  // The origin and spacing has to be set to the MRML node instead of the image data
  if (this->ContourNode->RasterizationOversamplingFactor != 1.0)
  {
    indexedLabelmapVolumeNode->SetIJKToRASMatrix(reslicedImageIjkToIndexedLabelmapRasTransformMatrix);

    vtkImageData* indexedLabelmapVolumeImageData = polyDataToLabelmapFilter->GetOutput();
    indexedLabelmapVolumeImageData->SetSpacing(1.0, 1.0, 1.0);
    indexedLabelmapVolumeImageData->SetOrigin(0.0, 0.0, 0.0);
  }

  std::string indexedLabelmapVolumeNodeName = std::string(this->ContourNode->Name) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
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
  if (this->ContourNode->GetTransformNodeID())
  {
    indexedLabelmapVolumeNode->SetAndObserveTransformNodeID(this->ContourNode->GetTransformNodeID());
  }

  this->ContourNode->SetAndObserveIndexedLabelmapVolumeNodeIdOnly(indexedLabelmapVolumeNode->GetID());

  return indexedLabelmapVolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkConvertContourRepresentations::ConvertFromIndexedLabelmapToClosedSurfaceModel(vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Invalid contour node!");
    return NULL;
  }

  vtkMRMLScene* mrmlScene = this->ContourNode->GetScene();
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
  this->ContourNode->GetColorIndex(structureColorIndex, colorNode);

  // Convert labelmap to model
  vtkSmartPointer<vtkLabelmapToModelFilter> labelmapToModelFilter = vtkSmartPointer<vtkLabelmapToModelFilter>::New();
  labelmapToModelFilter->SetInputLabelmap( indexedLabelmapVolumeNode->GetImageData() );
  labelmapToModelFilter->SetDecimateTargetReduction( this->ContourNode->DecimationTargetReductionFactor );
  labelmapToModelFilter->SetLabelValue( structureColorIndex );
  labelmapToModelFilter->Update();    

  // Create display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(mrmlScene->AddNode(displayNode));
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn();
  displayNode->SetBackfaceCulling(0);

  // Set visibility and opacity to match the existing ribbon model visualization
  if (this->ContourNode->RibbonModelNode && this->ContourNode->RibbonModelNode->GetModelDisplayNode())
  {
    displayNode->SetVisibility(this->ContourNode->RibbonModelNode->GetModelDisplayNode()->GetVisibility());
    displayNode->SetOpacity(this->ContourNode->RibbonModelNode->GetModelDisplayNode()->GetOpacity());
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

  std::string closedSurfaceModelNodeName = std::string(this->ContourNode->Name) + SlicerRtCommon::CONTOUR_CLOSED_SURFACE_MODEL_NODE_NAME_POSTFIX;
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
  if (this->ContourNode->GetTransformNodeID())
  {
    closedSurfaceModelNode->SetAndObserveTransformNodeID(this->ContourNode->GetTransformNodeID());
  }

  // Put new model in the same model hierarchy as the ribbons
  if (this->ContourNode->RibbonModelNode && this->ContourNode->RibbonModelNodeId)
  {
    vtkMRMLModelHierarchyNode* ribbonModelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
      vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->ContourNode->GetScene(), this->ContourNode->RibbonModelNodeId));
    if (ribbonModelHierarchyNode)
    {
      vtkMRMLModelHierarchyNode* parentModelHierarchyNode
        = vtkMRMLModelHierarchyNode::SafeDownCast(ribbonModelHierarchyNode->GetParentNode());

      vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
      this->ContourNode->GetScene()->AddNode(modelHierarchyNode);
      modelHierarchyNode->SetParentNodeID( parentModelHierarchyNode->GetID() );
      modelHierarchyNode->SetModelNodeID( closedSurfaceModelNode->GetID() );
    }
    else
    {
      vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: No hierarchy node found for ribbon '" << this->ContourNode->RibbonModelNode->GetName() << "'");
    }
  }

  // Set new model node as closed surface representation of this contour
  this->ContourNode->SetAndObserveClosedSurfaceModelNodeIdOnly(closedSurfaceModelNode->GetID());

  // TODO: Workaround (see above)
  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);

  return closedSurfaceModelNode;
}

//----------------------------------------------------------------------------
bool vtkConvertContourRepresentations::ConvertToRepresentation(vtkMRMLContourNode::ContourRepresentationType type)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertToRepresentation: Invalid contour node!");
    return false;
  }

  if (type == this->ContourNode->GetActiveRepresentationType())
  {
    return true;
  }

  // Set default parameters if none specified
  this->ContourNode->SetDefaultConversionParametersForRepresentation(type);

  // Active representation is a model of any kind and we want an indexed labelmap
  if ( type == vtkMRMLContourNode::IndexedLabelmap
    && ( this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::RibbonModel
      || this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::ClosedSurfaceModel ) )
  {
    if (!this->ContourNode->RasterizationReferenceVolumeNodeId)
    {
      vtkErrorMacro("Unable to convert to indexed labelmap without a reference volume node!");
      return false;
    }

    vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode = this->ConvertFromModelToIndexedLabelmap(
      (this->ContourNode->RibbonModelNode ? this->ContourNode->RibbonModelNode : this->ContourNode->ClosedSurfaceModelNode) );

    return (indexedLabelmapVolumeNode != NULL);
  }
  // Active representation is an indexed labelmap and we want a closed surface model
  else if ( this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::IndexedLabelmap && type == vtkMRMLContourNode::ClosedSurfaceModel )
  {
    vtkMRMLModelNode* closedSurfaceVolumeNode
      = this->ConvertFromIndexedLabelmapToClosedSurfaceModel(this->ContourNode->IndexedLabelmapVolumeNode);

    return (closedSurfaceVolumeNode != NULL);
  }
  // Active representation is a ribbon model and we want a closed surface model
  else if ( type == vtkMRMLContourNode::ClosedSurfaceModel
         && this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::RibbonModel )
  {
    // If the indexed labelmap is not created yet then we convert to it first
    if (!this->ContourNode->IndexedLabelmapVolumeNode)
    {
      if (!this->ContourNode->RasterizationReferenceVolumeNodeId)
      {
        vtkErrorMacro("Unable to convert to indexed labelmap without a reference volume node (it is needed to convert into closed surface model)!");
        return false;
      }
      if (this->ConvertFromModelToIndexedLabelmap(this->ContourNode->RibbonModelNode) == NULL)
      {
        vtkErrorMacro("Conversion to indexed labelmap failed (it is needed to convert into closed surface model)!");
        return false;
      }
    }

    vtkMRMLModelNode* closedSurfaceVolumeNode
      = this->ConvertFromIndexedLabelmapToClosedSurfaceModel(this->ContourNode->IndexedLabelmapVolumeNode);

    return (closedSurfaceVolumeNode != NULL);
  }
  else
  {
    vtkWarningMacro("Requested conversion not implemented yet!");
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::ReconvertRepresentation(vtkMRMLContourNode::ContourRepresentationType type)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertToRepresentation: Invalid contour node!");
    return;
  }
  if (!this->ContourNode->GetScene())
  {
    return;
  }
  if (type == vtkMRMLContourNode::None)
  {
    vtkWarningMacro("Cannot convert to 'None' representation type!");
    return;
  }

  // Not implemented yet, cannot be re-converted
  if (type == vtkMRMLContourNode::RibbonModel)
  {
    //TODO: Implement if algorithm is ready
    vtkWarningMacro("Convert to 'RibbonMode' representation type is not implemented yet!");
    return;
  }

  std::vector<vtkMRMLDisplayableNode*> representations = this->ContourNode->CreateTemporaryRepresentationsVector();

  // Delete current representation if it exists
  if (this->ContourNode->RepresentationExists(type))
  {
    vtkMRMLDisplayableNode* node = representations[(unsigned int)type];
    switch (type)
    {
    case vtkMRMLContourNode::IndexedLabelmap:
      this->ContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(NULL);
      break;
    case vtkMRMLContourNode::ClosedSurfaceModel:
      this->ContourNode->SetAndObserveClosedSurfaceModelNodeId(NULL);
      break;
    default:
      break;
    }

    this->ContourNode->GetScene()->RemoveNode(node);
  }

  // Set active representation type to the one that can be the source of the new conversion
  switch (type)
  {
  case vtkMRMLContourNode::IndexedLabelmap:
    // Prefer ribbon models over closed surface models as source of conversion to indexed labelmap
    if (this->ContourNode->RibbonModelNode)
    {
      this->ContourNode->SetActiveRepresentationByType(vtkMRMLContourNode::RibbonModel);
    }
    else if (this->ContourNode->ClosedSurfaceModelNode)
    {
      this->ContourNode->SetActiveRepresentationByType(vtkMRMLContourNode::ClosedSurfaceModel);
    }
    break;
  case vtkMRMLContourNode::ClosedSurfaceModel:
    // Prefer indexed labelmap over ribbon models as source of conversion to closed surface models
    if (this->ContourNode->IndexedLabelmapVolumeNode)
    {
      this->ContourNode->SetActiveRepresentationByType(vtkMRMLContourNode::IndexedLabelmap);
    }
    else if (this->ContourNode->RibbonModelNode)
    {
      this->ContourNode->SetActiveRepresentationByType(vtkMRMLContourNode::RibbonModel);
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
