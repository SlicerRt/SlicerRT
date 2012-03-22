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
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLChartNode.h"
#include "vtkMRMLLayoutNode.h"
#include "vtkMRMLChartViewNode.h"
#include "vtkMRMLDoubleArrayNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLModelDisplayNode.h"

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
vtkCxxSetObjectMacro(vtkSlicerDoseVolumeHistogramLogic, StructureSetModelNode, vtkMRMLModelNode);
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
::GetLabelmapVolumeNodeForSelectedStructureSet(vtkMRMLScalarVolumeNode* structureSetLabelmapVolumeNode)
{

  // Create model to doseRas to model transform
  vtkSmartPointer<vtkGeneralTransform> modelToDoseRasTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkSmartPointer<vtkGeneralTransform> doseRasToWorldTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  vtkMRMLTransformNode* modelTransformNode=this->StructureSetModelNode->GetParentTransformNode();
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
  transformPolyDataModelToDoseIjkFilter->SetInput( this->StructureSetModelNode->GetPolyData() );
  transformPolyDataModelToDoseIjkFilter->SetTransform(modelToDoseIjkTransform.GetPointer());

  // Convert model to labelmap
  vtkNew<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter;
  transformPolyDataModelToDoseIjkFilter->Update();
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToDoseIjkFilter->GetOutput() );
  polyDataToLabelmapFilter->SetReferenceImage( this->DoseVolumeNode->GetImageData() );
  polyDataToLabelmapFilter->SetLabelValue( this->CurrentLabelValue++ );
  polyDataToLabelmapFilter->Update();

  // Create labelmap node
  structureSetLabelmapVolumeNode->CopyOrientation( this->DoseVolumeNode );
  structureSetLabelmapVolumeNode->SetAndObserveTransformNodeID(this->DoseVolumeNode->GetTransformNodeID());
  std::string labelmapNodeName( this->StructureSetModelNode->GetName() );
  labelmapNodeName.append( "_Labelmap" );
  structureSetLabelmapVolumeNode->SetName( labelmapNodeName.c_str() );
  structureSetLabelmapVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  structureSetLabelmapVolumeNode->LabelMapOn();

}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::ComputeStatistics(std::vector<std::string> &names, std::vector<double> &counts, std::vector<double> &meanDoses, std::vector<double> &totalVolumeCCs, std::vector<double> &maxDoses, std::vector<double> &minDoses)
{
  names.clear();
  counts.clear();
  meanDoses.clear();
  totalVolumeCCs.clear();
  maxDoses.clear();
  minDoses.clear();

  vtkSmartPointer<vtkMRMLScalarVolumeNode> structureSetLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  GetLabelmapVolumeNodeForSelectedStructureSet(structureSetLabelmapVolumeNode);
  // this->GetMRMLScene()->AddNode( structureSetLabelmapVolumeNode ); // add the labelmap to the scene for testing/debugging

  double* structureSetLabelmapSpacing = structureSetLabelmapVolumeNode->GetSpacing();

  double cubicMMPerVoxel = structureSetLabelmapSpacing[0] * structureSetLabelmapSpacing[1] * structureSetLabelmapSpacing[2];
  double ccPerCubicMM = 0.001;

  vtkNew<vtkImageAccumulate> stataccum;
  stataccum->SetInput( structureSetLabelmapVolumeNode->GetImageData() );
  stataccum->Update();

  int lo = (int)(stataccum->GetMin()[0]);
  // don't compute DVH the background (voxels)
  if (lo == 0)
  {
    lo = 1;
  }
  int hi = (int)(stataccum->GetMax()[0]);

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

  // prevent long computations for non-labelmap images
  if (hi-lo > 128)
  {
    return;
  }

  for (int i=lo; i<=hi; ++i)
  {
    // logic copied from slicer3 DoseVolumeHistogram to create the binary volume of the label
    vtkNew<vtkImageThreshold> thresholder;
    thresholder->SetInput(structureSetLabelmapVolumeNode->GetImageData());
    thresholder->SetInValue(1);
    thresholder->SetOutValue(0);
    thresholder->ReplaceOutOn();
    thresholder->ThresholdBetween(i,i);
    thresholder->SetOutputScalarType(this->DoseVolumeNode->GetImageData()->GetScalarType());
    thresholder->Update();

    // use vtk's statistics class with the binary labelmap as a stencil
    vtkNew<vtkImageToImageStencil> stencil;
    stencil->SetInput(thresholder->GetOutput());
    stencil->ThresholdBetween(1,1);

    vtkNew<vtkImageAccumulate> stat;
    stat->SetInput(this->DoseVolumeNode->GetImageData());
    stat->SetStencil(stencil->GetOutput());
    stat->Update();

    if (stat->GetVoxelCount() > 0)
    {
      // add an entry to each list
      names.push_back( this->StructureSetModelNode->GetName() );
      counts.push_back( stat->GetVoxelCount() );
      meanDoses.push_back( stat->GetMean()[0] * doseGridScaling );
      totalVolumeCCs.push_back( stat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM );
      maxDoses.push_back( stat->GetMax()[0] * doseGridScaling );
      minDoses.push_back( stat->GetMin()[0] * doseGridScaling );
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramLogic
::AddDvhToSelectedChart()
{
  vtkCollection* layoutNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLLayoutNode");
  layoutNodes->InitTraversal();
  vtkObject* layoutNodeVtkObject = layoutNodes->GetNextItemAsObject();
  vtkMRMLLayoutNode* layoutNode = dynamic_cast<vtkMRMLLayoutNode*>(layoutNodeVtkObject);
  if (layoutNode == NULL)
  {
    std::cerr << "Error: unable to get layout node!" << std::endl;
    return;
  }
  layoutNode->SetViewArrangement( vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView );
  
  vtkCollection* chartViewNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLChartViewNode");
  chartViewNodes->InitTraversal();
  vtkObject* chartViewNodeVtkObject = chartViewNodes->GetNextItemAsObject();
  vtkMRMLChartViewNode* chartViewNode = dynamic_cast<vtkMRMLChartViewNode*>(chartViewNodeVtkObject);
  if (chartViewNode == NULL)
  {
    std::cerr << "Error: unable to get chart view node!" << std::endl;
    return;
  }

  vtkSmartPointer<vtkMRMLScalarVolumeNode> structureSetLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  GetLabelmapVolumeNodeForSelectedStructureSet(structureSetLabelmapVolumeNode);
  // this->GetMRMLScene()->AddNode( structureSetLabelmapVolumeNode ); // add the labelmap to the scene for testing/debugging

  vtkNew<vtkImageAccumulate> stataccum;
  stataccum->SetInput( structureSetLabelmapVolumeNode->GetImageData() );
  stataccum->Update();

  int lo = (int)(stataccum->GetMin()[0]);
  // don't compute DVH the background (voxels)
  if (lo == 0)
  {
    lo = 1;
  }
  int hi = (int)(stataccum->GetMax()[0]);

  // prevent long computations for non-labelmap images
  if (hi-lo > 128)
  {
    return;
  }

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

  // Add selected chart node to view  
  vtkMRMLChartNode* chartNode = this->ChartNode;
  chartViewNode->SetChartNodeID( chartNode->GetID() );

  for (int i=lo; i<=hi; ++i)
  {
    // logic copied from slicer3 DoseVolumeHistogram to create the binary volume of the label
    vtkNew<vtkImageThreshold> thresholder;
    thresholder->SetInput(structureSetLabelmapVolumeNode->GetImageData());
    thresholder->SetInValue(1);
    thresholder->SetOutValue(0);
    thresholder->ReplaceOutOn();
    thresholder->ThresholdBetween(i,i);
    thresholder->SetOutputScalarType(this->DoseVolumeNode->GetImageData()->GetScalarType());
    thresholder->Update();

    // use vtk's statistics class with the binary labelmap as a stencil
    vtkNew<vtkImageToImageStencil> stencil;
    stencil->SetInput(thresholder->GetOutput());
    stencil->ThresholdBetween(1,1);
    stencil->Update();

    vtkNew<vtkImageAccumulate> stat;
    stat->SetInput(this->DoseVolumeNode->GetImageData());
    stat->SetStencil(stencil->GetOutput());
    stat->Update();

    int numBins = 100;
    double rangeMin = stat->GetMin()[0];
    double rangeMax = stat->GetMax()[0];
    double spacing = (rangeMax - rangeMin) / (double)numBins;

    stat->SetComponentExtent(0,numBins-1,0,0,0,0);
    stat->SetComponentOrigin(rangeMin,0,0);
    stat->SetComponentSpacing(spacing,1,1);
    stat->Update();

    // TODO: set stat object as member?

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
      arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->AddNode( arrayNode ) );
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

      chartNode->AddArray( this->StructureSetModelNode->GetName(), arrayNode->GetID() );
    }
  }
  
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
