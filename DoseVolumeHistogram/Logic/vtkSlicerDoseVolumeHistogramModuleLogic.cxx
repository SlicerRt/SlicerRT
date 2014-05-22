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

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

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

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkImageAccumulate.h>
#include <vtkImageToImageStencil.h>
#include <vtkImageStencilData.h>
#include <vtkDoubleArray.h>
#include <vtkStringArray.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageReslice.h>
#include <vtkTimerLog.h>
#include <vtkPolyDataToLabelmapFilter.h>
#include <vtkObjectFactory.h>

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
  // Release double array nodes to prevent memory leak
  if (this->GetMRMLScene() && this->DoseVolumeHistogramNode)
  {
    std::vector<vtkMRMLNode*> dvhNodes;
    this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodes(dvhNodes);
    std::vector<vtkMRMLNode*>::iterator dvhIt;
    int dvhIndex = 0;
    for (dvhIt = dvhNodes.begin(); dvhIt != dvhNodes.end(); ++dvhIt, ++dvhIndex)
    {
      (*dvhIt)->Delete();
    }    
  }

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
      const char* type = doubleArrayNode->GetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str());
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

  if (node->IsA("vtkMRMLDoubleArrayNode") && this->DoseVolumeHistogramNode)
  {
    vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node);
    if (doubleArrayNode)
    {
      const char* type = doubleArrayNode->GetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str());
      if (type)
      {
        this->DoseVolumeHistogramNode->AddDvhDoubleArrayNode(doubleArrayNode);
      }
    }
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLContourNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
  if ( node->IsA("vtkMRMLSubjectHierarchyNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
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
    || node->IsA("vtkMRMLContourNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
  if ( node->IsA("vtkMRMLSubjectHierarchyNode")
    && node->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_IDENTIFIER_ATTRIBUTE_NAME.c_str()) )
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

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour( vtkMRMLContourNode* structureContourNode, vtkMRMLScalarVolumeNode* resampledDoseVolumeNode, vtkMRMLScalarVolumeNode* consolidatedStructureLabelmapNode )
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour: Invalid MRML scene or parameter set node!");
    return;
  }
  if ( !structureContourNode || !consolidatedStructureLabelmapNode || !resampledDoseVolumeNode )
  {
    vtkErrorMacro("GetOversampledDoseVolumeAndConsolidatedIndexedLabelmapForContour: Invalid input arguments!");
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
  vtkMRMLScalarVolumeNode* doseVolumeNode = this->DoseVolumeHistogramNode->GetDoseVolumeNode();
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
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    vtkErrorMacro("ComputeDvh: Invalid MRML scene or parameter set node!");
    return;
  }
  if (!structureContourNode)
  {
    vtkWarningMacro("ComputeDvh: Invalid structure contour node");
    return;
  }

  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double checkpointStart = timer->GetUniversalTime();

  vtkMRMLScalarVolumeNode* doseVolumeNode = this->DoseVolumeHistogramNode->GetDoseVolumeNode();
  if (!doseVolumeNode)
  {
    vtkErrorMacro("ComputeDvh: Invalid dose volume!");
    return;
  }

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

  // Set array node references
  arrayNode->SetNodeReferenceID(SlicerRtCommon::DVH_DOSE_VOLUME_NODE_REFERENCE_ROLE.c_str(), doseVolumeNode->GetID());
  arrayNode->SetNodeReferenceID(SlicerRtCommon::DVH_STRUCTURE_CONTOUR_NODE_REFERENCE_ROLE.c_str(), structureContourNode->GetID());

  // Set array node attributes
  arrayNode->SetAttribute(SlicerRtCommon::DVH_DVH_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");
  arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureName.c_str());
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

  // Get dose unit name
  const char* doseUnitName = NULL;
  vtkMRMLSubjectHierarchyNode* doseVolumeSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(doseVolumeNode);
  if (!doseVolumeSubjectHierarchyNode)
  {
    doseUnitName = doseVolumeSubjectHierarchyNode->GetAttributeFromAncestor(
      SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY);
  }
  bool isDoseVolume = this->DoseVolumeContainsDose();

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
    this->AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MEAN_ATTRIBUTE_NAME_PREFIX, (isDoseVolume?doseUnitName:NULL), attributeName);
    attributeValueStream << structureStat->GetMean()[0];
    metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // Max dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MAX_ATTRIBUTE_NAME_PREFIX, (isDoseVolume?doseUnitName:NULL), attributeName);
    attributeValueStream << structureStat->GetMax()[0];
    metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
    arrayNode->SetAttribute(attributeName.c_str(), attributeValueStream.str().c_str());
  }

  { // Min dose
    std::string attributeName;
    std::ostringstream attributeValueStream;
    this->AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MIN_ATTRIBUTE_NAME_PREFIX, (isDoseVolume?doseUnitName:NULL), attributeName);
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

  // Add DVH to subject hierarchy
  vtkMRMLSubjectHierarchyNode* dvhSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
    this->GetMRMLScene(), doseVolumeSubjectHierarchyNode, vtkMRMLSubjectHierarchyConstants::DICOMHIERARCHY_LEVEL_SUBSERIES,
    dvhArrayNodeName.c_str(), arrayNode);

  // Add connection attribute to input contour node
  vtkMRMLSubjectHierarchyNode* structureSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(structureContourNode);
  if (structureSubjectHierarchyNode)
  {
    structureSubjectHierarchyNode->AddNodeReferenceID(SlicerRtCommon::DVH_CREATED_DVH_NODE_REFERENCE_ROLE.c_str(), arrayNode->GetID());
  }

  // Log measured time
  double checkpointEnd = timer->GetUniversalTime();
  if (this->LogSpeedMeasurements)
  {
    vtkDebugMacro("ComputeDvh: DVH computation time for structure '" << (structureContourNode->GetStructureName() ? structureContourNode->GetStructureName() : "Invalid") << "': " << checkpointEnd-checkpointStart << " s");
  }
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
        SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), vtkMRMLSubjectHierarchyConstants::SUBJECTHIERARCHY_LEVEL_STUDY);
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

  // Get DVH array node
  vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast( this->GetMRMLScene()->GetNodeByID(dvhArrayNodeId) );
  if (dvhArrayNode == NULL)
  {
    vtkErrorMacro("AddDvhToSelectedChart: Unable to get double array node!");
    return;
  }
  const char* structureName = dvhArrayNode->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());

  // Get number of arrays showing plot for the same structure (for plot name and line style)
  vtkStringArray* arrayIds = chartNode->GetArrays();
  int numberOfStructuresWithSameName = 0;
  for (int arrayIndex = 0; arrayIndex < arrayIds->GetNumberOfValues(); ++arrayIndex)
  {
    vtkMRMLDoubleArrayNode* currentArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      scene->GetNodeByID(arrayIds->GetValue(arrayIndex).c_str()) );

    const char* currentStructureName = currentArrayNode->GetAttribute(
      SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str() );
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
    dvhArrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dashed");
    structurePlotNameStream << " [- -]";
  }
  else if (numberOfStructuresWithSameName % 4 == 2)
  {
    dvhArrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dotted");
    structurePlotNameStream << " [...]";
  }
  else if (numberOfStructuresWithSameName % 4 == 3)
  {
    dvhArrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "dashed-dotted");
    structurePlotNameStream << " [-.-]";
  }
  else
  {
    dvhArrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str(), "solid");
  }

  std::string structurePlotName = structurePlotNameStream.str();
  dvhArrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME.c_str(), structurePlotName.c_str());

  // Add chart to chart view
  chartViewNode->SetChartNodeID( chartNode->GetID() );

  // Add array to chart
  chartNode->AddArray( structurePlotName.c_str(), dvhArrayNodeId );

  // Set plot color and line style
  const char* color = dvhArrayNode->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str());
  chartNode->SetProperty(structurePlotName.c_str(), "color", color);
  const char* lineStyle = dvhArrayNode->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_PLOT_LINE_STYLE_ATTRIBUTE_NAME.c_str());
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
      vtkMRMLDoubleArrayNode* arrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(
        scene->GetNodeByID(arrayIds->GetValue(arrayIndex).c_str()) );
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
  attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str() << SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
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
  attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str() << SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME;
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
  separatorCharStream << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  std::string separatorCharacter = separatorCharStream.str();

  // Collect metrics
  std::ostringstream metricListAttributeNameStream;
  metricListAttributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME;
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
  std::vector<vtkMRMLNode*>::iterator dvhIt;
  for (dvhIt = dvhNodes.begin(); dvhIt != dvhNodes.end(); ++dvhIt)
  {
    if (!(*dvhIt))
    {
      continue;
    }

    outfile << (*dvhIt)->GetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) << (comma ? "," : "\t");

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
    ? (SlicerRtCommon::DVH_METRIC_DOSE_ATTRIBUTE_NAME_POSTFIX + " (" + doseUnitName + ")")
    : (SlicerRtCommon::DVH_METRIC_INTENSITY_ATTRIBUTE_NAME_POSTFIX) );
  std::ostringstream attributeNameStream;
  attributeNameStream << SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX << doseMetricAttributeNamePrefix << valueType;

  attributeName = attributeNameStream.str();
}
