/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkPolyDataToLabelmapFilter.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLContourModelDisplayNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkGeneralTransform.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageConstantPad.h>
#include <vtkImageReslice.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTimerLog.h>
#include <vtkTransformPolyDataFilter.h>

// Plastimatch includes
#include "itk_resample.h"
#include "plm_image_header.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkConvertContourRepresentations);

//----------------------------------------------------------------------------
vtkConvertContourRepresentations::vtkConvertContourRepresentations()
{
  this->ContourNode = NULL;
  this->LogSpeedMeasurementsOff();
  this->LabelmapResamplingThreshold = 0.5;
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
void vtkConvertContourRepresentations::GetTransformFromNodeToContourVolumeIjk( vtkMRMLTransformableNode* fromNode, vtkMRMLContourNode* toContourNode, vtkGeneralTransform* fromModelToContourIjkTransform )
{
  if (!fromModelToContourIjkTransform)
  {
    vtkErrorMacro("GetTransformFromModelToVolumeIjk: Invalid output transform given!");
    return;
  }
  if (!fromNode || !toContourNode)
  {
    vtkErrorMacro("GetTransformFromModelToVolumeIjk: Invalid input nodes given!");
    return;
  }

  vtkSmartPointer<vtkGeneralTransform> fromModelToToVolumeRasTransform = vtkSmartPointer<vtkGeneralTransform>::New();

  // Determine the 'to' node (the transform that is applied to the contour node and also this contour node is ignored)
  vtkMRMLTransformableNode* toNode = vtkMRMLTransformableNode::SafeDownCast(toContourNode);
  if (toContourNode->GetTransformNodeID())
  {
    toNode = vtkMRMLTransformableNode::SafeDownCast( this->ContourNode->GetScene()->GetNodeByID(toContourNode->GetTransformNodeID()) );  
  }

  // Get transform between the source node and the volume
  SlicerRtCommon::GetTransformBetweenTransformables(fromNode, toNode, fromModelToToVolumeRasTransform);

  // Create volumeRas to volumeIjk transform
  vtkSmartPointer<vtkMatrix4x4> toVolumeRasToToVolumeIjkTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  toContourNode->GetRASToIJKMatrix( toVolumeRasToToVolumeIjkTransformMatrix );  

  // Create model to volumeIjk transform
  fromModelToContourIjkTransform->Identity();
  fromModelToContourIjkTransform->Concatenate(toVolumeRasToToVolumeIjkTransformMatrix);
  fromModelToContourIjkTransform->Concatenate(fromModelToToVolumeRasTransform);
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::GetTransformFromNodeToVolumeIjk( vtkMRMLTransformableNode* fromNode, vtkMRMLScalarVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToToVolumeIjkTransform )
{
  if (!fromModelToToVolumeIjkTransform)
  {
    vtkErrorMacro("GetTransformFromModelToVolumeIjk: Invalid output transform given!");
    return;
  }
  if (!fromNode || !toVolumeNode)
  {
    vtkErrorMacro("GetTransformFromModelToVolumeIjk: Invalid input nodes given!");
    return;
  }

  vtkSmartPointer<vtkGeneralTransform> fromModelToToVolumeRasTransform = vtkSmartPointer<vtkGeneralTransform>::New();

  // Determine the 'to' node (the transform that is applied to the contour node and also this contour node is ignored)
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
vtkMRMLContourNode* vtkConvertContourRepresentations::ConvertFromModelToIndexedLabelmap(vtkMRMLContourNode::ContourRepresentationType type)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid contour node!");
    return NULL;
  }
  vtkMRMLScene* mrmlScene = this->ContourNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid scene!");
    return NULL;
  }

  vtkPolyData* modelData = NULL;
  switch (type)
  {
  case vtkMRMLContourNode::RibbonModel:
    modelData = this->ContourNode->GetRibbonModelPolyData();
    break;
  case vtkMRMLContourNode::ClosedSurfaceModel:
    modelData = this->ContourNode->GetClosedSurfacePolyData();
    break;
  default:
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid source representation type given!");
    return NULL;
  }

  // Sanity check
  if ( this->ContourNode->GetRasterizationOversamplingFactor() < 0.01
    || this->ContourNode->GetRasterizationOversamplingFactor() > 100.0 )
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Unreasonable rasterization oversampling factor is given: " << this->ContourNode->GetRasterizationOversamplingFactor());
    return NULL;
  }
  if (this->LabelmapResamplingThreshold < 0.0 || this->LabelmapResamplingThreshold > 1.0)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid labelmap resampling threshold is given: " << this->LabelmapResamplingThreshold);
    return NULL;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Get reference volume node
  vtkMRMLScalarVolumeNode* selectedReferenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast( this->ContourNode->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) );
  if (!selectedReferenceVolumeNode)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: No reference volume node!");
    return NULL;
  }

  // Get subject hierarchy node associated to the contour
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode_Contour = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(this->ContourNode);
  vtkMRMLScalarVolumeNode* referencedAnatomyVolumeNode = NULL;
  if (subjectHierarchyNode_Contour)
  {
    // Get referenced series (anatomy volume that was used for the contouring)
    const char* referencedSeriesUid = subjectHierarchyNode_Contour->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str());
    if (referencedSeriesUid)
    {
      vtkMRMLSubjectHierarchyNode* subjectHierarchyNode_ReferencedSeries =
        vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNodeByUID(
          mrmlScene, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_DICOM_UID_NAME, referencedSeriesUid );
      if (subjectHierarchyNode_ReferencedSeries)
      {
        referencedAnatomyVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(subjectHierarchyNode_ReferencedSeries->GetAssociatedNode());

        if (referencedAnatomyVolumeNode)
        {
          // Check if the referenced anatomy volume is in the same coordinate system as the contour
          vtkSmartPointer<vtkGeneralTransform> modelToReferencedAnatomyVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
          SlicerRtCommon::GetTransformBetweenTransformables(this->ContourNode, referencedAnatomyVolumeNode, modelToReferencedAnatomyVolumeIjkTransform);
          double* transformedPoint = modelToReferencedAnatomyVolumeIjkTransform->TransformPoint(1.0, 0.0, 0.0);
          if ( fabs(transformedPoint[0]-1.0) > EPSILON || fabs(transformedPoint[1]) > EPSILON || fabs(transformedPoint[2]) > EPSILON )
          {
            vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Referenced series '" << referencedAnatomyVolumeNode->GetName()
              << "' is not in the same coordinate system as contour '" << this->ContourNode->GetName() << "'!");
            referencedAnatomyVolumeNode = NULL;
          }
        }
      }
    }
    else
    {
      vtkDebugMacro("ConvertFromModelToIndexedLabelmap: No referenced series UID found for contour '" << this->ContourNode->GetName() << "'!");
    }
  }

  // If the selected reference is under a transform, then create a temporary copy and harden transform so that the whole transform is reflected in the result
  vtkSmartPointer<vtkMRMLScalarVolumeNode> temporaryHardenedSelectedReferenceVolumeCopy = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  if (selectedReferenceVolumeNode->GetParentTransformNode())
  {
    temporaryHardenedSelectedReferenceVolumeCopy->Copy(selectedReferenceVolumeNode);
    vtkSmartPointer<vtkGeneralTransform> referenceVolumeToModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
    referenceVolumeToModelTransform->Identity();
    vtkSmartPointer<vtkGeneralTransform> referenceVolumeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
    referenceVolumeToModelTransform->Concatenate(referenceVolumeToWorldTransform);

    selectedReferenceVolumeNode->GetParentTransformNode()->GetTransformToWorld(referenceVolumeToWorldTransform);
    if (this->ContourNode->GetParentTransformNode())
    {
      vtkSmartPointer<vtkGeneralTransform> worldToModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
      this->ContourNode->GetParentTransformNode()->GetTransformToWorld(worldToModelTransform);
      worldToModelTransform->Inverse();
      referenceVolumeToModelTransform->Concatenate(worldToModelTransform);
    }

    temporaryHardenedSelectedReferenceVolumeCopy->ApplyTransform(referenceVolumeToModelTransform);
    selectedReferenceVolumeNode = temporaryHardenedSelectedReferenceVolumeCopy.GetPointer();
  }

  // Use referenced anatomy volume for conversion if available, and has different orientation than the selected reference
  vtkMRMLScalarVolumeNode* referenceVolumeNodeUsedForConversion = selectedReferenceVolumeNode;
  if (referencedAnatomyVolumeNode && referencedAnatomyVolumeNode != selectedReferenceVolumeNode)
  {
    vtkSmartPointer<vtkMatrix4x4> referencedAnatomyVolumeDirectionMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    referencedAnatomyVolumeNode->GetIJKToRASDirectionMatrix(referencedAnatomyVolumeDirectionMatrix);
    vtkSmartPointer<vtkMatrix4x4> selectedReferenceVolumeDirectionMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    selectedReferenceVolumeNode->GetIJKToRASDirectionMatrix(selectedReferenceVolumeDirectionMatrix);
    bool differentOrientation = false;
    for (int row=0; row<3; ++row)
    {
      for (int column=0; column<3; ++column)
      {
        if ( fabs( referencedAnatomyVolumeDirectionMatrix->GetElement(row,column)
          - selectedReferenceVolumeDirectionMatrix->GetElement(row,column) ) > EPSILON*EPSILON)
        {
          differentOrientation = true;
          break;
        }
      }
    }
    if (differentOrientation)
    {
      referenceVolumeNodeUsedForConversion = referencedAnatomyVolumeNode;
    }
    else
    {
      // Set referenced anatomy to invalid if has the same geometry as the selected reference (the resampling step is unnecessary then)
      referencedAnatomyVolumeNode = NULL;
    }
  }
  else
  {
    // Set referenced anatomy to invalid if it is the same as the selected reference (the resampling step is unnecessary if the two are the same)
    referencedAnatomyVolumeNode = NULL;
  }

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  this->ContourNode->GetColor(structureColorIndex, colorNode);

  // Transform the model polydata to referenceIjk coordinate frame (the labelmap image coordinate frame is referenceIjk)
  // Transform from model space to reference space
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  this->GetTransformFromNodeToVolumeIjk( this->ContourNode, referenceVolumeNodeUsedForConversion, modelToReferenceVolumeIjkTransform );

  // Apply inverse of the transform applied on the contour (because the target representation will be under the same
  // transform and we want to avoid applying the transform twice)
  vtkSmartPointer<vtkGeneralTransform> transformedModelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  transformedModelToReferenceVolumeIjkTransform->Concatenate(modelToReferenceVolumeIjkTransform);
  if (this->ContourNode->GetParentTransformNode())
  {
    vtkSmartPointer<vtkGeneralTransform> worldToModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
    this->ContourNode->GetParentTransformNode()->GetTransformToWorld(worldToModelTransform);
    worldToModelTransform->Inverse();
    transformedModelToReferenceVolumeIjkTransform->Concatenate(worldToModelTransform);
  }

  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataModelToReferenceVolumeIjkFilter =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
