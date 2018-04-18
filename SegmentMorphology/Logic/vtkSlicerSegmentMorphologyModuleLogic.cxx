/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

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

// SegmentMorphology Logic includes
#include "vtkSlicerSegmentMorphologyModuleLogic.h"
#include "vtkMRMLSegmentMorphologyNode.h"

// SlicerRT includes
#include "vtkSlicerRtCommon.h"

// Segmentation includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkImageAccumulate.h>
#include <vtkImageContinuousDilate3D.h>
#include <vtkImageContinuousErode3D.h>
#include <vtkImageLogic.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkImageConstantPad.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSegmentMorphologyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerSegmentMorphologyModuleLogic::vtkSlicerSegmentMorphologyModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSegmentMorphologyModuleLogic::~vtkSlicerSegmentMorphologyModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
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
void vtkSlicerSegmentMorphologyModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLSegmentMorphologyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene!");
    return;
  }

  if (!node)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLSegmentationNode") || node->IsA("vtkMRMLSegmentMorphologyNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene!");
    return;
  }

  if (!node)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLSegmentationNode") || node->IsA("vtkMRMLSegmentMorphologyNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene or input node!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSlicerSegmentMorphologyModuleLogic::ApplyMorphologyOperation(vtkMRMLSegmentMorphologyNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("ApplyMorphologyOperation: Invalid parameter node!");
  }

  vtkMRMLSegmentationNode* inputSegmentationANode = parameterNode->GetSegmentationANode();
  vtkMRMLSegmentationNode* outputSegmentationNode = parameterNode->GetOutputSegmentationNode();
  int operation = parameterNode->GetOperation();

  // Make sure inputs are initialized
  if (!this->GetMRMLScene() || !inputSegmentationANode)
  {
    std::string errorMessage("Segmentation A is not selected");
    vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
    return errorMessage;
  }
  if (!outputSegmentationNode)
  {
    std::string errorMessage("Output segmentation is not selected");
    vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
    return errorMessage;
  }

  // Prepare segment A for processing
  vtkSmartPointer<vtkOrientedImageData> imageA = vtkSmartPointer<vtkOrientedImageData>::New();
  const char* segmentAID = parameterNode->GetSegmentAID();
  if ( !vtkSlicerSegmentationsModuleLogic::GetSegmentBinaryLabelmapRepresentation(
    inputSegmentationANode, segmentAID, imageA ) )
  {
    std::string errorMessage("Failed to get binary labelmap from segment A: " + std::string(segmentAID));
    vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
    return errorMessage;
  }

  // If binary operation is selected, prepare segment B for processing
  vtkSmartPointer<vtkOrientedImageData> imageB = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkMRMLSegmentationNode* inputSegmentationBNode = parameterNode->GetSegmentationBNode();
  const char* segmentBID = parameterNode->GetSegmentBID();
  if ( operation == vtkMRMLSegmentMorphologyNode::Union
    || operation == vtkMRMLSegmentMorphologyNode::Intersect
    || operation == vtkMRMLSegmentMorphologyNode::Subtract )
  {
    // Get segment B
    if (!inputSegmentationBNode)
    {
      std::string errorMessage("Segmentation B is not selected");
      vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
      return errorMessage;
    }
    if ( !vtkSlicerSegmentationsModuleLogic::GetSegmentBinaryLabelmapRepresentation(
      inputSegmentationBNode, segmentBID, imageB ) )
    {
      std::string errorMessage("Failed to get binary labelmap from segment B: " + std::string(segmentAID));
      vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
      return errorMessage;
    }

    // Resample image B if has a different geometry than image A
    if (!vtkOrientedImageDataResample::DoGeometriesMatch(imageA, imageB))
    {
      vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(imageB, imageA, imageB, true);
    }

    // Make sure the two volumes have the same extent (otherwise logic filter crashes)
    int aExtent[6] = {0,-1,0,-1,0,-1};
    imageA->GetExtent(aExtent);
    int bExtent[6] = {0,-1,0,-1,0,-1};
    imageB->GetExtent(bExtent);
    int unionExtent[6] = { std::min(aExtent[0],bExtent[0]), std::max(aExtent[1],bExtent[1]), std::min(aExtent[2],bExtent[2]), std::max(aExtent[3],bExtent[3]), std::min(aExtent[4],bExtent[4]), std::max(aExtent[5],bExtent[5]) };

    vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
    padder->SetInputData(imageA);
    padder->SetOutputWholeExtent(unionExtent);
    padder->Update();
    imageA->vtkImageData::DeepCopy(padder->GetOutput());
    padder->SetInputData(imageB);
    padder->Update();
    imageB->vtkImageData::DeepCopy(padder->GetOutput());
  }

  // Get kernel size
  double spacingA[3] = {0.0,0.0,0.0};
  imageA->GetSpacing(spacingA);

  double xSize = parameterNode->GetXSize();
  double ySize = parameterNode->GetYSize();
  double zSize = parameterNode->GetZSize();

  int kernelSize[3] = {1,1,1};
  kernelSize[0] = (int)( 2.0*(xSize/spacingA[0] + 0.5) );
  kernelSize[1] = (int)( 2.0*(ySize/spacingA[1] + 0.5) );
  kernelSize[2] = (int)( 2.0*(zSize/spacingA[2] + 0.5) );

  // Apply operation on image data
  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
  histogram->SetInputData(imageA);
  histogram->Update();
  double valueMax = histogram->GetMax()[0];

  vtkSmartPointer<vtkImageData> tempOutputImageData = NULL;
  switch (operation) 
  {
  // Expand
  case vtkMRMLSegmentMorphologyNode::Expand:
    {
    // Pad image by expansion extent (extents are fitted to the structure, dilate will reach the edge of the image)
    vtkSmartPointer<vtkImageConstantPad> padder = vtkSmartPointer<vtkImageConstantPad>::New();
    padder->SetInputData(imageA);
    int extent[6] = {0,-1,0,-1,0,-1};
    imageA->GetExtent(extent);

    // Now set the output extent to the new size
    int expansionExtent[3] = { int(xSize/spacingA[0] + 1.0), int(ySize/spacingA[1] + 1.0), int(zSize/spacingA[2] + 1.0) }; // Rounding up
    padder->SetOutputWholeExtent(extent[0]-expansionExtent[0], extent[1]+expansionExtent[0], extent[2]-expansionExtent[1], extent[3]+expansionExtent[1], extent[4]-expansionExtent[2], extent[5]+expansionExtent[2]);
    padder->Update();

    vtkSmartPointer<vtkImageContinuousDilate3D> dilateFilter = vtkSmartPointer<vtkImageContinuousDilate3D>::New();
    dilateFilter->SetInputConnection(padder->GetOutputPort());
    dilateFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
    dilateFilter->Update();
    tempOutputImageData = dilateFilter->GetOutput();
    break;
    }

  // Shrink
  case vtkMRMLSegmentMorphologyNode::Shrink:
    {
    vtkSmartPointer<vtkImageContinuousErode3D> erodeFilter = vtkSmartPointer<vtkImageContinuousErode3D>::New();
    erodeFilter->SetInputData(imageA);
    erodeFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
    erodeFilter->Update();
    tempOutputImageData = erodeFilter->GetOutput();
    break;
    }

  // Union
  case vtkMRMLSegmentMorphologyNode::Union:
    {
    vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
    logicFilter->SetInput1Data(imageA);
    logicFilter->SetInput2Data(imageB);
    logicFilter->SetOperationToOr();
    logicFilter->SetOutputTrueValue(valueMax);
    logicFilter->Update();
    tempOutputImageData = logicFilter->GetOutput();
    break;
    }

  // Intersect
  case vtkMRMLSegmentMorphologyNode::Intersect:
    {
    vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
    logicFilter->SetInput1Data(imageA);
    logicFilter->SetInput2Data(imageB);
    logicFilter->SetOperationToAnd();
    logicFilter->SetOutputTrueValue(valueMax);
    logicFilter->Update();
    tempOutputImageData = logicFilter->GetOutput();
    break;
    }

  // Subtract
  case vtkMRMLSegmentMorphologyNode::Subtract:
    {
    vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
    logicFilter->SetInput1Data(imageB);
    logicFilter->SetOperationToNot();
    logicFilter->SetOutputTrueValue(valueMax);
    logicFilter->Update();

    vtkSmartPointer<vtkImageLogic> logicFilter2 = vtkSmartPointer<vtkImageLogic>::New();
    logicFilter2->SetInput1Data(imageA);
    logicFilter2->SetInput2Data(logicFilter->GetOutput());
    logicFilter2->SetOperationToAnd();
    logicFilter2->SetOutputTrueValue(valueMax);
    logicFilter2->Update();
    tempOutputImageData = logicFilter2->GetOutput();
    break;
    }
  default:
    vtkErrorMacro("ApplyMorphologyOperation: Invalid operation!")
    break;
  }

  // Clear output segmentation and make sure master is binary labelmap
  std::vector<std::string> segmentIds;
  outputSegmentationNode->GetSegmentation()->GetSegmentIDs(segmentIds);
  for (std::vector<std::string>::iterator segmentIt = segmentIds.begin(); segmentIt != segmentIds.end(); ++segmentIt)
  {
    outputSegmentationNode->GetSegmentation()->RemoveSegment(*segmentIt);
  }
  outputSegmentationNode->GetSegmentation()->SetMasterRepresentationName(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );

  // Create segment for output image data
  vtkSmartPointer<vtkOrientedImageData> outputImage = vtkSmartPointer<vtkOrientedImageData>::New();
  outputImage->DeepCopy(tempOutputImageData);
  vtkSmartPointer<vtkMatrix4x4> imageAToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  imageA->GetImageToWorldMatrix(imageAToWorldMatrix);
  outputImage->SetGeometryFromImageToWorldMatrix(imageAToWorldMatrix);

  vtkSmartPointer<vtkSegment> newSegment = vtkSmartPointer<vtkSegment>::New();
  std::string newSegmentName = this->GenerateOutputSegmentName(parameterNode);
  newSegment->SetName(newSegmentName.c_str());
  newSegment->AddRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(), outputImage );

  outputSegmentationNode->GetSegmentation()->AddSegment(newSegment);

  // Set same name to the output segmentation too
  outputSegmentationNode->SetName(newSegmentName.c_str());

  // Clear output parent transform, the image data will be in the right coordinate frame
  outputSegmentationNode->SetAndObserveTransformNodeID(NULL);

  return "";
}

