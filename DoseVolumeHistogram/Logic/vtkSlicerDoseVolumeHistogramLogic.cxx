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
#include "vtkSlicerDoseVolumeHistogramLogic.h"
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLChartViewNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkGeneralTransform.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageAccumulate.h>
#include <vtkImageThreshold.h>
#include <vtkImageToImageStencil.h>
#include <vtkDoubleArray.h>
#include <vtkStringArray.h>
#include <vtkPiecewiseFunction.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <cassert>
#include <set>

//----------------------------------------------------------------------------
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TYPE_ATTRIBUTE_NAME = "Type";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TYPE_ATTRIBUTE_VALUE = "DVH";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME = "DoseVolumeNodeId";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = "StructureName";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME = "StructureModelNodeId";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = "StructureColor";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = "DVH_StructurePlotName";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX = "DVH_Metric_";
const char vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_LIST_SEPARATOR_CHARACTER = '|';
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_LIST_ATTRIBUTE_NAME = "List";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "Total volume (cc)";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX = "Mean dose";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX = "Max dose";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX = "Min dose";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_VOXEL_COUNT_ATTRIBUTE_NAME = "Voxel count";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_V_DOSE_ATTRIBUTE_NAME_PREFIX = "V";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerDoseVolumeHistogramLogic);

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::vtkSlicerDoseVolumeHistogramLogic()
{
  this->DoseVolumeHistogramNode = NULL;
  this->StartValue = 0.1;
  this->StepSize = 0.2;
  this->NumberOfSamplesForNonDoseVolumes = 100;
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::~vtkSlicerDoseVolumeHistogramLogic()
{
  if (this->GetMRMLScene() && this->DoseVolumeHistogramNode)
  {
    for (std::set<std::string>::iterator it = this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->begin();
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
void vtkSlicerDoseVolumeHistogramLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::SetAndObserveDoseVolumeHistogramNode(vtkMRMLDoseVolumeHistogramNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->DoseVolumeHistogramNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLDoseVolumeHistogramNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::RefreshDvhDoubleArrayNodesFromScene()
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
      const char* type = doubleArrayNode->GetAttribute(DVH_TYPE_ATTRIBUTE_NAME.c_str());
      if (type != NULL && stricmp(type, DVH_TYPE_ATTRIBUTE_VALUE.c_str()) == 0)
      {
        this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->insert(doubleArrayNode->GetID());
      }
    }

    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLDoubleArrayNode");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  RefreshDvhDoubleArrayNodesFromScene();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
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
      const char* type = doubleArrayNode->GetAttribute(DVH_TYPE_ATTRIBUTE_NAME.c_str());
      if (type != NULL && stricmp(type, DVH_TYPE_ATTRIBUTE_VALUE.c_str()) == 0)
      {
        this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->insert(doubleArrayNode->GetID());
      }
    }
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLModelNode") || node->IsA("vtkMRMLModelHierarchyNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
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
    this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->erase(
      vtkMRMLDoubleArrayNode::SafeDownCast(node)->GetID());
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLModelNode") || node->IsA("vtkMRMLModelHierarchyNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::OnMRMLSceneEndImport()
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
void vtkSlicerDoseVolumeHistogramLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::GetStenciledDoseVolumeForStructure(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModelNode)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));
  vtkMRMLNode* structureSetModelNode = this->GetMRMLScene()->GetNodeByID(
    this->DoseVolumeHistogramNode->GetStructureSetModelNodeId());

  // Create model to doseRas transform
  vtkSmartPointer<vtkGeneralTransform> modelToDoseRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkSmartPointer<vtkGeneralTransform> doseRasToWorldTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* modelTransformNode=structureModelNode->GetParentTransformNode();
  vtkMRMLTransformNode* doseTransformNode=doseVolumeNode->GetParentTransformNode();

  if (doseTransformNode!=NULL)
  {
    // the dosemap is transformed
    doseTransformNode->GetTransformToWorld(doseRasToWorldTransform);    
    if (modelTransformNode!=NULL)
    {      
      modelToDoseRasTransform->PostMultiply(); // GetTransformToNode assumes PostMultiply
      modelTransformNode->GetTransformToNode(doseTransformNode,modelToDoseRasTransform);
    }
    else
    {
      // modelTransformNode is NULL => the transform will be computed for the world coordinate frame
      doseTransformNode->GetTransformToWorld(modelToDoseRasTransform);
      modelToDoseRasTransform->Inverse();
    }
  }
  else
  {
    // the dosemap is not transformed => modelToDoseRasTransformMatrix = modelToWorldTransformMatrix
    if (modelTransformNode!=NULL)
    {
      // the model is transformed
      modelTransformNode->GetTransformToWorld(modelToDoseRasTransform);
    }
    else
    {
      // neither the model nor the dosemap is transformed
      modelToDoseRasTransform->Identity();
    }
  }  

  // Create doseRas to doseIjk transform
  vtkSmartPointer<vtkMatrix4x4> doseRasToDoseIjkTransformMatrix=vtkSmartPointer<vtkMatrix4x4>::New();
  doseVolumeNode->GetRASToIJKMatrix( doseRasToDoseIjkTransformMatrix );  
  
  // Create model to doseIjk transform
  vtkNew<vtkGeneralTransform> modelToDoseIjkTransform;
  modelToDoseIjkTransform->Concatenate(doseRasToDoseIjkTransformMatrix);
  modelToDoseIjkTransform->Concatenate(modelToDoseRasTransform);


  // Transform the model polydata to doseIjk coordinate frame (the labelmap image coordinate frame is doseIjk)
  vtkNew<vtkTransformPolyDataFilter> transformPolyDataModelToDoseIjkFilter;
  transformPolyDataModelToDoseIjkFilter->SetInput( structureModelNode->GetPolyData() );
  transformPolyDataModelToDoseIjkFilter->SetTransform(modelToDoseIjkTransform.GetPointer());

  // Convert model to labelmap
  vtkNew<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter;
  transformPolyDataModelToDoseIjkFilter->Update();
  polyDataToLabelmapFilter->SetBackgroundValue(VTK_DOUBLE_MIN);
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToDoseIjkFilter->GetOutput() );
  polyDataToLabelmapFilter->SetReferenceImage( doseVolumeNode->GetImageData() );
  polyDataToLabelmapFilter->Update();

  // Create labelmap node
  structureStenciledDoseVolumeNode->CopyOrientation( doseVolumeNode );
  structureStenciledDoseVolumeNode->SetAndObserveTransformNodeID( doseVolumeNode->GetTransformNodeID() );
  std::string labelmapNodeName( structureSetModelNode->GetName() );
  labelmapNodeName.append( "_StenciledDose" );
  structureStenciledDoseVolumeNode->SetName( labelmapNodeName.c_str() );
  structureStenciledDoseVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  //structureStenciledDoseVolumeNode->LabelMapOn();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::GetSelectedStructureModelNodes(std::vector<vtkMRMLModelNode*> &structureModelNodes)
{
  structureModelNodes.clear();

  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  vtkMRMLNode* structureSetModelNode = this->GetMRMLScene()->GetNodeByID(
    this->DoseVolumeHistogramNode->GetStructureSetModelNodeId());

  if (structureSetModelNode->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(structureSetModelNode);
    if (modelNode)
    {
      structureModelNodes.push_back(modelNode);
    }
  }
  else if (structureSetModelNode->IsA("vtkMRMLModelHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLModelHierarchyNode::SafeDownCast(structureSetModelNode)->GetChildrenModelNodes(childModelNodes);
    childModelNodes->InitTraversal();
    if (childModelNodes->GetNumberOfItems() < 1)
    {
      vtkErrorMacro("Error: Selected Structure Set hierarchy node has no children model nodes!");
      return;
    }
    
    for (int i=0; i<childModelNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(childModelNodes->GetItemAsObject(i));
      if (modelNode)
      {
        structureModelNodes.push_back(modelNode);
      }
    }
  }
  else
  {
    vtkErrorMacro("Error: Invalid node type for StructureSetModelNode!");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::ComputeDvh()
{
  std::vector<vtkMRMLModelNode*> structureModelNodes;
  GetSelectedStructureModelNodes(structureModelNodes);

  for (std::vector<vtkMRMLModelNode*>::iterator it = structureModelNodes.begin(); it != structureModelNodes.end(); ++it)
  {
    vtkSmartPointer<vtkMRMLScalarVolumeNode> structureStenciledDoseVolumeNode
      = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    GetStenciledDoseVolumeForStructure(structureStenciledDoseVolumeNode, (*it));
    // this->GetMRMLScene()->AddNode( structureStenciledDoseVolumeNode ); // add the labelmap to the scene for testing/debugging

    ComputeDvh(structureStenciledDoseVolumeNode.GetPointer(), (*it));
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeDvh(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModelNode)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));

  double* structureStenciledDoseVolumeSpacing = structureStenciledDoseVolumeNode->GetSpacing();

  double cubicMMPerVoxel = structureStenciledDoseVolumeSpacing[0] * structureStenciledDoseVolumeSpacing[1] * structureStenciledDoseVolumeSpacing[2];
  double ccPerCubicMM = 0.001;

  // Get dose grid scaling and dose units
  const char* doseGridScalingString = doseVolumeNode->GetAttribute("DoseUnitValue");
  double doseGridScaling = 1.0;
  if (doseGridScalingString!=NULL)
  {
    doseGridScaling = atof(doseGridScalingString);
  }
  else
  {
    vtkWarningMacro("Dose grid scaling attribute is not set for the selected dose volume. Assuming scaling = 1.");
  }

  const char* doseUnitName = doseVolumeNode->GetAttribute("DoseUnitName");

  // Compute statistics
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInput(structureStenciledDoseVolumeNode->GetImageData());
  stencil->ThresholdByUpper(VTK_DOUBLE_MIN+1.0); // Do not include background values

  vtkNew<vtkImageAccumulate> stat;
  stat->SetInput(structureStenciledDoseVolumeNode->GetImageData());
  stat->SetStencil(stencil->GetOutput());
  stat->Update();

  if (stat->GetVoxelCount() < 1)
  {
    vtkWarningMacro("No voxels in the structure. DVH computation aborted.");
    return;
  }

  // Create node and fill statistics
  vtkMRMLDoubleArrayNode* arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLDoubleArrayNode") );
  std::string dvhArrayNodeName = std::string(structureModelNode->GetName()) + "_DVH";
  arrayNode->SetName(dvhArrayNodeName.c_str());
  //arrayNode->HideFromEditorsOff();

  arrayNode->SetAttribute(DVH_TYPE_ATTRIBUTE_NAME.c_str(), DVH_TYPE_ATTRIBUTE_VALUE.c_str());
  arrayNode->SetAttribute(DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME.c_str(), doseVolumeNode->GetID());
  arrayNode->SetAttribute(DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureModelNode->GetName());
  arrayNode->SetAttribute(DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME.c_str(), structureModelNode->GetID());

  char attributeValue[64];
  double* color = structureModelNode->GetDisplayNode()->GetColor();
  sprintf(attributeValue, "#%02X%02X%02X", (int)(color[0]*255.0+0.5), (int)(color[1]*255.0+0.5), (int)(color[2]*255.0+0.5));
  arrayNode->SetAttribute(DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str(), attributeValue);

  char attributeName[64];
  std::ostringstream metricList;

  sprintf(attributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  sprintf(attributeValue, "%g", stat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s (%s)", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName);
  sprintf(attributeValue, "%g", stat->GetMean()[0]);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s (%s)", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName);
  sprintf(attributeValue, "%g", stat->GetMax()[0]);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s (%s)", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName);
  sprintf(attributeValue, "%g", stat->GetMin()[0]);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_LIST_ATTRIBUTE_NAME.c_str());
  arrayNode->SetAttribute(attributeName, metricList.str().c_str());

  arrayNode = vtkMRMLDoubleArrayNode::SafeDownCast( this->GetMRMLScene()->AddNode( arrayNode ) );

  double rangeMin = stat->GetMin()[0];
  double rangeMax = stat->GetMax()[0];

  // Create DVH plot values
  int numSamples = 0;
  double startValue;
  double stepSize;
  bool isDoseVolume = this->DoseVolumeContainsDose();
  if (isDoseVolume)
  {
    startValue = this->StartValue;
    stepSize = this->StepSize;
    numSamples = (int)ceil( (rangeMax-startValue)/stepSize ) + 1;
  }
  else
  {
    startValue = rangeMin;
    numSamples = this->NumberOfSamplesForNonDoseVolumes;
    stepSize = (rangeMax - rangeMin) / (double)(numSamples-1);
  }

  if (rangeMin<startValue)
  {
    startValue -= stepSize * ceil((startValue-rangeMin)/stepSize);
  }

  // We put a fixed point at (0.0, 100%), but only if there are only positive values in the histogram
  // Negative values can occur when the user requests histogram for an image, such as s CT volume (in this case Intensity Volume Histogram is computed),
  // or the startValue became negative for the dose volume because the range minimum was smaller than the original start value.
  bool insertPointAtOrigin=true;
  if (startValue<0)
  {
    //vtkWarningMacro("There are negative values in the histogram. Probably the input is not a dose volume.");
    insertPointAtOrigin=false;
  }

  stat->SetComponentExtent(0,numSamples-1,0,0,0,0);
  stat->SetComponentOrigin(startValue,0,0);
  stat->SetComponentSpacing(stepSize,1,1);
  stat->Update();

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

  vtkImageData* statArray = stat->GetOutput();
  unsigned long totalVoxels = stat->GetVoxelCount();
  unsigned long voxelBelowDose = 0;
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
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::AddDvhToSelectedChart(const char* structurePlotName, const char* dvhArrayNodeId)
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
  chartViewNode->SetChartNodeID( chartNode->GetID() );

  std::string doseAxisName;
  std::string chartTitle;
  const char* doseUnitName=doseVolumeNode->GetAttribute("DoseUnitName");
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

  // Change plot line style if there is already a structure with the same name in the chart
  vtkMRMLDoubleArrayNode* dvhArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast( this->GetMRMLScene()->GetNodeByID(dvhArrayNodeId) );
  if (dvhArrayNode == NULL)
  {
    vtkErrorMacro("Error: unable to get double array node!");
    return;
  }

  const char* structureNameToAdd = dvhArrayNode->GetAttribute(DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());

  int numberOfStructuresWithSameName = 0;
  vtkStringArray* plotNames = chartNode->GetArrayNames();
  const char* color = NULL;
  for (int i=0; i<plotNames->GetNumberOfValues(); ++i)
  {
    vtkStdString plotName = plotNames->GetValue(i);
    vtkMRMLDoubleArrayNode* arrayNode = 
      vtkMRMLDoubleArrayNode::SafeDownCast( this->GetMRMLScene()->GetNodeByID( chartNode->GetArray(plotName) ) );
    if (arrayNode == NULL)
    {
      continue;
    }

    const char* structureNameInChart = arrayNode->GetAttribute(DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str());
    if (stricmp(structureNameInChart, structureNameToAdd) == 0)
    {
      numberOfStructuresWithSameName++;
      if (numberOfStructuresWithSameName == 1)
      {
        color = chartNode->GetProperty(plotName, "color");
      }
    }
  }

  // Add array to chart
  chartNode->AddArray( structurePlotName, dvhArrayNodeId );

  // Set plot properties according to the number of structures with the same name
  if (numberOfStructuresWithSameName % 3 == 1 && color)
  {
    chartNode->SetProperty(structurePlotName, "color", color);
    chartNode->SetProperty(structurePlotName, "showLines", "off");
    chartNode->SetProperty(structurePlotName, "showMarkers", "on");
  }
  else if (numberOfStructuresWithSameName % 3 == 2 && color)
  {
    chartNode->SetProperty(structurePlotName, "color", color);
    chartNode->SetProperty(structurePlotName, "showLines", "on");
    chartNode->SetProperty(structurePlotName, "showMarkers", "on");
  }
  else
  {
    chartNode->SetProperty(structurePlotName, "showLines", "on");
    chartNode->SetProperty(structurePlotName, "showMarkers", "off");
    color = dvhArrayNode->GetAttribute(DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str());
    chartNode->SetProperty(structurePlotName, "color", color);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::RemoveDvhFromSelectedChart(const char* structureName)
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
vtkMRMLChartViewNode* vtkSlicerDoseVolumeHistogramLogic::GetChartViewNode()
{
  vtkCollection* layoutNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLLayoutNode");
  layoutNodes->InitTraversal();
  vtkObject* layoutNodeVtkObject = layoutNodes->GetNextItemAsObject();
  vtkMRMLLayoutNode* layoutNode = vtkMRMLLayoutNode::SafeDownCast(layoutNodeVtkObject);
  if (layoutNode == NULL)
  {
    vtkErrorMacro("Error: unable to get layout node!");
    return NULL;
  }
  layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );
  layoutNodes->Delete();
  
  vtkCollection* chartViewNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLChartViewNode");
  chartViewNodes->InitTraversal();
  vtkMRMLChartViewNode* chartViewNode = vtkMRMLChartViewNode::SafeDownCast( chartViewNodes->GetNextItemAsObject() );
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("Error: unable to get chart view node!");
    return NULL;
  }
  chartViewNodes->Delete();

  return chartViewNode;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeVMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> doseValues, std::vector<double> &vMetricsCc, std::vector<double> &vMetricsPercent)
{
  vMetricsCc.clear();
  vMetricsPercent.clear();

  // Get structure volume
  char attributeName[64];
  sprintf(attributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeName);
  if (!structureVolumeStr)
  {
    vtkErrorMacro("Error: Failed to get total volume attribute from DVH double array MRML node!");
    return;
  }

  double structureVolume = atof(structureVolumeStr);
  if (structureVolume == 0.0)
  {
    vtkErrorMacro("Error: Failed to parse structure total volume attribute value!");
    return;
  }

  // Compute volume for all V's
  vtkDoubleArray* doubleArray = dvhArrayNode->GetArray();
  vtkNew<vtkPiecewiseFunction> interpolator;
  interpolator->ClampingOn();
  for (int i=0; i<doubleArray->GetNumberOfTuples(); ++i)
  {
    interpolator->AddPoint(doubleArray->GetComponent(i, 0), doubleArray->GetComponent(i, 1));
  }

  for (std::vector<double>::iterator it = doseValues.begin(); it != doseValues.end(); ++it)
  {
    double volumePercentEstimated = interpolator->GetValue(*it);
    vMetricsCc.push_back( volumePercentEstimated*structureVolume/100.0 );
    vMetricsPercent.push_back( volumePercentEstimated );
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeDMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> volumeSizes, std::vector<double> &dMetrics, bool isPercent)
{
  dMetrics.clear();

  // Get structure volume
  char attributeName[64];
  sprintf(attributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeName);
  if (!structureVolumeStr)
  {
    vtkErrorMacro("Error: Failed to get total volume attribute from DVH double array MRML node!");
    return;
  }

  double structureVolume = atof(structureVolumeStr);
  if (structureVolume == 0.0)
  {
    vtkErrorMacro("Error: Failed to parse structure total volume attribute value!");
    return;
  }

  // Compute volume for all D's
  vtkDoubleArray* doubleArray = dvhArrayNode->GetArray();
  double maximumDose = 0.0;
  for (int d=-1; d<(int)volumeSizes.size(); ++d)
  {
    double volumeSize = 0.0;
    double doseForVolume = 0.0;

    // First we get the maximum dose
    // (D0.1cc can be taken as an approximation of the maximum point dose as far
    //  as clinically relevant toxicity (e.g. micro-ulceration) is concerned)
    if (d == -1)
    {
      volumeSize = 0.1;
    }
    else if (isPercent)
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
    if (d == -1)
    {
      maximumDose = doseForVolume;
    }
    else
    {
      dMetrics.push_back( maximumDose - doseForVolume );
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerDoseVolumeHistogramLogic
::DoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return false;
  }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));

  const char* doseUnitName = doseVolumeNode->GetAttribute("DoseUnitName");

  if (doseUnitName != NULL)
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::CollectMetricsForDvhNodes(std::set<std::string>* dvhNodeIds, std::vector<std::string> &metricList)
{
  metricList.clear();

  if (dvhNodeIds->size() < 1 || !this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  // Convert separator character to string
  std::ostringstream separatorCharStream;
  separatorCharStream << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  std::string separatorCharacter = separatorCharStream.str();

  // Collect metrics
  char metricListAttributeName[64];
  sprintf(metricListAttributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_LIST_ATTRIBUTE_NAME.c_str());
  std::set<std::string> metricSet;
  for (std::set<std::string>::iterator it = dvhNodeIds->begin(); it != dvhNodeIds->end(); ++it)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(it->c_str()));
    if (!dvhNode)
    {
      continue;
    }

    std::string metricListString = dvhNode->GetAttribute(metricListAttributeName);
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
  const char* metricSearchList[4] = {"volume", "mean", "min", "max"};
  for (int i=0; i<4; ++i)
  {
    for (std::set<std::string>::iterator it = metricSet.begin(); it != metricSet.end(); ++it)
    {
      if (vtksys::SystemTools::LowerCase(*it).find(metricSearchList[i]) != std::string::npos)
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
bool vtkSlicerDoseVolumeHistogramLogic
::ExportDvhToCsv(const char* fileName, bool comma/*=true*/)
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
  outfile.open(fileName);

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
  for (int i=0; i<structureNames->GetNumberOfValues(); ++i)
  {
  	outfile << structureNames->GetValue(i).c_str() << " Dose (Gy)" << (comma ? "," : "\t");
    outfile << structureNames->GetValue(i).c_str() << " Value (%)" << (comma ? "," : "\t");
  }
	outfile << std::endl;

  // Write values
	for (int row=0; row<maxNumberOfValues; ++row)
  {
	  for (int column=0; column<arrayIDs->GetNumberOfValues(); ++column)
	  {
      vtkMRMLNode *node = this->GetMRMLScene()->GetNodeByID( arrayIDs->GetValue(column) );
      vtkMRMLDoubleArrayNode* doubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(node);

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
bool vtkSlicerDoseVolumeHistogramLogic
::ExportDvhMetricsToCsv(const char* fileName,
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
  outfile.open(fileName);

	if ( !outfile )
	{
    vtkErrorMacro("Error: Output file '" << fileName << "' cannot be opened!");
		return false;
	}

  std::set<std::string>* dvhDoubleArrayNodeIds = this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds();

  // Collect metrics for all included nodes
  std::vector<std::string> metricList;
  CollectMetricsForDvhNodes(dvhDoubleArrayNodeIds, metricList);

  // Write header
  outfile << "Structure" << (comma ? "," : "\t");
  for (std::vector<std::string>::iterator it = metricList.begin(); it != metricList.end(); ++it)
  {
    outfile << it->substr(DVH_METRIC_ATTRIBUTE_NAME_PREFIX.size()) << (comma ? "," : "\t");
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
  for (std::set<std::string>::iterator it = dvhDoubleArrayNodeIds->begin(); it != dvhDoubleArrayNodeIds->end(); ++it)
  {
    vtkMRMLDoubleArrayNode* dvhNode = vtkMRMLDoubleArrayNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(it->c_str()));
    if (!dvhNode)
    {
      continue;
    }

    outfile << dvhNode->GetAttribute(DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str()) << (comma ? "," : "\t");

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
      ComputeVMetrics(dvhNode, vDoseValuesCc, volumes, dummy);
      for (std::vector<double>::iterator it = volumes.begin(); it != volumes.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }
    if (vDoseValuesPercent.size() > 0)
    {
      std::vector<double> percents;
      ComputeVMetrics(dvhNode, vDoseValuesPercent, dummy, percents);
      for (std::vector<double>::iterator it = percents.begin(); it != percents.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }

    // Add D metric values
    if (dVolumeValuesCc.size() > 0)
    {
      std::vector<double> doses;
      ComputeDMetrics(dvhNode, dVolumeValuesCc, doses, false);
      for (std::vector<double>::iterator it = doses.begin(); it != doses.end(); ++it)
      {
        outfile << (*it) << (comma ? "," : "\t");
      }
    }
    if (dVolumeValuesPercent.size() > 0)
    {
      std::vector<double> doses;
      ComputeDMetrics(dvhNode, dVolumeValuesPercent, doses, true);
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
