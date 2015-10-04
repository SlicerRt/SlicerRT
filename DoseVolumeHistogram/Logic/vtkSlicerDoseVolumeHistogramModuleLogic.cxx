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
#include "SlicerRtCommon.h"

// Segmentations includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkClosedSurfaceToBinaryLabelmapConversionRule.h"
#include "vtkCalculateOversamplingFactor.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"

// MRML includes
#include <vtkMRMLChartNode.h>
#include <vtkMRMLChartViewNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkImageAccumulate.h>
#include <vtkImageStencilData.h>
#include <vtkImageToImageStencil.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPiecewiseFunction.h>
#include <vtkStringArray.h>
#include <vtkTimerLog.h>
#include <vtkImageConstantPad.h>
#include <vtkMath.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <set>

//----------------------------------------------------------------------------
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX = "DoseVolumeHistogram.";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "DVH"; // Identifier
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "doseVolume" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CREATED_DVH_NODE_REFERENCE_ROLE = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "createdDvhArray" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_SEGMENTATION_NODE_REFERENCE_ROLE = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "structureSetSegmentation" + SlicerRtCommon::SLICERRT_REFERENCE_ROLE_ATTRIBUTE_NAME_POSTFIX; // Reference
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "DoseVolumeOversamplingFactor";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "StructureName";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "StructureColor";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "StructurePlotName";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "StructurePlotLineStyle";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "DvhMetric_";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_ATTRIBUTE_NAME = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ATTRIBUTE_PREFIX + "DvhMetricList";

