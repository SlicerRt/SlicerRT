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

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TYPE_ATTRIBUTE_NAME = "Type";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TYPE_ATTRIBUTE_VALUE = "DVH";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME = "DVH_StructureName";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_STRUCTURE_PLOTS_NAME_ATTRIBUTE_NAME = "DVH_StructurePlotName";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_TOTAL_VOLUME_CC_ATTRIBUTE_NAME = "DVH_TotalVolumeCC";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_MEAN_DOSE_GY_ATTRIBUTE_NAME = "DVH_MeanDoseGy";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_MAX_DOSE_GY_ATTRIBUTE_NAME = "DVH_MaxDoseGy";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_MIN_DOSE_GY_ATTRIBUTE_NAME = "DVH_MinDoseGy";
const std::string vtkSlicerDoseVolumeHistogramLogic::DVH_VOXEL_COUNT_ATTRIBUTE_NAME = "DVH_VoxelCount";

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
  events->InsertNextValue(vtkMRMLScene::SceneEventType::EndCloseEvent);
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
    const char* structurePlotName = node->GetAttribute(DVH_STRUCTURE_PLOTS_NAME_ATTRIBUTE_NAME.c_str());

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
::GetStenciledDoseVolumeForStructure(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLModelNode* structureModel)
{
  // Create model to doseRas transform
  vtkSmartPointer<vtkGeneralTransform> modelToDoseRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkSmartPointer<vtkGeneralTransform> doseRasToWorldTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* modelTransformNode=structureModel->GetParentTransformNode();
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
  transformPolyDataModelToDoseIjkFilter->SetInput( structureModel->GetPolyData() );
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
void vtkSlicerDoseVolumeHistogramLogic::ComputeDvh()
{
  if (this->StructureSetModelNode->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(this->StructureSetModelNode);
    vtkSmartPointer<vtkMRMLScalarVolumeNode> structureStenciledDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    GetStenciledDoseVolumeForStructure(structureStenciledDoseVolumeNode, modelNode);
    // this->GetMRMLScene()->AddNode( structureStenciledDoseVolumeNode ); // add the labelmap to the scene for testing/debugging

    ComputeDvh(structureStenciledDoseVolumeNode.GetPointer(), modelNode->GetName());
  }
  else if (this->StructureSetModelNode->IsA("vtkMRMLModelHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childModelNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLModelHierarchyNode::SafeDownCast(this->StructureSetModelNode)->GetChildrenModelNodes(childModelNodes);
    childModelNodes->InitTraversal();
    if (childModelNodes->GetNumberOfItems() < 1)
    {
      std::cerr << "Error: Selected Structure Set hierarchy node has no children model nodes!" << std::endl;
      return;
    }
    
    for (int i=0; i<childModelNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(childModelNodes->GetItemAsObject(i));
      vtkSmartPointer<vtkMRMLScalarVolumeNode> structureStenciledDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      GetStenciledDoseVolumeForStructure(structureStenciledDoseVolumeNode, modelNode);
      // this->GetMRMLScene()->AddNode( structureStenciledDoseVolumeNode ); // add the labelmap to the scene for testing/debugging\

      ComputeDvh(structureStenciledDoseVolumeNode.GetPointer(), modelNode->GetName());
    }
  }
  else
  {
    std::cerr << "Error: Invalid node type for StructureSetModelNode!" << std::endl;
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeDvh(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, char* structureName)
{
  double* structureStenciledDoseVolumeSpacing = structureStenciledDoseVolumeNode->GetSpacing();

  double cubicMMPerVoxel = structureStenciledDoseVolumeSpacing[0] * structureStenciledDoseVolumeSpacing[1] * structureStenciledDoseVolumeSpacing[2];
  double ccPerCubicMM = 0.001;

  // Get dose grid scaling
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

  // Compute statistics
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInput(structureStenciledDoseVolumeNode->GetImageData());
  stencil->ThresholdByUpper(1);

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
  std::string dvhArrayNodeName = std::string(structureName) + "_DVH";
  arrayNode->SetName(dvhArrayNodeName.c_str());
  //arrayNode->HideFromEditorsOff();

  arrayNode->SetAttribute(DVH_TYPE_ATTRIBUTE_NAME.c_str(), DVH_TYPE_ATTRIBUTE_VALUE.c_str());
  arrayNode->SetAttribute(DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureName);

  char attributeValue[64];
  sprintf(attributeValue, "%g", stat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM);
  arrayNode->SetAttribute(DVH_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str(), attributeValue);
  sprintf(attributeValue, "%g", stat->GetMean()[0] * doseGridScaling);
  arrayNode->SetAttribute(DVH_MEAN_DOSE_GY_ATTRIBUTE_NAME.c_str(), attributeValue);
  sprintf(attributeValue, "%g", stat->GetMax()[0] * doseGridScaling);
  arrayNode->SetAttribute(DVH_MAX_DOSE_GY_ATTRIBUTE_NAME.c_str(), attributeValue);
  sprintf(attributeValue, "%g", stat->GetMin()[0] * doseGridScaling);
  arrayNode->SetAttribute(DVH_MIN_DOSE_GY_ATTRIBUTE_NAME.c_str(), attributeValue);
  sprintf(attributeValue, "%g", stat->GetVoxelCount());
  arrayNode->SetAttribute(DVH_VOXEL_COUNT_ATTRIBUTE_NAME.c_str(), attributeValue);

  arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->AddNode( arrayNode ) );

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
    doubleArray->SetComponent( outputArrayIndex, 0, (rangeMin+sampleIndex*spacing)*doseGridScaling );
    doubleArray->SetComponent( outputArrayIndex, 1, (1.0-(double)voxelBelowDose/(double)totalVoxels)*100.0 );
    doubleArray->SetComponent( outputArrayIndex, 2, 0 );
    ++outputArrayIndex;
    voxelBelowDose += voxelsInBin;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::AddDvhToSelectedChart(const char* structureName, const char* dvhArrayId)
{
  if (this->ChartNode == NULL)
  {
    std::cerr << "Error: no chart node is selected!" << std::endl;
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
    return;
  }

  vtkMRMLChartNode* chartNode = this->ChartNode;
  chartViewNode->SetChartNodeID( chartNode->GetID() );
  chartNode->AddArray( structureName, dvhArrayId );

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
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::RemoveDvhFromSelectedChart(const char* structureName)
{
  if (this->ChartNode == NULL)
  {
    std::cerr << "Error: no chart node is selected!" << std::endl;
    return;
  }

  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
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
    std::cerr << "Error: unable to get layout node!" << std::endl;
    return NULL;
  }
  layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );
  layoutNodes->Delete();
  
  vtkCollection* chartViewNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLChartViewNode");
  chartViewNodes->InitTraversal();
  vtkMRMLChartViewNode* chartViewNode = vtkMRMLChartViewNode::SafeDownCast( chartViewNodes->GetNextItemAsObject() );
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
    return NULL;
  }
  chartViewNodes->Delete();

  return chartViewNode;
}