#if (VTK_MAJOR_VERSION <= 5)
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInput( modelData );
#else
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInputData( modelData );
#endif
  transformPolyDataModelToReferenceVolumeIjkFilter->SetTransform( transformedModelToReferenceVolumeIjkTransform.GetPointer() );

  // Initialize polydata to labelmap filter for conversion
  double checkpointLabelmapConversionStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointLabelmapConversionStart); // Although it is used later, a warning is logged so needs to be suppressed

  vtkSmartPointer<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter = vtkSmartPointer<vtkPolyDataToLabelmapFilter>::New();
  transformPolyDataModelToReferenceVolumeIjkFilter->Update();
  polyDataToLabelmapFilter->SetBackgroundValue(0);
  polyDataToLabelmapFilter->SetLabelValue(structureColorIndex);
  polyDataToLabelmapFilter->UseReferenceValuesOff();
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToReferenceVolumeIjkFilter->GetOutput() );

  // Oversample used reference volume (anatomy if present, selected reference otherwise) and set it to the converter
  double oversamplingFactor = this->ContourNode->GetRasterizationOversamplingFactor();
  double oversampledReferenceVolumeUsedForConversionSpacingMultiplier[3] = {1.0, 1.0, 1.0};
  if ( oversamplingFactor != 1.0 && referenceVolumeNodeUsedForConversion == selectedReferenceVolumeNode )
  {
    int oversampledReferenceVolumeUsedForConversionExtent[6] = {0, 0, 0, 0, 0, 0};
    SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor(referenceVolumeNodeUsedForConversion,
      oversamplingFactor, oversampledReferenceVolumeUsedForConversionExtent, oversampledReferenceVolumeUsedForConversionSpacingMultiplier);

    vtkSmartPointer<vtkImageReslice> reslicer = vtkSmartPointer<vtkImageReslice>::New();
#if (VTK_MAJOR_VERSION <= 5)
    reslicer->SetInput(referenceVolumeNodeUsedForConversion->GetImageData());
#else
    reslicer->SetInputData(referenceVolumeNodeUsedForConversion->GetImageData());
#endif
    reslicer->SetInterpolationMode(VTK_RESLICE_LINEAR);
    reslicer->SetOutputExtent(oversampledReferenceVolumeUsedForConversionExtent);
    reslicer->SetOutputSpacing(oversampledReferenceVolumeUsedForConversionSpacingMultiplier);
    reslicer->Update();

    polyDataToLabelmapFilter->SetReferenceImage( reslicer->GetOutput() );
  }
  else
  {
    polyDataToLabelmapFilter->SetReferenceImage( referenceVolumeNodeUsedForConversion->GetImageData() );
  }

  polyDataToLabelmapFilter->Update();

  if (polyDataToLabelmapFilter->GetOutput() == NULL)
  {
    vtkErrorMacro("Unable to convert from polydata to labelmap.");
    return NULL;
  }

  // Resample to selected reference coordinate system if referenced anatomy was used
  double checkpointResamplingStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointResamplingStart); // Although it is used later, a warning is logged so needs to be suppressed

  int oversampledSelectedReferenceVolumeExtent[6] = {0, 0, 0, 0, 0, 0};
  double oversampledSelectedReferenceVtkVolumeSpacing[3] = {0.0, 0.0, 0.0};
  SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor( selectedReferenceVolumeNode,
    oversamplingFactor, oversampledSelectedReferenceVolumeExtent, oversampledSelectedReferenceVtkVolumeSpacing );
  double selectedReferenceSpacing[3] = {0.0, 0.0, 0.0};
  selectedReferenceVolumeNode->GetSpacing(selectedReferenceSpacing);
  double oversampledSelectedReferenceVolumeSpacing[3] = { selectedReferenceSpacing[0] * oversampledSelectedReferenceVtkVolumeSpacing[0],
    selectedReferenceSpacing[1] * oversampledSelectedReferenceVtkVolumeSpacing[1],
    selectedReferenceSpacing[2] * oversampledSelectedReferenceVtkVolumeSpacing[2] };

  double calculatedOrigin[3] = {0.0, 0.0, 0.0};
  double calculatedOriginHomogeneous[4] = {0.0, 0.0, 0.0, 1.0};
  polyDataToLabelmapFilter->GetOutput()->GetOrigin(calculatedOrigin);
  calculatedOriginHomogeneous[0] = calculatedOrigin[0];
  calculatedOriginHomogeneous[1] = calculatedOrigin[1];
  calculatedOriginHomogeneous[2] = calculatedOrigin[2];

  vtkSmartPointer<vtkImageData> indexedLabelmapVolumeImageData;
  if (referencedAnatomyVolumeNode)
  {
    // Create temporary volume node so that the oversampled reference can be converted to ITK image
    vtkSmartPointer<vtkMRMLScalarVolumeNode> intermediateLabelmapNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    intermediateLabelmapNode->SetAndObserveImageData(polyDataToLabelmapFilter->GetOutput());
    intermediateLabelmapNode->CopyOrientation(referencedAnatomyVolumeNode);
    double newOriginHomogeneous[4] = {0, 0, 0, 1};
    this->CalculateOriginInRas(intermediateLabelmapNode, calculatedOriginHomogeneous, &newOriginHomogeneous[0]);
    intermediateLabelmapNode->SetOrigin(newOriginHomogeneous[0], newOriginHomogeneous[1], newOriginHomogeneous[2]);

    double referencedAnatomyVolumeNodeSpacing[3] = {0.0, 0.0, 0.0};
    referencedAnatomyVolumeNode->GetSpacing(referencedAnatomyVolumeNodeSpacing);
    intermediateLabelmapNode->SetSpacing( referencedAnatomyVolumeNodeSpacing[0] * oversampledReferenceVolumeUsedForConversionSpacingMultiplier[0],
      referencedAnatomyVolumeNodeSpacing[1] * oversampledReferenceVolumeUsedForConversionSpacingMultiplier[1],
      referencedAnatomyVolumeNodeSpacing[2] * oversampledReferenceVolumeUsedForConversionSpacingMultiplier[2] );

    // Determine output (oversampled selected reference volume node) geometry for the resampling
    plm_long oversampledSelectedReferenceDimensionsLong[3] = { oversampledSelectedReferenceVolumeExtent[1]-oversampledSelectedReferenceVolumeExtent[0]+1,
      oversampledSelectedReferenceVolumeExtent[3]-oversampledSelectedReferenceVolumeExtent[2]+1,
      oversampledSelectedReferenceVolumeExtent[5]-oversampledSelectedReferenceVolumeExtent[4]+1 };
    double selectedReferenceOrigin[3] = {0.0, 0.0, 0.0};
    selectedReferenceVolumeNode->GetOrigin(selectedReferenceOrigin);
    float selectedReferenceOriginFloat[3] = { selectedReferenceOrigin[0], selectedReferenceOrigin[1], selectedReferenceOrigin[2] };
    float oversampledSelectedReferenceSpacingFloat[3] = { oversampledSelectedReferenceVolumeSpacing[0], oversampledSelectedReferenceVolumeSpacing[1], oversampledSelectedReferenceVolumeSpacing[2] };

    float selectedReferenceDirectionCosines[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double selectedReferenceDirections[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    selectedReferenceVolumeNode->GetIJKToRASDirections(selectedReferenceDirections);
    for (int matrixIndex=0; matrixIndex<9; ++matrixIndex)
    {
      selectedReferenceDirectionCosines[matrixIndex] = selectedReferenceDirections[matrixIndex/3][matrixIndex%3];
    }

    // TODO: Try to use vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode here to make the conversion faster
    //       https://www.assembla.com/spaces/slicerrt/tickets/516-use-vtkvolumesorientedresampleutility-for-contour-conversion
    // Convert intermediate labelmap to ITK image
    itk::Image<unsigned char, 3>::Pointer intermediateLabelmapItkVolume = itk::Image<unsigned char, 3>::New();
    SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(intermediateLabelmapNode, intermediateLabelmapItkVolume, true, false);

    Plm_image_header resampledItkImageHeader(oversampledSelectedReferenceDimensionsLong, selectedReferenceOriginFloat, oversampledSelectedReferenceSpacingFloat, selectedReferenceDirectionCosines);
    itk::Image<unsigned char, 3>::Pointer resampledIntermediateLabelmapItkVolume = resample_image(intermediateLabelmapItkVolume, resampledItkImageHeader, 0, 1);

    // Convert resampled image back to VTK
    indexedLabelmapVolumeImageData = vtkSmartPointer<vtkImageData>::New();
    SlicerRtCommon::ConvertItkImageToVtkImageData<unsigned char>(resampledIntermediateLabelmapItkVolume, indexedLabelmapVolumeImageData, VTK_UNSIGNED_CHAR);
  }
  else
  {
    indexedLabelmapVolumeImageData = polyDataToLabelmapFilter->GetOutput();
  }

  // Set VTK volume properties to default, as Slicer uses the MRML node to store these
  indexedLabelmapVolumeImageData->SetSpacing(1.0, 1.0, 1.0);
  indexedLabelmapVolumeImageData->SetOrigin(0.0, 0.0, 0.0);

  // CopyOrientation copies both ijk orientation, origin and spacing
  this->ContourNode->CopyOrientation( selectedReferenceVolumeNode );
  // Override the origin with the calculated origin
  double newOriginHomogeneous[4] = {0, 0, 0, 1};
  this->CalculateOriginInRas(this->ContourNode, calculatedOriginHomogeneous, &newOriginHomogeneous[0]);
  this->ContourNode->SetOrigin(newOriginHomogeneous[0], newOriginHomogeneous[1], newOriginHomogeneous[2]);
  this->ContourNode->SetSpacing( oversampledSelectedReferenceVolumeSpacing );

  // Update the contour node with the new image data
  this->ContourNode->SetAndObserveLabelmapImageData( indexedLabelmapVolumeImageData );

  // TODO : 2d vis readdition
  //this->ContourNode->CreateLabelmapDisplayNode();

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed

    vtkDebugMacro("ConvertFromModelToIndexedLabelmap: Total model-labelmap conversion time for contour " << this->ContourNode->GetName() << ": " << checkpointEnd-checkpointStart << " s\n"
      << "\tAccessing associated nodes and transform model: " << checkpointLabelmapConversionStart-checkpointStart << " s\n"
      << "\tConverting to labelmap (to referenced series if available): " << checkpointResamplingStart-checkpointLabelmapConversionStart << " s\n"
      << "\tResampling referenced series to selected reference: " << checkpointEnd-checkpointResamplingStart << " s");
  }

  return this->ContourNode;
}