const char        vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER = '|';
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "Volume (cc)";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX = "Mean ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX = "Min ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX = "Max ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX = "dose";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX = "intensity";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX = "V";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ARRAY_NODE_NAME_POSTFIX = "_DvhArray";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE = " Value (% of ";
const std::string vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_END = " cc)";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseVolumeHistogramModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic::vtkSlicerDoseVolumeHistogramModuleLogic()
{
  this->DoseVolumeHistogramNode = NULL;
  this->StartValue = 0.1;
  this->StepSize = 0.2;
  this->NumberOfSamplesForNonDoseVolumes = 100;
  this->DefaultDoseVolumeOversamplingFactor = 2.0;

  this->LogSpeedMeasurements = false;
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic::~vtkSlicerDoseVolumeHistogramModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseVolumeHistogramNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::SetAndObserveDoseVolumeHistogramNode(vtkMRMLDoseVolumeHistogramNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseVolumeHistogramNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
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
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::RefreshDvhDoubleArrayNodesFromScene()
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("RefreshDvhDoubleArrayNodesFromScene: Invalid MRML scene or parameter set node!");
    return;
  }

  this->DoseVolumeHistogramNode->RemoveAllDvhDoubleArrayNodes();

  if (this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLDoubleArrayNode") < 1)
  {
    return;
  }

  this->GetMRMLScene()->InitTraversal();
  vtkMRMLNode *node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLDoubleArrayNode");
  while (node != NULL)
  {
    vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node);
    if (doubleArrayNode)
    {
      const char* type = doubleArrayNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (type)
      {
        this->DoseVolumeHistogramNode->AddDvhDoubleArrayNode(doubleArrayNode);
      }
    }

    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLDoubleArrayNode");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML Scene!");
    return;
  }

  if (this->DoseVolumeHistogramNode)
  {
    this->RefreshDvhDoubleArrayNodesFromScene();
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene, input node, or parameter set node!");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node);
  if (doubleArrayNode && this->DoseVolumeHistogramNode)
  {
    const char* type = doubleArrayNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str());
    if (type)
    {
      this->DoseVolumeHistogramNode->AddDvhDoubleArrayNode(doubleArrayNode);
    }
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLSegmentationNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  // Release removed double array node to prevent memory leak
  if (node->IsA("vtkMRMLDoubleArrayNode"))
  {
    node->Delete();
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLSegmentationNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLDoseVolumeHistogramNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLDoseVolumeHistogramNode");
  if (node)
  {
    paramNode = vtkMRMLDoseVolumeHistogramNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->DoseVolumeHistogramNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene!");
    return;
  }

  if (this->DoseVolumeHistogramNode)
  {
    this->DoseVolumeHistogramNode->RemoveAllDvhDoubleArrayNodes();
  }
  this->SetAndObserveDoseVolumeHistogramNode(NULL);

  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDvh()
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  this->DoseVolumeHistogramNode->ClearAutomaticOversamplingFactors();
  vtkMRMLSegmentationNode* segmentationNode = this->DoseVolumeHistogramNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* doseVolumeNode = this->DoseVolumeHistogramNode->GetDoseVolumeNode();
  if ( !segmentationNode || !doseVolumeNode )
  {
    std::string errorMessage("Both segmentation node and dose volume node need to be set");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Fire only one modified event when the computation is done
  this->SetDisableModifiedEvent(1);
  int disabledNodeModify = this->DoseVolumeHistogramNode->StartModify();

  // Get maximum dose from dose volume for number of DVH bins
  vtkNew<vtkImageAccumulate> doseStat;
#if (VTK_MAJOR_VERSION <= 5)
  doseStat->SetInput(doseVolumeNode->GetImageData());
#else
  doseStat->SetInputData(doseVolumeNode->GetImageData());
#endif
  doseStat->Update();
  double maxDose = doseStat->GetMax()[0];

  // Get selected segmentation
  vtkSegmentation* selectedSegmentation = segmentationNode->GetSegmentation();

  // If segment IDs list is empty then include all segments
  std::vector<std::string> segmentIDs;
  this->DoseVolumeHistogramNode->GetSelectedSegmentIDs(segmentIDs);
  if (segmentIDs.empty())
  {
    vtkSegmentation::SegmentMap segmentMap = selectedSegmentation->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      segmentIDs.push_back(segmentIt->first);
    }
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
  vtkSmartPointer<vtkMatrix4x4> doseIjkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetIJKToRASMatrix(doseIjkToRasMatrix);
  std::string doseGeometryString = vtkSegmentationConverter::SerializeImageGeometry(doseIjkToRasMatrix, doseVolumeNode->GetImageData());
  segmentationCopy->SetConversionParameter( vtkSegmentationConverter::GetReferenceImageGeometryParameterName(),
    doseGeometryString );
  std::stringstream fixedOversamplingValuStream;
  fixedOversamplingValuStream << this->DefaultDoseVolumeOversamplingFactor;
  segmentationCopy->SetConversionParameter( vtkClosedSurfaceToBinaryLabelmapConversionRule::GetOversamplingFactorParameterName(),
    this->DoseVolumeHistogramNode->GetAutomaticOversampling() ? "A" : fixedOversamplingValuStream.str().c_str() );
  
  // Reconvert segments to specified geometry if possible
  bool resamplingRequired = false;
  if ( !segmentationCopy->CreateRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(), true) )
  {
    // If conversion failed and there is no binary labelmap in the segmentation, then cannot calculate DVH
    if (!segmentationCopy->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
      std::string errorMessage("Unable to acquire binary labelmap from segmentation");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    // If conversion failed, then resample binary labelmaps in the segments
    resamplingRequired = true;
  }

  // Calculate and store oversampling factors if automatically calculated for reporting purposes
  if (this->DoseVolumeHistogramNode->GetAutomaticOversampling())
  {
    // Get spacing for dose volume
    double doseSpacing[3] = {0.0,0.0,0.0};
    doseVolumeNode->GetSpacing(doseSpacing);

    // Calculate oversampling factors for all segments (need to calculate as it is not stored per segment)
    vtkSegmentation::SegmentMap segmentMap = segmentationCopy->GetSegments();
    for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
      vtkSegment* currentSegment = segmentIt->second;
      vtkOrientedImageData* currentBinaryLabelmap = vtkOrientedImageData::SafeDownCast(
        currentSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) );
      if (!currentBinaryLabelmap)
      {
        std::string errorMessage("Binary representation missing after converting with automatic oversampling factor!");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
      double currentSpacing[3] = {0.0,0.0,0.0};
      currentBinaryLabelmap->GetSpacing(currentSpacing);

      double voxelSizeRatio = ((doseSpacing[0]*doseSpacing[1]*doseSpacing[2]) / (currentSpacing[0]*currentSpacing[1]*currentSpacing[2]));
      // Round oversampling to two decimals
      // Note: We need to round to some degree, because e.g. pow(64,1/3) is not exactly 4. It may be debated whether to round to integer or to a certain number of decimals
      double oversamplingFactor = vtkMath::Round( pow( voxelSizeRatio, 1.0/3.0 ) * 100.0 ) / 100.0;
      this->DoseVolumeHistogramNode->AddAutomaticOversamplingFactor(segmentIt->first, oversamplingFactor);
    }
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
  // Apply parent transform on dose volume if necessary
  if (doseVolumeNode->GetParentTransformNode())
  {
    if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(doseVolumeNode, doseImageData))
    {
      std::string errorMessage("Failed to apply parent transformation to dose!");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }
  }

  // Use the same resampled dose volume if oversampling is fixed
  vtkSmartPointer<vtkOrientedImageData> fixedOversampledDoseVolume;
  if (!this->DoseVolumeHistogramNode->GetAutomaticOversampling())
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

  // Compute DVH for each selected segment
  vtkSegmentation::SegmentMap segmentMap = segmentationCopy->GetSegments();
  int counter = 1; // Start at one so that progress can reach 100%
  int numberOfSelectedSegments = segmentationCopy->GetNumberOfSegments();
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++counter)
  {
    // Get segment binary labelmap
    vtkOrientedImageData* segmentBinaryLabelmap = vtkOrientedImageData::SafeDownCast( segmentIt->second->GetRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) );
    if (!segmentBinaryLabelmap)
    {
      std::string errorMessage("Failed to get binary labelmap for segments");
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    // Apply parent transformation nodes if necessary
    if (segmentationNode->GetParentTransformNode())
    {
      if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(segmentationNode, segmentBinaryLabelmap))
      {
        std::string errorMessage("Failed to apply parent transformation to segment!");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
      resamplingRequired = true;
    }
    // Resample binary labelmap if necessary (if it was master, and could not be re-converted using the oversampled geometry, or if there was a parent transform)
    if (resamplingRequired)
    {
      // Resample dose volume using linear interpolation
      if ( !vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
        segmentBinaryLabelmap, fixedOversampledDoseVolume, segmentBinaryLabelmap ) )
      {
        std::string errorMessage("Failed to resample segment binary labelmap");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
    }

    // Get oversampled dose volume
    vtkSmartPointer<vtkOrientedImageData> oversampledDoseVolume;
    // Use the same resampled dose volume if oversampling is fixed
    if (!this->DoseVolumeHistogramNode->GetAutomaticOversampling())
    {
      oversampledDoseVolume = fixedOversampledDoseVolume;
    }
    // Resample dose volume to match automatically oversampled segment labelmap geometry
    else
    {
      oversampledDoseVolume = vtkSmartPointer<vtkOrientedImageData>::New();
      if ( !vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
        doseImageData, segmentBinaryLabelmap, oversampledDoseVolume, true ) )
      {
        std::string errorMessage("Failed to resample dose volume");
        vtkErrorMacro("ComputeDvh: " << errorMessage);
        return errorMessage;
      }
    }

    // Make sure the segment labelmap is the same dimension as the dose volume
    vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
#if (VTK_MAJOR_VERSION <= 5)
    padder->SetInput(segmentBinaryLabelmap);
#else
    padder->SetInputData(segmentBinaryLabelmap);
#endif
    int extent[6] = {0,-1,0,-1,0,-1};
#if (VTK_MAJOR_VERSION <= 5)
    oversampledDoseVolume->GetWholeExtent(extent);
#else
    oversampledDoseVolume->GetExtent(extent);
#endif
    padder->SetOutputWholeExtent(extent);
    padder->Update();
    segmentBinaryLabelmap->vtkImageData::DeepCopy(padder->GetOutput());

    // Get segment color from display node
    double segmentColor[3] = {0.0,0.0,0.0};
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    if (displayNode && displayNode->GetSegmentDisplayProperties(segmentIt->first, properties))
    {
      segmentColor[0] = properties.Color[0];
      segmentColor[1] = properties.Color[1];
      segmentColor[2] = properties.Color[2];
    }
    else
    {
      // If no display node is found, use the default color from the segment
      segmentIt->second->GetDefaultColor(segmentColor);
    }

    // Calculate DVH for current segment
    std::string errorMessage = this->ComputeDvh(segmentBinaryLabelmap, oversampledDoseVolume, segmentIt->first, segmentColor, maxDose);
    if (!errorMessage.empty())
    {
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return errorMessage;
    }

    // Update progress bar
    double progress = (double)counter / (double)numberOfSelectedSegments;
    this->InvokeEvent(SlicerRtCommon::ProgressUpdated, (void*)&progress);
  }

  // Fire only one modified event when the computation is done
  this->SetDisableModifiedEvent(0);
  this->Modified();
  this->DoseVolumeHistogramNode->EndModify(disabledNodeModify);

  return "";
}

