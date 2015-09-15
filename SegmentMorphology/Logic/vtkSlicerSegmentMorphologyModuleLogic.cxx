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

// Segmentation includes
#include "vtkMRMLSegmentationNode.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

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
  this->SegmentMorphologyNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerSegmentMorphologyModuleLogic::~vtkSlicerSegmentMorphologyModuleLogic()
{
  this->SetAndObserveSegmentMorphologyNode(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerSegmentMorphologyModuleLogic::SetAndObserveSegmentMorphologyNode(vtkMRMLSegmentMorphologyNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->SegmentMorphologyNode, node);
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

  if (!node || !this->SegmentMorphologyNode)
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

  if (!node || !this->SegmentMorphologyNode)
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
void vtkSlicerSegmentMorphologyModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLSegmentMorphologyNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLSegmentMorphologyNode");
  if (node)
  {
    paramNode = vtkMRMLSegmentMorphologyNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->SegmentMorphologyNode, paramNode);
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
std::string vtkSlicerSegmentMorphologyModuleLogic::ApplyMorphologyOperation()
{
  vtkMRMLSegmentationNode* inputSegmentationANode = this->SegmentMorphologyNode->GetSegmentationANode();
  vtkMRMLSegmentationNode* outputSegmentationNode = this->SegmentMorphologyNode->GetOutputSegmentationNode();
  int operation = this->SegmentMorphologyNode->GetOperation();

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
  vtkSmartPointer<vtkOrientedImageData> imageA;
  const char* segmentAID = this->SegmentMorphologyNode->GetSegmentAID();

  // Get segment A
  if (!inputSegmentationANode)
  {
    std::string errorMessage("Segmentation A is not selected");
    vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
    return errorMessage;
  }
  vtkSegmentation* segmentationA = inputSegmentationANode->GetSegmentation();
  vtkSegment* segmentA = segmentationA->GetSegment(segmentAID);
  if (!segmentA)
  {
    std::string errorMessage("Failed to get segment A: " + std::string(segmentAID));
    vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
    return errorMessage;
  }

  // Get binary labelmap from segment A
  if ( inputSegmentationANode->GetSegmentation()->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) )
  {
    // Make a copy in case the parent transform has to be hardened on it
    imageA = vtkSmartPointer<vtkOrientedImageData>::New();
    imageA->DeepCopy( vtkOrientedImageData::SafeDownCast(
      segmentA->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
  }
  else // Need to convert
  {
    // Temporarily duplicate selected segment to only convert them, not the whole segmentation (to save time)
    imageA = vtkSmartPointer<vtkOrientedImageData>::Take( vtkOrientedImageData::SafeDownCast(
      vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment( segmentationA, segmentAID,
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) ) );
    if (!imageA.GetPointer())
    {
      std::string errorMessage("Failed to convert segment A into binary labelmap\nPlease convert it in Segmentations module using Advanced conversion");
      vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
      return errorMessage;
    }
  }

  // Apply parent transformation nodes if necessary
  if (inputSegmentationANode->GetParentTransformNode())
  {
    if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(inputSegmentationANode, imageA))
    {
      std::string errorMessage("Failed to apply parent transformation to segmentation A!");
      vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
      return errorMessage;
    }
  }

  // If binary operation is selected, prepare segment B for processing
  vtkSmartPointer<vtkOrientedImageData> imageB;
  vtkMRMLSegmentationNode* inputSegmentationBNode = this->SegmentMorphologyNode->GetSegmentationBNode();
  const char* segmentBID = this->SegmentMorphologyNode->GetSegmentBID();
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
    vtkSegmentation* segmentationB = inputSegmentationBNode->GetSegmentation();
    vtkSegment* segmentB = segmentationB->GetSegment(segmentBID);
    if (!segmentB)
    {
      std::string errorMessage("Failed to get segment B: " + std::string(segmentBID));
      vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
      return errorMessage;
    }

    // Get binary labelmap from segment B
    if ( inputSegmentationBNode->GetSegmentation()->ContainsRepresentation(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) )
    {
      // Temporarily duplicate segment, as it may be resampled
      imageB = vtkSmartPointer<vtkOrientedImageData>::New();
      imageB->DeepCopy( vtkOrientedImageData::SafeDownCast(
        segmentB->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()) ) );
    }
    else // Need to convert
    {
      // Temporarily duplicate selected segment to only convert them, not the whole segmentation (to save time)
      imageB = vtkSmartPointer<vtkOrientedImageData>::Take( vtkOrientedImageData::SafeDownCast(
        vtkSlicerSegmentationsModuleLogic::CreateRepresentationForOneSegment( segmentationB, segmentBID,
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() ) ) );
      if (!imageB.GetPointer())
      {
        std::string errorMessage("Failed to convert segment B into binary labelmap\nPlease convert it in Segmentations module using Advanced conversion");
        vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
        return errorMessage;
      }
    }
    // Apply parent transformation nodes if necessary
    if (inputSegmentationBNode->GetParentTransformNode())
    {
      if (!vtkSlicerSegmentationsModuleLogic::ApplyParentTransformToOrientedImageData(inputSegmentationBNode, imageB))
      {
        std::string errorMessage("Failed to apply parent transformation to segmentation B!");
        vtkErrorMacro("ApplyMorphologyOperation: " << errorMessage);
        return errorMessage;
      }
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
#if (VTK_MAJOR_VERSION <= 5)
    padder->SetInput(imageA);
#else
    padder->SetInputData(imageA);
#endif
    padder->SetOutputWholeExtent(unionExtent);
    padder->Update();
    imageA->vtkImageData::DeepCopy(padder->GetOutput());
#if (VTK_MAJOR_VERSION <= 5)
    padder->SetInput(imageB);
#else
    padder->SetInputData(imageB);
#endif
    padder->Update();
    imageB->vtkImageData::DeepCopy(padder->GetOutput());
  }

  // Get kernel size
  double spacingA[3] = {0.0,0.0,0.0};
  imageA->GetSpacing(spacingA);

  double xSize = this->GetSegmentMorphologyNode()->GetXSize();
  double ySize = this->GetSegmentMorphologyNode()->GetYSize();
  double zSize = this->GetSegmentMorphologyNode()->GetZSize();

  int kernelSize[3] = {1,1,1};
  kernelSize[0] = (int)( 2.0*(xSize/spacingA[0] + 0.5) );
  kernelSize[1] = (int)( 2.0*(ySize/spacingA[1] + 0.5) );
  kernelSize[2] = (int)( 2.0*(zSize/spacingA[2] + 0.5) );

  // Apply operation on image data
  vtkSmartPointer<vtkImageAccumulate> histogram = vtkSmartPointer<vtkImageAccumulate>::New();
#if (VTK_MAJOR_VERSION <= 5)
  histogram->SetInput(imageA);
#else
  histogram->SetInputData(imageA);
#endif

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
#if (VTK_MAJOR_VERSION <= 5)
      padder->SetInput(imageA);
#else
      padder->SetInputData(imageA);
#endif
      int extent[6] = {0,-1,0,-1,0,-1};
#if (VTK_MAJOR_VERSION <= 5)
      imageA->GetWholeExtent(extent);
#else
      imageA->GetExtent(extent);
#endif
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
#if (VTK_MAJOR_VERSION <= 5)
      erodeFilter->SetInput(imageA);
#else
      erodeFilter->SetInputData(imageA);
#endif
      erodeFilter->SetKernelSize(kernelSize[0], kernelSize[1], kernelSize[2]);
      erodeFilter->Update();
      tempOutputImageData = erodeFilter->GetOutput();
      break;
    }

  // Union
  case vtkMRMLSegmentMorphologyNode::Union:
    {
      vtkSmartPointer<vtkImageLogic> logicFilter = vtkSmartPointer<vtkImageLogic>::New();
#if (VTK_MAJOR_VERSION <= 5)
      logicFilter->SetInput1(imageA);
      logicFilter->SetInput2(imageB);
#else
      logicFilter->SetInput1Data(imageA);
      logicFilter->SetInput2Data(imageB);
#endif
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
#if (VTK_MAJOR_VERSION <= 5)
      logicFilter->SetInput1(imageA);
      logicFilter->SetInput2(imageB);
#else
      logicFilter->SetInput1Data(imageA);
      logicFilter->SetInput2Data(imageB);
#endif
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
#if (VTK_MAJOR_VERSION <= 5)
      logicFilter->SetInput1(imageB);
#else
      logicFilter->SetInput1Data(imageB);
#endif
      logicFilter->SetOperationToNot();
      logicFilter->SetOutputTrueValue(valueMax);
      logicFilter->Update();

      vtkSmartPointer<vtkImageLogic> logicFilter2 = vtkSmartPointer<vtkImageLogic>::New();
#if (VTK_MAJOR_VERSION <= 5)
      logicFilter2->SetInput1(imageA);
      logicFilter2->SetInput2(logicFilter->GetOutput());
#else
      logicFilter2->SetInput1Data(imageA);
      logicFilter2->SetInput2Data(logicFilter->GetOutput());
#endif
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
  std::string newSegmentName = this->GenerateOutputSegmentName();
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
std::string vtkSlicerSegmentMorphologyModuleLogic::GenerateOutputSegmentName()
{
  vtkMRMLSegmentationNode* inputSegmentationANode = this->SegmentMorphologyNode->GetSegmentationANode();
  const char* segmentAID = this->SegmentMorphologyNode->GetSegmentAID();
  int operation = this->SegmentMorphologyNode->GetOperation();
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
  vtkMRMLSegmentationNode* inputSegmentationBNode = this->SegmentMorphologyNode->GetSegmentationBNode();
  const char* segmentBID = this->SegmentMorphologyNode->GetSegmentBID();
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

  double xSize = this->GetSegmentMorphologyNode()->GetXSize();
  double ySize = this->GetSegmentMorphologyNode()->GetYSize();
  double zSize = this->GetSegmentMorphologyNode()->GetZSize();

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
