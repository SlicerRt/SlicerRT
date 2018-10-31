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

// DoseVolumeHistogram includes
#include "vtkMRMLDoseVolumeHistogramNode.h"
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"
#include "vtkFractionalImageAccumulate.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSegmentation.h"
#include "vtkSegment.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"
#include "vtkCalculateOversamplingFactor.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"

// MRML includes
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLPlotSeriesNode.h>
#include <vtkMRMLPlotChartNode.h>
#include <vtkMRMLPlotViewNode.h>
#include <vtkMRMLTableNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>
#include <vtkEventBroker.h>

// VTK includes
#include <vtkBitArray.h>
#include <vtkCallbackCommand.h>
#include <vtkDelimitedTextWriter.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkImageAccumulate.h>
#include <vtkImageConstantPad.h>
#include <vtkImageDilateErode3D.h>
#include <vtkImageMathematics.h>
#include <vtkImageStencilData.h>
#include <vtkImageToImageStencil.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPiecewiseFunction.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTimerLog.h>
#include <vtkWeakPointer.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <set>

//----------------------------------------------------------------------------
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "DVH"; // Identifier
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CREATED_DVH_NODE_REFERENCE_ROLE = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "createdDvhArrayRef"; // Reference
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_PLOTSERIES_REFERENCE_ROLE = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "dvhPlotSeries"; // Reference

const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "DoseVolumeOversamplingFactor";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_SEGMENT_ID_ATTRIBUTE_NAME = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "SegmentID";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_TABLE_ROW_ATTRIBUTE_NAME = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "TableRow";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_SURFACE_ATTRIBUTE_NAME = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "Surface";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_SURFACE_INSIDE_ATTRIBUTE_NAME = vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX + "SurfaceInside";

const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_STRUCTURE = "Structure";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC = "Volume (cc)";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MEAN_PREFIX = "Mean ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MIN_PREFIX = "Min ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MAX_PREFIX = "Max ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_DOSE_POSTFIX = "dose";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_INTENSITY_POSTFIX = "intensity";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_TABLE_NODE_NAME_POSTFIX = "_DvhTable";

const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE = " Value (% of ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_END = " cc)";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseVolumeHistogramModuleLogic);

//---------------------------------------------------------------------------
class vtkDoseVolumeHistogramEventCallbackCommand : public vtkCallbackCommand
{
public:
  static vtkDoseVolumeHistogramEventCallbackCommand *New()
  {
    return new vtkDoseVolumeHistogramEventCallbackCommand;
  }