//---------------------------------------------------------------------------
std::string vtkSlicerSegmentMorphologyModuleLogic::GenerateOutputSegmentName(vtkMRMLSegmentMorphologyNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("GenerateOutputSegmentName: Invalid parameter node!");
  }

  vtkMRMLSegmentationNode* inputSegmentationANode = parameterNode->GetSegmentationANode();
  const char* segmentAID = parameterNode->GetSegmentAID();
  int operation = parameterNode->GetOperation();
  if (!this->GetMRMLScene() || !inputSegmentationANode || !segmentAID)
  {
    return "";
  }

  // Get segment A name
  vtkSegmentation* segmentationA = inputSegmentationANode->GetSegmentation();
  vtkSegment* segmentA = segmentationA->GetSegment(segmentAID);
  if (!segmentA)
  {
    return "";
  }
  std::string segmentAName(segmentA->GetName());

  // If binary operation is selected, get segment B name
  std::string segmentBName("");
  vtkMRMLSegmentationNode* inputSegmentationBNode = parameterNode->GetSegmentationBNode();
  const char* segmentBID = parameterNode->GetSegmentBID();
  if ( operation == vtkMRMLSegmentMorphologyNode::Union
    || operation == vtkMRMLSegmentMorphologyNode::Intersect
    || operation == vtkMRMLSegmentMorphologyNode::Subtract )
  {
    if (!inputSegmentationBNode || !segmentBID)
    {
      return "";
    }

    vtkSegmentation* segmentationB = inputSegmentationBNode->GetSegmentation();
    vtkSegment* segmentB = segmentationB->GetSegment(segmentBID);
    if (!segmentB)
    {
      return "";
    }
    segmentBName = std::string(segmentB->GetName());
  }

  double xSize = parameterNode->GetXSize();
  double ySize = parameterNode->GetYSize();
  double zSize = parameterNode->GetZSize();

  std::string newSegmentName("");
  UNUSED_VARIABLE(newSegmentName); // Although it is used later, a warning is logged so needs to be suppressed
  switch(operation)
  {
  case vtkMRMLSegmentMorphologyNode::Expand:
    {
      std::stringstream ss;
      ss << "Expanded_" << xSize << "_" << ySize << "_" << zSize << "_" << segmentAName;
      newSegmentName = ss.str();
      break;
    }
  case vtkMRMLSegmentMorphologyNode::Shrink:
    {
      std::stringstream ss;
      ss << "Shrunk_" << xSize << "_" << ySize << "_" << zSize << "_" << segmentAName;
      newSegmentName = ss.str();
      break;
    }
  case vtkMRMLSegmentMorphologyNode::Intersect:
    {
      std::stringstream ss;
      ss << segmentAName << std::string("_Intersect_") << segmentBName;
      newSegmentName = ss.str();
      break;
    }
  case vtkMRMLSegmentMorphologyNode::Union:
    {
      std::stringstream ss;
      ss << segmentAName << std::string("_Union_") << segmentBName;
      newSegmentName = ss.str();
      break;
    }
  case vtkMRMLSegmentMorphologyNode::Subtract:
    {
      std::stringstream ss;
      ss << segmentAName << std::string("_Subtract_") << segmentBName;
      newSegmentName = ss.str();
      break;
    }
  default:
    {
      vtkErrorMacro("Unknown operation detected in vtkSlicerSegmentMorphologyModuleLogic::ApplyMorphologyOperation().");
      return "";
    }
  }

  return newSegmentName;
}
