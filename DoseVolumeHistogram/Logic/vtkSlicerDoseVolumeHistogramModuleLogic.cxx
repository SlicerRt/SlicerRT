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

// DoseVolumeHistogram includes
#include "vtkSlicerDoseVolumeHistogramModuleLogic.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLContourNode.h"
#include "vtkConvertContourRepresentations.h"
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkVolumesOrientedResampleUtility.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLChartViewNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkImageAccumulate.h>
#include <vtkImageThreshold.h>
#include <vtkImageToImageStencil.h>
#include <vtkImageStencilData.h>
#include <vtkDoubleArray.h>
#include <vtkStringArray.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageReslice.h>
#include <vtkTimerLog.h>
#include <vtkPolyDataToLabelmapFilter.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <set>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseVolumeHistogramModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic::vtkSlicerDoseVolumeHistogramModuleLogic()
{
  this->DoseVolumeHistogramNode = NULL;
  this->StartValue = 0.1;
  this->StepSize = 0.2;
  this->NumberOfSamplesForNonDoseVolumes = 100;
  this->DoseVolumeOversamplingFactor = 2.0;

  this->LogSpeedMeasurementsOff();
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramModuleLogic::~vtkSlicerDoseVolumeHistogramModuleLogic()
{
  if (this->GetMRMLScene() && this->DoseVolumeHistogramNode)
  {
    for (std::vector<std::string>::iterator it = this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->begin();
      it != this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->end(); ++it)
    {
      vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
        this->GetMRMLScene()->GetNodeByID(it->c_str()));
      dvhNode->Delete();
    }    
  }

  vtkSetAndObserveMRMLNodeMacro(this->DoseVolumeHistogramNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::RefreshDvhDoubleArrayNodesFromScene()
{
  if (!this->DoseVolumeHistogramNode)
  {
    return;
  }
  this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->clear();

  if (this->GetMRMLScene() == NULL || this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLDoubleArrayNode") < 1)
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
      const char* type = doubleArrayNode->GetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (type)
      {
        this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->push_back(doubleArrayNode->GetID());
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

  this->RefreshDvhDoubleArrayNodesFromScene();

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLDoubleArrayNode"))
  {
    vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node);
    if (doubleArrayNode)
    {
      const char* type = doubleArrayNode->GetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (type)
      {
        this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->push_back(doubleArrayNode->GetID());
      }
    }
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLDisplayableHierarchyNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLDoubleArrayNode"))
  {
    const char* removedNodeId = vtkMRMLDoubleArrayNode::SafeDownCast(node)->GetID();
    for (std::vector<std::string>::iterator it = this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->begin();
      it != this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->end(); ++it)
    {
      if (!it->compare(removedNodeId))
      {
        this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->erase(it);
        break;
      }
    }
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLDisplayableHierarchyNode")
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
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour( vtkMRMLContourNode* structureContourNode, vtkMRMLScalarVolumeNode* resampledDoseVolumeNode, vtkMRMLScalarVolumeNode* consolidatedStructureLabelmapNode )
{
  if ( !this->GetMRMLScene() || !this->DoseVolumeHistogramNode
    || !structureContourNode || !consolidatedStructureLabelmapNode || !resampledDoseVolumeNode )
  {
    vtkErrorMacro("GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour: Invalid input arguments or scene!");
    return;
  }

  // Get indexed labelmap representation (convert if does not exist yet)
  vtkMRMLScalarVolumeNode* indexedLabelmapNode = structureContourNode->GetIndexedLabelmapVolumeNode();
  if (!indexedLabelmapNode)
  {
    vtkErrorMacro("GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour: Failed to get indexed labelmap representation from contour node '" << structureContourNode->GetName() << "' !");
    return;
  }

  // Resample the dose volume to match the indexed labelmap so that we can compute the stencil
  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));
  if (!doseVolumeNode)
  {
    vtkErrorMacro("GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour: Unable to get dose volume node!");
    return;
  }
  if (!doseVolumeNode->GetImageData())
  {
    vtkErrorMacro("GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour: Dose volume contains no image data!");
    return;
  }

  // Copy dose volume node to the output resampled dose volume node object
  resampledDoseVolumeNode->Copy(doseVolumeNode);

  // Set the proper spacing to the resampled dose volume node
  double doseVolumeSpacing[3] = {0.0, 0.0, 0.0};
  doseVolumeNode->GetSpacing(doseVolumeSpacing);
  resampledDoseVolumeNode->SetSpacing( doseVolumeSpacing[0] / this->DoseVolumeOversamplingFactor,
                                   doseVolumeSpacing[1] / this->DoseVolumeOversamplingFactor,
                                   doseVolumeSpacing[2] / this->DoseVolumeOversamplingFactor );

  // Resample the dose volume if is required based on the oversampling factor
  if (this->DoseVolumeOversamplingFactor != 1.0)
  {
    int outputExtent[6] = {0, 0, 0, 0, 0, 0};
    double outputSpacing[3] = {0.0, 0.0, 0.0};
    SlicerRtCommon::GetExtentAndSpacingForOversamplingFactor(doseVolumeNode, this->DoseVolumeOversamplingFactor, outputExtent, outputSpacing);

    vtkSmartPointer<vtkImageReslice> reslicer = vtkSmartPointer<vtkImageReslice>::New();
    reslicer->SetInput(doseVolumeNode->GetImageData());
    reslicer->SetInterpolationMode(VTK_RESLICE_LINEAR);
    reslicer->SetOutputExtent(outputExtent);
    reslicer->SetOutputSpacing(outputSpacing);
    reslicer->Update();

    vtkSmartPointer<vtkImageData> resampledDoseVolume = vtkSmartPointer<vtkImageData>::New();
    resampledDoseVolume->DeepCopy(reslicer->GetOutput());
    resampledDoseVolume->SetSpacing(1.0, 1.0, 1.0);
    resampledDoseVolume->SetOrigin(0.0, 0.0, 0.0);
    resampledDoseVolumeNode->SetAndObserveImageData(resampledDoseVolume);
  }

  // The lattices of the resampled dose volume and the structure labelmap volume should match.
  // Resample indexed labelmap representation temporarily if it has a lattice different from the resampled dose volume.
  if (SlicerRtCommon::DoVolumeLatticesMatch(indexedLabelmapNode, resampledDoseVolumeNode))
  {
    consolidatedStructureLabelmapNode->Copy(indexedLabelmapNode);
  }
  else
  {
    vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode(indexedLabelmapNode, resampledDoseVolumeNode, consolidatedStructureLabelmapNode);
  }

  // Set proper names to the temporary volumes for easier debugging
  std::string resampledDoseVolumeNodeName(doseVolumeNode->GetName());
  resampledDoseVolumeNodeName.append("_Temporary_Resampled");
  resampledDoseVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(resampledDoseVolumeNodeName);
  resampledDoseVolumeNode->SetName(resampledDoseVolumeNodeName.c_str());

  std::string consolidatedStructureLabelmapNodeName(indexedLabelmapNode->GetName());
  consolidatedStructureLabelmapNodeName.append("_Temporary_Consolidated");
  consolidatedStructureLabelmapNodeName = this->GetMRMLScene()->GenerateUniqueName(consolidatedStructureLabelmapNodeName);
  consolidatedStructureLabelmapNode->SetName(consolidatedStructureLabelmapNodeName.c_str());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDvh(vtkMRMLContourNode* structureContourNode, std::string &errorMessage)
{
  if ( !this->GetMRMLScene() || !this->DoseVolumeHistogramNode || !structureContourNode )
  {
    return;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));

  // Get dose unit name
  const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  // Get maximum dose from dose volume
  vtkNew<vtkImageAccumulate> doseStat;
  doseStat->SetInput(doseVolumeNode->GetImageData());
  doseStat->Update();
  double maxDose = doseStat->GetMax()[0];

  // Get resampled dose volume and matching structure labelmap (the function makes sure their lattices are the same)
  vtkSmartPointer<vtkMRMLScalarVolumeNode> resampledDoseVolume = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  vtkSmartPointer<vtkMRMLScalarVolumeNode> consolidatedStructureLabelmap = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  this->GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour(structureContourNode, resampledDoseVolume, consolidatedStructureLabelmap);

  // Create stencil for structure
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInput(consolidatedStructureLabelmap->GetImageData());
  stencil->ThresholdByUpper(0.5); // Thresholds only the labelmap, so the point is to keep the ones bigger than 0
  stencil->Update();

  vtkSmartPointer<vtkImageStencilData> structureStencil = vtkSmartPointer<vtkImageStencilData>::New();
  structureStencil->DeepCopy(stencil->GetOutput());

  int stencilExtent[6] = {0, 0, 0, 0, 0, 0};
  structureStencil->GetExtent(stencilExtent);
  if (stencilExtent[1]-stencilExtent[0] <= 0 || stencilExtent[3]-stencilExtent[2] <= 0 || stencilExtent[5]-stencilExtent[4] <= 0)
  {
    errorMessage = "Invalid stenciled dose volume";
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return;
  }

  // Compute statistics
  vtkNew<vtkImageAccumulate> structureStat;
  structureStat->SetInput(resampledDoseVolume->GetImageData());
  structureStat->SetStencil(structureStencil);
  structureStat->Update();

  // Report error if there are no voxels in the stenciled dose volume (no non-zero voxels in the resampled labelmap)
  if (structureStat->GetVoxelCount() < 1)
  {
    errorMessage = "Dose volume and the structure do not overlap"; // User-friendly error to help troubleshooting
    vtkErrorMacro("ComputeDvh: " << errorMessage);
    return;
  }

  // Get structure name
  std::string structureName(structureContourNode->GetStructureName());

  // Create DVH array node
  vtkMRMLDoubleArrayNode* arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLDoubleArrayNode") );
  std::string dvhArrayNodeName = structureName + SlicerRtCommon::DVH_ARRAY_NODE_NAME_POSTFIX;
  dvhArrayNodeName = this->GetMRMLScene()->GenerateUniqueName(dvhArrayNodeName);
  arrayNode->SetName(dvhArrayNodeName.c_str());
  //arrayNode->HideFromEditorsOff();

  // Set array node attributes
  arrayNode->SetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  arrayNode->SetAttribute(SlicerRtCommon::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME.c_str(), doseVolumeNode->GetID());
  arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureName.c_str());
  arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_CONTOUR_NODE_ID_ATTRIBUTE_NAME.c_str(), structureContourNode->GetID());
  {
    std::ostringstream attributeValueStream;
    attributeValueStream << this->DoseVolumeOversamplingFactor;
    arrayNode->SetAttribute(SlicerRtCommon::DVH_DOSE_VOLUME_OVERSAMPLING_FACTOR_ATTRIBUTE_NAME.c_str(), attributeValueStream.str().c_str());
  }

  // Retrieve color from the contour and set it as a DVH attribute
  {
    std::ostringstream attributeValueStream;
    attributeValueStream.setf( ios::hex, ios::basefield );
    double color[4] = {SlicerRtCommon::COLOR_VALUE_INVALID[0], SlicerRtCommon::COLOR_VALUE_INVALID[1], SlicerRtCommon::COLOR_VALUE_INVALID[2], SlicerRtCommon::COLOR_VALUE_INVALID[3]};

    if ( structureContourNode->GetRibbonModelNodeId() || structureContourNode->GetClosedSurfaceModelNodeId() ) // If there is a model representation
    {
      vtkMRMLDisplayNode* modelDisplayNode = (structureContourNode->GetRibbonModelNode() ? structureContourNode->GetRibbonModelNode()->GetDisplayNode() : structureContourNode->GetClosedSurfaceModelNode()->GetDisplayNode() );
      if (modelDisplayNode)
      {
        // Get color from the model itself
        modelDisplayNode->GetColor(color[0], color[1], color[2]);
      }
      else
      {
        vtkErrorMacro("ComputeDvh: No display node for model representation of structure '" << structureContourNode->GetStructureName() << "'");
      }
    }
    else // Created from labelmap
    {
      vtkMRMLColorTableNode* colorNode = NULL;
      int structureColorIndex = -1;
      structureContourNode->GetColor(structureColorIndex, colorNode);
      colorNode->GetColor(structureColorIndex, color);
    }

    attributeValueStream << "#" << std::setw(2) << std::setfill('0') << (int)(color[0]*255.0+0.5)
      << std::setw(2) << std::setfill('0') << (int)(color[1]*255.0+0.5)
      << std::setw(2) << std::setfill('0') << (int)(color[2]*255.0+0.5);

    arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str(), attributeValueStream.str().c_str());
  }

  // Get spacing and voxel volume
  double* structureLabelmapSpacing = structureContourNode->GetIndexedLabelmapVolumeNode()->GetSpacing();
  double cubicMMPerVoxel = structureLabelmapSpacing[0] * structureLabelmapSpacing[1] * structureLabelmapSpacing[2];
  double ccPerCubicMM = 0.001;

  // Compute and store DVH metrics
  std::ostringstream metricList;

  { // Voxel count
    std::ostringstream attributeNameStream;
    std::ostringstream attributeValueStream;
    attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
    attributeValueStream << structureStat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM;
    metricList << attributeNameStream.str() << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeNameStream.str().c_str(), attributeValueStream.str().c_str());
  }

  { // Mean dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX, doseUnitName, attributeName);
    attributeValueStream << structureStat->GetMean()[0];
    metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // Max dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX, doseUnitName, attributeName);
    attributeValueStream << structureStat->GetMax()[0];
    metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // Min dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX, doseUnitName, attributeName);
    attributeValueStream << structureStat->GetMin()[0];
    metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // String containing all metrics (for easier ordered bulk retrtieval of the metrics from the DVH node without knowing about the metric types)
    std::ostringstream attributeNameStream;
    attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME;
    arrayNode->SetAttribute(attributeNameStream.str().c_str(), metricList.str().c_str());
  }

  double rangeMin = structureStat->GetMin()[0];
  double rangeMax = structureStat->GetMax()[0];

  // Create DVH plot values
  int numSamples = 0;
  double startValue;
  double stepSize;
  bool isDoseVolume = this->DoseVolumeContainsDose();
  if (isDoseVolume)
  {
    if (rangeMin<0)
    {
      errorMessage = "The dose volume contains negative dose values";
      vtkErrorMacro("ComputeDvh: " << errorMessage);
      return;
    }

    startValue = this->StartValue;
    stepSize = this->StepSize;
    numSamples = (int)ceil( (maxDose-startValue)/stepSize ) + 1;
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

  // Add DVH array node to the MRML scene
  this->GetMRMLScene()->AddNode(arrayNode);

  // Add connection attribute to input contour node
  vtkMRMLHierarchyNode* structurePatientHierarchyNode = vtkSlicerPatientHierarchyModuleLogic::GetAssociatedPatientHierarchyNode(structureContourNode);
  if (structurePatientHierarchyNode)
  {
    structurePatientHierarchyNode->SetAttribute(SlicerRtCommon::DVH_CREATED_DVH_NODE_ID_ATTRIBUTE_NAME.c_str(), arrayNode->GetID());
  }

  // Log measured time
  double checkpointEnd = timer->GetUniversalTime();
  if (this->LogSpeedMeasurements)
  {
    vtkDebugMacro("ComputeDvh: DVH computation time for structure '" << (structureContourNode->GetStructureName() ? structureContourNode->GetStructureName() : "Invalid") << "': " << checkpointEnd-checkpointStart << " s");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::AddDvhToSelectedChart(const char* structurePlotName, const char* dvhArrayNodeId)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetChartNodeId()));
  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));

  if (!chartNode || !doseVolumeNode)
  {
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("Error: unable to get chart view node!");
    return;
  }

  // Set chart properties
  std::string doseAxisName;
  std::string chartTitle;
  const char* doseUnitName=doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());
  if (doseUnitName!=NULL)
  {
    doseAxisName=std::string("Dose [")+doseUnitName+"]";
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

  vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast( this->GetMRMLScene()->GetNodeByID(dvhArrayNodeId) );
  if (dvhArrayNode == NULL)
  {
    vtkErrorMacro("Error: unable to get double array node!");
    return;
  }

  // Add array to chart
  chartNode->AddArray( structurePlotName, dvhArrayNodeId );

  // Set plot color and line style
  const char* color = dvhArrayNode->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str());
  chartNode->SetProperty(structurePlotName, "color", color);
  const char* lineStyle = dvhArrayNode->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str());
  chartNode->SetProperty(structurePlotName, "linePattern", lineStyle);

  // Add chart to chart view
  chartViewNode->SetChartNodeID( chartNode->GetID() );
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::RemoveDvhFromSelectedChart(const char* structureName)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetChartNodeId()));

  if (!chartNode)
  {
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("Error: unable to get chart view node!");
    return;
  }

  chartViewNode->SetChartNodeID( chartNode->GetID() );
  chartNode->RemoveArray(structureName);
}

