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

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkLabelmapToModelFilter.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
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

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

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
void vtkConvertContourRepresentations::GetTransformFromNodeToVolumeIjk(
  vtkMRMLTransformableNode* fromNode, vtkMRMLScalarVolumeNode* toVolumeNode, vtkGeneralTransform* fromModelToToVolumeIjkTransform)
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
vtkMRMLScalarVolumeNode* vtkConvertContourRepresentations::ConvertFromModelToIndexedLabelmap(vtkMRMLContourNode::ContourRepresentationType type)
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

  vtkMRMLModelNode* modelNode = NULL;
  switch (type)
  {
  case vtkMRMLContourNode::RibbonModel:
    modelNode = this->ContourNode->RibbonModelNode;
    break;
  case vtkMRMLContourNode::ClosedSurfaceModel:
    modelNode = this->ContourNode->ClosedSurfaceModelNode;
    break;
  default:
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid source representation type given!");
    return NULL;
  }

  // Sanity check
  if ( this->ContourNode->RasterizationOversamplingFactor < 0.01
    || this->ContourNode->RasterizationOversamplingFactor > 100.0 )
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Unreasonable rasterization oversampling factor is given: " << this->ContourNode->RasterizationOversamplingFactor);
    return NULL;
  }
  if (this->LabelmapResamplingThreshold < 0.0 || this->LabelmapResamplingThreshold > 1.0)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Invalid labelmap resampling threshold is given: " << this->LabelmapResamplingThreshold);
    return NULL;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  // Get reference volume node
  vtkMRMLScalarVolumeNode* selectedReferenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    mrmlScene->GetNodeByID(this->ContourNode->RasterizationReferenceVolumeNodeId));
  if (!selectedReferenceVolumeNode)
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: No reference volume node!");
    return NULL;
  }

  // Get patient hierarchy node associated to the contour
  vtkMRMLHierarchyNode* patientHierarchyNode_Contour = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(mrmlScene, this->ContourNode->GetID());
  vtkMRMLScalarVolumeNode* referencedAnatomyVolumeNode = NULL;
  if (patientHierarchyNode_Contour)
  {
    // Get referenced series (anatomy volume that was used for the contouring)
    const char* referencedSeriesUid = patientHierarchyNode_Contour->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_ROI_REFERENCED_SERIES_UID_ATTRIBUTE_NAME.c_str());
    if (referencedSeriesUid)
    {
      vtkMRMLHierarchyNode* patientHierarchyNode_ReferencedSeries =
        vtkSlicerPatientHierarchyModuleLogic::GetPatientHierarchyNodeByUID(mrmlScene, referencedSeriesUid);
      if (patientHierarchyNode_ReferencedSeries)
      {
        referencedAnatomyVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(patientHierarchyNode_ReferencedSeries->GetAssociatedNode());

        // Check if the referenced anatomy volume is in the same coordinate system as the contour
        vtkSmartPointer<vtkGeneralTransform> modelToReferencedAnatomyVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
        SlicerRtCommon::GetTransformBetweenTransformables(modelNode, referencedAnatomyVolumeNode, modelToReferencedAnatomyVolumeIjkTransform);
        double* transformedPoint = modelToReferencedAnatomyVolumeIjkTransform->TransformPoint(1.0, 0.0, 0.0);
        if (transformedPoint[0] != 1.0 || transformedPoint[1] != 0.0 || transformedPoint[2] != 0.0)
        {
          vtkErrorMacro("ConvertFromModelToIndexedLabelmap: Referenced series '" << referencedAnatomyVolumeNode->GetName()
            << "' is not in the same coordinate system as contour '" << this->ContourNode->Name << "'!");
          referencedAnatomyVolumeNode = NULL;
        }
      }
      else
      {
        vtkErrorMacro("ConvertFromModelToIndexedLabelmap: No referenced series found for contour '" << this->ContourNode->Name << "'!");
      }
    }
    else
    {
      vtkDebugMacro("ConvertFromModelToIndexedLabelmap: No referenced series UID found for contour '" << this->ContourNode->Name << "'!");
    }
  }
  else
  {
    vtkErrorMacro("ConvertFromModelToIndexedLabelmap: No patient hierarchy node found for contour '" << this->ContourNode->Name << "'!");
  }

  // If the selected reference is under a transform, then create a temporary copy and harden transform so that the whole transform is reflected in the result
  vtkSmartPointer<vtkMRMLScalarVolumeNode> temporaryHardenedSelectedReferenceVolumeCopy = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  if (selectedReferenceVolumeNode->GetParentTransformNode())
  {
    temporaryHardenedSelectedReferenceVolumeCopy->Copy(selectedReferenceVolumeNode);
    vtkSmartPointer<vtkMatrix4x4> hardeningMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    selectedReferenceVolumeNode->GetParentTransformNode()->GetMatrixTransformToWorld(hardeningMatrix);
    temporaryHardenedSelectedReferenceVolumeCopy->ApplyTransformMatrix(hardeningMatrix);

    selectedReferenceVolumeNode = temporaryHardenedSelectedReferenceVolumeCopy.GetPointer();
  }

  // Use referenced anatomy volume for conversion if available, and has different geometry than the selected reference
  vtkMRMLScalarVolumeNode* referenceVolumeNodeUsedForConversion = selectedReferenceVolumeNode;
  if (referencedAnatomyVolumeNode && referencedAnatomyVolumeNode != selectedReferenceVolumeNode)
  {
    vtkSmartPointer<vtkMatrix4x4> referencedAnatomyVolumeDirectionMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    referencedAnatomyVolumeNode->GetIJKToRASDirectionMatrix(referencedAnatomyVolumeDirectionMatrix);
    vtkSmartPointer<vtkMatrix4x4> selectedReferenceVolumeDirectionMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    selectedReferenceVolumeNode->GetIJKToRASDirectionMatrix(selectedReferenceVolumeDirectionMatrix);
    bool differentGeometry = false;
    for (int row=0; row<4; ++row)
    {
      for (int column=0; column<4; ++column)
      {
        if (referencedAnatomyVolumeDirectionMatrix->GetElement(row,column) - selectedReferenceVolumeDirectionMatrix->GetElement(row,column) > EPSILON)
        {
          differentGeometry = true;
          break;
        }
      }
    }
    if (differentGeometry)
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
  this->GetTransformFromNodeToVolumeIjk( modelNode, referenceVolumeNodeUsedForConversion, modelToReferenceVolumeIjkTransform );

  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataModelToReferenceVolumeIjkFilter
    = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  transformPolyDataModelToReferenceVolumeIjkFilter->SetInput( modelNode->GetPolyData() );
  transformPolyDataModelToReferenceVolumeIjkFilter->SetTransform( modelToReferenceVolumeIjkTransform.GetPointer() );

  // Initialize polydata to labelmap filter for conversion
  double checkpointLabelmapConversionStart = timer->GetUniversalTime();
  vtkSmartPointer<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter = vtkSmartPointer<vtkPolyDataToLabelmapFilter>::New();
  transformPolyDataModelToReferenceVolumeIjkFilter->Update();
  polyDataToLabelmapFilter->SetBackgroundValue(0);
  polyDataToLabelmapFilter->SetLabelValue(structureColorIndex);
  polyDataToLabelmapFilter->UseReferenceValuesOff();
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToReferenceVolumeIjkFilter->GetOutput() );

  // Oversample used reference volume (anatomy if present, selected reference otherwise) and set it to the converter
  double oversamplingFactor = this->ContourNode->RasterizationOversamplingFactor;
  double oversampledReferenceVolumeUsedForConversionSpacingMultiplier[3] = {1.0, 1.0, 1.0};
  if ( oversamplingFactor != 1.0 && referenceVolumeNodeUsedForConversion == selectedReferenceVolumeNode )
  {
    int oversampledReferenceVolumeUsedForConversionExtent[6] = {0, 0, 0, 0, 0, 0};
    SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor(referenceVolumeNodeUsedForConversion,
      oversamplingFactor, oversampledReferenceVolumeUsedForConversionExtent, oversampledReferenceVolumeUsedForConversionSpacingMultiplier);

    vtkSmartPointer<vtkImageReslice> reslicer = vtkSmartPointer<vtkImageReslice>::New();
    reslicer->SetInput(referenceVolumeNodeUsedForConversion->GetImageData());
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

  if( polyDataToLabelmapFilter->GetOutput() == NULL )
  {
    vtkErrorMacro("Unable to convert from polydata to labelmap.");
    return NULL;
  }

  // Resample to selected reference coordinate system if referenced anatomy was used
  double checkpointResamplingStart = timer->GetUniversalTime();
  int oversampledSelectedReferenceVolumeExtent[6] = {0, 0, 0, 0, 0, 0};
  double oversampledSelectedReferenceVtkVolumeSpacing[3] = {0.0, 0.0, 0.0};
  SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor( selectedReferenceVolumeNode,
    oversamplingFactor, oversampledSelectedReferenceVolumeExtent, oversampledSelectedReferenceVtkVolumeSpacing );
  double selectedReferenceSpacing[3];
  selectedReferenceVolumeNode->GetSpacing(selectedReferenceSpacing);
  double oversampledSelectedReferenceVolumeSpacing[3] = { selectedReferenceSpacing[0] * oversampledSelectedReferenceVtkVolumeSpacing[0],
    selectedReferenceSpacing[1] * oversampledSelectedReferenceVtkVolumeSpacing[1],
    selectedReferenceSpacing[2] * oversampledSelectedReferenceVtkVolumeSpacing[2] };

  double calculatedOrigin[3];
  double calculatedOriginHomogeneous[4] = {0, 0, 0, 1};
  polyDataToLabelmapFilter->GetOutput()->GetOrigin(calculatedOrigin);
  calculatedOriginHomogeneous[0] = calculatedOrigin[0];
  calculatedOriginHomogeneous[1] = calculatedOrigin[1];
  calculatedOriginHomogeneous[2] = calculatedOrigin[2];

  vtkSmartPointer<vtkImageData> indexedLabelmapVolumeImageData;
  if (referencedAnatomyVolumeNode)
  {
    // Create temporary volume node so that the oversampled reference an be converted to ITK image
    vtkSmartPointer<vtkMRMLScalarVolumeNode> intermediateLabelmapNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    intermediateLabelmapNode->SetAndObserveImageData(polyDataToLabelmapFilter->GetOutput());
    intermediateLabelmapNode->CopyOrientation(referencedAnatomyVolumeNode);
    double newOriginHomogeneous[4] = {0, 0, 0, 1};
    this->CalculateOriginInRAS(intermediateLabelmapNode, calculatedOriginHomogeneous, &newOriginHomogeneous[0]);
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
    double selectedReferenceOrigin[3];
    selectedReferenceVolumeNode->GetOrigin(selectedReferenceOrigin);
    float selectedReferenceOriginFloat[3] = { selectedReferenceOrigin[0], selectedReferenceOrigin[1], selectedReferenceOrigin[2] };
    float oversampledSelectedReferenceSpacingFloat[3] = { oversampledSelectedReferenceVolumeSpacing[0], oversampledSelectedReferenceVolumeSpacing[1], oversampledSelectedReferenceVolumeSpacing[2] };

    float selectedReferenceDirectionCosines[9];
    double selectedReferenceDirections[3][3];
    selectedReferenceVolumeNode->GetIJKToRASDirections(selectedReferenceDirections);
    for (int matrixIndex=0; matrixIndex<9; ++matrixIndex)
    {
      selectedReferenceDirectionCosines[matrixIndex] = selectedReferenceDirections[matrixIndex/3][matrixIndex%3];
    }

    // Convert intermediate labelmap to ITK image
    itk::Image<unsigned char, 3>::Pointer intermediateLabelmapItkVolume = itk::Image<unsigned char, 3>::New();
    SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(intermediateLabelmapNode, intermediateLabelmapItkVolume);

    Plm_image_header resampledItkImageHeader(oversampledSelectedReferenceDimensionsLong, selectedReferenceOriginFloat, oversampledSelectedReferenceSpacingFloat, selectedReferenceDirectionCosines);
    itk::Image<unsigned char, 3>::Pointer resampledIntermediateLabelmapItkVolume = resample_image(intermediateLabelmapItkVolume, resampledItkImageHeader, 0, 1);

    // Convert resampled image back to VTK
    indexedLabelmapVolumeImageData = vtkSmartPointer<vtkImageData>::New();
    int extent[6]={0, oversampledSelectedReferenceDimensionsLong[0]-1, 0, oversampledSelectedReferenceDimensionsLong[1]-1, 0, oversampledSelectedReferenceDimensionsLong[2]-1};
    indexedLabelmapVolumeImageData->SetExtent(extent);
    indexedLabelmapVolumeImageData->SetScalarType(VTK_UNSIGNED_CHAR);
    indexedLabelmapVolumeImageData->SetNumberOfScalarComponents(1);
    indexedLabelmapVolumeImageData->AllocateScalars();

    double labelmapThreshold = (double)structureColorIndex * this->LabelmapResamplingThreshold;
    unsigned char* indexedLabelmapVolumeImageDataPtr = (unsigned char*)indexedLabelmapVolumeImageData->GetScalarPointer();
    itk::ImageRegionIteratorWithIndex< itk::Image<unsigned char, 3> > itLabelmapItk( resampledIntermediateLabelmapItkVolume, resampledIntermediateLabelmapItkVolume->GetLargestPossibleRegion() );
    for ( itLabelmapItk.GoToBegin(); !itLabelmapItk.IsAtEnd(); ++itLabelmapItk )
    {
      itk::Image<unsigned char, 3>::IndexType index = itLabelmapItk.GetIndex();
      unsigned char resampledVoxelValue = resampledIntermediateLabelmapItkVolume->GetPixel(index);
      (*indexedLabelmapVolumeImageDataPtr) = ( resampledVoxelValue < labelmapThreshold ? 0 : (unsigned char)structureColorIndex );
      indexedLabelmapVolumeImageDataPtr++;
    }
  }
  else
  {
    indexedLabelmapVolumeImageData = polyDataToLabelmapFilter->GetOutput();
  }

  // Set VTK volume properties to default, as Slicer uses the MRML node to store these
  indexedLabelmapVolumeImageData->SetSpacing(1.0, 1.0, 1.0);
  indexedLabelmapVolumeImageData->SetOrigin(0.0, 0.0, 0.0);

  // Create indexed labelmap volume node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> indexedLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  std::string indexedLabelmapVolumeNodeName = std::string(this->ContourNode->Name) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  indexedLabelmapVolumeNodeName = mrmlScene->GenerateUniqueName(indexedLabelmapVolumeNodeName);

  // CopyOrientation copies both ijk orientation, origin and spacing
  indexedLabelmapVolumeNode->CopyOrientation( selectedReferenceVolumeNode );
  // Override the origin with the calculated origin
  double newOriginHomogeneous[4] = {0, 0, 0, 1};
  this->CalculateOriginInRAS(indexedLabelmapVolumeNode, calculatedOriginHomogeneous, &newOriginHomogeneous[0]);
  indexedLabelmapVolumeNode->SetOrigin(newOriginHomogeneous[0], newOriginHomogeneous[1], newOriginHomogeneous[2]);
  
  indexedLabelmapVolumeNode->SetSpacing( oversampledSelectedReferenceVolumeSpacing );

  indexedLabelmapVolumeNode->SetName( indexedLabelmapVolumeNodeName.c_str() );
  indexedLabelmapVolumeNode->SetAndObserveImageData( indexedLabelmapVolumeImageData );
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

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    vtkDebugMacro("ConvertFromModelToIndexedLabelmap: Total model-labelmap conversion time for contour " << this->ContourNode->GetName() << ": " << checkpointEnd-checkpointStart << " s\n"
      << "\tAccessing associated nodes and transform model: " << checkpointLabelmapConversionStart-checkpointStart << " s\n"
      << "\tConverting to labelmap (to referenced series if available): " << checkpointResamplingStart-checkpointLabelmapConversionStart << " s\n"
      << "\tResampling referenced series to selected reference: " << checkpointEnd-checkpointResamplingStart << " s");
  }

  return indexedLabelmapVolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLModelNode* vtkConvertContourRepresentations::ConvertFromIndexedLabelmapToClosedSurfaceModel()
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Invalid contour node!");
    return NULL;
  }
  vtkMRMLScene* mrmlScene = this->ContourNode->GetScene();
  if (!mrmlScene || !this->ContourNode->IndexedLabelmapVolumeNode)
  {
    vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Invalid scene!");
    return NULL;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  // TODO: Workaround for the the issue that slice intersections are not visible of newly converted models
  mrmlScene->StartState(vtkMRMLScene::BatchProcessState);

  // Get color index
  vtkMRMLColorTableNode* colorNode = NULL;
  int structureColorIndex = -1;
  this->ContourNode->GetColor(structureColorIndex, colorNode);

  // Determine here if we need to pad the labelmap to create a completely closed surface
  vtkSmartPointer<vtkImageConstantPad> padder;
  bool pad(false);
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(mrmlScene->GetNodeByID(this->ContourNode->GetRasterizationReferenceVolumeNodeId()));
  if (referenceVolumeNode != NULL)
  {
    int referenceExtents[6] = {0, 0, 0, 0, 0, 0};
    referenceVolumeNode->GetImageData()->GetExtent(referenceExtents);
    int labelmapExtents[6] = {0, 0, 0, 0, 0, 0};
    this->ContourNode->IndexedLabelmapVolumeNode->GetImageData()->GetExtent(labelmapExtents);

    if( SlicerRtCommon::AreBoundsEqual(referenceExtents, labelmapExtents) )
    {
      pad = true;
      vtkDebugMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Adding 1 pixel padding around the image, shifting origin.");

      padder = vtkSmartPointer<vtkImageConstantPad>::New();

      vtkSmartPointer<vtkImageChangeInformation> translator = vtkSmartPointer<vtkImageChangeInformation>::New();
      translator->SetInput(this->ContourNode->IndexedLabelmapVolumeNode->GetImageData());
      // Translate the extent by 1 pixel
      translator->SetExtentTranslation(1, 1, 1);
      // Args are: -padx*xspacing, -pady*yspacing, -padz*zspacing but padding and spacing are both 1
      translator->SetOriginTranslation(-1.0, -1.0, -1.0);
      padder->SetInput(translator->GetOutput());
      padder->SetConstant(0);

      translator->Update();
      int extent[6];
      this->ContourNode->IndexedLabelmapVolumeNode->GetImageData()->GetWholeExtent(extent);
      // Now set the output extent to the new size, padded by 2 on the positive side
      padder->SetOutputWholeExtent(extent[0], extent[1] + 2,
        extent[2], extent[3] + 2,
        extent[4], extent[5] + 2);
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
    labelmapToModelFilter->SetInputLabelmap( this->ContourNode->IndexedLabelmapVolumeNode->GetImageData() );
  }
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
  double color[4] = {0.0, 0.0, 0.0, 0.0};
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
  closedSurfaceModelNode->SetAndObserveTransformNodeID( this->ContourNode->IndexedLabelmapVolumeNode->GetTransformNodeID() );

  // Create model to referenceIjk transform
  vtkSmartPointer<vtkGeneralTransform> modelToReferenceVolumeIjkTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  this->GetTransformFromNodeToVolumeIjk(closedSurfaceModelNode, this->ContourNode->IndexedLabelmapVolumeNode, modelToReferenceVolumeIjkTransform);

  vtkSmartPointer<vtkGeneralTransform> referenceVolumeIjkToModelTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  referenceVolumeIjkToModelTransform->Concatenate(modelToReferenceVolumeIjkTransform);
  referenceVolumeIjkToModelTransform->Inverse();

  // Transform the model polydata to referenceIjk coordinate frame (the labelmap image coordinate frame is referenceIjk)
  vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyDataModelToReferenceVolumeIjkFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
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
      vtkMRMLModelHierarchyNode* parentModelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(ribbonModelHierarchyNode->GetParentNode());

      // Use the existing hierarchy node if any
      vtkSmartPointer<vtkMRMLModelHierarchyNode> modelHierarchyNode;
      if (this->ContourNode->RepresentationExists(vtkMRMLContourNode::ClosedSurfaceModel))
      {
        vtkMRMLHierarchyNode* associatedHierarchyNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(mrmlScene, this->ContourNode->ClosedSurfaceModelNodeId);
        if (associatedHierarchyNode && associatedHierarchyNode->IsA("vtkMRMLModelHierarchyNode"))
        {
          modelHierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(associatedHierarchyNode);
        }
        else
        {
          vtkErrorMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Invalid model hierarchy node for closed surface representation of contour '" << this->ContourNode->GetName() << "'");
        }
      }
      else
      {
        modelHierarchyNode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
        this->ContourNode->GetScene()->AddNode(modelHierarchyNode);
      }
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

  if (this->LogSpeedMeasurements)
  {
    double checkpointEnd = timer->GetUniversalTime();
    vtkDebugMacro("ConvertFromIndexedLabelmapToClosedSurfaceModel: Total labelmap-model conversion time for contour " << this->ContourNode->GetName() << ": " << checkpointEnd-checkpointStart << " s");
  }

  return closedSurfaceModelNode;
}

//----------------------------------------------------------------------------
bool vtkConvertContourRepresentations::ConvertToRepresentation(vtkMRMLContourNode::ContourRepresentationType desiredType)
{
  if (!this->ContourNode)
  {
    vtkErrorMacro("ConvertToRepresentation: Invalid contour node!");
    return false;
  }

  if (desiredType == this->ContourNode->GetActiveRepresentationType())
  {
    return true;
  }

  // Set default parameters if none specified
  this->ContourNode->SetDefaultConversionParametersForRepresentation(desiredType);

  // Active representation is a model of any kind and we want an indexed labelmap
  if ( desiredType == vtkMRMLContourNode::IndexedLabelmap
    && ( this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::RibbonModel
    || this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::ClosedSurfaceModel ) )
  {
    if (!this->ContourNode->RasterizationReferenceVolumeNodeId)
    {
      vtkErrorMacro("ConvertToRepresentation: Unable to convert to indexed labelmap without a reference volume node!");
      return false;
    }

    vtkMRMLScalarVolumeNode* indexedLabelmapVolumeNode = this->ConvertFromModelToIndexedLabelmap(
      (this->ContourNode->RibbonModelNode ? vtkMRMLContourNode::RibbonModel : vtkMRMLContourNode::ClosedSurfaceModel) );

    return (indexedLabelmapVolumeNode != NULL);
  }
  // Active representation is an indexed labelmap and we want a closed surface model
  else if ( this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::IndexedLabelmap && desiredType == vtkMRMLContourNode::ClosedSurfaceModel )
  {
    vtkMRMLModelNode* closedSurfaceVolumeNode = this->ConvertFromIndexedLabelmapToClosedSurfaceModel();

    return (closedSurfaceVolumeNode != NULL);
  }
  // Active representation is a ribbon model and we want a closed surface model
  else if ( desiredType == vtkMRMLContourNode::ClosedSurfaceModel
    && this->ContourNode->GetActiveRepresentationType() == vtkMRMLContourNode::RibbonModel )
  {
    // If the indexed labelmap is not created yet then we convert to it first
    if (!this->ContourNode->IndexedLabelmapVolumeNode)
    {
      if (!this->ContourNode->RasterizationReferenceVolumeNodeId)
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

    vtkMRMLModelNode* closedSurfaceVolumeNode = this->ConvertFromIndexedLabelmapToClosedSurfaceModel();

    return (closedSurfaceVolumeNode != NULL);
  }
  else
  {
    vtkWarningMacro("ConvertToRepresentation: Requested conversion not implemented yet!");
  }

  return false;
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
    vtkErrorMacro("ReconvertRepresentation: Failed to re-convert representation to type #" << (unsigned int)type);
  }
}

//----------------------------------------------------------------------------
void vtkConvertContourRepresentations::CalculateOriginInRas( vtkMRMLScalarVolumeNode* volumeNodeToSet, double originInIJKHomogeneous[4], double* newOriginHomogeneous )
{
    vtkMatrix4x4* refToRASMatrix = vtkMatrix4x4::New();
    volumeNodeToSet->GetIJKToRASMatrix(refToRASMatrix);
    refToRASMatrix->MultiplyPoint(originInIJKHomogeneous, newOriginHomogeneous);
    refToRASMatrix->Delete();
}
