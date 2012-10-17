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
#include "vtkPolyDataToLabelmapFilter.h"
#include "vtkMRMLDoseVolumeHistogramNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLContourNode.h>
#include <vtkMRMLChartNode.h>
#include <vtkMRMLLayoutNode.h>
#include <vtkMRMLChartViewNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLContourHierarchyNode.h>
#include <vtkMRMLColorTableNode.h>

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
#include <vtkImageResample.h>
#include <vtkTimerLog.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// STD includes
#include <cassert>
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
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
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
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourHierarchyNode>::New());
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
      const char* type = doubleArrayNode->GetAttribute(SlicerRtCommon::DVH_TYPE_ATTRIBUTE_NAME.c_str());
      if (type != NULL && strcmp(type, SlicerRtCommon::DVH_TYPE_ATTRIBUTE_VALUE.c_str()) == 0)
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
  assert(this->GetMRMLScene() != 0);

  RefreshDvhDoubleArrayNodesFromScene();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
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
      const char* type = doubleArrayNode->GetAttribute(SlicerRtCommon::DVH_TYPE_ATTRIBUTE_NAME.c_str());
      if (type != NULL && strcmp(type, SlicerRtCommon::DVH_TYPE_ATTRIBUTE_VALUE.c_str()) == 0)
      {
        this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds()->push_back(doubleArrayNode->GetID());
      }
    }
  }

  if (node->IsA("vtkMRMLVolumeNode") || node->IsA("vtkMRMLDoubleArrayNode")
    || node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLContourHierarchyNode")
    || node->IsA("vtkMRMLChartNode") || node->IsA("vtkMRMLDoseVolumeHistogramNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
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
    || node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLContourHierarchyNode")
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
void vtkSlicerDoseVolumeHistogramModuleLogic
::GetStenciledDoseVolumeForContour(vtkMRMLScalarVolumeNode* structureStenciledDoseVolumeNode, vtkMRMLContourNode* structureContourNode)
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode || !structureContourNode)
  {
    return;
  }

  vtkMRMLModelNode* structureModelNode = structureContourNode->GetRibbonModelNode();
  if (!structureModelNode)
  {
    vtkErrorMacro("Failed to get ribbon model node for contour!");
    return;
  }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));

  // Create model to doseIjk transform
  vtkSmartPointer<vtkGeneralTransform> modelToDoseIjkTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  SlicerRtCommon::GetTransformFromModelToVolumeIjk(structureModelNode, doseVolumeNode, modelToDoseIjkTransform);

  // Transform the model polydata to doseIjk coordinate frame (the labelmap image coordinate frame is doseIjk)
  vtkNew<vtkTransformPolyDataFilter> transformPolyDataModelToDoseIjkFilter;
  transformPolyDataModelToDoseIjkFilter->SetInput( structureModelNode->GetPolyData() );
  transformPolyDataModelToDoseIjkFilter->SetTransform(modelToDoseIjkTransform.GetPointer());

  // Convert model to stenciled dose volume
  vtkNew<vtkPolyDataToLabelmapFilter> polyDataToLabelmapFilter;
  transformPolyDataModelToDoseIjkFilter->Update();
  polyDataToLabelmapFilter->SetBackgroundValue(VTK_DOUBLE_MIN);
  polyDataToLabelmapFilter->SetInputPolyData( transformPolyDataModelToDoseIjkFilter->GetOutput() );

  double rasterizationDownsamplingFactor = structureContourNode->GetRasterizationDownsamplingFactor();
  if (rasterizationDownsamplingFactor != 1.0)
  {
    vtkSmartPointer<vtkImageResample> resampler = vtkSmartPointer<vtkImageResample>::New();
    resampler->SetInput(doseVolumeNode->GetImageData());
    resampler->SetAxisMagnificationFactor(0, rasterizationDownsamplingFactor);
    resampler->SetAxisMagnificationFactor(1, rasterizationDownsamplingFactor);
    resampler->SetAxisMagnificationFactor(2, rasterizationDownsamplingFactor);
    resampler->Update();

    polyDataToLabelmapFilter->SetReferenceImage( resampler->GetOutput() );
  }
  else
  {
    polyDataToLabelmapFilter->SetReferenceImage( doseVolumeNode->GetImageData() );
  }
  polyDataToLabelmapFilter->Update();

  // Create node
  structureStenciledDoseVolumeNode->CopyOrientation( doseVolumeNode );
  if (rasterizationDownsamplingFactor != 1.0)
  {
    double* doseSpacing = doseVolumeNode->GetSpacing();
    structureStenciledDoseVolumeNode->SetSpacing(
      doseSpacing[0]/rasterizationDownsamplingFactor,
      doseSpacing[1]/rasterizationDownsamplingFactor,
      doseSpacing[2]/rasterizationDownsamplingFactor );

    vtkImageData* structureStenciledDoseVolumeImageData = polyDataToLabelmapFilter->GetOutput();
    structureStenciledDoseVolumeImageData->SetSpacing(1.0, 1.0, 1.0); // The spacing is set to the MRML node
  }

  std::string stenciledDoseNodeName = std::string(structureContourNode->GetStructureName()) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
  stenciledDoseNodeName = this->GetMRMLScene()->GenerateUniqueName(stenciledDoseNodeName);

  structureStenciledDoseVolumeNode->SetAndObserveTransformNodeID( doseVolumeNode->GetTransformNodeID() );
  structureStenciledDoseVolumeNode->SetName( stenciledDoseNodeName.c_str() );
  structureStenciledDoseVolumeNode->SetAndObserveImageData( polyDataToLabelmapFilter->GetOutput() );
  structureStenciledDoseVolumeNode->SetAttribute( SlicerRtCommon::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME.c_str(), doseVolumeNode->GetID() );
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
::GetSelectedContourNodes(std::vector<vtkMRMLContourNode*> &contourNodes)
{
  contourNodes.clear();

  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return;
  }

  vtkMRMLNode* structureSetContourNode = this->GetMRMLScene()->GetNodeByID(
    this->DoseVolumeHistogramNode->GetStructureSetContourNodeId());

  if (structureSetContourNode->IsA("vtkMRMLContourNode"))
  {
    vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(structureSetContourNode);
    if (contourNode)
    {
      contourNodes.push_back(contourNode);
    }
  }
  else if (structureSetContourNode->IsA("vtkMRMLContourHierarchyNode"))
  {
    vtkSmartPointer<vtkCollection> childContourNodes = vtkSmartPointer<vtkCollection>::New();
    vtkMRMLContourHierarchyNode::SafeDownCast(structureSetContourNode)->GetChildrenContourNodes(childContourNodes);
    childContourNodes->InitTraversal();
    if (childContourNodes->GetNumberOfItems() < 1)
    {
      vtkErrorMacro("Error: Selected Structure Set hierarchy node has no children contour nodes!");
      return;
    }
    
    for (int i=0; i<childContourNodes->GetNumberOfItems(); ++i)
    {
      vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(childContourNodes->GetItemAsObject(i));
      if (contourNode)
      {
        contourNodes.push_back(contourNode);
      }
    }
  }
  else
  {
    vtkErrorMacro("Error: Invalid node type for ContourNode!");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic::ComputeDvh()
{
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  double sumRasterization = 0.0;
  double sumDvhComputation = 0.0;
  double checkpointStart = timer->GetUniversalTime();

  std::vector<vtkMRMLContourNode*> structureContourNodes;
  this->GetSelectedContourNodes(structureContourNodes);

  if (structureContourNodes.size() == 0)
  {
    vtkErrorMacro("Error: Empty structure list!");
    return;
  }

  // Get structure set hierarchy node
  vtkMRMLContourHierarchyNode* structureContourHierarchyNode = vtkMRMLContourHierarchyNode::SafeDownCast(
    vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(this->GetMRMLScene(), structureContourNodes[0]->GetID()));

  if (!structureContourHierarchyNode)
  {
    vtkErrorMacro("Error: No hierarchy node found for structure '" << structureContourNodes[0]->GetName() << "'");
    return;
  }
  vtkMRMLContourHierarchyNode* structureSetHierarchyNode
    = vtkMRMLContourHierarchyNode::SafeDownCast(structureContourHierarchyNode->GetParentNode());

  // Get color node created for the structure set
  const char* seriesNameChars = structureSetHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SERIES_NAME_ATTRIBUTE_NAME.c_str());
  std::string seriesName = (seriesNameChars ? seriesNameChars : "EmptySeriesName");

  std::string colorNodeName = seriesName + SlicerRtCommon::DICOMRTIMPORT_COLOR_TABLE_NODE_NAME_POSTFIX;

  vtkCollection* colorNodes = this->GetMRMLScene()->GetNodesByName(colorNodeName.c_str());

  if (colorNodes->GetNumberOfItems() == 0)
  {
    vtkErrorMacro("Error: No color table found for structure set '" << structureSetHierarchyNode->GetName() << "'");
  }

  // Compute DVH for each structure
  for (std::vector<vtkMRMLContourNode*>::iterator it = structureContourNodes.begin(); it != structureContourNodes.end(); ++it)
  {
    double checkpointStructureStart = timer->GetUniversalTime();

    vtkMRMLModelNode* modelNode = (*it)->GetRibbonModelNode();
    if (!modelNode)
    {
      vtkErrorMacro("Failed to get ribbon model node for contour!");
      continue;
    }

    vtkSmartPointer<vtkMRMLScalarVolumeNode> structureStenciledDoseVolumeNode
      = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();

    double checkpointRasterizationStart = timer->GetUniversalTime();
    this->GetStenciledDoseVolumeForContour(structureStenciledDoseVolumeNode, (*it));

    double checkpointDvhStart = timer->GetUniversalTime();
    this->ComputeDvh(structureStenciledDoseVolumeNode.GetPointer(), modelNode);

    double checkpointLabelmapCreationStart = timer->GetUniversalTime();

    // Get color table node and structure color index
    colorNodes->InitTraversal();
    vtkMRMLColorTableNode* colorNode = vtkMRMLColorTableNode::SafeDownCast(colorNodes->GetNextItemAsObject());
    int structureColorIndex = -1;
    while (colorNode)
    {
      int colorIndex = -1;
      if ((colorIndex = colorNode->GetColorIndexByName((*it)->GetStructureName())) != -1)
      {
        double modelColor[3];
        double foundColor[4];
        modelNode->GetDisplayNode()->GetColor(modelColor);
        colorNode->GetColor(colorIndex, foundColor);
        if ((modelColor[0] == foundColor[0]) && (modelColor[1] == foundColor[1]) && (modelColor[2] == foundColor[2]))
        {
          structureColorIndex = colorIndex;
          break;
        }
      }
      colorNode = vtkMRMLColorTableNode::SafeDownCast(colorNodes->GetNextItemAsObject());
    }
    if (structureColorIndex == -1)
    {
      vtkWarningMacro("No matching entry found in the color tables for structure '" << (*it)->GetStructureName() << "'");
      structureColorIndex = 1; // Gray 'invalid' color
    }

    // Convert stenciled dose volume to labelmap
    if ((*it)->GetIndexedLabelmapVolumeNodeId() == NULL)
    {
      vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
      threshold->SetInput(structureStenciledDoseVolumeNode->GetImageData());
      threshold->SetInValue(structureColorIndex);
      threshold->SetOutValue(0);
      threshold->ThresholdByUpper(VTK_DOUBLE_MIN+1.0);
      threshold->SetOutputScalarTypeToUnsignedChar();
      threshold->Update();
      structureStenciledDoseVolumeNode->GetImageData()->DeepCopy(threshold->GetOutput());

      vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode> displayNode = vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode>::New();
      displayNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
      if (colorNode)
      {
        displayNode->SetAndObserveColorNodeID(colorNode->GetID());
      }
      else
      {
        displayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeLabels");
      }

      structureStenciledDoseVolumeNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
      structureStenciledDoseVolumeNode->LabelMapOn();
      this->GetMRMLScene()->AddNode(structureStenciledDoseVolumeNode);
      (*it)->SetAndObserveIndexedLabelmapVolumeNodeId(structureStenciledDoseVolumeNode->GetID());
    }

    double checkpointStructureEnd = timer->GetUniversalTime();
    sumRasterization += checkpointDvhStart-checkpointRasterizationStart;
    sumDvhComputation += checkpointLabelmapCreationStart-checkpointDvhStart;

    if (this->LogSpeedMeasurements)
    {
      std::cout << "\tStructure '" << (*it)->GetStructureName() << "':\n\t\tTotal: " << checkpointStructureEnd-checkpointStructureStart
        << " s\n\t\tRasterization: " << checkpointDvhStart-checkpointRasterizationStart
        << " s\n\t\tDVH computation: " << checkpointLabelmapCreationStart-checkpointDvhStart
        << " s\n\t\tLabelmap creation: " /*<< (this->GetDoseVolumeHistogramNode()->GetSaveLabelmaps()?"On":"Off") << "): "*/
        << checkpointStructureEnd-checkpointLabelmapCreationStart << std::endl;
    }
  } // for all contours

  double checkpointEnd = timer->GetUniversalTime();
  if (this->LogSpeedMeasurements)
  {
    std::cout << "Sum rasterization time: " << sumRasterization << " s" << std::endl
      << "Sum DVH computation time: " << sumDvhComputation << " s" << std::endl
      << "Total: " << checkpointEnd-checkpointStart << " s" << std::endl;
  }

  colorNodes->Delete();
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
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
  const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  // Get maximum dose from dose volume
  vtkNew<vtkImageAccumulate> doseStat;
  doseStat->SetInput(doseVolumeNode->GetImageData());
  doseStat->Update();
  double maxDose = doseStat->GetMax()[0];

  // Compute statistics
  vtkNew<vtkImageToImageStencil> stencil;
  stencil->SetInput(structureStenciledDoseVolumeNode->GetImageData());
  stencil->ThresholdByUpper(VTK_DOUBLE_MIN+1.0); // Do not include background values

  vtkNew<vtkImageAccumulate> structureStat;
  structureStat->SetInput(structureStenciledDoseVolumeNode->GetImageData());
  structureStat->SetStencil(stencil->GetOutput());
  structureStat->Update();

  if (structureStat->GetVoxelCount() < 1)
  {
    vtkWarningMacro("No voxels in the structure. DVH computation aborted.");
    return;
  }

  //TODO: use contour's structure name
  std::string modelNodeName(structureModelNode->GetName());
  std::string structureName = modelNodeName.substr(0, modelNodeName.find(" - "));

  // Create node and fill statistics
  vtkMRMLDoubleArrayNode* arrayNode = (vtkMRMLDoubleArrayNode*)( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLDoubleArrayNode") );
  std::string dvhArrayNodeName = structureName + SlicerRtCommon::DVH_ARRAY_NODE_NAME_POSTFIX;
  dvhArrayNodeName = this->GetMRMLScene()->GenerateUniqueName(dvhArrayNodeName);
  arrayNode->SetName(dvhArrayNodeName.c_str());
  //arrayNode->HideFromEditorsOff();

  arrayNode->SetAttribute(SlicerRtCommon::DVH_TYPE_ATTRIBUTE_NAME.c_str(), SlicerRtCommon::DVH_TYPE_ATTRIBUTE_VALUE.c_str());
  arrayNode->SetAttribute(SlicerRtCommon::DVH_DOSE_VOLUME_NODE_ID_ATTRIBUTE_NAME.c_str(), doseVolumeNode->GetID());
  arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_NAME_ATTRIBUTE_NAME.c_str(), structureName.c_str());
  arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_MODEL_NODE_ID_ATTRIBUTE_NAME.c_str(), structureModelNode->GetID());

  char attributeValue[64];
  double* color = structureModelNode->GetDisplayNode()->GetColor();
  sprintf(attributeValue, "#%02X%02X%02X", (int)(color[0]*255.0+0.5), (int)(color[1]*255.0+0.5), (int)(color[2]*255.0+0.5));
  arrayNode->SetAttribute(SlicerRtCommon::DVH_STRUCTURE_COLOR_ATTRIBUTE_NAME.c_str(), attributeValue);

  char attributeName[64];
  std::ostringstream metricList;

  sprintf(attributeName, "%s%s", SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  sprintf(attributeValue, "%g", structureStat->GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM);
  metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName, attributeName);
  sprintf(attributeValue, "%g", structureStat->GetMean()[0]);
  metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName, attributeName);
  sprintf(attributeValue, "%g", structureStat->GetMax()[0]);
  metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  AssembleDoseMetricAttributeName(SlicerRtCommon::DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), doseUnitName, attributeName);
  sprintf(attributeValue, "%g", structureStat->GetMin()[0]);
  metricList << attributeName << SlicerRtCommon::DVH_METRIC_LIST_SEPARATOR_CHARACTER;
  arrayNode->SetAttribute(attributeName, attributeValue);

  sprintf(attributeName, "%s%s", SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME.c_str());
  arrayNode->SetAttribute(attributeName, metricList.str().c_str());

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
      vtkWarningMacro("The dose volume contains negative dose values.");
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

  this->GetMRMLScene()->AddNode(arrayNode);
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
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
void vtkSlicerDoseVolumeHistogramModuleLogic
::ComputeVMetrics(vtkMRMLDoubleArrayNode* dvhArrayNode, std::vector<double> doseValues, std::vector<double> &vMetricsCc, std::vector<double> &vMetricsPercent)
{
  vMetricsCc.clear();
  vMetricsPercent.clear();

  // Get structure volume
  char attributeName[64];
  sprintf(attributeName, "%s%s", SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeName);
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
  char attributeName[64];
  sprintf(attributeName, "%s%s", SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str());
  const char* structureVolumeStr = dvhArrayNode->GetAttribute(attributeName);
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
bool vtkSlicerDoseVolumeHistogramModuleLogic
::DoseVolumeContainsDose()
{
  if (!this->GetMRMLScene() || !this->DoseVolumeHistogramNode)
  {
    return false;
  }

  vtkMRMLVolumeNode* doseVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->DoseVolumeHistogramNode->GetDoseVolumeNodeId()));

  const char* doseUnitName = doseVolumeNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str());

  if (doseUnitName != NULL)
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
::CollectMetricsForDvhNodes(std::vector<std::string>* dvhNodeIds, std::vector<std::string> &metricList)
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
  char metricListAttributeName[64];
  sprintf(metricListAttributeName, "%s%s", SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_LIST_ATTRIBUTE_NAME.c_str());
  std::set<std::string> metricSet;
  for (std::vector<std::string>::iterator it = dvhNodeIds->begin(); it != dvhNodeIds->end(); ++it)
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
  const char* metricSearchList[4] = { SlicerRtCommon::DVH_METRIC_TOTAL_VOLUME_CC_ATTRIBUTE_NAME.c_str(), SlicerRtCommon::DVH_METRIC_MEAN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(),
                                      SlicerRtCommon::DVH_METRIC_MIN_DOSE_ATTRIBUTE_NAME_PREFIX.c_str(), SlicerRtCommon::DVH_METRIC_MAX_DOSE_ATTRIBUTE_NAME_PREFIX.c_str() };
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
bool vtkSlicerDoseVolumeHistogramModuleLogic
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
bool vtkSlicerDoseVolumeHistogramModuleLogic
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
  outfile.open(fileName, std::ios_base::out | std::ios_base::trunc);

	if ( !outfile )
	{
    vtkErrorMacro("Error: Output file '" << fileName << "' cannot be opened!");
		return false;
	}

  std::vector<std::string>* dvhDoubleArrayNodeIds = this->DoseVolumeHistogramNode->GetDvhDoubleArrayNodeIds();

  // Collect metrics for all included nodes
  std::vector<std::string> metricList;
  CollectMetricsForDvhNodes(dvhDoubleArrayNodeIds, metricList);

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

//---------------------------------------------------------------------------
void vtkSlicerDoseVolumeHistogramModuleLogic
::AssembleDoseMetricAttributeName(const char* doseMetricAttributeNamePrefix, const char* doseUnitName, char* attributeName)
{
  sprintf(attributeName, "%s%s (%s)", SlicerRtCommon::DVH_METRIC_ATTRIBUTE_NAME_PREFIX.c_str(), doseMetricAttributeNamePrefix, doseUnitName);
}