//---------------------------------------------------------------------------
std::string vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDvh(vtkOrientedImageData* segmentLabelmap, vtkOrientedImageData* oversampledDoseVolume, std::string segmentID, double segmentColor[3], double maxDoseGy)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
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
  vtkMRMLSegmentationNode* segmentationNode = this->DoseVolumeHistogramNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* doseVolumeNode = this->DoseVolumeHistogramNode->GetDoseVolumeNode();
  if ( !segmentationNode || !doseVolumeNode )
  {
    std::string errorMessage("Both segmentation node and dose volume node need to be set");
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointStart); // Although it is used later, a warning is logged so needs to be suppressed

  // Create stencil for structure
  vtkNew<vtkImageToImageStencil> stencil;
#if (VTK_MAJOR_VERSION <= 5)
  stencil->SetInput(segmentLabelmap);
#else
  stencil->SetInputData(segmentLabelmap);
#endif
  stencil->ThresholdByUpper(0.5); // Thresholds only the labelmap, so the point is to keep the ones bigger than 0
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
  vtkNew<vtkImageAccumulate> structureStat;
#if (VTK_MAJOR_VERSION <= 5)
  structureStat->SetInput(oversampledDoseVolume);
  structureStat->SetStencil(structureStencil);
#else
  structureStat->SetInputData(oversampledDoseVolume);
  structureStat->SetStencilData(structureStencil);
