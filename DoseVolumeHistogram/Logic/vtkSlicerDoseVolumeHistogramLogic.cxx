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

==============================================================================*/

// ModuleTemplate includes
#include "vtkSlicerDoseVolumeHistogramLogic.h"

#include "vtkPolyDataToLabelmapFilter.h"

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

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TYPE_ATTRIBUTE_NAME = "Type";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TYPE_ATTRIBUTE_VALUE = "DVH";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME = "DoseVolumeNodeId";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = "StructureName";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME = "StructureModelNodeId";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME = "StructureColor";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME = "DVH_StructurePlotName";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_METRIC_ATTRIBUTE_NAME_PREFIX = "DVH_Metric:";
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

vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, DoseVolumeNode, vtkMRMLVolumeNode);
vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, StructureSetModelNode, vtkMRMLNode);
vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, ChartNode, vtkMRMLChartNode);

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::vtkSlicerDoseVolumeHistogramLogic()
{
  this->DoseVolumeNode = NULL;
  this->StructureSetModelNode = NULL;
  this->ChartNode = NULL;

  this->DvhDoubleArrayNodes = NULL;
  vtkSmartPointer<vtkCollection> dvhDoubleArrayNodes = vtkSmartPointer<vtkCollection>::New();
  this->SetDvhDoubleArrayNodes(dvhDoubleArrayNodes);

  this->SceneChangedOff();
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::~vtkSlicerDoseVolumeHistogramLogic()
{
  this->SetDvhDoubleArrayNodes(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::UpdateFromMRMLScene()
{
  this->DvhDoubleArrayNodes->RemoveAllItems();
  this->SceneChangedOn();

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
        this->DvhDoubleArrayNodes->AddItem(doubleArrayNode);
      }
    }

    node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLDoubleArrayNode");
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
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
        this->DvhDoubleArrayNodes->AddItem(doubleArrayNode);
      }
    }
  }

  this->SceneChangedOn();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
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
    this->DvhDoubleArrayNodes->RemoveItem( vtkMRMLDoubleArrayNode::SafeDownCast(node) );

    // Remove the structure corresponding the deleted node from all charts
    const char* structurePlotName = node->GetAttribute(DVH_STRUCTURE_PLOT_NAME_ATTRIBUTE_NAME.c_str());

    this->GetMRMLScene()->InitTraversal();
    vtkMRMLNode *node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLChartNode");
    while (node != NULL)
    {
      vtkMRMLChartNode* chartNode = vtkMRMLChartNode::SafeDownCast(node);
      if (chartNode)
      {
        chartNode->RemoveArray(structurePlotName);
      }

      node = this->GetMRMLScene()->GetNextNodeByClass("vtkMRMLDoubleArrayNode");
    }
  }

  this->SceneChangedOn();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::GetStenciledDoseVolumeForStructure(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModelNode)
{
  // Create model to doseRas transform
  vtkSmartPointer<vtkGeneralTransform> modelToDoseRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkSmartPointer<vtkGeneralTransform> doseRasToWorldTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* modelTransformNode=structureModelNode->GetParentTransformNode();
  vtkMRMLTransformNode* doseTransformNode=this->DoseVolumeNode->GetParentTransformNode();

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
  this->DoseVolumeNode->GetRASToIJKMatrix( doseRasToDoseIjkTransformMatrix );  
  
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
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToDoseIjkFilter->GetOutput() );
  polyDataToLabelmapFilter->SetReferenceImage( this->DoseVolumeNode->GetImageData() );
  polyDataToLabelmapFilter->Update();

  // Create labelmap node
  structureStenciledDoseVolumeNode->CopyOrientation( this->DoseVolumeNode );
  structureStenciledDoseVolumeNode->SetAndObserveTransformNodeID(this->DoseVolumeNode->GetTransformNodeID());
  std::string labelmapNodeName( this->StructureSetModelNode->GetName() );
  labelmapNodeName.append( "_Labelmap" );
  structureStenciledDoseVolumeNode->SetName( labelmapNodeName.c_str() );
  structureStenciledDoseVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  structureStenciledDoseVolumeNode->LabelMapOn();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic::GetSelectedStructureModelNodes(std::vector<vtkMRMLModelNode*> &structureModelNodes)
{
  structureModelNodes.clear();

  if (this->StructureSetModelNode->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(this->StructureSetModelNode);
    if (modelNode)
    {
      structureModelNodes.push_back(modelNode);
    }
  }
  else if (this->StructureSetModelNode->IsA("vtkMRMLModelHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLModelHierarchyNode::SafeDownCast(this->StructureSetModelNode)->GetChildrenModelNodes(childModelNodes);
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
    vtkSmartPointer<vtkMRMLScalarVolumeNode> structureStenciledDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    GetStenciledDoseVolumeForStructure(structureStenciledDoseVolumeNode, (*it));
    // this->GetMRMLScene()->AddNode( structureStenciledDoseVolumeNode ); // add the labelmap to the scene for testing/debugging

    ComputeDvh(structureStenciledDoseVolumeNode.GetPointer(), (*it));
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeDvh(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModelNode)
{
  double* structureStenciledDoseVolumeSpacing = structureStenciledDoseVolumeNode->GetSpacing();

  double cubicMMPerVoxel = structureStenciledDoseVolumeSpacing[0] * structureStenciledDoseVolumeSpacing[1] * structureStenciledDoseVolumeSpacing[2];
  double ccPerCubicMM = 0.001;

  // Get dose grid scaling and dose units
  const char* doseGridScalingString = this->DoseVolumeNode->GetAttribute("DoseGridScaling");
  double doseGridScaling = 1.0;
  if (doseGridScalingString!=NULL)
  {
    doseGridScaling = atof(doseGridScalingString);
  }
  else
  {
    vtkWarningMacro("Dose grid scaling attribute is not set for the selected dose volume. Assuming scaling = 1.");
  }

  const char* doseUnits = this->DoseVolumeNode->GetAttribute("DoseUnits");

  // Compute statistics
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInput(structureStenciledDoseVolumeNode->GetImageData());
  stencil->ThresholdByUpper(0.5 * doseGridScaling); // 0.5 to make sure that 1*doseGridScaling is larger or equal than the threshold

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
  arrayNode->SetAttribute(DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME.c_str(), this->DoseVolumeNode->GetID());
  arrayNode->SetAttribute(DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureModelNode->GetName());
  arrayNode->SetAttribute(DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME.c_str(), structureModelNode->GetID());

  char attributeValue[64];
  double* color = structureModelNode->GetDisplayNode()->GetColor();
  sprintf(attributeValue, "#%X%X%X", (int)(color[0]*256.0), (int)(color[1]*256.0), (int)(color[2]*256.0));
  arrayNode->SetAttribute(DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str(), attributeValue);

  char attributeName[64];
  std::ostringstream metricList;

  sprintf(attributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  sprintf(attributeValue, "%g", stat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s (%s)", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnits);
  sprintf(attributeValue, "%g", stat->GetMean()[0]);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s (%s)", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnits);
  sprintf(attributeValue, "%g", stat->GetMax()[0]);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s (%s)", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnits);
  sprintf(attributeValue, "%g", stat->GetMin()[0]);
  metricList << attributeName << DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s", DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), DVH_METRIC_LIST_ATTRIBUTE_NAME.c_str());
  arrayNode->SetAttribute(attributeName, metricList.str().c_str());

  arrayNode = vtkMRMLDoubleArrayNode::SafeDownCast( this->GetMRMLScene()->AddNode( arrayNode ) );

  // Create DVH plot values
  int numBins = 100;
  double rangeMin = stat->GetMin()[0];
  double rangeMax = stat->GetMax()[0];
  double spacing = (rangeMax - rangeMin) / (double)numBins;

  stat->SetComponentExtent(0,numBins-1,0,0,0,0);
  stat->SetComponentOrigin(rangeMin,0,0);
  stat->SetComponentSpacing(spacing,1,1);
  stat->Update();

  // We put a fixed point at (0.0, 100%), but only if there are only positive values in the histogram
  // Negative values can occur when the user requests histogram for an image, such as s CT volume.
  // In this case Intensity Volume Histogram is computed.
  bool insertPointAtOrigin=true;
  if (rangeMin<0)
  {
    vtkWarningMacro("There are negative values in the histogram. Probably the input is not a dose volume.");
    insertPointAtOrigin=false;
  }

  vtkDoubleArray* doubleArray = arrayNode->GetArray();
  doubleArray->SetNumberOfTuples( numBins + (insertPointAtOrigin?1:0) ); 

  int outputArrayIndex=0;

  if (insertPointAtOrigin)
  {
    // Add first fixed point at (0.0, 100%)
    doubleArray->SetComponent(outputArrayIndex, 0, 0.0);
    doubleArray->SetComponent(outputArrayIndex, 1, 100.0);
    doubleArray->SetComponent(outputArrayIndex, 2, 0);
    ++outputArrayIndex;
  }

  unsigned long totalVoxels = stat->GetVoxelCount();
  unsigned long voxelBelowDose = 0;
  for (int sampleIndex=0; sampleIndex<numBins; ++sampleIndex)
  {
    unsigned long voxelsInBin = stat->GetOutput()->GetScalarComponentAsDouble(sampleIndex,0,0,0);
    doubleArray->SetComponent( outputArrayIndex, 0, rangeMin+sampleIndex*spacing );
    doubleArray->SetComponent( outputArrayIndex, 1, (1.0-(double)voxelBelowDose/(double)totalVoxels)*100.0 );
    doubleArray->SetComponent( outputArrayIndex, 2, 0 );
    ++outputArrayIndex;
    voxelBelowDose += voxelsInBin;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::AddDvhToSelectedChart(const char* structurePlotName, const char* dvhArrayNodeId)
{
  if (this->ChartNode == NULL)
  {
    vtkErrorMacro("Error: no chart node is selected!");
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("Error: unable to get chart view node!");
    return;
  }

  // Set chart properties
  vtkMRMLChartNode* chartNode = this->ChartNode;
  chartViewNode->SetChartNodeID( chartNode->GetID() );

  std::string doseAxisName;
  std::string chartTitle;
  const char* doseUnits=this->DoseVolumeNode->GetAttribute("DoseUnits");
  if (doseUnits!=NULL)
  {
    doseAxisName=std::string("Dose [")+doseUnits+"]";
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
void vtkSlicerDoseVolumeHistogramLogic
::RemoveDvhFromSelectedChart(const char* structureName)
{
  if (this->ChartNode == NULL)
  {
    vtkErrorMacro("Error: no chart node is selected!");
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    vtkErrorMacro("Error: unable to get chart view node!");
    return;
  }

  vtkMRMLChartNode* chartNode = this->ChartNode;
  chartViewNode->SetChartNodeID( chartNode->GetID() );
  chartNode->RemoveArray(structureName);
}

//---------------------------------------------------------------------------
vtkMRMLChartViewNode* vtkSlicerDoseVolumeHistogramLogic
::GetChartViewNode()
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
::ComputeVMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> doseValues, std::vector<double> &vMetrics)
{
  vMetrics.clear();

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
  for (std::vector<double>::iterator it = doseValues.begin(); it != doseValues.end(); ++it)
  {
    double doseValue = (*it);

    // Check if the given dose is below the lowest value in the array then assign the whole volume of the structure
    if (doseValue < doubleArray->GetComponent(0, 0))
    {
      vMetrics.push_back(structureVolume);
      continue;
    }

    // If dose is above the highest value in the array then assign 0 Cc volume
    if (doseValue >= doubleArray->GetComponent(doubleArray->GetNumberOfTuples()-1, 0))
    {
      vMetrics.push_back(0.0);
      continue;
    }

    // Look for the dose in the array
    for (int i=0; i<doubleArray->GetNumberOfTuples()-1; ++i)
    {
      double doseBelow = doubleArray->GetComponent(i, 0);
      double doseAbove = doubleArray->GetComponent(i+1, 0);
      if (doseBelow <= doseValue && doseValue < doseAbove)
      {
        // Compute the volume percent using linear interpolation
        double volumePercentBelow = doubleArray->GetComponent(i, 1);
        double volumePercentAbove = doubleArray->GetComponent(i+1, 1);
        double volumePercentEstimated = volumePercentBelow + (volumePercentAbove-volumePercentBelow)*(doseValue-doseBelow)/(doseAbove-doseBelow);
        vMetrics.push_back( volumePercentEstimated*structureVolume/100.0 );

        break;
      }
    }
  }
}
