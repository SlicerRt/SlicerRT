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

  this->CurrentLabelValue = 2;
}

//----------------------------------------------------------------------------
vtkSlicerDoseVolumeHistogramLogic::~vtkSlicerDoseVolumeHistogramLogic()
{
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
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::GetStenciledDoseVolumeForStructureSet(vtkMRMLScalarVolumeNode* structureSetStenciledDoseVolumeNode, vtkMRMLModelNode* structureSetModel)
{
  // Create model to doseRas transform
  vtkSmartPointer<vtkGeneralTransform> modelToDoseRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkSmartPointer<vtkGeneralTransform> doseRasToWorldTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* modelTransformNode=structureSetModel->GetParentTransformNode();
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
  transformPolyDataModelToDoseIjkFilter->SetInput( structureSetModel->GetPolyData() );
  transformPolyDataModelToDoseIjkFilter->SetTransform(modelToDoseIjkTransform.GetPointer());

  // Convert model to labelmap
  vtkNew<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter;
  transformPolyDataModelToDoseIjkFilter->Update();
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToDoseIjkFilter->GetOutput() );
  polyDataToLabelmapFilter->SetReferenceImage( this->DoseVolumeNode->GetImageData() );
  polyDataToLabelmapFilter->SetLabelValue( this->CurrentLabelValue++ );
  polyDataToLabelmapFilter->Update();

  // Create labelmap node
  structureSetStenciledDoseVolumeNode->CopyOrientation( this->DoseVolumeNode );
  structureSetStenciledDoseVolumeNode->SetAndObserveTransformNodeID(this->DoseVolumeNode->GetTransformNodeID());
  std::string labelmapNodeName( this->StructureSetModelNode->GetName() );
  labelmapNodeName.append( "_Labelmap" );
  structureSetStenciledDoseVolumeNode->SetName( labelmapNodeName.c_str() );
  structureSetStenciledDoseVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  structureSetStenciledDoseVolumeNode->LabelMapOn();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeDvh()
{
  std::vector<std::string> names;
  std::vector<std::string> dvhArrayIDs;
  std::vector<double> counts;
  std::vector<double> meanDoses;
  std::vector<double> totalVolumeCCs;
  std::vector<double> maxDoses;
  std::vector<double> minDoses;

  ComputeDvh(names, dvhArrayIDs, counts, meanDoses, totalVolumeCCs, maxDoses, minDoses);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeDvh(std::vector<std::string> &names, std::vector<std::string> &dvhArrayIDs, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses)
{
  names.clear();
  dvhArrayIDs.clear();
  counts.clear();
  meanDoses.clear();
  totalVolumeCCs.clear();
  maxDoses.clear();
  minDoses.clear();

  if (this->StructureSetModelNode->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(this->StructureSetModelNode);
    vtkSmartPointer<vtkMRMLScalarVolumeNode> structureSetStenciledDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    GetStenciledDoseVolumeForStructureSet(structureSetStenciledDoseVolumeNode, modelNode);
    // this->GetMRMLScene()->AddNode( structureSetStenciledDoseVolumeNode ); // add the labelmap to the scene for testing/debugging\

    ComputeDvh(structureSetStenciledDoseVolumeNode.GetPointer(), modelNode->GetName(), names, dvhArrayIDs, counts, meanDoses, totalVolumeCCs, maxDoses, minDoses);
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
      vtkSmartPointer<vtkMRMLScalarVolumeNode> structureSetStenciledDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
      GetStenciledDoseVolumeForStructureSet(structureSetStenciledDoseVolumeNode, modelNode);
      // this->GetMRMLScene()->AddNode( structureSetStenciledDoseVolumeNode ); // add the labelmap to the scene for testing/debugging\

      ComputeDvh(structureSetStenciledDoseVolumeNode.GetPointer(), modelNode->GetName(), names, dvhArrayIDs, counts, meanDoses, totalVolumeCCs, maxDoses, minDoses);
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
::ComputeDvh(vtkMRMLScalarVolumeNode* structureSetStenciledDoseVolumeNode, char* structureSetName, std::vector<std::string> &names, std::vector<std::string> &dvhArrayIDs, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses)
{
  double* structureSetStenciledDoseVolumeSpacing = structureSetStenciledDoseVolumeNode->GetSpacing();

  double cubicMMPerVoxel = structureSetStenciledDoseVolumeSpacing[0] * structureSetStenciledDoseVolumeSpacing[1] * structureSetStenciledDoseVolumeSpacing[2];
  double ccPerCubicMM = 0.001;

  // get dose grid scaling
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

  // compute statistics
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInput(structureSetStenciledDoseVolumeNode->GetImageData());
  stencil->ThresholdByUpper(1);

  vtkNew<vtkImageAccumulate> stat;
  stat->SetInput(structureSetStenciledDoseVolumeNode->GetImageData());
  stat->SetStencil(stencil->GetOutput());
  stat->Update();

  if (stat->GetVoxelCount() > 0)
  {
    // add an entry to each list
    names.push_back( structureSetName );
    counts.push_back( stat->GetVoxelCount() );
    meanDoses.push_back( stat->GetMean()[0] * doseGridScaling );
    totalVolumeCCs.push_back( stat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM );
    maxDoses.push_back( stat->GetMax()[0] * doseGridScaling );
    minDoses.push_back( stat->GetMin()[0] * doseGridScaling );
  }

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

  if (stat->GetVoxelCount() > 0)
  {
    vtkMRMLDoubleArrayNode* arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLDoubleArrayNode") );
    std::string dvhArrayNodeName = std::string(structureSetName) + "_DVH";
    arrayNode->SetName(dvhArrayNodeName.c_str());
    arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->AddNode( arrayNode ) );
    dvhArrayIDs.push_back(arrayNode->GetID());

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
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::AddDvhToSelectedChart(const char* structureSetName, const char* dvhArrayId)
{
  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
    return;
  }
  vtkMRMLChartNode* chartNode = this->ChartNode;
  chartViewNode->SetChartNodeID( chartNode->GetID() );
  chartNode->AddArray( structureSetName, dvhArrayId );

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
::RemoveDvhFromSelectedChart(const char* structureSetName)
{
  vtkMRMLChartViewNode* chartViewNode = GetChartViewNode();
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
    return;
  }
  vtkMRMLChartNode* chartNode = this->ChartNode;
  chartViewNode->SetChartNodeID( chartNode->GetID() );
  chartNode->RemoveArray(structureSetName);
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