#endif
  structureStat->Update();

  // Report error if there are no voxels in the stenciled dose volume (no non-zero voxels in the resampled labelmap)
  if (structureStat->GetVoxelCount() < 1)
  {
    std::string errorMessage("Dose volume and the structure do not overlap"); // User-friendly error to help troubleshooting
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return errorMessage;
  }

  // Create DVH array node
  vtkSmartPointer<vtkMRMLDoubleArrayNode> arrayNode = vtkSmartPointer<vtkMRMLDoubleArrayNode>::New();
  std::string dvhArrayNodeName = segmentID + vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ARRAY_NODE_NAME_POSTFIX;
  dvhArrayNodeName = this->GetMRMLScene()->GenerateUniqueName(dvhArrayNodeName);
  arrayNode->SetName(dvhArrayNodeName.c_str());

  // Set array node basic attributes
  arrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  std::string segmentName =
      this->DoseVolumeHistogramNode->GetSegmentationNode()->GetSegmentation()->GetSegment(segmentID)->GetName();
  arrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), segmentName.c_str());
  {
    std::ostringstream attributeValueStream;
    attributeValueStream << (this->DoseVolumeHistogramNode->GetAutomaticOversampling() ? (-1.0) : this->DefaultDoseVolumeOversamplingFactor);
    arrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME.c_str(), attributeValueStream.str().c_str());
  }

  // Set segment color as a DVH attribute
  std::ostringstream attributeValueStream;
  attributeValueStream.setf( ios::hex, ios::basefield );
  attributeValueStream << "#" << std::setw(2) << std::setfill('0') << (int)(segmentColor[0]*255.0+0.5)
    << std::setw(2) << std::setfill('0') << (int)(segmentColor[1]*255.0+0.5)
    << std::setw(2) << std::setfill('0') << (int)(segmentColor[2]*255.0+0.5);

  arrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str(), attributeValueStream.str().c_str());

  // Get spacing and voxel volume
  double* segmentLabelmapSpacing = segmentLabelmap->GetSpacing();
  double cubicMMPerVoxel = segmentLabelmapSpacing[0] * segmentLabelmapSpacing[1] * segmentLabelmapSpacing[2];
  double ccPerCubicMM = 0.001;

  // Get dose unit name
  const char* doseUnitName = NULL;
  vtkMRMLSubjectHierarchyNode* doseVolumeSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(doseVolumeNode);
  if (doseVolumeSubjectHierarchyNode)
  {
    doseUnitName = doseVolumeSubjectHierarchyNode->GetAttributeFromAncestor(
      SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
  }

  bool isDoseVolume = this->DoseVolumeContainsDose();

  // Compute and store DVH metrics
  std::ostringstream metricList;

  { // Voxel count
    std::ostringstream attributeNameStream;
    std::ostringstream attributeValueStream;
    attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
    attributeValueStream << structureStat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM;
    metricList << attributeNameStream.str() << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeNameStream.str().c_str(), attributeValueStream.str().c_str());
  }

  { // Mean dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX, (isDoseVolume?doseUnitName:NULL), attributeName);
    attributeValueStream << structureStat->GetMean()[0];
    metricList << attributeName << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // Max dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX, (isDoseVolume?doseUnitName:NULL), attributeName);
    attributeValueStream << structureStat->GetMax()[0];
    metricList << attributeName << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // Min dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX, (isDoseVolume?doseUnitName:NULL), attributeName);
    attributeValueStream << structureStat->GetMin()[0];
    metricList << attributeName << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // String containing all metrics (for easier ordered bulk retrieval of the metrics from the DVH node without knowing about the metric types)
    std::ostringstream attributeNameStream;
    attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_ATTRIBUTE_NAME;
    arrayNode->SetAttribute(attributeNameStream.str().c_str(), metricList.str().c_str());
  }

  double rangeMin = structureStat->GetMin()[0];
  double rangeMax = structureStat->GetMax()[0];

  // Create DVH plot values
  int numSamples = 0;
  double startValue;
  double stepSize;
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
  unsigned long voxelBelowDose = structureStat->GetOutput()->GetScalarComponentAsDouble(0,0,0,0);

  // We put a fixed point at (0.0, 100%), but only if there are only positive values in the histogram
  // Negative values can occur when the user requests histogram for an image, such as s CT volume (in this case Intensity Volume Histogram is computed),
  // or the startValue became negative for the dose volume because the range minimum was smaller than the original start value.
  bool insertPointAtOrigin=true;
  if (startValue<0)
  {
    insertPointAtOrigin=false;
  }

  structureStat->SetComponentExtent(0,numSamples-1,0,0,0,0);
  structureStat->SetComponentOrigin(startValue,0,0);
  structureStat->SetComponentSpacing(stepSize,1,1);
  structureStat->Update();

  vtkDoubleArray* doubleArray = arrayNode->GetArray();
  doubleArray->SetNumberOfTuples(numSamples + (insertPointAtOrigin?1:0));

  int outputArrayIndex=0;

  if (insertPointAtOrigin)
  {
    // Add first fixed point at (0.0, 100%)
    doubleArray->SetComponent(outputArrayIndex, 0, 0.0);
    doubleArray->SetComponent(outputArrayIndex, 1, 100.0);
    doubleArray->SetComponent(outputArrayIndex, 2, 0);
    ++outputArrayIndex;
  }

  vtkImageData* statArray = structureStat->GetOutput();
  unsigned long totalVoxels = structureStat->GetVoxelCount();
  for (int sampleIndex=0; sampleIndex<numSamples; ++sampleIndex)
  {
    unsigned long voxelsInBin = statArray->GetScalarComponentAsDouble(sampleIndex,0,0,0);
    doubleArray->SetComponent( outputArrayIndex, 0, startValue + sampleIndex * stepSize );
    doubleArray->SetComponent( outputArrayIndex, 1, (1.0-(double)voxelBelowDose/(double)totalVoxels)*100.0 );
    doubleArray->SetComponent( outputArrayIndex, 2, 0 );
    ++outputArrayIndex;
    voxelBelowDose += voxelsInBin;
  }

  // Set the start of the first bin to 0 if the volume contains dose and the start value was negative
  if (isDoseVolume && !insertPointAtOrigin)
  {
    doubleArray->SetComponent(0,0,0);
  }

  // Add DVH node to the scene
  this->GetMRMLScene()->AddNode(arrayNode);

  // Set array node references
  arrayNode->SetNodeReferenceID(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE.c_str(), doseVolumeNode->GetID());
  arrayNode->SetNodeReferenceID(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_SEGMENTATION_NODE_REFERENCE_ROLE.c_str(), segmentationNode->GetID());

  // Add DVH to subject hierarchy
  vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    this->GetMRMLScene(), doseVolumeSubjectHierarchyNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(),
    dvhArrayNodeName.c_str(), arrayNode);

  // Add connection attribute to input segmentation node
  vtkMRMLSubjectHierarchyNode* segmentSubjectHierarchyNode = segmentationNode->GetSegmentSubjectHierarchyNode(segmentID);
  if (segmentSubjectHierarchyNode)
  {
    segmentSubjectHierarchyNode->AddNodeReferenceID(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CREATED_DVH_NODE_REFERENCE_ROLE.c_str(), arrayNode->GetID());
  }

  // Log measured time
  double checkpointEnd = timer->GetUniversalTime();
  UNUSED_VARIABLE(checkpointEnd); // Although it is used just below, a warning is logged so needs to be suppressed
  if (this->LogSpeedMeasurements)
  {
    vtkDebugMacro("ComputeDvh: DVH computation time for structure '" << segmentID << "': " << checkpointEnd-checkpointStart << " s");
  }

  return "";
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::AddDvhToSelectedChart(const char* dvhArrayNodeId)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Invalid MRML scene or parameter set node!");
    return;
  }

  // Get selected chart and dose volume nodes
  vtkMRMLChartNode* chartNode = this->DoseVolumeHistogramNode->GetChartNode();
  vtkMRMLScalarVolumeNode* doseVolumeNode = this->DoseVolumeHistogramNode->GetDoseVolumeNode();
  if (!chartNode || !doseVolumeNode)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Invalid chart or dose volume node!");
    return;
  }
  
  this->AddDvhToChart(dvhArrayNodeId, chartNode->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::AddDvhToChart(const char* dvhArrayNodeId, const char* chartNodeID){

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Invalid MRML scene!");
    return;
  }

  // Get DVH array node
  vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(scene->GetNodeByID(dvhArrayNodeId));
  if (dvhArrayNode == NULL)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Unable to get double array node!");
    return;
  }
  const char* structureName = dvhArrayNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());


  // Get selected chart and dose volume nodes
  vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(scene->GetNodeByID(chartNodeID));
  vtkMRMLScalarVolumeNode* doseVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(dvhArrayNode->GetNodeReference("DoseVolumeHistogram.doseVolumeRef"));
  if (!chartNode || !doseVolumeNode)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Invalid chart or dose volume node!");
    return;
  }

  // Get chart view node
  vtkMRMLChartViewNode* chartViewNode = this->GetChartViewNode();
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Unable to get chart view node!");
    return;
  }

  // Set chart general properties
  std::string doseAxisName;
  std::string chartTitle;
  const char* doseIdentifier = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str());
  if (doseIdentifier)
  {
    vtkMRMLSubjectHierarchyNode* doseSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(doseVolumeNode);
    if (doseSubjectHierarchyNode)
    {
      const char* doseUnitName = doseSubjectHierarchyNode->GetAttributeFromAncestor(
        SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
      doseAxisName=std::string("Dose [")+doseUnitName+"]";
    }
    else
    {
      vtkErrorMacro("AddDvhToSelectedChart: Invalid subject hierarchy node for dose volume!");
      doseAxisName=std::string("Dose");
    }
    chartTitle="Dose Volume Histogram";
  }
  else
  {
    doseAxisName="Intensity";
    chartTitle="Intensity Volume Histogram";
  }

  chartNode->SetProperty("default", "title", chartTitle.c_str());
  chartNode->SetProperty("default", "xAxisLabel", doseAxisName.c_str());
  chartNode->SetProperty("default", "yAxisLabel", "Fractional volume [%]");
  chartNode->SetProperty("default", "type", "Line");
  chartNode->SetProperty("default", "xAxisPad", "0");
  chartNode->SetProperty("default", "yAxisPad", "0");

  // Get number of arrays showing plot for the same structure (for plot name and line style)
  vtkStringArray* arrayIds = chartNode->GetArrays();
  int numberOfStructuresWithSameName = 0;
  for (int arrayIndex = 0; arrayIndex < arrayIds->GetNumberOfValues(); ++arrayIndex)
  {
    vtkMRMLDoubleArrayNode* currentArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      scene->GetNodeByID(arrayIds->GetValue(arrayIndex).c_str()) );

    const char* currentStructureName = currentArrayNode->GetAttribute(
      vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str() );
    if (!STRCASECMP(currentStructureName, structureName))
    {
      ++numberOfStructuresWithSameName;
    }
  }

  // Assemble plot name and determine style
  std::stringstream structurePlotNameStream;
  structurePlotNameStream << structureName << " (" << arrayIds->GetNumberOfValues() + 1 << ")";
  if (numberOfStructuresWithSameName % 4 == 1)
  {
    dvhArrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dashed");
    structurePlotNameStream << " [- -]";
  }
  else if (numberOfStructuresWithSameName % 4 == 2)
  {
    dvhArrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dotted");
    structurePlotNameStream << " [...]";
  }
  else if (numberOfStructuresWithSameName % 4 == 3)
  {
    dvhArrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dashed-dotted");
    structurePlotNameStream << " [-.-]";
  }
  else
  {
    dvhArrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "solid");
  }

  std::string structurePlotName = structurePlotNameStream.str();
  dvhArrayNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME.c_str(), structurePlotName.c_str());

  // Add chart to chart view
  chartViewNode->SetChartNodeID( chartNode->GetID() );

  // Add array to chart
  chartNode->AddArray( structurePlotName.c_str(), dvhArrayNodeId );

  // Set plot color and line style
  const char* color = dvhArrayNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str());
  chartNode->SetProperty(structurePlotName.c_str(), "color", color);
  const char* lineStyle = dvhArrayNode->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str());
  chartNode->SetProperty(structurePlotName.c_str(), "linePattern", lineStyle);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::RemoveDvhFromSelectedChart(const char* dvhArrayNodeId)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("RemoveDvhFromSelectedChart: Invalid MRML scene or parameter set node!");
    return;
  }

  vtkMRMLChartNode* chartNode = this->DoseVolumeHistogramNode->GetChartNode();
  if (!chartNode)
  {
    vtkErrorMacro("RemoveDvhFromSelectedChart: Invalid chart node!");
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("RemoveDvhFromSelectedChart: Unable to get chart view node!");
    return;
  }

  vtkStringArray* arrayIds = chartNode->GetArrays();
  for (int arrayIndex = 0; arrayIndex < arrayIds->GetNumberOfValues(); ++arrayIndex)
  {
    if (!STRCASECMP(arrayIds->GetValue(arrayIndex).c_str(), dvhArrayNodeId))
    {
      chartNode->RemoveArray(chartNode->GetArrayNames()->GetValue(arrayIndex).c_str());
      return;
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::IsDvhAddedToSelectedChart(const char* dvhArrayNodeId)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("RemoveDvhFromSelectedChart: Invalid MRML scene or parameter set node!");
    return false;
  }

  vtkMRMLChartNode* chartNode = this->DoseVolumeHistogramNode->GetChartNode();
  if (!chartNode)
  {
    vtkErrorMacro("RemoveDvhFromSelectedChart: Invalid chart node!");
    return false;
  }

  vtkStringArray* arrayIds = chartNode->GetArrays();
  for (int arrayIndex = 0; arrayIndex < arrayIds->GetNumberOfValues(); ++arrayIndex)
  {
    if (!STRCASECMP(arrayIds->GetValue(arrayIndex).c_str(), dvhArrayNodeId))
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
vtkMRMLChartViewNode* vtkSlicerDoseVolumeHistogramModuleLogic::GetChartViewNode()
{
  vtkSmartPointer<vtkCollection> layoutNodes =
    vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLLayoutNode") );
  layoutNodes->InitTraversal();
  vtkObject* layoutNodeVtkObject = layoutNodes->GetNextItemAsObject();
  vtkMRMLLayoutNode* layoutNode = vtkMRMLLayoutNode::SafeDownCast(layoutNodeVtkObject);
  if (!layoutNode)
  {
    vtkErrorMacro("GetChartViewNode: Unable to get layout node!");
    return NULL;
  }
  layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );
  
  vtkSmartPointer<vtkCollection> chartViewNodes =
    vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLChartViewNode") );
  chartViewNodes->InitTraversal();
  vtkMRMLChartViewNode* chartViewNode = vtkMRMLChartViewNode::SafeDownCast( chartViewNodes->GetNextItemAsObject() );
  if (!chartViewNode)
  {
    vtkErrorMacro("GetChartViewNode: Unable to get chart view node!");
    return NULL;
  }

  return chartViewNode;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
::ComputeVMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> doseValues, std::vector<double> &vMetricsCc, std::vector<double> &vMetricsPercent)
{
  vMetricsCc.clear();
  vMetricsPercent.clear();

  // Get structure volume
  std::stringstream attributeNameStream;
  attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str() << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeNameStream.str().c_str());
  if (!structureVolumeStr)
  {
    vtkErrorMacro("ComputeVMetrics: Failed to get total volume attribute from DVH double array MRML node!");
    return;
  }

  std::stringstream ss;
  ss << structureVolumeStr;
  double doubleValue;
  ss >> doubleValue;
  double structureVolume = doubleValue;
  if (structureVolume == 0.0)
  {
    vtkErrorMacro("ComputeVMetrics: Failed to parse structure total volume attribute value!");
    return;
  }

  // Compute volume for all V's
  vtkDoubleArray* doubleArray = dvhArrayNode->GetArray();
  vtkNew<vtkPiecewiseFunction> interpolator;
  interpolator->ClampingOn();

  // Starting from second point, because BuildFunctionFromTable needs uniform distance between X coordinates
  //  and the first point may be 0 (with different distance to neighbors than the second)
  double* dvhArrayDouble = new double[doubleArray->GetNumberOfTuples()-1];
  for (int i=1; i<doubleArray->GetNumberOfTuples(); ++i)
  {
    dvhArrayDouble[i-1] = doubleArray->GetComponent(i, 1);
  }
  interpolator->BuildFunctionFromTable(doubleArray->GetComponent(1, 0), doubleArray->GetComponent(doubleArray->GetNumberOfTuples()-1, 0), doubleArray->GetNumberOfTuples()-1, dvhArrayDouble);

  // Add first point
  interpolator->AddPoint(doubleArray->GetComponent(0, 0), doubleArray->GetComponent(0, 1));

  // Fill results
  for (std::vector<double>::iterator it = doseValues.begin(); it != doseValues.end(); ++it)
  {
    double volumePercentEstimated = interpolator->GetValue(*it);
    vMetricsCc.push_back( volumePercentEstimated*structureVolume/100.0 );
    vMetricsPercent.push_back( volumePercentEstimated );
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
::ComputeDMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> volumeSizes, std::vector<double> &dMetrics, bool isPercent)
{
  dMetrics.clear();

  // Get structure volume
  std::stringstream attributeNameStream;
  attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str() << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeNameStream.str().c_str());
  if (!structureVolumeStr)
  {
    vtkErrorMacro("ComputeDMetrics: Failed to get total volume attribute from DVH double array MRML node!");
    return;
  }

  std::stringstream ss;
  ss << structureVolumeStr;
  double doubleValue;
  ss >> doubleValue;
  double structureVolume = doubleValue;
  if (structureVolume == 0.0)
  {
    vtkErrorMacro("ComputeDMetrics: Failed to parse structure total volume attribute value!");
    return;
  }

  // Compute volume for all D's
  vtkDoubleArray* doubleArray = dvhArrayNode->GetArray();
  for (int d=0; d<(int)volumeSizes.size(); ++d)
  {
    double volumeSize = 0.0;
    double doseForVolume = 0.0;

    if (isPercent)
    {
      volumeSize = volumeSizes[d] * structureVolume / 100.0;
    }
    else
    {
      volumeSize = volumeSizes[d];
    }

    // Check if the given volume is above the highest (first) in the array then assign no dose
    if (volumeSize >= doubleArray->GetComponent(0, 1) / 100.0 * structureVolume)
    {
      doseForVolume = 0.0;
    }
    // If volume is below the lowest (last) in the array then assign maximum dose
    else if (volumeSize < doubleArray->GetComponent(doubleArray->GetNumberOfTuples()-1, 1) / 100.0 * structureVolume)
    {
      doseForVolume = doubleArray->GetComponent(doubleArray->GetNumberOfTuples()-1, 0);
    }
    else
    {
      for (int i=0; i<doubleArray->GetNumberOfTuples()-1; ++i)
      {
        double volumePrevious = doubleArray->GetComponent(i, 1) / 100.0 * structureVolume;
        double volumeNext = doubleArray->GetComponent(i+1, 1) / 100.0 * structureVolume;
        if (volumePrevious > volumeSize && volumeSize >= volumeNext)
        {
          // Compute the dose using linear interpolation
          double dosePrevious = doubleArray->GetComponent(i, 0);
          double doseNext = doubleArray->GetComponent(i+1, 0);
          double doseEstimated = dosePrevious + (doseNext-dosePrevious)*(volumeSize-volumePrevious)/(volumeNext-volumePrevious);
          doseForVolume = doseEstimated;

          break;
        }
      }
    }

    // Set found dose
    dMetrics.push_back(doseForVolume);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::DoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("DoseVolumeContainsDose: Invalid MRML scene or parameter set node!");
    return false;
  }

  vtkMRMLScalarVolumeNode* doseVolumeNode = this->DoseVolumeHistogramNode->GetDoseVolumeNode();
  return SlicerRtCommon::IsDoseVolumeNode(doseVolumeNode);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::CollectMetricsForDvhNodes(std::vector<vtkMRMLNode*> dvhNodes, std::vector<std::string> &metricList)
{
  metricList.clear();

  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("CollectMetricsForDvhNodes: Invalid MRML scene or parameter set node!");
    return;
  }

  if (dvhNodes.size() < 1)
  {
    return;
  }

  // Convert separator character to string
  std::ostringstream separatorCharStream;
  separatorCharStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  std::string separatorCharacter = separatorCharStream.str();

  // Collect metrics
  std::ostringstream metricListAttributeNameStream;
  metricListAttributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_LIST_ATTRIBUTE_NAME;
  std::set<std::string> metricSet;
  std::vector<vtkMRMLNode*>::iterator dvhIt;
  for (dvhIt = dvhNodes.begin(); dvhIt != dvhNodes.end(); ++dvhIt)
  {
    if (!(*dvhIt))
    {
      continue;
    }

    std::string metricListString = (*dvhIt)->GetAttribute(metricListAttributeNameStream.str().c_str());
    if (metricListString.empty())
    {
      continue;
    }

    // Split metric list string into set of metric strings
    size_t separatorPosition = metricListString.find( separatorCharacter );
    while (separatorPosition != std::string::npos)
    {
      metricSet.insert( metricListString.substr(0, separatorPosition) );
      metricListString = metricListString.substr(separatorPosition+1);
      separatorPosition = metricListString.find( separatorCharacter );
    }
    if (! metricListString.empty() )
    {
      metricSet.insert( metricListString );
    }
  }

  // Create an ordered list from the set
  const char* metricSearchList[4] = { vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str(), vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX.c_str(),
                                      vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX.c_str(), vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX.c_str() };
  for (int i=0; i<4; ++i)
  {
    for (std::set<std::string>::iterator it = metricSet.begin(); it != metricSet.end(); ++it)
    {
      if (it->find(metricSearchList[i]) != std::string::npos)
      {
        metricList.push_back(*it);
        metricSet.erase(it);
        break;
      }
    }
  }

  // Append all other metrics in undefined order
  for (std::set<std::string>::iterator it = metricSet.begin(); it != metricSet.end(); ++it)
  {
    metricList.push_back(*it);
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::ExportDvhToCsv(const char* fileName, bool comma/*=true*/)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("ExportDvhToCsv: Invalid MRML scene or parameter set node!");
    return false;
  }

  vtkMRMLChartNode* chartNode = this->DoseVolumeHistogramNode->GetChartNode();
  if (!chartNode)
  {
    vtkErrorMacro("ExportDvhToCsv: Unable to get chart node");
		return false;
  }

  // Open output file
  std::ofstream outfile;
  outfile.open(fileName, std::ios_base::out | std::ios_base::trunc);

	if ( !outfile )
	{
    vtkErrorMacro("ExportDvhToCsv: Output file '" << fileName << "' cannot be opened!");
		return false;
	}

  vtkStringArray* structureNames = chartNode->GetArrayNames();
  vtkStringArray* arrayIDs = chartNode->GetArrays();

  // Determine the maximum number of values
  int maxNumberOfValues = -1;
	for (int i=0; i<arrayIDs->GetNumberOfValues(); ++i)
	{
    vtkMRMLNode *node = this->GetMRMLScene()->GetNodeByID( arrayIDs->GetValue(i) );
    vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node);
    if (doubleArrayNode)
    {
      if (doubleArrayNode->GetArray()->GetNumberOfTuples() > maxNumberOfValues)
      {
        maxNumberOfValues = doubleArrayNode->GetArray()->GetNumberOfTuples();
      }
    }
    else
    {
      vtkErrorMacro("ExportDvhToCsv: Invalid double array node in selected chart!");
      return false;
    }
  }

  // Write header
  std::string totalVolumeAttributeName = vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX + vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  for (int i=0; i<structureNames->GetNumberOfValues(); ++i)
  {
    vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID( arrayIDs->GetValue(i)) );
    const char* totalVolumeStr = doubleArrayNode->GetAttribute(totalVolumeAttributeName.c_str());

    std::stringstream ss;
    ss << totalVolumeStr;
    double doubleValue;
    ss >> doubleValue;
    double totalVolume = doubleValue;

    outfile << structureNames->GetValue(i).c_str() << " Dose (Gy)" << (comma ? "," : "\t");
    outfile << structureNames->GetValue(i).c_str() << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE
      << std::fixed << std::setprecision(3) << totalVolume << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_END << (comma ? "," : "\t");
  }
	outfile << std::endl;

  // Write values
	for (int row=0; row<maxNumberOfValues; ++row)
  {
	  for (int column=0; column<arrayIDs->GetNumberOfValues(); ++column)
	  {
      vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
        this->GetMRMLScene()->GetNodeByID( arrayIDs->GetValue(column)) );

      if (row < doubleArrayNode->GetArray()->GetNumberOfTuples())
      {
    	  std::ostringstream doseStringStream;
			  doseStringStream << std::fixed << std::setprecision(6) <<
          doubleArrayNode->GetArray()->GetComponent(row, 0);
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

      if (row < doubleArrayNode->GetArray()->GetNumberOfTuples())
      {
    	  std::ostringstream valueStringStream;
			  valueStringStream << std::fixed << std::setprecision(6) <<
          doubleArrayNode->GetArray()->GetComponent(row, 1);
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

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramModuleLogic::ExportDvhMetricsToCsv(const char* fileName,
                        std::vector<double> vDoseValuesCc,
                        std::vector<double> vDoseValuesPercent,
                        std::vector<double> dVolumeValuesCc,
                        std::vector<double> dVolumeValuesPercent,
                        bool comma/*=true*/)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("ExportDvhMetricsToCsv: Invalid MRML scene or parameter set node!");
    return false;
  }

  // Open output file
  std::ofstream outfile;
  outfile.open(fileName, std::ios_base::out | std::ios_base::trunc);

	if ( !outfile )
	{
    vtkErrorMacro("ExportDvhMetricsToCsv: Output file '" << fileName << "' cannot be opened!");
		return false;
	}

  std::vector<vtkMRMLNode*> dvhNodes;
  this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodes(dvhNodes);

  // Collect metrics for all included nodes
  std::vector<std::string> metricList;
  this->CollectMetricsForDvhNodes(dvhNodes, metricList);

  // Write header
  outfile << "Structure" << (comma ? "," : "\t");
  for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
  {
    outfile << it->substr(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.size()) << (comma ? "," : "\t");
  }
  for (std::vector<double>::iterator it = vDoseValuesCc.begin(); it != vDoseValuesCc.end(); ++it)
  {
    outfile << "V" << (*it) << " (cc)" << (comma ? "," : "\t");
  }
  for (std::vector<double>::iterator it = vDoseValuesPercent.begin(); it != vDoseValuesPercent.end(); ++it)
  {
    outfile << "V" << (*it) << " (%)" << (comma ? "," : "\t");
  }
  for (std::vector<double>::iterator it = dVolumeValuesCc.begin(); it != dVolumeValuesCc.end(); ++it)
  {
    outfile << "D" << (*it) << "cc (Gy)" << (comma ? "," : "\t");
  }
  for (std::vector<double>::iterator it = dVolumeValuesPercent.begin(); it != dVolumeValuesPercent.end(); ++it)
  {
    outfile << "D" << (*it) << "% (Gy)" << (comma ? "," : "\t");
  }
  outfile << std::endl;

  outfile.setf(std::ostream::fixed);
  outfile.precision(6);

  // Fill the table
  std::vector<vtkMRMLNode*>::iterator dvhIt;
  for (dvhIt = dvhNodes.begin(); dvhIt != dvhNodes.end(); ++dvhIt)
  {
    if (!(*dvhIt))
    {
      continue;
    }

    outfile << (*dvhIt)->GetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) << (comma ? "," : "\t");

    // Add default metric values
    for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
    {
      std::string metricValue( (*dvhIt)->GetAttribute( it->c_str() ) );
      if (metricValue.empty())
      {
        outfile << (comma ? "," : "\t");
        continue;
      }

      outfile << metricValue << (comma ? "," : "\t");
    }

    // Add V metric values
    std::vector<double> dummy;
    if (vDoseValuesCc.size() > 0)
    {
      std::vector<double> volumes;
      this->ComputeVMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), vDoseValuesCc, volumes, dummy);
      for (std::vector<double>::iterator it = volumes.begin(); it != volumes.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }
    if (vDoseValuesPercent.size() > 0)
    {
      std::vector<double> percents;
      this->ComputeVMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), vDoseValuesPercent, dummy, percents);
      for (std::vector<double>::iterator it = percents.begin(); it != percents.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }

    // Add D metric values
    if (dVolumeValuesCc.size() > 0)
    {
      std::vector<double> doses;
      this->ComputeDMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), dVolumeValuesCc, doses, false);
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }
    if (dVolumeValuesPercent.size() > 0)
    {
      std::vector<double> doses;
      this->ComputeDMetrics(vtkMRMLDoubleArrayNode::SafeDownCast(*dvhIt), dVolumeValuesPercent, doses, true);
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }

    outfile << std::endl;
  }

	outfile.close();

  return true;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::AssembleDoseMetricAttributeName( std::string doseMetricAttributeNamePrefix, const char* doseUnitName, std::string &attributeName )
{
  std::string valueType = ( doseUnitName
    ? (vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX + " (" + doseUnitName + ")")
    : (vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX) );
  std::ostringstream attributeNameStream;
  attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << doseMetricAttributeNamePrefix << valueType;

  attributeName = attributeNameStream.str();
}

