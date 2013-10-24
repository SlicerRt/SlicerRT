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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ContourMorpholgyModule Logic includes
#include "vtkSlicerContourMorphologyModuleLogic.h"
#include "vtkMRMLContourMorphologyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkVolumesOrientedResampleUtility.h"

// MRML includes
#include <vtkMRMLContourNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeDisplayNode.h>
#include <vtkMRMLModelHierarchyNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLLabelMapVolumeDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageMarchingCubes.h>
#include <vtkImageChangeInformation.h>
#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkImageContinuousDilate3D.h>
#include <vtkImageContinuousErode3D.h>
#include <vtkImageLogic.h>
#include <vtkImageAccumulate.h>
#include <vtkImageReslice.h>
#include <vtkGeneralTransform.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContourMorphologyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerContourMorphologyModuleLogic::vtkSlicerContourMorphologyModuleLogic()
{
  this->ContourMorphologyNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerContourMorphologyModuleLogic::~vtkSlicerContourMorphologyModuleLogic()
{
  this->SetAndObserveContourMorphologyNode(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::SetAndObserveContourMorphologyNode(vtkMRMLContourMorphologyNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ContourMorphologyNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerContourMorphologyModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourMorphologyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene!");
    return;
  }

  if (!node || !this->ContourMorphologyNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLContourMorphologyNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene!");
    return;
  }

  if (!node || !this->ContourMorphologyNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLContourMorphologyNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLContourMorphologyNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLContourMorphologyNode");
  if (node)
  {
    paramNode = vtkMRMLContourMorphologyNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ContourMorphologyNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene or input node!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::SetContourARepresentationToLabelmap()
{
  if (!this->GetMRMLScene() || !this->ContourMorphologyNode)
  {
    vtkErrorMacro("SetContourARepresentationToLabelmap: Invalid MRML scene or parameter set node!");
    return -1;
  }

  vtkMRMLContourNode* inputContourANode = this->ContourMorphologyNode->GetContourANode();

  if (inputContourANode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ContourMorphologyNode->GetReferenceVolumeNode();
    if (!referenceVolumeNode)
    {
      vtkErrorMacro("SetContourARepresentationToLabelmap: Reference Volume is not initialized!")
      return -1;
    }
    inputContourANode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourMorphologyNode->GetReferenceVolumeNode()->GetID() );
  }
  
  inputContourANode->SetRasterizationOversamplingFactor(1.0);
  vtkMRMLScalarVolumeNode* inputContourALabelmapVolumeNode
    = inputContourANode->GetIndexedLabelmapVolumeNode();
  if (!inputContourALabelmapVolumeNode)
  {
    vtkErrorMacro("SetContourARepresentationToLabelmap: Failed to get indexed labelmap representation from selected contours");
    return -1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::SetContourBRepresentationToLabelmap()
{
  if (!this->GetMRMLScene() || !this->ContourMorphologyNode)
  {
    vtkErrorMacro("SetContourBRepresentationToLabelmap: Invalid MRML scene or parameter set node!");
    return -1;
  }

  vtkMRMLContourNode* inputContourBNode = this->ContourMorphologyNode->GetContourBNode();

  if (inputContourBNode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ContourMorphologyNode->GetReferenceVolumeNode();
    if (!referenceVolumeNode)
    {
      vtkErrorMacro("SetContourBRepresentationToLabelmap: Reference Volume is not initialized!")
      return -1;
    }
    inputContourBNode->SetAndObserveRasterizationReferenceVolumeNodeId(
      this->ContourMorphologyNode->GetReferenceVolumeNode()->GetID() );
  }

  inputContourBNode->SetRasterizationOversamplingFactor(1.0);
  vtkMRMLScalarVolumeNode* inputContourBLabelmapVolumeNode
    = inputContourBNode->GetIndexedLabelmapVolumeNode();
  if (!inputContourBLabelmapVolumeNode)
  {
    vtkErrorMacro("SetContourBRepresentationToLabelmap: Failed to get indexed labelmap representation from selected contours");
    return -1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::MorphContour()
{
  int dimensions[3] = {0, 0, 0};
  double spacingX, spacingY, spacingZ;

  vtkMRMLContourNode* inputContourANode = this->ContourMorphologyNode->GetContourANode();
  vtkMRMLScalarVolumeNode* referenceVolumeNode = this->ContourMorphologyNode->GetReferenceVolumeNode();
  vtkMRMLContourNode* outputContourNode = this->ContourMorphologyNode->GetOutputContourNode();
  // Make sure inputs are initialized
  if (!this->GetMRMLScene() || !inputContourANode || !referenceVolumeNode || !outputContourNode)
  {
    vtkErrorMacro("MorphContour: Inputs are not specified!")
    return -1;
  }

  if (this->SetContourARepresentationToLabelmap() != 0)
  {
    vtkErrorMacro("MorphContour: Failed to set contour A representation to labelmap!")
    return -1;
  }

  vtkMRMLScalarVolumeNode* inputLabelmapANode = inputContourANode->GetIndexedLabelmapVolumeNode();
  inputLabelmapANode->GetSpacing(spacingX, spacingY, spacingZ);

  vtkSmartPointer<vtkMRMLScalarVolumeNode> tempVolumeNodeA= vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode(inputLabelmapANode, referenceVolumeNode, tempVolumeNodeA);
  vtkImageData* tempImageA = tempVolumeNodeA->GetImageData();

  referenceVolumeNode->GetImageData()->GetDimensions(dimensions);

  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(tempImageA);
  histogram->Update();
  double valueMax = histogram->GetMax()[0];

  vtkMRMLScalarVolumeNode* inputLabelmapBNode = NULL;
  vtkImageData* tempImageB = NULL;
  vtkSmartPointer<vtkMRMLScalarVolumeNode> tempVolumeNodeB= vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  vtkMRMLContourNode* inputContourBNode = this->ContourMorphologyNode->GetContourBNode();

  vtkMRMLContourMorphologyNode::ContourMorphologyOperationType operation = this->ContourMorphologyNode->GetOperation();
  if (operation == vtkMRMLContourMorphologyNode::Union || operation == vtkMRMLContourMorphologyNode::Intersect || operation == vtkMRMLContourMorphologyNode::Subtract) 
  {
    if (!inputContourBNode)
    {
      vtkErrorMacro("MorphContour: Inputs are not initialized!")
      return -1;
    }
    if (this->SetContourBRepresentationToLabelmap() != 0)
    {
     vtkErrorMacro("MorphContour: Failed to set contour B representation to labelmap!")
     return -1;
    }
    inputLabelmapBNode = inputContourBNode->GetIndexedLabelmapVolumeNode();

    vtkVolumesOrientedResampleUtility::ResampleInputVolumeNodeToReferenceVolumeNode(inputLabelmapBNode, referenceVolumeNode, tempVolumeNodeB);
    tempImageB = tempVolumeNodeB->GetImageData();
  }

  vtkSmartPointer<vtkImageData> tempImageOutput = NULL;
  vtkSmartPointer<vtkImageContinuousDilate3D> dilateFilter = vtkSmartPointer<vtkImageContinuousDilate3D>::New();
  vtkSmartPointer<vtkImageContinuousErode3D> erodeFilter = vtkSmartPointer<vtkImageContinuousErode3D>::New();
  vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
  vtkSmartPointer<vtkImageLogic> logicFilter2 = vtkSmartPointer<vtkImageLogic>::New();

  int kernelSize[3] = {1,1,1};
  kernelSize[0] = (int)( 2*(this->GetContourMorphologyNode()->GetXSize()/spacingX + 0.5) );
  kernelSize[1] = (int)( 2*(this->GetContourMorphologyNode()->GetYSize()/spacingY + 0.5) );
  kernelSize[2] = (int)( 2*(this->GetContourMorphologyNode()->GetZSize()/spacingZ + 0.5) );
  switch (operation) 
  {
    case vtkMRMLContourMorphologyNode::Expand:
      dilateFilter->SetInput(tempImageA);
      dilateFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      dilateFilter->Update();
      tempImageOutput = dilateFilter->GetOutput();
      break;
    case vtkMRMLContourMorphologyNode::Shrink:
      erodeFilter->SetInput(tempImageA);
      erodeFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      erodeFilter->Update();
      tempImageOutput = erodeFilter->GetOutput();
      break;
    case vtkMRMLContourMorphologyNode::Union:
      logicFilter->SetInput1(tempImageA);
      logicFilter->SetInput2(tempImageB);
      logicFilter->SetOperationToOr();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();
      tempImageOutput = logicFilter->GetOutput();
      break;
    case vtkMRMLContourMorphologyNode::Intersect:
      logicFilter->SetInput1(tempImageA);
      logicFilter->SetInput2(tempImageB);
      logicFilter->SetOperationToAnd();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();
      tempImageOutput = logicFilter->GetOutput();
      break;
    case vtkMRMLContourMorphologyNode::Subtract:
      logicFilter->SetInput1(tempImageB);
      logicFilter->SetOperationToNot();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();

      logicFilter2->SetInput1(tempImageA);
      logicFilter2->SetInput2(logicFilter->GetOutput());
      logicFilter2->SetOperationToAnd();
      logicFilter2->SetOutputTrueValue(valueMax);
      logicFilter2->Update();
      tempImageOutput = logicFilter2->GetOutput();
      break;
    default:
      vtkErrorMacro("MorphContour: Invalid operation!")
      break;
  }

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 
  
  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputIndexedLabelmapVolumeNode = NULL;
  std::string outputIndexedLabelmapVolumeNodeName;
  if (outputContourNode->GetIndexedLabelmapVolumeNodeId() == NULL)
  {
    outputIndexedLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    outputIndexedLabelmapVolumeNodeName = std::string(outputContourNode->GetName()) + SlicerRtCommon::CONTOUR_INDEXED_LABELMAP_NODE_NAME_POSTFIX;
    outputIndexedLabelmapVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(outputIndexedLabelmapVolumeNodeName);
    outputIndexedLabelmapVolumeNode->SetName( outputIndexedLabelmapVolumeNodeName.c_str() );
    this->GetMRMLScene()->AddNode(outputIndexedLabelmapVolumeNode);
  }
  else
  {
    outputIndexedLabelmapVolumeNode = outputContourNode->GetIndexedLabelmapVolumeNode();
  }
  outputIndexedLabelmapVolumeNode->CopyOrientation( referenceVolumeNode );
  outputIndexedLabelmapVolumeNode->SetAndObserveTransformNodeID( outputIndexedLabelmapVolumeNode->GetTransformNodeID() );
  outputIndexedLabelmapVolumeNode->SetAndObserveImageData( tempImageOutput );
  outputIndexedLabelmapVolumeNode->LabelMapOn();
  outputIndexedLabelmapVolumeNode->HideFromEditorsOff();

  // Create display node
  vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode> labelmapDisplayNode = NULL;
  labelmapDisplayNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(outputIndexedLabelmapVolumeNode->GetDisplayNode());
  if (labelmapDisplayNode == NULL)
  {
    labelmapDisplayNode = vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode>::New();
    labelmapDisplayNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(labelmapDisplayNode));
    outputIndexedLabelmapVolumeNodeName.append("Display");
    labelmapDisplayNode->SetName(outputIndexedLabelmapVolumeNodeName.c_str());
  }

  if (this->GetMRMLScene()->GetNodeByID("vtkMRMLColorTableNodeLabels") != NULL)
  {
    labelmapDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeLabels");
  }
  labelmapDisplayNode->SetVisibility(1);
  
  outputIndexedLabelmapVolumeNode->SetAndObserveDisplayNodeID( labelmapDisplayNode->GetID() );

  outputContourNode->SetAndObserveIndexedLabelmapVolumeNodeId(outputIndexedLabelmapVolumeNode->GetID());

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  return 0;
}