//----------------------------------------------------------------------------
vtkMRMLContourNode* vtkConvertContourRepresentations::ConvertFromIndexedLabelmapToClosedSurfaceModel()
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Invalid contour node!");
    return NULL;
  }
  vtkMRMLScene* mrmlScene = this->ContourNode->GetScene();
  if (!mrmlScene || !this->ContourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
  {
    vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Invalid scene!");
    return NULL;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // TODO: Workaround for the the issue that slice intersections are not visible of newly converted models
  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  this->ContourNode->GetColor(structureColorIndex, colorNode);

  // Determine here if we need to pad the labelmap to create a completely closed surface
  vtkSmartPointer<vtkImageConstantPad> padder;
  bool pad(false);
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(this->ContourNode->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()));
  if (referenceVolumeNode != NULL)
  {
    int referenceExtents[6] = {0, 0, 0, 0, 0, 0};
    referenceVolumeNode->GetImageData()->GetExtent(referenceExtents);
    int labelmapExtents[6] = {0, 0, 0, 0, 0, 0};
    this->ContourNode->GetLabelmapImageData()->GetExtent(labelmapExtents);

    if (SlicerRtCommon::AreBoundsEqual(referenceExtents, labelmapExtents))
    {
      pad = true;
      vtkDebugMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Adding 1 pixel padding around the image, shifting origin.");

      padder = vtkSmartPointer<vtkImageConstantPad>::New();

      vtkSmartPointer<vtkImageChangeInformation> translator = vtkSmartPointer<vtkImageChangeInformation>::New();
#if (VTK_MAJOR_VERSION <= 5)
      translator->SetInput(this->ContourNode->GetLabelmapImageData());
#else
      translator->SetInputData(this->ContourNode->GetLabelmapImageData());
#endif
      // Translate the extent by 1 pixel
      translator->SetExtentTranslation(1, 1, 1);
      // Args are: -padx*xspacing, -pady*yspacing, -padz*zspacing but padding and spacing are both 1
      translator->SetOriginTranslation(-1.0, -1.0, -1.0);
#if (VTK_MAJOR_VERSION <= 5)
      padder->SetInput(translator->GetOutput());
#else
      padder->SetInputData(translator->GetOutput());
#endif
      padder->SetConstant(0);

      translator->Update();
      int extent[6] = {0, 0, 0, 0, 0, 0};
#if (VTK_MAJOR_VERSION <= 5)
      this->ContourNode->GetLabelmapImageData()->GetWholeExtent(extent);
#else
      this->ContourNode->GetLabelmapImageData()->GetExtent(extent);
#endif

      // Now set the output extent to the new size, padded by 2 on the positive side
      padder->SetOutputWholeExtent(extent[0], extent[1] + 2,
        extent[2], extent[3] + 2,
        extent[4], extent[5] + 2);

      padder->Update();
    }
  }

  // Convert labelmap to model
  vtkSmartPointer<vtkLabelmapToModelFilter> labelmapToModelFilter = vtkSmartPointer<vtkLabelmapToModelFilter>::New();
  if (pad)
  {
    labelmapToModelFilter->SetInputLabelmap( padder->GetOutput() );
  }
  else
  {
    labelmapToModelFilter->SetInputLabelmap( this->ContourNode->GetLabelmapImageData() );
  }
  labelmapToModelFilter->SetDecimateTargetReduction( this->ContourNode->GetDecimationTargetReductionFactor() );
  labelmapToModelFilter->SetLabelValue( structureColorIndex );
  labelmapToModelFilter->Update();    

  // Create reference volume IJK to transformed model transform
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  this->GetTransformFromNodeToContourVolumeIjk(this->ContourNode, this->ContourNode, modelToReferenceVolumeIjkTransform);

  vtkSmartPointer<vtkGeneralTransform> referenceVolumeIjkToTransformedModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  referenceVolumeIjkToTransformedModelTransform->Concatenate(modelToReferenceVolumeIjkTransform);

  // Apply inverse of the transform applied on the contour (because the target representation will be under the same
  // transform and we want to avoid applying the transform twice)
  if (this->ContourNode->GetParentTransformNode())
  {
    vtkSmartPointer<vtkGeneralTransform> worldToModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
    this->ContourNode->GetParentTransformNode()->GetTransformToWorld(worldToModelTransform);
    worldToModelTransform->Inverse();
    referenceVolumeIjkToTransformedModelTransform->Concatenate(worldToModelTransform);
  }

  referenceVolumeIjkToTransformedModelTransform->Inverse();

  // Transform the model polydata to referenceIjk coordinate frame (the labelmap image coordinate frame is referenceIjk)
  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataModelToReferenceVolumeIjkFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
#if (VTK_MAJOR_VERSION <= 5)
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInput( labelmapToModelFilter->GetOutput() );
#else
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInputData( labelmapToModelFilter->GetOutput() );
#endif
  transformPolyDataModelToReferenceVolumeIjkFilter->SetTransform(referenceVolumeIjkToTransformedModelTransform.GetPointer());
  transformPolyDataModelToReferenceVolumeIjkFilter->Update();

  this->ContourNode->SetAndObserveClosedSurfacePolyData( transformPolyDataModelToReferenceVolumeIjkFilter->GetOutput() );

  // Set parent transform node
  if (this->ContourNode->GetTransformNodeID())
  {
    this->ContourNode->SetAndObserveTransformNodeID(this->ContourNode->GetTransformNodeID());
  }

  vtkMRMLContourModelDisplayNode* closedSurfaceModelDisplayNode = this->ContourNode->CreateClosedSurfaceDisplayNode();

  // Set visibility and opacity to match the existing ribbon model visualization
  if (closedSurfaceModelDisplayNode)
  {
    closedSurfaceModelDisplayNode->SetSliceIntersectionVisibility(this->ContourNode->GetClosedSurfaceModelDisplayNode()->GetSliceIntersectionVisibility());
    closedSurfaceModelDisplayNode->SetVisibility(this->ContourNode->GetClosedSurfaceModelDisplayNode()->GetVisibility());
    closedSurfaceModelDisplayNode->SetOpacity(this->ContourNode->GetClosedSurfaceModelDisplayNode()->GetOpacity());

    // Set color
    double color[4] = {0.0, 0.0, 0.0, 0.0};
    if (colorNode)
    {
      colorNode->GetColor(structureColorIndex, color);
      closedSurfaceModelDisplayNode->SetColor(color);
    }
  }

  // TODO: Workaround (see above)
  mrmlScene->EndState(vtkMRMLScene::BatchProcessState);

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
    vtkDebugMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Total labelmap-model conversion time for contour " << this->ContourNode->GetName() << ": " << checkpointEnd-checkpointStart << " s");
  }

  return this->ContourNode;
}