//---------------------------------------------------------------------------
vtkMRMLChartViewNode* vtkSlicerDoseVolumeHistogramModuleLogic::GetChartViewNode()
{
  vtkSmartPointer<vtkCollection> layoutNodes
    = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLLayoutNode") );
  layoutNodes->InitTraversal();
  vtkObject* layoutNodeVtkObject = layoutNodes->GetNextItemAsObject();
  vtkMRMLLayoutNode* layoutNode = vtkMRMLLayoutNode::SafeDownCast(layoutNodeVtkObject);
  if (layoutNode == NULL)
  {
    vtkErrorMacro("Error: unable to get layout node!");
    return NULL;
  }
  layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );
  
  vtkSmartPointer<vtkCollection> chartViewNodes
    = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLChartViewNode") );
  chartViewNodes->InitTraversal();
  vtkMRMLChartViewNode* chartViewNode = vtkMRMLChartViewNode::SafeDownCast( chartViewNodes->GetNextItemAsObject() );
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("Error: unable to get chart view node!");
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
  attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str() << SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeNameStream.str().c_str());
  if (!structureVolumeStr)
  {
    vtkErrorMacro("Error: Failed to get total volume attribute from DVH double array MRML node!");
    return;
  }

  std::stringstream ss;
  ss << structureVolumeStr;
  double doubleValue;
  ss >> doubleValue;
  double structureVolume = doubleValue;
  if (structureVolume == 0.0)
  {
    vtkErrorMacro("Error: Failed to parse structure total volume attribute value!");
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
  attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str() << SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeNameStream.str().c_str());
  if (!structureVolumeStr)
  {
    vtkErrorMacro("Error: Failed to get total volume attribute from DVH double array MRML node!");
    return;
  }

  std::stringstream ss;
  ss << structureVolumeStr;
  double doubleValue;
  ss >> doubleValue;
  double structureVolume = doubleValue;
  if (structureVolume == 0.0)
  {
    vtkErrorMacro("Error: Failed to parse structure total volume attribute value!");
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
    return false;
  }

  vtkMRMLNode* doseVolumeNode = this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId());
  return SlicerRtCommon::IsDoseVolumeNode(doseVolumeNode);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::CollectMetricsForDvhNodes(std::vector<std::string>* dvhNodeIds, std::vector<std::string> &metricList)
{
  metricList.clear();

  if (dvhNodeIds->size() < 1 || !this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  // Convert separator character to string
  std::ostringstream separatorCharStream;
  separatorCharStream << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  std::string separatorCharacter = separatorCharStream.str();

  // Collect metrics
  std::ostringstream metricListAttributeNameStream;
  metricListAttributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME;
  std::set<std::string> metricSet;
  for (std::vector<std::string>::iterator it = dvhNodeIds->begin(); it != dvhNodeIds->end(); ++it)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(it->c_str()));
    if (!dvhNode)
    {
      continue;
    }

    std::string metricListString = dvhNode->GetAttribute(metricListAttributeNameStream.str().c_str());
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
  const char* metricSearchList[4] = { SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str(), SlicerRtCommon::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX.c_str(),
                                      SlicerRtCommon::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX.c_str() };
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
    return false;
  }

  vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetChartNodeId()));

  if (chartNode == NULL)
  {
		return false;
  }

  // Open output file
  std::ofstream outfile;
  outfile.open(fileName, std::ios_base::out | std::ios_base::trunc);

	if ( !outfile )
	{
    vtkErrorMacro("Error: Output file '" << fileName << "' cannot be opened!");
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
      vtkErrorMacro("Invalid double array node in selected chart!");
      return false;
    }
  }

  // Write header
  std::string totalVolumeAttributeName = SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX + SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
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
    outfile << structureNames->GetValue(i).c_str() << SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_MIDDLE
      << std::fixed << std::setprecision(3) << totalVolume << SlicerRtCommon::DVH_CSV_HEADER_VOLUME_FIELD_END << (comma ? "," : "\t");
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
    return false;
  }

  // Open output file
  std::ofstream outfile;
  outfile.open(fileName, std::ios_base::out | std::ios_base::trunc);

	if ( !outfile )
	{
    vtkErrorMacro("Error: Output file '" << fileName << "' cannot be opened!");
		return false;
	}

  std::vector<std::string>* dvhDoubleArrayNodeIds = this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds();

  // Collect metrics for all included nodes
  std::vector<std::string> metricList;
  this->CollectMetricsForDvhNodes(dvhDoubleArrayNodeIds, metricList);

  // Write header
  outfile << "Structure" << (comma ? "," : "\t");
  for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
  {
    outfile << it->substr(SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.size()) << (comma ? "," : "\t");
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
  for (std::vector<std::string>::iterator it = dvhDoubleArrayNodeIds->begin(); it != dvhDoubleArrayNodeIds->end(); ++it)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(it->c_str()));
    if (!dvhNode)
    {
      continue;
    }

    outfile << dvhNode->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) << (comma ? "," : "\t");

    // Add default metric values
    for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
    {
      std::string metricValue( dvhNode->GetAttribute( it->c_str() ) );
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
      this->ComputeVMetrics(dvhNode, vDoseValuesCc, volumes, dummy);
      for (std::vector<double>::iterator it = volumes.begin(); it != volumes.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }
    if (vDoseValuesPercent.size() > 0)
    {
      std::vector<double> percents;
      this->ComputeVMetrics(dvhNode, vDoseValuesPercent, dummy, percents);
      for (std::vector<double>::iterator it = percents.begin(); it != percents.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }

    // Add D metric values
    if (dVolumeValuesCc.size() > 0)
    {
      std::vector<double> doses;
      this->ComputeDMetrics(dvhNode, dVolumeValuesCc, doses, false);
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }
    if (dVolumeValuesPercent.size() > 0)
    {
      std::vector<double> doses;
      this->ComputeDMetrics(dvhNode, dVolumeValuesPercent, doses, true);
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
    ? (SlicerRtCommon::DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX + " (" + doseUnitName + ")")
    : (SlicerRtCommon::DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX) );
  std::ostringstream attributeNameStream;
  attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << doseMetricAttributeNamePrefix << valueType;

  attributeName = attributeNameStream.str();
}