//-----------------------------------------------------------------------------
vtkCollection* vtkSlicerDoseVolumeHistogramModuleLogic::ReadCsvToDoubleArrayNode(std::string csvFilename)
{
  std::string csvSeparatorCharacter = ",";
  
  std::vector< vtkSmartPointer< vtkDoubleArray > > currentDvh;
  
  // Vectors containing the names and total volumes of structures
  std::vector<std::string> structureNames;
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
        if (fieldCount%2==1)
        {
          // Get the structure's name
          std::string field = lineStr.substr(0, commaPosition);
          size_t middlePosition = field.find(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE);
          structureNames.push_back(field.substr(0, middlePosition - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ARRAY_NODE_NAME_POSTFIX.size()));

          // Get the structure's total volume and add it to the vector
          double volumeCCs = 0;
          {
            std::string structureVolumeString = field.substr( middlePosition + vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size(),
              field.size() - middlePosition - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size() - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_END.size() );
            std::stringstream ss;
            ss << structureVolumeString;
            double doubleValue;
            ss >> doubleValue;
            volumeCCs = doubleValue;
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
        size_t middlePosition = lineStr.find(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE);
        if (middlePosition != std::string::npos)
        {
          structureNames.push_back(lineStr.substr(0, middlePosition - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ARRAY_NODE_NAME_POSTFIX.size()));

          // Get the structure's total volume and add it to the vector
          double volumeCCs = 0;
          {
            std::string structureVolumeString = lineStr.substr( middlePosition + vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size(),
              lineStr.size() - middlePosition - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE.size() - vtkSlicerDoseVolumeHistogramModuleLogic::DVH_CSV_HEADER_VOLUME_FIELD_END.size() );
            std::stringstream ss;
            ss << structureVolumeString;
            double doubleValue;
            ss >> doubleValue;
            volumeCCs = doubleValue;
          }
          structureVolumeCCs.push_back(volumeCCs);

          if (volumeCCs == 0)
          {
            std::cerr << "Invalid structure volume in CSV header field " << lineStr << std::endl;
          }

          fieldCount++;
        }
      }
        
      // Add a vtkDoubleArray for each structure into the vector
      for(int structureIndex=0; structureIndex < fieldCount/2; ++structureIndex)
      {
        vtkSmartPointer<vtkDoubleArray> tempArray = vtkSmartPointer<vtkDoubleArray>::New();
        tempArray->SetNumberOfComponents(3);
        currentDvh.push_back(tempArray);
      }
      firstLine = false;
      continue;
    }

    // Read all tuples from the current line
    int structureNumber = 0;
    while (commaPosition != std::string::npos)
    {
      // Tuple to be inserted into the vtkDoubleArray object
      double *tupleToInsert = new double[3];
      for(int j=0; j<3; ++j)
        tupleToInsert[j] = 0;
      
      // Get the current bin's dose from the string
      double doseGy = 0;
      {
        double doubleValue;
        std::stringstream ss;
        ss << lineStr.substr(0, commaPosition);
        ss >> doubleValue;
        doseGy = doubleValue;
      }
      tupleToInsert[0] = doseGy;

      // Get the current bin's volume from the string
      double volumePercent = 0;
      {
        double doubleValue;
        lineStr = lineStr.substr(commaPosition+1);
        commaPosition = lineStr.find(csvSeparatorCharacter);
        std::stringstream ss;
        ss << lineStr.substr(0, commaPosition);
        ss >> doubleValue;
        volumePercent = doubleValue;
      }
      tupleToInsert[1] = volumePercent;
      
      if ((doseGy != 0.0 || volumePercent != 0.0) && (commaPosition > 0))
      {
        // Add the current bin into the vtkDoubleArray for the current structure
        currentDvh.at(structureNumber)->InsertTuple(lineNumber, tupleToInsert);        
      }
      
      // Destroy the tuple
      delete tupleToInsert;
      tupleToInsert = NULL;
      
      // Move to the next structure's bin in the string
      lineStr = lineStr.substr(commaPosition+1);
      commaPosition = lineStr.find(csvSeparatorCharacter);
      structureNumber++;
    }
    lineNumber++;
  }
 
  dvhStream.close();
  
  vtkCollection* doubleArrayNodes = vtkCollection::New();
  for (unsigned int structureIndex=0; structureIndex < currentDvh.size(); structureIndex++)
  {
    // Create the vtkMRMLDoubleArrayNodes which will be passed to the logic function.
    vtkNew<vtkMRMLDoubleArrayNode> currentNode;
    currentNode->SetArray(currentDvh.at(structureIndex));
    
    // Set the total volume attribute in the vtkMRMLDoubleArrayNode attributes
    std::ostringstream attributeNameStream;
    attributeNameStream << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << vtkSlicerDoseVolumeHistogramModuleLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
    std::ostringstream attributeValueStream;
    attributeValueStream << structureVolumeCCs[structureIndex];
    currentNode->SetAttribute(attributeNameStream.str().c_str(), attributeValueStream.str().c_str());
    
    // Set the structure's name attribute and variables
    currentNode->SetAttribute(vtkSlicerDoseVolumeHistogramModuleLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureNames.at(structureIndex).c_str());
    std::string nameAttribute = structureNames.at(structureIndex) + vtkSlicerDoseVolumeHistogramModuleLogic::DVH_ARRAY_NODE_NAME_POSTFIX;
    currentNode->SetName(nameAttribute.c_str());

    // add the new node to the vector
    doubleArrayNodes->AddItem(currentNode.GetPointer());
  }

  return doubleArrayNodes;
}