  vtkWeakPointer<vtkSlicerDoseVolumeHistogramModuleLogic> Logic;
  vtkWeakPointer<vtkMRMLDoseVolumeHistogramNode> ParameterNode;
};

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic::vtkSlicerDoseVolumeHistogramModuleLogic()
{
  this->StartValue = 0.1;
  this->StepSize = 0.2;
  this->NumberOfSamplesForNonDoseVolumes = 100;
  this->DefaultDoseVolumeOversamplingFactor = 2.0;

  this->LogSpeedMeasurements = false;
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic::~vtkSlicerDoseVolumeHistogramModuleLogic()
{
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDvh(vtkMRMLDoseVolumeHistogramNode* parameterNode)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  parameterNode->ClearAutomaticOversamplingFactors();
  vtkMRMLSegmentationNode* segmentationNode = parameterNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if ( !segmentationNode || !doseVolumeNode )
  {
    std::string errorMessage("Both segmentation node and dose volume node need to be set");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Fire only one modified event when the computation is done
  this->SetDisableModifiedEvent(1);
  int disabledNodeModify = parameterNode->StartModify();

  // Get maximum dose from dose volume for number of DVH bins
  vtkNew<vtkImageAccumulate> doseStat;
  doseStat->SetInputData(doseVolumeNode->GetImageData());
  doseStat->Update();
  double maxDose = doseStat->GetMax()[0];

  // Get selected segmentation
  vtkSegmentation* selectedSegmentation = segmentationNode->GetSegmentation();

  // If segment IDs list is empty then include all segments
  std::vector<std::string> segmentIDs;
  parameterNode->GetSelectedSegmentIDs(segmentIDs);
  if (segmentIDs.empty())
  {
    selectedSegmentation->GetSegmentIDs(segmentIDs);
  }

  // Create oriented image data from dose volume
  vtkSmartPointer<vtkOrientedImageData> doseImageData = vtkSmartPointer<vtkOrientedImageData>::Take(
    vtkSlicerSegmentationsModuleLogic::CreateOrientedImageDataFromVolumeNode(doseVolumeNode) );
  if (!doseImageData.GetPointer())
  {
    std::string errorMessage("Failed to get image data from dose volume");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Temporarily duplicate selected segments to contain binary labelmap of a different geometry (tied to dose volume)
  vtkSmartPointer<vtkSegmentation> segmentationCopy = vtkSmartPointer<vtkSegmentation>::New();
  segmentationCopy->SetMasterRepresentationName(selectedSegmentation->GetMasterRepresentationName());
  segmentationCopy->CopyConversionParameters(selectedSegmentation);
  for (std::vector<std::string>::iterator segmentIt = segmentIDs.begin(); segmentIt != segmentIDs.end(); ++segmentIt)
  {
    segmentationCopy->CopySegmentFromSegmentation(selectedSegmentation, (*segmentIt));
  }

  // Use dose volume geometry as reference, with oversampling of fixed 2 or automatic (as selected)
  std::string doseGeometryString = vtkSegmentationConverter::SerializeImageGeometry(doseImageData);
  segmentationCopy->SetConversionParameter( vtkSegmentationConverter::GetReferenceImageGeometryParameterName(),
    doseGeometryString );
  std::stringstream fixedOversamplingValueStream;
  fixedOversamplingValueStream << this->DefaultDoseVolumeOversamplingFactor;
  segmentationCopy->SetConversionParameter( vtkClosedSurfaceToBinaryLabelmapConversionRule::GetOversamplingFactorParameterName(),
    parameterNode->GetAutomaticOversampling() ? "A" : fixedOversamplingValueStream.str().c_str() );

  char* representationName = 0;
  bool useFractionalLabelmap = parameterNode->GetUseFractionalLabelmap();
  if (useFractionalLabelmap)
  {
    representationName = (char*)vtkSegmentationConverter::GetSegmentationFractionalLabelmapRepresentationName();
  }
  else
  {
    representationName = (char*)vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName();
  }

  // Reconvert segments to specified geometry if possible
  bool resamplingRequired = false;
  if ( !segmentationCopy->CreateRepresentation(representationName, true) )
  {
    // If conversion failed and there is no binary labelmap in the segmentation, then cannot calculate DVH
    if (!segmentationCopy->ContainsRepresentation(representationName) )
    {
      std::string errorMessage("Unable to acquire binary labelmap from segmentation");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    // If conversion failed, then resample binary labelmaps in the segments
    resamplingRequired = true;
  }

  // Calculate and store oversampling factors if automatically calculated for reporting purposes
  if (parameterNode->GetAutomaticOversampling())
  {
    // Get spacing for dose volume
    double doseSpacing[3] = {0.0,0.0,0.0};
    doseVolumeNode->GetSpacing(doseSpacing);

    // Calculate oversampling factors for all segments (need to calculate as it is not stored per segment)
    std::vector< std::string > segmentIDsCopy;
    segmentationCopy->GetSegmentIDs(segmentIDsCopy);
    for (std::vector< std::string >::const_iterator segmentIdIt = segmentIDsCopy.begin(); segmentIdIt != segmentIDsCopy.end(); ++segmentIdIt)
    {
      std::string segmentID = *segmentIdIt;
      vtkSegment* currentSegment = segmentationCopy->GetSegment(*segmentIdIt);

      vtkOrientedImageData* currentLabelmap = vtkOrientedImageData::SafeDownCast(
        currentSegment->GetRepresentation(representationName) );
      if (!currentLabelmap)
      {
        std::string errorMessage("Representation missing after converting with automatic oversampling factor");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
      double currentSpacing[3] = {0.0,0.0,0.0};
      currentLabelmap->GetSpacing(currentSpacing);

      double voxelSizeRatio = ((doseSpacing[0]*doseSpacing[1]*doseSpacing[2]) / (currentSpacing[0]*currentSpacing[1]*currentSpacing[2]));
      // Round oversampling to two decimals
      // Note: We need to round to some degree, because e.g. pow(64,1/3) is not exactly 4. It may be debated whether to round to integer or to a certain number of decimals
      double oversamplingFactor = vtkMath::Round( pow( voxelSizeRatio, 1.0/3.0 ) * 100.0 ) / 100.0;
      parameterNode->AddAutomaticOversamplingFactor(segmentID, oversamplingFactor);
    }
  }

  // Use the same resampled dose volume if oversampling is fixed
  vtkSmartPointer<vtkOrientedImageData> fixedOversampledDoseVolume;
  if (!parameterNode->GetAutomaticOversampling())
  {
    // Get geometry of oversampled dose volume
    fixedOversampledDoseVolume = vtkSmartPointer<vtkOrientedImageData>::New();
    fixedOversampledDoseVolume->ShallowCopy(doseImageData);
    vtkCalculateOversamplingFactor::ApplyOversamplingOnImageGeometry(fixedOversampledDoseVolume, this->DefaultDoseVolumeOversamplingFactor);

    // Resample dose volume using linear interpolation
    if ( !vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
      doseImageData, fixedOversampledDoseVolume, fixedOversampledDoseVolume, true ) )
    {
      std::string errorMessage("Failed to resample dose volume");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }
  }

  //
  // Compute DVH for each selected segment
  //
  int counter = 1; // Start at one so that progress can reach 100%
  int numberOfSelectedSegments = segmentationCopy->GetNumberOfSegments();
  for (std::vector< std::string >::const_iterator segmentIdIt = segmentIDs.begin(); segmentIdIt != segmentIDs.end(); ++segmentIdIt, ++counter)
  {
    std::string segmentID = *segmentIdIt;
    vtkSegment* segment = segmentationCopy->GetSegment(*segmentIdIt);

    // Get segment labelmap
    vtkOrientedImageData* segmentLabelmap = vtkOrientedImageData::SafeDownCast( segment->GetRepresentation(
      representationName ) );
    if (!segmentLabelmap)
    {
      std::string errorMessage("Failed to get labelmap for segments");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    double minimumValue = 0.0;
    vtkDoubleArray* scalarRange = vtkDoubleArray::SafeDownCast(
      segmentLabelmap->GetFieldData()->GetAbstractArray(vtkSegmentationConverter::GetScalarRangeFieldName()));
    if (scalarRange && scalarRange->GetNumberOfValues() == 2)
    {
      minimumValue = scalarRange->GetValue(0);
    }

    // Apply parent transformation nodes if necessary
    if (segmentationNode->GetParentTransformNode())
    {
      double backgroundValue[4] = {minimumValue, minimumValue, minimumValue, 0.0};
      if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(segmentationNode, segmentLabelmap, useFractionalLabelmap, backgroundValue))
      {
        std::string errorMessage("Failed to apply parent transformation to segment");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
      resamplingRequired = true;
    }
    // Resample labelmap if necessary (if it was master, and could not be re-converted using the oversampled geometry, or if there was a parent transform)
    if (resamplingRequired)
    {
      // Resample segmentation labelmap volume
      if ( !vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
        segmentLabelmap, fixedOversampledDoseVolume, segmentLabelmap, useFractionalLabelmap, false, NULL, minimumValue ) )
      {
        std::string errorMessage("Failed to resample segment binary labelmap");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
    }

    // Get oversampled dose volume
    vtkSmartPointer<vtkOrientedImageData> oversampledDoseVolume;
    // Use the same resampled dose volume if oversampling is fixed
    if (!parameterNode->GetAutomaticOversampling())
    {
      oversampledDoseVolume = fixedOversampledDoseVolume;
    }
    // Resample dose volume to match automatically oversampled segment labelmap geometry
    else
    {
      oversampledDoseVolume = vtkSmartPointer<vtkOrientedImageData>::New();
      if ( !vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
        doseImageData, segmentLabelmap, oversampledDoseVolume, true ) )
      {
        std::string errorMessage("Failed to resample dose volume");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
    }

    // Make sure the segment labelmap is the same dimension as the dose volume
    vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
    padder->SetInputData(segmentLabelmap);
    padder->SetConstant(minimumValue);
    int extent[6] = {0,-1,0,-1,0,-1};
    oversampledDoseVolume->GetExtent(extent);
    padder->SetOutputWholeExtent(extent);
    padder->Update();
    segmentLabelmap->vtkImageData::DeepCopy(padder->GetOutput());

    // Calculate DVH for current segment
    std::string errorMessage = this->ComputeDvh(parameterNode, segmentLabelmap, oversampledDoseVolume, segmentID, maxDose);
    if (!errorMessage.empty())
    {
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    // Update progress bar
    double progress = (double)counter / (double)numberOfSelectedSegments;
    this->InvokeEvent(vtkSlicerRtCommon::ProgressUpdated, (void*)&progress);
  } // For each segment

  // Fire only one modified event when the computation is done
  this->SetDisableModifiedEvent(0);
  this->Modified();
  parameterNode->EndModify(disabledNodeModify);
  // Trigger update of table
  if (parameterNode->GetMetricsTableNode())
  {
    parameterNode->GetMetricsTableNode()->Modified();
  }

  return "";
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDvh(vtkMRMLDoseVolumeHistogramNode* parameterNode, vtkOrientedImageData* segmentLabelmap, vtkOrientedImageData* oversampledDoseVolume, std::string segmentID, double maxDoseGy)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }
  if (!segmentLabelmap)
  {
    std::string errorMessage("Invalid segment labelmap");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }
  if (!oversampledDoseVolume)
  {
    std::string errorMessage("Invalid oversampled dose volume");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }
  vtkMRMLSegmentationNode* segmentationNode = parameterNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if ( !segmentationNode || !doseVolumeNode )
  {
    std::string errorMessage("Both segmentation node and dose volume node need to be set");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }
  std::string segmentName = parameterNode->GetSegmentationNode()->GetSegmentation()->GetSegment(segmentID)->GetName();
  bool isDoseVolume = vtkSlicerRtCommon::IsDoseVolumeNode(doseVolumeNode);

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // If the user has enabled the flag to calculate the dose surface histogram, then extract the surface from the labelmap
  if (parameterNode->GetDoseSurfaceHistogram())
  {
    if (parameterNode->GetUseFractionalLabelmap())
    {
      std::string errorMessage("Dose surface histogram is not currently supported for fractional labelmaps");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    double dilateValue = 0.0;
    double erodeValue = 1.0;
    if (!parameterNode->GetUseInsideDoseSurface())
    {
      dilateValue = 1.0;
      erodeValue = 0.0;
    }

    // Current implementation uses the segment labelmap and gets its inner or outer shell to calculate the DSH.
    // However, the limitation of this is that it does not support open contours. It would be more comprehensive
    // to use the original planar contour and probe filter to get the surface dose points.
    vtkNew<vtkImageDilateErode3D> dilateErodeFilter;
    dilateErodeFilter->SetInputData(segmentLabelmap);
    dilateErodeFilter->SetErodeValue(erodeValue);
    dilateErodeFilter->SetDilateValue(dilateValue);
    dilateErodeFilter->SetKernelSize(3, 3, 3);

    vtkNew<vtkImageMathematics> imageMathematics;
    imageMathematics->SetOperationToSubtract();
    if (parameterNode->GetUseInsideDoseSurface())
    {
      imageMathematics->SetInput1Data(segmentLabelmap);
      imageMathematics->SetInputConnection(1, dilateErodeFilter->GetOutputPort());
    }
    else
    {
      imageMathematics->SetInputConnection(0, dilateErodeFilter->GetOutputPort());
      imageMathematics->SetInput2Data(segmentLabelmap);
    }
    imageMathematics->Update();
    segmentLabelmap->vtkImageData::DeepCopy(imageMathematics->GetOutput());
  }

  // Create stencil for structure
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInputData(segmentLabelmap);
  // Foreground voxels are all those with an intensity > 0.
  // Unfortunately vtkImageToImageStencil only have options for < and >= comparison.
  // So, we have to choose >=epsilon (epsilon is a very small positive number).
  // How small the number is has a significance when the segmentLabelmap is a floating-point image,
  // which is a rare scenario, but may still happen.
  double minimumValue = 0.0;
  double maximumValue = 1.0;
  segmentLabelmap->GetFieldData();
  vtkDoubleArray* scalarRange = vtkDoubleArray::SafeDownCast(
    segmentLabelmap->GetFieldData()->GetAbstractArray( vtkSegmentationConverter::GetScalarRangeFieldName() )
    );
  if (scalarRange && scalarRange->GetNumberOfValues() == 2)
  {
    minimumValue = scalarRange->GetValue(0);
    maximumValue = scalarRange->GetValue(1);
  }

  bool useFractionalLabelmap = parameterNode->GetUseFractionalLabelmap();
  if (useFractionalLabelmap)
  {
    stencil->ThresholdByUpper(minimumValue + 1e-10);
  }
  else
  {
    stencil->ThresholdByUpper(1e-10);
  }
  stencil->Update();

  vtkSmartPointer<vtkImageStencilData> structureStencil = vtkSmartPointer<vtkImageStencilData>::New();
  structureStencil->DeepCopy(stencil->GetOutput());

  int stencilExtent[6] = {0,-1,0,-1,0,-1};
  structureStencil->GetExtent(stencilExtent);
  if (stencilExtent[1]-stencilExtent[0] <= 0 || stencilExtent[3]-stencilExtent[2] <= 0 || stencilExtent[5]-stencilExtent[4] <= 0)
  {
    std::string errorMessage("Invalid stenciled dose volume");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Compute statistics
  vtkSmartPointer<vtkImageAccumulate> structureStat;
  if (useFractionalLabelmap)
  {
    structureStat = vtkSmartPointer<vtkFractionalImageAccumulate>::New();
    vtkFractionalImageAccumulate::SafeDownCast(structureStat)->UseFractionalLabelmapOn();
    vtkFractionalImageAccumulate::SafeDownCast(structureStat)->SetFractionalLabelmap(segmentLabelmap);
    vtkFractionalImageAccumulate::SafeDownCast(structureStat)->SetMinimumFractionalValue(minimumValue);
    vtkFractionalImageAccumulate::SafeDownCast(structureStat)->SetMaximumFractionalValue(maximumValue);
  }
  else
  {
    structureStat = vtkSmartPointer<vtkImageAccumulate>::New();
  }
  structureStat->SetInputData(oversampledDoseVolume);
  structureStat->SetStencilData(structureStencil);
  structureStat->Update();

  // Report error if there are no voxels in the stenciled dose volume (no non-zero voxels in the resampled labelmap)
  if (structureStat->GetVoxelCount() < 1)
  {
    std::string errorMessage("Dose volume and the structure do not overlap"); // User-friendly error to help troubleshooting
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Get metrics table for the parameter node; Create one if missing
  vtkMRMLTableNode* metricsTableNode = parameterNode->GetMetricsTableNode();
  vtkTable* metricsTable = metricsTableNode->GetTable();
  // Setup table if empty
  if (metricsTable->GetNumberOfColumns() == 0)
  {
    this->InitializeMetricsTable(parameterNode);
  }

  // Get DVH table node for the inputs (dose volume, segmentation, segment).
  // If found, then it gets overwritten by the new computation, otherwise
  std::string structureDvhNodeRef = parameterNode->AssembleDvhNodeReference(segmentID);
  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(metricsTableNode->GetNodeReference(structureDvhNodeRef.c_str()));
  int tableRow = -1;
  if (!tableNode)
  {
    // Create DVH table node
    tableNode = vtkMRMLTableNode::New();
    std::string dvhTableNodeName = segmentID + DVH_TABLE_NODE_NAME_POSTFIX;
    dvhTableNodeName = this->GetMRMLScene()->GenerateUniqueName(dvhTableNodeName);
    tableNode->SetName(dvhTableNodeName.c_str());
    tableNode->SetAttribute(DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
    vtkNew<vtkTable> table;
    tableNode->SetAndObserveTable(table);
    this->GetMRMLScene()->AddNode(tableNode);

    //TODO: Add schema?

    // Add new row in metrics table
    tableRow = metricsTable->GetNumberOfRows();
    std::stringstream ss;
    ss << tableRow;
    tableNode->SetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str(), ss.str().c_str());
    tableNode->Delete(); // Release ownership to scene only
    metricsTable->InsertNextBlankRow();

    // Dose surface histogram attributes
    if (parameterNode->GetDoseSurfaceHistogram())
    {
      tableNode->SetAttribute(DVH_SURFACE_ATTRIBUTE_NAME.c_str(), "1");
      tableNode->SetAttribute(DVH_SURFACE_INSIDE_ATTRIBUTE_NAME.c_str(), parameterNode->GetUseInsideDoseSurface() ? "1" : "0");
    }

    // Set node references
    metricsTableNode->SetNodeReferenceID(structureDvhNodeRef.c_str(), tableNode->GetID());
    tableNode->SetNodeReferenceID(vtkMRMLDoseVolumeHistogramNode::DOSE_VOLUME_REFERENCE_ROLE, doseVolumeNode->GetID());
    tableNode->SetNodeReferenceID(vtkMRMLDoseVolumeHistogramNode::SEGMENTATION_REFERENCE_ROLE, segmentationNode->GetID());
    tableNode->SetNodeReferenceID(vtkMRMLDoseVolumeHistogramNode::DVH_METRICS_TABLE_REFERENCE_ROLE, metricsTableNode->GetID());
  }
  else if (tableNode->GetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str()))
  {
    tableRow = vtkVariant(tableNode->GetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str())).ToInt();
  }
  else
  {
    std::string errorMessage("Failed to find metrics table row for structure " + segmentName);
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Set table node attributes:
  // Structure name and segment color for visualization in the chart view
  tableNode->SetAttribute(DVH_SEGMENT_ID_ATTRIBUTE_NAME.c_str(), segmentID.c_str());
  // Oversampling factor
  std::ostringstream oversamplingAttrValueStream;
  oversamplingAttrValueStream << (parameterNode->GetAutomaticOversampling() ? (-1.0) : this->DefaultDoseVolumeOversamplingFactor);
  tableNode->SetAttribute(DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME.c_str(), oversamplingAttrValueStream.str().c_str());

  // Get spacing and voxel volume
  double* segmentLabelmapSpacing = segmentLabelmap->GetSpacing();
  double cubicMMPerVoxel = segmentLabelmapSpacing[0] * segmentLabelmapSpacing[1] * segmentLabelmapSpacing[2];
  double ccPerCubicMM = 0.001;

  // Set default column values

  // Structure name
  metricsTable->SetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnStructure, vtkVariant(segmentName));
  // Volume name
  metricsTable->SetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnDoseVolume, vtkVariant(doseVolumeNode->GetName()));
  // Volume (cc) - save as attribute too (the DVH contains percentages that often need to be converted to volume)
  double volumeCc = 0;
  if (useFractionalLabelmap)
  {
    volumeCc = vtkFractionalImageAccumulate::SafeDownCast(structureStat)->GetFractionalVoxelCount() * cubicMMPerVoxel * ccPerCubicMM;
  }
  else
  {
    volumeCc = structureStat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM;
  }
  metricsTable->SetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnVolumeCc, vtkVariant(volumeCc));
  std::ostringstream attributeNameStream;
  std::ostringstream attributeValueStream;
  attributeNameStream << vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC;
  attributeValueStream << volumeCc;
  tableNode->SetAttribute(attributeNameStream.str().c_str(), attributeValueStream.str().c_str());
  // Mean dose
  metricsTable->SetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnMeanDose, vtkVariant(structureStat->GetMean()[0]));
  // Min dose
  metricsTable->SetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnMinDose, vtkVariant(structureStat->GetMin()[0]));
  // Max dose
  metricsTable->SetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnMaxDose, vtkVariant(structureStat->GetMax()[0]));

  // Create DVH plot values
  int numSamples = 0;
  double startValue = 0.0;
  double stepSize = 0.0;
  double rangeMin = structureStat->GetMin()[0];
  double rangeMax = structureStat->GetMax()[0];
  if (isDoseVolume)
  {
    if (rangeMin<0)
    {
      std::string errorMessage("The dose volume contains negative dose values");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    startValue = this->StartValue;
    stepSize = this->StepSize;
    numSamples = (int)ceil( (maxDoseGy-startValue)/stepSize ) + 1;
  }
  else
  {
    startValue = rangeMin;
    numSamples = this->NumberOfSamplesForNonDoseVolumes;
    stepSize = (rangeMax - rangeMin) / (double)(numSamples-1);
  }

  // Get the number of voxels with smaller dose than at the start value
  structureStat->SetComponentExtent(0,1,0,0,0,0);
  structureStat->SetComponentOrigin(0,0,0);
  structureStat->SetComponentSpacing(startValue,1,1);
  structureStat->Update();
  double voxelBelowDose = structureStat->GetOutput()->GetScalarComponentAsDouble(0,0,0,0);

  // We put a fixed point at (0.0, 100%), but only if there are only positive values in the histogram
  // Negative values can occur when the user requests histogram for an image, such as s CT volume (in
  // this case Intensity Volume Histogram is computed), or the startValue became negative for the dose
  // volume because the range minimum was smaller than the original start value.
  bool insertPointAtOrigin = true;
  if (startValue < 0.0)
  {
    insertPointAtOrigin = false;
  }

  structureStat->SetComponentExtent(0,numSamples-1,0,0,0,0);
  structureStat->SetComponentOrigin(startValue,0,0);
  structureStat->SetComponentSpacing(stepSize,1,1);
  structureStat->Update();

  // Allocate table
  vtkTable* table = tableNode->GetTable();
  int numberOfRows = numSamples + (insertPointAtOrigin?1:0);
  vtkNew<vtkDoubleArray> columnDose;
  columnDose->SetName(isDoseVolume ? "Dose" : "Intensity");
  columnDose->SetNumberOfTuples(numberOfRows);
  table->AddColumn(columnDose);
  vtkNew<vtkDoubleArray> columnVolume;
  columnVolume->SetName("Volume");
  columnVolume->SetNumberOfTuples(numberOfRows);
  table->AddColumn(columnVolume);
  table->SetNumberOfRows(numberOfRows);

  int rowIndex = 0;

  if (insertPointAtOrigin)
  {
    // Add first fixed point at (0.0, 100%)
    table->SetValue(rowIndex, 0, 0.0);
    table->SetValue(rowIndex, 1, 100.0);
    table->SetValue(rowIndex, 2, 0);
    ++rowIndex;
  }

  vtkImageData* statArray = structureStat->GetOutput();
  double totalVoxels = 0;
  if (useFractionalLabelmap)
  {
    totalVoxels = vtkFractionalImageAccumulate::SafeDownCast(structureStat)->GetFractionalVoxelCount();
  }
  else
  {
    totalVoxels = structureStat->GetVoxelCount();
  }

  for (int sampleIndex=0; sampleIndex<numSamples; ++sampleIndex)
  {
    double voxelsInBin = statArray->GetScalarComponentAsDouble(sampleIndex,0,0,0);
    table->SetValue(rowIndex, 0, startValue + sampleIndex * stepSize);
    if (useFractionalLabelmap)
    {
      table->SetValue(rowIndex, 1, std::max(0.0, (1.0-(double)voxelBelowDose/(double)totalVoxels)*100.0));
    }
    else
    {
      table->SetValue(rowIndex, 1, (1.0-(double)voxelBelowDose/(double)totalVoxels)*100.0);
    }
    table->SetValue(rowIndex, 2, 0);
    ++rowIndex;
    voxelBelowDose += voxelsInBin;
  }

  // Set the start of the first bin to 0 if the volume contains dose and the start value was negative
  if (isDoseVolume && !insertPointAtOrigin)
  {
    table->SetValue(0, 0, 0.0);
  }

  // Setup DVH subject hierarchy items
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    std::string errorMessage("Failed to access subject hierarchy node");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);

  // Add metrics table and chart to under the study of the dose in subject hierarchy
  vtkIdType studyItemID = shNode->GetItemAncestorAtLevel(doseShItemID, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  if (studyItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    vtkIdType metricsShItemID = shNode->CreateItem(studyItemID, metricsTableNode);
    shNode->CreateItem(metricsShItemID, tableNode);

    vtkMRMLPlotChartNode* chartNode = parameterNode->GetChartNode();
    shNode->CreateItem(studyItemID, chartNode);
  }

  // Add connection attribute to input segmentation and dose volume nodes
  segmentationNode->AddNodeReferenceID(DVH_CREATED_DVH_NODE_REFERENCE_ROLE.c_str(), tableNode->GetID());
  doseVolumeNode->AddNodeReferenceID(DVH_CREATED_DVH_NODE_REFERENCE_ROLE.c_str(), tableNode->GetID());

  // Log measured time
  double checkpointEnd = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
  if (this->LogSpeedMeasurements)
  {
    vtkDebugMacro("ComputeDvh: DVH computation time for structure '" << segmentID << "': " << checkpointEnd-checkpointStart << " s");
  }

  return ""; // No error
} // end ComputeDvh

//---------------------------------------------------------------------------
vtkMRMLPlotViewNode* vtkSlicerDoseVolumeHistogramModuleLogic::GetPlotViewNode()
{
  vtkMRMLPlotViewNode* plotViewNode = vtkMRMLPlotViewNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByClass("vtkMRMLPlotViewNode") );
  if (!plotViewNode)
  {
    vtkErrorMacro("GetPlotViewNode: Unable to get chart view node");
    return NULL;
  }

  return plotViewNode;
}

//---------------------------------------------------------------------------
vtkMRMLPlotSeriesNode* vtkSlicerDoseVolumeHistogramModuleLogic::AddDvhToChart(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode)
{
  if (!this->GetMRMLScene() || !chartNode || !tableNode)
  {
    vtkErrorMacro("AddDvhToChart: Invalid MRML scene, chart node, or DVH node");
    return NULL;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("AddDvhToChart: Failed to access subject hierarchy node");
    return NULL;
  }

  const char* segmentId = tableNode->GetAttribute(DVH_SEGMENT_ID_ATTRIBUTE_NAME.c_str());

  // Get selected segmentation and dose volume nodes
  vtkMRMLScalarVolumeNode* doseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(tableNode->GetNodeReference(vtkMRMLDoseVolumeHistogramNode::DOSE_VOLUME_REFERENCE_ROLE));
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(tableNode->GetNodeReference(vtkMRMLDoseVolumeHistogramNode::SEGMENTATION_REFERENCE_ROLE));
  if (!doseVolumeNode || !segmentationNode)
  {
    vtkErrorMacro("AddDvhToChart: Unable to find all referenced nodes");
    return NULL;
  }
  vtkSegment* segment = segmentationNode->GetSegmentation()->GetSegment(segmentId);
  if (!segment)
  {
    vtkErrorMacro("AddDvhToChart: Unable to get segment");
    return NULL;
  }
  std::string segmentName(segment->GetName());

  // Switch to plot layout if it has not been set before.
  // This step is necessary to take before accessing the plot view node, as switching makes sure there is one
  vtkMRMLLayoutNode* layoutNode = vtkMRMLLayoutNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByClass("vtkMRMLLayoutNode") );
  if ( layoutNode
    && layoutNode->GetViewArrangement() != vtkMRMLLayoutNode::SlicerLayoutConventionalPlotView
    && layoutNode->GetViewArrangement() != vtkMRMLLayoutNode::SlicerLayoutFourUpPlotView
    && layoutNode->GetViewArrangement() != vtkMRMLLayoutNode::SlicerLayoutOneUpPlotView )
  {
    layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutFourUpPlotView );
  }

  // Get plot view node
  vtkMRMLPlotViewNode* plotViewNode = this->GetPlotViewNode();
  if (plotViewNode == NULL)
  {
    vtkErrorMacro("AddDvhToChart: Unable to get plot view node");
    return NULL;
  }

  // Create plot series node for the DVH table if needed
  vtkMRMLPlotSeriesNode* plotSeriesNode = this->GetPlotSeriesNodeForTable(chartNode, tableNode);
  if (!plotSeriesNode)
  {
    plotSeriesNode = vtkMRMLPlotSeriesNode::New();
    plotSeriesNode->SetAttribute(DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
    plotSeriesNode->SetName(segmentName.c_str()); // Legend can only show the name of the plot series node
    this->GetMRMLScene()->AddNode(plotSeriesNode);
    plotSeriesNode->SetAndObserveTableNodeID(tableNode->GetID());
    plotSeriesNode->Delete(); // Release ownership to scene only

    // Set persistent reference indicating correspondence
    // (plot series in chart indicates visibility so need a separate reference for correspondence)
    chartNode->AddAndObserveNodeReferenceID(DVH_PLOTSERIES_REFERENCE_ROLE.c_str(), plotSeriesNode->GetID());

    // Setup plot series subject hierarchy item
    vtkIdType chartShItemID = shNode->GetItemByDataNode(chartNode);
    shNode->CreateItem(chartShItemID, plotSeriesNode);
  }

  // Set chart general properties
  std::string doseAxisName("");
  std::string chartTitle("");
  const char* doseIdentifier = doseVolumeNode->GetAttribute(vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  if (doseIdentifier)
  {
    vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
    if (doseShItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
      std::string doseUnitName = shNode->GetAttributeFromItemAncestor(
        doseShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
      doseAxisName = std::string("Dose [") + (doseUnitName.empty() ? "?" : doseUnitName) + "]";
    }
    else
    {
      vtkErrorMacro("AddDvhToChart: Invalid subject hierarchy node for dose volume");
      doseAxisName = std::string("Dose");
    }

    const char* doseSurfaceHistogramIdentifier = tableNode->GetAttribute(DVH_SURFACE_ATTRIBUTE_NAME.c_str());
    if (doseSurfaceHistogramIdentifier)
    {
      chartTitle = "Dose Surface Histogram";
    }
    else
    {
      chartTitle = "Dose Volume Histogram";
    }
  }
  else
  {
    doseAxisName="Intensity";
    chartTitle="Intensity Volume Histogram";
  }

  chartNode->SetTitle(chartTitle.c_str());
  chartNode->SetXAxisTitle(doseAxisName.c_str());
  plotSeriesNode->SetXColumnName(doseIdentifier ? "Dose" : "Intensity");
  chartNode->SetYAxisTitle("Fractional volume [%]");
  plotSeriesNode->SetYColumnName("Volume");
  plotSeriesNode->SetPlotType(vtkMRMLPlotSeriesNode::PlotTypeScatter);
  plotSeriesNode->SetMarkerStyle(vtkMRMLPlotSeriesNode::MarkerStyleNone);

  // Get number of tables showing plot for the same structure (for plot name and line style)
  int indexOfTableNodeForSameStructure = 0;
  std::vector<vtkMRMLNode*> tableNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLTableNode", tableNodes);
  for (std::vector<vtkMRMLNode*>::iterator tableNodeIt = tableNodes.begin(); tableNodeIt != tableNodes.end(); ++tableNodeIt)
  {
    vtkMRMLNode* currentTableNode = (*tableNodeIt);
    if (currentTableNode == tableNode)
    {
      // Current table reached, stop counting
      // (because we are interested in the ordinality of the displayed table node in the scene)
      break;
    }

    std::string currentSegmentName("");
    const char* currentSegmentId = currentTableNode->GetAttribute(DVH_SEGMENT_ID_ATTRIBUTE_NAME.c_str());
    vtkMRMLSegmentationNode* currentSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(currentTableNode->GetNodeReference(vtkMRMLDoseVolumeHistogramNode::SEGMENTATION_REFERENCE_ROLE));
    if (!currentSegmentId)
    {
      // Not a DVH table node
      continue;
    }
    if (currentSegmentId && currentSegmentationNode == segmentationNode)
    {
      vtkSegment* currentSegment = currentSegmentationNode->GetSegmentation()->GetSegment(currentSegmentId);
      if (currentSegment)
      {
        currentSegmentName = std::string(currentSegment->GetName());
      }
    }
    if (currentSegmentName.empty())
    {
      vtkErrorMacro("AddDvhToChart: Failed to get segment name for DVH array " << currentTableNode->GetName());
      continue;
    }
    if (!currentSegmentName.compare(segmentName))
    {
      ++indexOfTableNodeForSameStructure;
    }
  }

  // Set plot style
  switch (indexOfTableNodeForSameStructure % 5)
  {
  case 0:
    plotSeriesNode->SetLineStyle(vtkMRMLPlotSeriesNode::LineStyleSolid);
    break;
  case 1:
    plotSeriesNode->SetLineStyle(vtkMRMLPlotSeriesNode::LineStyleDash);
    break;
  case 2:
    plotSeriesNode->SetLineStyle(vtkMRMLPlotSeriesNode::LineStyleDot);
    break;
  case 3:
    plotSeriesNode->SetLineStyle(vtkMRMLPlotSeriesNode::LineStyleDashDot);
    break;
  case 4:
    plotSeriesNode->SetLineStyle(vtkMRMLPlotSeriesNode::LineStyleDashDotDot);
    break;
  }

  // Set plot color
  double* segmentColor = segment->GetColor();
  plotSeriesNode->SetColor(segmentColor[0], segmentColor[1], segmentColor[2]);

  // Add chart to chart view
  plotViewNode->SetPlotChartNodeID(chartNode->GetID());

  // Add series to chart
  chartNode->AddAndObservePlotSeriesNodeID(plotSeriesNode->GetID());

  return plotSeriesNode;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::RemoveDvhFromChart(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode)
{
  if (!this->GetMRMLScene() || !chartNode || !tableNode)
  {
    vtkErrorMacro("RemoveDvhFromChart: Invalid MRML scene, chart node, or DVH node");
    return;
  }

  vtkMRMLPlotViewNode* plotViewNode = this->GetPlotViewNode();
  if (plotViewNode == NULL)
  {
    vtkErrorMacro("RemoveDvhFromChart: Unable to get plot view node");
    return;
  }

  vtkMRMLPlotSeriesNode* plotSeriesNode = this->GetPlotSeriesNodeForTable(chartNode, tableNode);
  if (!plotSeriesNode)
  {
    vtkErrorMacro("RemoveDvhFromChart: Unable to get plot series node for table");
    return;
  }
  chartNode->RemovePlotSeriesNodeID(plotSeriesNode->GetID());
}

//---------------------------------------------------------------------------
vtkMRMLPlotSeriesNode* vtkSlicerDoseVolumeHistogramModuleLogic::IsDvhAddedToChart(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode)
{
  if (!this->GetMRMLScene() || !chartNode || !tableNode)
  {
    vtkErrorMacro("IsDvhAddedToChart: Invalid MRML scene, chart node, or DVH node");
    return false;
  }

  // Find plot series node currently shown in chart for a DVH table
  for (int i=0; i<chartNode->GetNumberOfPlotSeriesNodes(); ++i)
  {
    vtkMRMLPlotSeriesNode* currentSeriesNode = chartNode->GetNthPlotSeriesNode(i);
    if (currentSeriesNode->GetTableNode() == tableNode)
    {
      return currentSeriesNode;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
vtkMRMLPlotSeriesNode* vtkSlicerDoseVolumeHistogramModuleLogic::GetPlotSeriesNodeForTable(vtkMRMLPlotChartNode* chartNode, vtkMRMLTableNode* tableNode)
{
  if (!chartNode || !tableNode)
  {
    vtkErrorMacro("GetPlotSeriesNodeForTable: Invalid chart node or table node");
    return NULL;
  }

  // Find plot series node created for chart for a DVH table. Finds also if not currently visible
  std::vector<vtkMRMLNode*> referencedPlotSeriesNodes;
  chartNode->GetNodeReferences(DVH_PLOTSERIES_REFERENCE_ROLE.c_str(), referencedPlotSeriesNodes);
  for ( std::vector<vtkMRMLNode*>::iterator seriesIt = referencedPlotSeriesNodes.begin();
    seriesIt != referencedPlotSeriesNodes.end(); ++seriesIt )
  {
    vtkMRMLPlotSeriesNode* currentSeriesNode = vtkMRMLPlotSeriesNode::SafeDownCast(*seriesIt);
    if (currentSeriesNode->GetTableNode() == tableNode)
    {
      return currentSeriesNode;
    }
  }

  return NULL;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::GetNumbersFromMetricString(std::string metricStr, std::vector<double> &metricNumbers)
{
  metricNumbers.clear();

  size_t commaPosition = metricStr.find(",");
  while (commaPosition != std::string::npos)
  {
    std::stringstream ss;
    ss << metricStr.substr(0, commaPosition);
    double num = -1;
    ss >> num;
    if (ss.fail())
    {
      vtkWarningMacro("GetNumbersFromMetricString: Invalid metric value in string: '" << metricStr.substr(0, commaPosition) << "'");
    }
    else
    {
      metricNumbers.push_back(num);
    }
    metricStr = metricStr.substr(commaPosition+1);
    commaPosition = metricStr.find(",");
  }
  if (!metricStr.empty())
  {
    std::stringstream ss;
    ss << metricStr;
    double num = -1;
    ss >> num;
    if (ss.fail())
    {
      vtkWarningMacro("GetNumbersFromMetricString: Invalid metric value in string: '" << metricStr << "'");
    }
    else
    {
      metricNumbers.push_back(num);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::IsVMetricName(std::string name)
{
  if (name.empty())
  {
    return false;
  }
  // First character needs to be a 'V'
  if (name.substr(0,1).compare("V"))
  {
    return false;
  }
  // If second character is a number, then we consider it a V metric
  std::stringstream secondCharStream;
  secondCharStream << name.substr(1,1);
  int secondCharNumber = -1;
  secondCharStream >> secondCharNumber;
  return !secondCharStream.fail();
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::ComputeVMetrics(vtkMRMLDoseVolumeHistogramNode* parameterNode)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("ComputeVMetrics: Invalid MRML scene or parameter set node");
    return false;
  }
  vtkMRMLTableNode* metricsTableNode = parameterNode->GetMetricsTableNode();
  if (!metricsTableNode)
  {
    vtkErrorMacro("ComputeVMetrics: Unable to access DVH metrics table");
    return false;
  }

  // Remove all V metrics from the table
  vtkTable* metricsTable = metricsTableNode->GetTable();
  int numberOfColumnsBeforeRemoval = -1;
  do
  {
    numberOfColumnsBeforeRemoval = metricsTable->GetNumberOfColumns();
    for (int col=0; col<metricsTable->GetNumberOfColumns(); ++col)
    {
      std::string columnName(metricsTable->GetColumnName(col));
      if (this->IsVMetricName(columnName))
      {
        metricsTable->RemoveColumn(col);
        break;
      }
    }
  }
  while (numberOfColumnsBeforeRemoval != metricsTable->GetNumberOfColumns());

  // If no V metrics need to be shown then exit
  if (!parameterNode->GetShowVMetricsCc() && !parameterNode->GetShowVMetricsPercent())
  {
    return true;
  }

  // Get V metric dose values from input string
  std::string doseValuesStr(parameterNode->GetVDoseValues()?parameterNode->GetVDoseValues():"");
  std::vector<double> doseValues;
  this->GetNumbersFromMetricString(doseValuesStr, doseValues);

  // Create table columns for requested V metrics
  int numberOfColumnsBefore = metricsTable->GetNumberOfColumns();
  for (std::vector<double>::iterator doseValueIt=doseValues.begin(); doseValueIt!=doseValues.end(); ++doseValueIt)
  {
    if (parameterNode->GetShowVMetricsCc())
    {
      std::stringstream newColumnName;
      newColumnName << "V" << (*doseValueIt) << " (cc)";
      vtkAbstractArray* newColumn = metricsTableNode->AddColumn();
      newColumn->SetName(newColumnName.str().c_str());
      metricsTable->AddColumn(newColumn);
    }
    if (parameterNode->GetShowVMetricsPercent())
    {
      std::stringstream newColumnName;
      newColumnName << "V" << (*doseValueIt) << " (%)";
      vtkAbstractArray* newColumn = metricsTableNode->AddColumn();
      newColumn->SetName(newColumnName.str().c_str());
      metricsTable->AddColumn(newColumn);
    }
  }

  // Traverse all DVH nodes referenced from metrics table and calculate V metrics
  std::vector<std::string> roles;
  metricsTableNode->GetNodeReferenceRoles(roles);
  for (std::vector<std::string>::iterator roleIt=roles.begin(); roleIt!=roles.end(); ++roleIt)
  {
    if ( roleIt->substr(0, vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX.size()).compare(
      vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX ) )
    {
      // Not a DVH reference
      continue;
    }

    // Get DVH node
    vtkMRMLTableNode* dvhTableNode = vtkMRMLTableNode::SafeDownCast(metricsTableNode->GetNodeReference(roleIt->c_str()));
    if (!dvhTableNode)
    {
      vtkErrorMacro("ComputeVMetrics: Metrics table node reference '" << (*roleIt) << "' does not contain DVH node");
      continue;
    }

    // Get corresponding table row
    int tableRow = -1;
    std::stringstream ss;
    ss << dvhTableNode->GetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str());
    ss >> tableRow;
    if (ss.fail())
    {
      vtkErrorMacro("ComputeVMetrics: Failed to get metrics table row from DVH node " << dvhTableNode->GetName());
      continue;
    }

    // Get structure volume
    double structureVolume = metricsTable->GetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnVolumeCc).ToDouble();
    if (structureVolume == 0)
    {
      vtkErrorMacro("ComputeVMetrics: Failed to get structure volume for structure " << metricsTable->GetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnStructure).ToString());
      continue;
    }

    // Compute volume for all V's
    vtkTable* table = dvhTableNode->GetTable();
    vtkNew<vtkPiecewiseFunction> interpolator;
    interpolator->ClampingOn();

    // Starting from second point, because BuildFunctionFromTable needs uniform distance between X coordinates
    //  and the first point may be 0 (with different distance to neighbors than the second)
    double* dvhArrayDouble = new double[table->GetNumberOfRows()-1];
    for (int i=1; i<table->GetNumberOfRows(); ++i)
    {
      dvhArrayDouble[i-1] = table->GetValue(i, 1).ToDouble();
    }
    interpolator->BuildFunctionFromTable(
      table->GetValue(1, 0).ToDouble(), table->GetValue(table->GetNumberOfRows()-1, 0).ToDouble(), table->GetNumberOfRows()-1,
      dvhArrayDouble );

    // Add first point
    interpolator->AddPoint(table->GetValue(0, 0).ToDouble(), table->GetValue(0, 1).ToDouble());

    // Calculate metrics and set table entries
    int tableColumn = numberOfColumnsBefore;
    for (std::vector<double>::iterator it = doseValues.begin(); it != doseValues.end(); ++it)
    {
      double volumePercentEstimated = interpolator->GetValue(*it);
      if (parameterNode->GetShowVMetricsCc())
      {
        metricsTable->SetValue( tableRow, tableColumn++, vtkVariant(volumePercentEstimated*structureVolume/100.0) );
      }
      if (parameterNode->GetShowVMetricsPercent())
      {
        metricsTable->SetValue( tableRow, tableColumn++, vtkVariant(volumePercentEstimated) );
      }
    }
    delete[] dvhArrayDouble;
  } // For all DVHs

  metricsTableNode->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::IsDMetricName(std::string name)
{
  if (name.empty())
  {
    return false;
  }
  // First character needs to be a 'D'
  if (name.substr(0,1).compare("D"))
  {
    return false;
  }
  // If second character is a number, then we consider it a D metric
  std::stringstream secondCharStream;
  secondCharStream << name.substr(1,1);
  int secondCharNumber = -1;
  secondCharStream >> secondCharNumber;
  return !secondCharStream.fail();
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDMetrics(vtkMRMLDoseVolumeHistogramNode* parameterNode)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("ComputeDMetrics: Invalid MRML scene or parameter set node");
    return false;
  }
  vtkMRMLTableNode* metricsTableNode = parameterNode->GetMetricsTableNode();
  if (!metricsTableNode)
  {
    vtkErrorMacro("ComputeDMetrics: Unable to access DVH metrics table");
    return false;
  }
  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    vtkErrorMacro("ComputeDMetrics: Unable to find dose volume node");
    return false;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("ComputeDMetrics: Failed to access subject hierarchy node");
    return false;
  }

  // Get dose unit name
  std::string doseUnitPostfix = "";
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (doseShItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    doseUnitPostfix = " (" +
      shNode->GetAttributeFromItemAncestor(
        doseShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy())
      + ")";
  }

  // Remove all D metrics from the table
  vtkTable* metricsTable = metricsTableNode->GetTable();
  int numberOfColumnsBeforeRemoval = -1;
  do
  {
    numberOfColumnsBeforeRemoval = metricsTable->GetNumberOfColumns();
    for (int col=0; col<metricsTable->GetNumberOfColumns(); ++col)
    {
      std::string columnName(metricsTable->GetColumnName(col));
      if (this->IsDMetricName(columnName))
      {
        metricsTable->RemoveColumn(col);
        break;
      }
    }
  }
  while (numberOfColumnsBeforeRemoval != metricsTable->GetNumberOfColumns());

  // If no D metrics need to be shown then exit
  if (!parameterNode->GetShowDMetrics())
  {
    return true;
  }

  // Get D metric dose values from input string
  std::vector<double> volumeValuesCc;
  std::vector<double> volumeValuesPercent;
  if (parameterNode->GetDVolumeValuesCc())
  {
    std::string volumeValuesCcStr(parameterNode->GetDVolumeValuesCc()?parameterNode->GetDVolumeValuesCc():"");
    this->GetNumbersFromMetricString(volumeValuesCcStr, volumeValuesCc);
  }
  if (parameterNode->GetDVolumeValuesPercent())
  {
    std::string volumeValuesPercentStr(parameterNode->GetDVolumeValuesPercent()?parameterNode->GetDVolumeValuesPercent():"");
    this->GetNumbersFromMetricString(volumeValuesPercentStr, volumeValuesPercent);
  }

  // Create table columns for requested D metrics
  int numberOfColumnsBefore = metricsTable->GetNumberOfColumns();
  for (std::vector<double>::iterator ccIt=volumeValuesCc.begin(); ccIt!=volumeValuesCc.end(); ++ccIt)
  {
    std::stringstream newColumnName;
    newColumnName << "D" << (*ccIt) << "cc" << doseUnitPostfix;
    vtkAbstractArray* newColumn = metricsTableNode->AddColumn();
    newColumn->SetName(newColumnName.str().c_str());
    metricsTable->AddColumn(newColumn);
  }
  for (std::vector<double>::iterator percentIt=volumeValuesPercent.begin(); percentIt!=volumeValuesPercent.end(); ++percentIt)
  {
    std::stringstream newColumnName;
    newColumnName << "D" << (*percentIt) << "%" << doseUnitPostfix;
    vtkAbstractArray* newColumn = metricsTableNode->AddColumn();
    newColumn->SetName(newColumnName.str().c_str());
    metricsTable->AddColumn(newColumn);
  }

  // Traverse all DVH nodes referenced from metrics table and calculate V metrics
  std::vector<std::string> roles;
  metricsTableNode->GetNodeReferenceRoles(roles);
  for (std::vector<std::string>::iterator roleIt=roles.begin(); roleIt!=roles.end(); ++roleIt)
  {
    if ( roleIt->substr(0, vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX.size()).compare(
      vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX ) )
    {
      // Not a DVH reference
      continue;
    }

    // Get DVH node
    vtkMRMLTableNode* dvhTableNode = vtkMRMLTableNode::SafeDownCast(metricsTableNode->GetNodeReference(roleIt->c_str()));
    if (!dvhTableNode)
    {
      vtkErrorMacro("ComputeDMetrics: Metrics table node reference '" << (*roleIt) << "' does not contain DVH node");
      continue;
    }

    // Get corresponding table row
    int tableRow = -1;
    std::stringstream ss;
    ss << dvhTableNode->GetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str());
    ss >> tableRow;
    if (ss.fail())
    {
      vtkErrorMacro("ComputeDMetrics: Failed to get metrics table row from DVH node " << dvhTableNode->GetName());
      continue;
    }

    // Get structure volume
    double structureVolume = metricsTable->GetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnVolumeCc).ToDouble();
    if (structureVolume == 0)
    {
      vtkErrorMacro("ComputeDMetrics: Failed to get structure volume for structure " << metricsTable->GetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnStructure).ToString());
      continue;
    }

    // Calculate metrics and set table entries
    int tableColumn = numberOfColumnsBefore;
    for (std::vector<double>::iterator ccIt=volumeValuesCc.begin(); ccIt!=volumeValuesCc.end(); ++ccIt)
    {
      double d = ComputeDMetric(dvhTableNode, (*ccIt), structureVolume, false);
      metricsTable->SetValue(tableRow, tableColumn++, vtkVariant(d));
    }
    for (std::vector<double>::iterator percentIt=volumeValuesPercent.begin(); percentIt!=volumeValuesPercent.end(); ++percentIt)
    {
      double d = ComputeDMetric(dvhTableNode, (*percentIt), structureVolume, true);
      metricsTable->SetValue(tableRow, tableColumn++, vtkVariant(d));
    }
  } // For all DVHs

  metricsTableNode->Modified();
  return true;
}

//---------------------------------------------------------------------------
double vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDMetric(vtkMRMLTableNode* tableNode, double volume, double structureVolume, bool isPercent)
{
  if (!tableNode)
  {
    vtkErrorMacro("ComputeDMetric: Invalid DVH array node");
    return 0.0;
  }
  if (isPercent && structureVolume == 0.0)
  {
    vtkErrorMacro("ComputeDMetric: Invalid structure volume");
    return 0.0;
  }

  vtkTable* table = tableNode->GetTable();
  double volumeSize = 0.0;
  double doseForVolume = 0.0;

  if (isPercent)
  {
    volumeSize = volume * structureVolume / 100.0;
  }
  else
  {
    volumeSize = volume;
  }

  // Check if the given volume is above the highest (first) in the array then assign no dose
  if (volumeSize >= table->GetValue(0, 1).ToDouble() / 100.0 * structureVolume)
  {
    doseForVolume = 0.0;
  }
  // If volume is below the lowest (last) in the array then assign maximum dose
  else if (volumeSize < table->GetValue(table->GetNumberOfRows()-1, 1).ToDouble() / 100.0 * structureVolume)
  {
    doseForVolume = table->GetValue(table->GetNumberOfRows()-1, 0).ToDouble();
  }
  else
  {
    for (int i=0; i<table->GetNumberOfRows()-1; ++i)
    {
      double volumePrevious = table->GetValue(i, 1).ToDouble() / 100.0 * structureVolume;
      double volumeNext = table->GetValue(i+1, 1).ToDouble() / 100.0 * structureVolume;
      if (volumePrevious > volumeSize && volumeSize >= volumeNext)
      {
        // Compute the dose using linear interpolation
        double dosePrevious = table->GetValue(i, 0).ToDouble();
        double doseNext = table->GetValue(i+1, 0).ToDouble();
        double doseEstimated = dosePrevious + (doseNext-dosePrevious)*(volumeSize-volumePrevious)/(volumeNext-volumePrevious);
        doseForVolume = doseEstimated;

        break;
      }
    }
  }

  return doseForVolume;
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::ExportDvhToCsv(vtkMRMLDoseVolumeHistogramNode* parameterNode, const char* fileName, bool comma/*=true*/)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("ExportDvhToCsv: Invalid MRML scene or parameter set node");
    return false;
  }
  vtkMRMLScalarVolumeNode* doseVolumeNode = parameterNode->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    vtkErrorMacro("ExportDvhToCsv: Unable to find dose volume node");
    return false;
  }
  vtkMRMLTableNode* metricsTableNode = parameterNode->GetMetricsTableNode();
  if (!metricsTableNode)
  {
    vtkErrorMacro("ExportDvhToCsv: Unable to access DVH metrics table node");
    return false;
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("ExportDvhToCsv: Failed to access subject hierarchy node");
    return false;
  }

  vtkTable* metricsTable = metricsTableNode->GetTable();

  // Get dose unit name
  std::string doseUnitName("");
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (doseShItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    doseUnitName = shNode->GetAttributeFromItemAncestor(
      doseShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  }

  // Get all DVH array nodes from the parameter set node
  std::vector<vtkMRMLTableNode*> dvhTableNodes;
  parameterNode->GetDvhTableNodes(dvhTableNodes);

  // Open output file
  std::ofstream outfile;
  outfile.open(fileName, std::ios_base::out | std::ios_base::trunc);
  if (!outfile)
  {
    vtkErrorMacro("ExportDvhToCsv: Output file '" << fileName << "' cannot be opened");
    return false;
  }

  // Determine the maximum number of values
  int maxNumberOfValues = -1;
  for (std::vector<vtkMRMLTableNode*>::iterator dvhIt=dvhTableNodes.begin(); dvhIt!=dvhTableNodes.end(); ++dvhIt)
  {
    vtkMRMLTableNode* dvhTableNode = (*dvhIt);
    if (dvhTableNode->GetNumberOfRows() > maxNumberOfValues)
    {
      maxNumberOfValues = dvhTableNode->GetNumberOfRows();
    }
  }

  // Write header
  for (std::vector<vtkMRMLTableNode*>::iterator dvhIt=dvhTableNodes.begin(); dvhIt!=dvhTableNodes.end(); ++dvhIt)
  {
    vtkMRMLTableNode* dvhTableNode = (*dvhIt);
    int tableRow = vtkVariant(dvhTableNode->GetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str())).ToInt();

    double volume = metricsTable->GetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnVolumeCc).ToDouble();
    std::string structureName = metricsTable->GetValue(tableRow, vtkMRMLDoseVolumeHistogramNode::MetricColumnStructure).ToString();

    outfile << structureName << " Dose (" << doseUnitName << ")" << (comma ? "," : "\t");
    outfile << structureName << DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE
      << std::fixed << std::setprecision(3) << volume << DVH_CSV_HEADER_VOLUME_FIELD_END << (comma ? "," : "\t");
  }
  outfile << std::endl;

  // Write values
  for (int row=0; row<maxNumberOfValues; ++row)
  {
    for (std::vector<vtkMRMLTableNode*>::iterator dvhIt=dvhTableNodes.begin(); dvhIt!=dvhTableNodes.end(); ++dvhIt)
    {
      vtkMRMLTableNode* dvhTableNode = (*dvhIt);

      if (row < dvhTableNode->GetNumberOfRows())
      {
        std::ostringstream doseStringStream;
        doseStringStream << std::fixed << std::setprecision(6) <<
          dvhTableNode->GetTable()->GetValue(row, 0).ToDouble();
        std::string dose = doseStringStream.str();
        if (!comma)
        {
          size_t periodPosition = dose.find(".");
          if (periodPosition != std::string::npos)
          {
            dose.replace(periodPosition, 1, ",");
          }
        }
        outfile << dose;
      }
      outfile << (comma ? "," : "\t");

      if (row < dvhTableNode->GetNumberOfRows())
      {
        std::ostringstream valueStringStream;
        valueStringStream << std::fixed << std::setprecision(6) <<
          dvhTableNode->GetTable()->GetValue(row, 1).ToDouble();
        std::string value = valueStringStream.str();
        if (!comma)
        {
          size_t periodPosition = value.find(".");
          if (periodPosition != std::string::npos)
          {
            value.replace(periodPosition, 1, ",");
          }
        }
        outfile << value;
      }
      outfile << (comma ? "," : "\t");
    }
    outfile << std::endl;
  }

  outfile.close();

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::ExportDvhMetricsToCsv(vtkMRMLDoseVolumeHistogramNode* parameterNode, const char* fileName, bool comma/*=true*/)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("ExportDvhMetricsToCsv: Invalid MRML scene or parameter set node");
    return false;
  }
  vtkMRMLTableNode* metricsTableNode = parameterNode->GetMetricsTableNode();

  // Make a copy of the metrics table without the visualization and dose volume name columns
  vtkSmartPointer<vtkTable> metricsTableCopy = vtkSmartPointer<vtkTable>::New();
  metricsTableCopy->DeepCopy(metricsTableNode->GetTable());
  metricsTableCopy->RemoveColumn(vtkMRMLDoseVolumeHistogramNode::MetricColumnDoseVolume);
  metricsTableCopy->RemoveColumn(vtkMRMLDoseVolumeHistogramNode::MetricColumnVisible);

  vtkNew<vtkDelimitedTextWriter> writer;
  writer->SetFileName(fileName);
  writer->SetInputData(metricsTableCopy);
  writer->SetUseStringDelimiter(false);

  if (!comma)
  {
    writer->SetFieldDelimiter("\t");
  }
  else
  {
    writer->SetFieldDelimiter(",");
  }

  try
  {
    writer->Write();
  }
  catch (...)
  {
    vtkErrorMacro("ExportDvhMetricsToCsv: Failed to write DVH metrics table to file " << fileName);
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseVolumeHistogramModuleLogic::ReadCsvToTableNode(std::string csvFilename)
{
  std::string csvSeparatorCharacter = ",";

  vtkNew<vtkCollection> dvhTables;

  // Vectors containing the names and total volumes of structures
  std::vector<std::string> structureNames;
  std::vector<std::string> readLines;
  std::vector<double> structureVolumeCCs;

  // Load current DVH from CSV
  std::ifstream dvhStream;
  dvhStream.open(csvFilename.c_str(), std::ifstream::in);

  bool firstLine = true;
  int fieldCount = 0;
  int lineNumber = 0;
  char line[16384];

  while (dvhStream.getline(line, 16384, '\n'))
  {
    std::string lineStr(line);
    size_t commaPosition = lineStr.find(csvSeparatorCharacter);

    // Determine number of fields (twice the number of structures)
    if (firstLine)
    {
      while (commaPosition != std::string::npos)
      {
        if (fieldCount % 2 == 1)
        {
          // Get the structure's name
          std::string field = lineStr.substr(0, commaPosition);
          size_t middlePosition = field.find(DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE);
          std::string structureName = field.substr(0, middlePosition);
          if ( structureName.size() > DVH_TABLE_NODE_NAME_POSTFIX.size()
            && structureName.substr(structureName.size() - DVH_TABLE_NODE_NAME_POSTFIX.size()) == DVH_TABLE_NODE_NAME_POSTFIX)
            {
            structureName = structureName.substr(0, structureName.size() - DVH_TABLE_NODE_NAME_POSTFIX.size());
            }
          structureNames.push_back(structureName);

          // Get the structure's total volume and add it to the vector
          double volumeCCs = 0;
          {
            std::string structureVolumeString = field.substr( middlePosition + DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size(),
              field.size() - middlePosition - DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size() - DVH_CSV_HEADER_VOLUME_FIELD_END.size() );
            volumeCCs = vtkVariant(structureVolumeString).ToDouble();
          }
          structureVolumeCCs.push_back(volumeCCs);

          if (volumeCCs == 0)
          {
            std::cerr << "Invalid structure volume in CSV header field " << field << std::endl;
          }
        }

        // Move to the next structure's location in the string
        fieldCount++;
        lineStr = lineStr.substr(commaPosition+1);
        commaPosition = lineStr.find(csvSeparatorCharacter);
      }

      // Handle last field (if there was no comma at the end)
      if (!lineStr.empty() )
      {
        // Get the structure's name
        size_t middlePosition = lineStr.find(DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE);
        if (middlePosition != std::string::npos)
        {
          structureNames.push_back(lineStr.substr(0, middlePosition - DVH_TABLE_NODE_NAME_POSTFIX.size()));

          // Get the structure's total volume and add it to the vector
          double volumeCCs = 0;
          {
            std::string structureVolumeString = lineStr.substr( middlePosition + DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size(),
              lineStr.size() - middlePosition - DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size() - DVH_CSV_HEADER_VOLUME_FIELD_END.size() );
            volumeCCs = vtkVariant(structureVolumeString).ToDouble();
          }
          structureVolumeCCs.push_back(volumeCCs);

          if (volumeCCs == 0)
          {
            std::cerr << "Invalid structure volume in CSV header field " << lineStr << std::endl;
          }

          fieldCount++;
        }
      }

      firstLine = false;
      continue;
    } // If firstLine then determine number of fields

    // Store current line
    readLines.push_back(lineStr);
    lineNumber++;
  } // Read all lines

  // Close file stream
  dvhStream.close();

  // Add a table for each structure into the vector
  for (int structureIndex=0; structureIndex<structureNames.size(); ++structureIndex)
  {
    vtkNew<vtkTable> structureDvhTable;
    vtkNew<vtkDoubleArray> columnDose;
    columnDose->SetName("Dose");
    columnDose->SetNumberOfTuples(lineNumber);
    structureDvhTable->AddColumn(columnDose);
    vtkNew<vtkDoubleArray> columnVolume;
    columnVolume->SetName("Volume");
    columnVolume->SetNumberOfTuples(lineNumber);
    structureDvhTable->AddColumn(columnVolume);
    structureDvhTable->SetNumberOfRows(lineNumber);
    dvhTables->AddItem(structureDvhTable);
  }

  // Parse lines into the tables
  lineNumber = 0;
  for (std::vector<std::string>::iterator lineIt=readLines.begin(); lineIt!=readLines.end(); ++lineIt, ++lineNumber)
  {
    std::string lineStr = (*lineIt);
    size_t commaPosition = lineStr.find(csvSeparatorCharacter);

    // Read all tuples from the current line
    int structureIndex = 0;
    while (commaPosition != std::string::npos && structureIndex < structureNames.size())
    {
      // Get the current bin's dose from the string
      double doseGy = 0.0;
      {
        vtkStdString doseStr(lineStr.substr(0, commaPosition));
        doseGy = vtkVariant(doseStr).ToDouble();
      }

      // Get the current bin's volume from the string
      double volumePercent = 0.0;
      {
        lineStr = lineStr.substr(commaPosition+1);
        commaPosition = lineStr.find(csvSeparatorCharacter);
        vtkStdString volumePercentStr(lineStr.substr(0, commaPosition));
        volumePercent = vtkVariant(volumePercentStr).ToDouble();
      }

      vtkTable* currentStructureDvhTable = vtkTable::SafeDownCast(dvhTables->GetItemAsObject(structureIndex));
      if ((doseGy != 0.0 || volumePercent != 0.0) && (commaPosition > 0))
      {
        // Add the current bin into the table for the current structure
        currentStructureDvhTable->SetValue(lineNumber, 0, doseGy);
        currentStructureDvhTable->SetValue(lineNumber, 1, volumePercent);
      }

      // Move to the next structure's bin in the string
      lineStr = lineStr.substr(commaPosition+1);
      commaPosition = lineStr.find(csvSeparatorCharacter);
      structureIndex++;
    } // For each tuple in current line
  }

  vtkCollection* tableNodes = vtkCollection::New();
  for (unsigned int structureIndex=0; structureIndex<dvhTables->GetNumberOfItems(); structureIndex++)
  {
    // Create the table nodes which will be passed to the logic function.
    vtkNew<vtkMRMLTableNode> currentNode;
    vtkTable* currentStructureDvhTable = vtkTable::SafeDownCast(dvhTables->GetItemAsObject(structureIndex));
    currentNode->SetAndObserveTable(currentStructureDvhTable);

    // Set the total volume attribute in the vtkMRMLDoubleArrayNode attributes
    std::ostringstream attributeNameStream;
    attributeNameStream << vtkMRMLDoseVolumeHistogramNode::DVH_ATTRIBUTE_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC;
    std::ostringstream attributeValueStream;
    attributeValueStream << structureVolumeCCs[structureIndex];
    currentNode->SetAttribute(attributeNameStream.str().c_str(), attributeValueStream.str().c_str());

    // Set the structure's name attribute and variables
    currentNode->SetAttribute(DVH_SEGMENT_ID_ATTRIBUTE_NAME.c_str(), structureNames.at(structureIndex).c_str());
    std::string nameAttribute = structureNames.at(structureIndex) + DVH_TABLE_NODE_NAME_POSTFIX;
    currentNode->SetName(nameAttribute.c_str());

    // add the new node to the vector
    tableNodes->AddItem(currentNode);
  }

  return tableNodes;
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseVolumeHistogramModuleLogic::AssembleDoseMetricName(vtkMRMLScalarVolumeNode* doseVolumeNode, std::string doseMetricAttributeNamePrefix)
{
  if (!this->GetMRMLScene() || !doseVolumeNode)
  {
    vtkErrorMacro("AssembleDoseMetricName: Invalid MRML scene or dose volume node");
    return "";
  }
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if (!shNode)
  {
    vtkErrorMacro("AssembleDoseMetricName: Failed to access subject hierarchy node");
    return "";
  }

  // Get dose unit name
  std::string doseUnitName("");
  vtkIdType doseShItemID = shNode->GetItemByDataNode(doseVolumeNode);
  if (doseShItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    doseUnitName = shNode->GetAttributeFromItemAncestor(
      doseShItemID, vtkSlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  }

  // Assemble metric name
  std::string valueType = ( !doseUnitName.empty()
    ? (DVH_METRIC_DOSE_POSTFIX + " (" + doseUnitName + ")")
    : (DVH_METRIC_INTENSITY_POSTFIX) );
  std::ostringstream metricNameStream;
  metricNameStream << doseMetricAttributeNamePrefix << valueType;

  return metricNameStream.str();
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::InitializeMetricsTable(vtkMRMLDoseVolumeHistogramNode* parameterNode)
{
  if (!this->GetMRMLScene() || !parameterNode)
  {
    vtkErrorMacro("InitializeMetricsTable: Invalid MRML scene or parameter set node");
    return;
  }
  vtkMRMLTableNode* metricsTableNode = parameterNode->GetMetricsTableNode();
  if (!metricsTableNode)
  {
    vtkErrorMacro("InitializeMetricsTable: Unable to find metrics table");
    return;
  }

  // Empty the table first
  metricsTableNode->RemoveAllColumns();
  vtkEventBroker::GetInstance()->RemoveObservations(this);

  // Assemble metric names
  std::string meanDoseMetricName = this->AssembleDoseMetricName(parameterNode->GetDoseVolumeNode(), DVH_METRIC_MEAN_PREFIX);
  std::string minDoseMetricName = this->AssembleDoseMetricName(parameterNode->GetDoseVolumeNode(), DVH_METRIC_MIN_PREFIX);
  std::string maxDoseMetricName = this->AssembleDoseMetricName(parameterNode->GetDoseVolumeNode(), DVH_METRIC_MAX_PREFIX);
  if (meanDoseMetricName.empty() || minDoseMetricName.empty() || maxDoseMetricName.empty())
  {
    vtkErrorMacro("InitializeMetricsTable: Failed to assemble metric names");
    return;
  }

  // Add default columns
  vtkSmartPointer<vtkBitArray> visColumn = vtkSmartPointer<vtkBitArray>::New();
  visColumn->SetName("Show");
  metricsTableNode->AddColumn(visColumn);

  // Observe visualization column changes to show/hide DVH
  vtkNew<vtkDoseVolumeHistogramEventCallbackCommand> callbackCommand;
  callbackCommand->Logic = this;
  callbackCommand->ParameterNode = parameterNode;
  callbackCommand->SetClientData( reinterpret_cast<void*>(callbackCommand.GetPointer()) );
  callbackCommand->SetCallback( vtkSlicerDoseVolumeHistogramModuleLogic::OnVisibilityChanged );
  visColumn->AddObserver(vtkCommand::ModifiedEvent, callbackCommand.GetPointer(), 1.0);

  vtkAbstractArray* structureNameColumn = metricsTableNode->AddColumn();
  structureNameColumn->SetName(DVH_METRIC_STRUCTURE.c_str());

  vtkAbstractArray* volumeNameColumn = metricsTableNode->AddColumn();
  volumeNameColumn->SetName("Volume name");

  vtkAbstractArray* volumeCcColumn = metricsTableNode->AddColumn();
  volumeCcColumn->SetName(DVH_METRIC_TOTAL_VOLUME_CC.c_str());

  vtkAbstractArray* meanDoseColumn = metricsTableNode->AddColumn();
  meanDoseColumn->SetName(meanDoseMetricName.c_str());

  vtkAbstractArray* minDoseColumn = metricsTableNode->AddColumn();
  minDoseColumn->SetName(minDoseMetricName.c_str());

  vtkAbstractArray* maxDoseColumn = metricsTableNode->AddColumn();
  maxDoseColumn->SetName(maxDoseMetricName.c_str());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnVisibilityChanged(vtkObject* caller,
                                                                  unsigned long vtkNotUsed(eid),
                                                                  void* clientData,
                                                                  void* vtkNotUsed(callData))
{
  vtkDoseVolumeHistogramEventCallbackCommand* callbackCommand = reinterpret_cast<vtkDoseVolumeHistogramEventCallbackCommand*>(clientData);
  vtkSlicerDoseVolumeHistogramModuleLogic* self = callbackCommand->Logic;
  vtkMRMLDoseVolumeHistogramNode* parameterNode = callbackCommand->ParameterNode;
  vtkBitArray* visArray = reinterpret_cast<vtkBitArray*>(caller);
  if (!self || !parameterNode || !visArray || !self->GetMRMLScene())
  {
    vtkErrorWithObjectMacro(self,"OnVisibilityChanged: Invalid MRML scene, logic, parameter set node, or visualization column");
    return;
  }

  // Go through DVHs and change chart visibility for DVH for which the visibility value was modified
  std::set<int> rows;
  std::vector<vtkMRMLTableNode*> dvhTableNodes;
  parameterNode->GetDvhTableNodes(dvhTableNodes);
  for (std::vector<vtkMRMLTableNode*>::iterator dvhIt=dvhTableNodes.begin(); dvhIt!=dvhTableNodes.end(); ++dvhIt)
  {
    vtkMRMLTableNode* dvhtableNode = (*dvhIt);

    // Get corresponding table row
    int tableRow = -1;
    std::stringstream ss;
    ss << dvhtableNode->GetAttribute(DVH_TABLE_ROW_ATTRIBUTE_NAME.c_str());
    ss >> tableRow;
    if (ss.fail())
    {
      vtkErrorWithObjectMacro(self,"OnVisibilityChanged: Failed to get metrics table row from DVH node " << dvhtableNode->GetName());
      continue;
    }
    rows.insert(tableRow); // Safety check

    // Change chart visibility if necessary
    bool visibleInChart = self->IsDvhAddedToChart(parameterNode->GetChartNode(), dvhtableNode);
    bool visibilityInTable = (bool)visArray->GetValue(tableRow);
    if (visibleInChart != visibilityInTable)
    {
      if (visibilityInTable)
      {
        self->AddDvhToChart(parameterNode->GetChartNode(), dvhtableNode);
      }
      else
      {
        self->RemoveDvhFromChart(parameterNode->GetChartNode(), dvhtableNode);
      }
    }
  }

  if (rows.size() != static_cast<size_t>(parameterNode->GetMetricsTableNode()->GetNumberOfRows()))
  {
    vtkErrorWithObjectMacro(self, "OnVisibilityChanged: Mismatch between referenced DVH arrays and metrics table");
  }
}
