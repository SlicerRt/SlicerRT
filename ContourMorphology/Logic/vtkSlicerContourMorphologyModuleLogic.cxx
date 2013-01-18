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

// STD includes
#include <cassert>

#define THRESHOLD 0.001

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
  vtkSetAndObserveMRMLNodeMacro(this->ContourMorphologyNode, NULL);
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
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourMorphologyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->ContourMorphologyNode)
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLContourMorphologyNode"))
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSlicerContourMorphologyModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->ContourMorphologyNode)
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode") || node->IsA("vtkMRMLContourMorphologyNode"))
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
  this->Modified();
}

//---------------------------------------------------------------------------
bool vtkSlicerContourMorphologyModuleLogic::ContourContainsLabelmap()
{
  if (!this->GetMRMLScene() || !this->ContourMorphologyNode)
  {
    return false;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourMorphologyNode->GetReferenceContourNodeID()));

  if (contourNode->GetActiveRepresentationType() == vtkMRMLContourNode::IndexedLabelmap)
  {
    return true;
  }

  return false;
}

//---------------------------------------------------------------------------
int vtkSlicerContourMorphologyModuleLogic::MorphContour()
{
  double originX, originY, originZ;
  double spacingX, spacingY, spacingZ;
  int dimensions[3] = {0, 0, 0};
  double originX2, originY2, originZ2;
  double spacingX2, spacingY2, spacingZ2;
  int dimensions2[3] = {0, 0, 0};

  vtkMRMLContourNode* referenceContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourMorphologyNode->GetReferenceContourNodeID()));
  vtkMRMLContourNode* inputContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourMorphologyNode->GetInputContourNodeID()));
  vtkMRMLContourNode* outputContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourMorphologyNode->GetOutputContourNodeID()));
  // Make sure inputs are initialized
  if (!this->GetMRMLScene() || !referenceContourNode || !inputContourNode || !outputContourNode)
  {
    vtkErrorMacro("ContourMorphology: Inputs are not initialized!")
    return -1;
  }

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  if (referenceContourNode->GetActiveRepresentationType() != vtkMRMLContourNode::IndexedLabelmap)
  {
    vtkErrorMacro("Cannot Morph the contour when its representation is not labelmap!");
    return -1;
  }
  vtkMRMLScalarVolumeNode* referenceLabelmapNode = referenceContourNode->GetIndexedLabelmapVolumeNode();
  referenceLabelmapNode->GetOrigin(originX, originY, originZ); 
  referenceLabelmapNode->GetSpacing(spacingX, spacingY, spacingZ);
  referenceLabelmapNode->GetImageData()->GetDimensions(dimensions);
  vtkSmartPointer<vtkImageData> tempImage2 = referenceLabelmapNode->GetImageData();

  if (inputContourNode->GetActiveRepresentationType() != vtkMRMLContourNode::IndexedLabelmap)
  {
    vtkErrorMacro("Cannot Morph the contour when its representation is not labelmap!");
    return -1;
  }
  vtkMRMLScalarVolumeNode* inputLabelmapNode = inputContourNode->GetIndexedLabelmapVolumeNode();
  inputLabelmapNode->GetOrigin(originX2, originY2, originZ2); 
  inputLabelmapNode->GetSpacing(spacingX2, spacingY2, spacingZ2);
  inputLabelmapNode->GetImageData()->GetDimensions(dimensions2);

  vtkSmartPointer<vtkGeneralTransform> referenceLabelmapNodeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  referenceLabelmapNodeToWorldTransform->Identity();
  vtkSmartPointer<vtkMRMLTransformNode> referenceLabelmapNodeTransformNode = referenceLabelmapNode->GetParentTransformNode();
  if (referenceLabelmapNodeTransformNode!=NULL)
  {
    referenceLabelmapNodeTransformNode->GetTransformToWorld(referenceLabelmapNodeToWorldTransform);    
    referenceLabelmapNodeToWorldTransform->Inverse();
  }

  vtkSmartPointer<vtkMatrix4x4> inputIJK2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  inputLabelmapNode->GetIJKToRASMatrix(inputIJK2RASMatrix);
  vtkSmartPointer<vtkMatrix4x4> referenceRAS2IJKMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  referenceLabelmapNode->GetRASToIJKMatrix(referenceRAS2IJKMatrix);

  vtkSmartPointer<vtkTransform> outputResliceTransform = vtkSmartPointer<vtkTransform>::New();
  outputResliceTransform->Identity();
  outputResliceTransform->PostMultiply();
  outputResliceTransform->SetMatrix(inputIJK2RASMatrix);

  vtkSmartPointer<vtkMRMLTransformNode> inputLabelmapNodeTransformNode = inputLabelmapNode->GetParentTransformNode();
  vtkSmartPointer<vtkMatrix4x4> inputRAS2RASMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (inputLabelmapNodeTransformNode!=NULL)
  {
    inputLabelmapNodeTransformNode->GetMatrixTransformToWorld(inputRAS2RASMatrix);  
    outputResliceTransform->Concatenate(inputRAS2RASMatrix);
  }
  outputResliceTransform->Concatenate(referenceRAS2IJKMatrix);
  outputResliceTransform->Inverse();

  vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInput(inputLabelmapNode->GetImageData());
  reslice->SetOutputOrigin(0, 0, 0);
  reslice->SetOutputSpacing(1, 1, 1);
  reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  reslice->SetResliceTransform(outputResliceTransform);
  reslice->Update();

  vtkSmartPointer<vtkImageData> tempImage = NULL;
  tempImage = reslice->GetOutput();

  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInput(tempImage);
  histogram->Update();
  double valueMax = histogram->GetMax()[0];

  int op = this->ContourMorphologyNode->GetOperation();

  vtkSmartPointer<vtkImageData> tempImageData = NULL;
  vtkSmartPointer<vtkImageContinuousDilate3D> dilateFilter = vtkSmartPointer<vtkImageContinuousDilate3D>::New();
  vtkSmartPointer<vtkImageContinuousErode3D> erodeFilter = vtkSmartPointer<vtkImageContinuousErode3D>::New();
  vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
  vtkSmartPointer<vtkImageLogic> logicFilter2 = vtkSmartPointer<vtkImageLogic>::New();

  // temp work around for calculating the kernel size
  
  //double spacing[3] = {0,0,0};
  //volumeNode->GetSpacing(spacing);
  int kernelSize[3] = {1,1,1};
  kernelSize[0] = (int)(this->GetContourMorphologyNode()->GetXSize()/spacingX2);
  kernelSize[1] = (int)(this->GetContourMorphologyNode()->GetYSize()/spacingY2);
  kernelSize[2] = (int)(this->GetContourMorphologyNode()->GetZSize()/spacingZ2);
  switch (op) 
  {
    case SLICERRT_EXPAND:
      dilateFilter->SetInput(tempImage);
      dilateFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      dilateFilter->Update();
      tempImageData = dilateFilter->GetOutput();
      break;
    case SLICERRT_SHRINK:
      erodeFilter->SetInput(tempImage);
      erodeFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      erodeFilter->Update();
      tempImageData = erodeFilter->GetOutput();
      break;
    case SLICERRT_UNION:
      logicFilter->SetInput1(tempImage);
      logicFilter->SetInput2(tempImage2);
      logicFilter->SetOperationToOr();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();
      tempImageData = logicFilter->GetOutput();
      break;
    case SLICERRT_INTERSECT:
      logicFilter->SetInput1(tempImage);
      logicFilter->SetInput2(tempImage2);
      logicFilter->SetOperationToAnd();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();
      tempImageData = logicFilter->GetOutput();
      break;
    case SLICERRT_SUBTRACT:
      logicFilter->SetInput1(tempImage);
      logicFilter->SetOperationToNot();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();

      logicFilter2->SetInput1(logicFilter->GetOutput());
      logicFilter2->SetInput2(tempImage2);
      logicFilter2->SetOperationToAnd();
      logicFilter2->SetOutputTrueValue(valueMax);
      tempImageData = logicFilter2->GetOutput();
      break;
  }

  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputIndexedLabelmapVolumeNode = NULL;
  outputIndexedLabelmapVolumeNode = outputContourNode->GetIndexedLabelmapVolumeNode();
  std::string outputIndexedLabelmapVolumeNodeName;
  if (outputIndexedLabelmapVolumeNode == NULL)
  {
    outputIndexedLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
    outputIndexedLabelmapVolumeNodeName = std::string(outputContourNode->GetName()) + std::string(" - Labelmap");
    outputIndexedLabelmapVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(outputIndexedLabelmapVolumeNodeName);
    outputIndexedLabelmapVolumeNode->SetName( outputIndexedLabelmapVolumeNodeName.c_str() );
    this->GetMRMLScene()->AddNode(outputIndexedLabelmapVolumeNode);
  }
  outputIndexedLabelmapVolumeNode->CopyOrientation( referenceLabelmapNode );
  outputIndexedLabelmapVolumeNode->SetAndObserveTransformNodeID( outputIndexedLabelmapVolumeNode->GetTransformNodeID() );
  outputIndexedLabelmapVolumeNode->SetAndObserveImageData( tempImageData );
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
  outputContourNode->SetActiveRepresentationByNode(outputIndexedLabelmapVolumeNode);

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  return 0;
}