//----------------------------------------------------------------------------
bool vtkConvertContourRepresentations::ConvertToRepresentation(vtkMRMLContourNode::ContourRepresentationType desiredType)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertToRepresentation: Invalid contour node!");
    return false;
  }

  if ( this->ContourNode->HasRepresentation(desiredType) )
  {
    return true;
  }

  // Set default parameters if none specified
  this->ContourNode->SetDefaultConversionParametersForRepresentation(desiredType);

  // Pointer to the new representation to be created
  vtkMRMLDisplayableNode* newRepresentation(NULL);

  // Active representation is a model of any kind and we want an indexed labelmap
  if ( desiredType == vtkMRMLContourNode::IndexedLabelmap && 
    (this->ContourNode->HasRepresentation(vtkMRMLContourNode::ClosedSurfaceModel) || this->ContourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) ) )
  {
    if ( this->ContourNode->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) == NULL )
    {
      vtkErrorMacro("ConvertToRepresentation: Unable to convert to indexed labelmap without a reference volume node!");
      return false;
    }

    newRepresentation = this->ConvertFromModelToIndexedLabelmap(
      (this->ContourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) ? vtkMRMLContourNode::RibbonModel : vtkMRMLContourNode::ClosedSurfaceModel) );
  }
  // Active representation is an indexed labelmap and we want a closed surface model
  else if ( this->ContourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) && desiredType == vtkMRMLContourNode::ClosedSurfaceModel )
  {
    newRepresentation = this->ConvertFromIndexedLabelmapToClosedSurfaceModel();
  }
  // Active representation is a ribbon model and we want a closed surface model
  else if ( desiredType == vtkMRMLContourNode::ClosedSurfaceModel
    && this->ContourNode->HasRepresentation(vtkMRMLContourNode::RibbonModel) )
  {
    // If the indexed labelmap is not created yet then we convert to it first
    if ( !this->ContourNode->HasRepresentation(vtkMRMLContourNode::IndexedLabelmap) )
    {
      if ( this->ContourNode->GetNodeReference(SlicerRtCommon::CONTOUR_RASTERIZATION_VOLUME_REFERENCE_ROLE.c_str()) == NULL )
      {
        vtkErrorMacro("ConvertToRepresentation: Unable to convert to indexed labelmap without a reference volume node (it is needed to convert into closed surface model)!");
        return false;
      }
      if (this->ConvertFromModelToIndexedLabelmap(vtkMRMLContourNode::RibbonModel) == NULL)
      {
        vtkErrorMacro("ConvertToRepresentation: Conversion to indexed labelmap failed (it is needed to convert into closed surface model)!");
        return false;
      }
    }

    newRepresentation = this->ConvertFromIndexedLabelmapToClosedSurfaceModel();
  }
  else
  {
    vtkWarningMacro("ConvertToRepresentation: Requested conversion not implemented yet!");
  }

  return newRepresentation != NULL;
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::ReconvertRepresentation(vtkMRMLContourNode::ContourRepresentationType type)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ReconvertRepresentation: Invalid contour node!");
    return;
  }
  if (!this->ContourNode->GetScene())
  {
    return;
  }
  if (type == vtkMRMLContourNode::None)
  {
    vtkWarningMacro("ReconvertRepresentation: Cannot convert to 'None' representation type!");
    return;
  }

  // Not implemented yet, cannot be re-converted
  if (type == vtkMRMLContourNode::RibbonModel)
  {
    //TODO: Implement if algorithm is ready
    vtkWarningMacro("ReconvertRepresentation: Convert to 'RibbonMode' representation type is not implemented yet!");
    return;
  }

  // Do the conversion as normally
  bool success = this->ConvertToRepresentation(type);
  if (!success)
  {
    vtkErrorMacro("ReconvertRepresentation: Failed to re-convert representation to type #" << (unsigned int)type);
  }
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::CalculateOriginInRas( vtkMRMLContourNode* contourNodeToSet, double originInIJKHomogeneous[4], double* newOriginHomogeneous )
{
  vtkMatrix4x4* refToRASMatrix = vtkMatrix4x4::New();
  contourNodeToSet->GetIJKToRASMatrix(refToRASMatrix);
  refToRASMatrix->MultiplyPoint(originInIJKHomogeneous, newOriginHomogeneous);
  refToRASMatrix->Delete();
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::CalculateOriginInRas( vtkMRMLScalarVolumeNode* volumeNodeToSet, double originInIJKHomogeneous[4], double* newOriginHomogeneous )
{
  vtkMatrix4x4* refToRASMatrix = vtkMatrix4x4::New();
  volumeNodeToSet->GetIJKToRASMatrix(refToRASMatrix);
  refToRASMatrix->MultiplyPoint(originInIJKHomogeneous, newOriginHomogeneous);
  refToRASMatrix->Delete();
}
