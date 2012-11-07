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

// STD includes
#include <cassert>

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
int vtkSlicerContourMorphologyModuleLogic::MorphContour()
{
  // Make sure inputs are initialized
  if (!this->GetMRMLScene())
  {
    return 0;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourMorphologyNode->GetContourNodeID()));
  vtkMRMLContourNode* outputContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ContourMorphologyNode->GetOutputContourNodeID()));

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  vtkMRMLScalarVolumeNode* volumeNode = contourNode->GetIndexedLabelmapVolumeNode();
  if (!volumeNode)
  {
    vtkErrorMacro("Failed to get indexed labelmap node for contour!");
    return 0;
  }

  // to do ....
  vtkSmartPointer<vtkMRMLScalarVolumeNode> outputIndexedLabelmapVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  outputIndexedLabelmapVolumeNode->CopyOrientation( volumeNode );

  vtkSmartPointer<vtkImageData> tempImageData = NULL;
  if (this->GetContourMorphologyNode()->GetExpansion())
  {
    vtkSmartPointer<vtkImageContinuousDilate3D> dilateFilter = vtkSmartPointer<vtkImageContinuousDilate3D>::New();
    dilateFilter->SetInput(volumeNode->GetImageData());
    dilateFilter->SetKernelSize(this->GetContourMorphologyNode()->GetXSize(),
                                this->GetContourMorphologyNode()->GetYSize(),
                                this->GetContourMorphologyNode()->GetZSize());
    dilateFilter->Update();
    tempImageData = dilateFilter->GetOutput();
  }
  else
  {
    vtkSmartPointer<vtkImageContinuousErode3D> erodeFilter = vtkSmartPointer<vtkImageContinuousErode3D>::New();
    erodeFilter->SetInput(volumeNode->GetImageData());
    erodeFilter->SetKernelSize(this->GetContourMorphologyNode()->GetXSize(),
                               this->GetContourMorphologyNode()->GetYSize(),
                               this->GetContourMorphologyNode()->GetZSize());
    erodeFilter->Update();
    tempImageData = erodeFilter->GetOutput();
  }

  std::string outputIndexedLabelmapVolumeNodeName = std::string(outputContourNode->GetName()) + std::string(" - Labelmap");
  outputIndexedLabelmapVolumeNodeName = this->GetMRMLScene()->GenerateUniqueName(outputIndexedLabelmapVolumeNodeName);
  this->GetMRMLScene()->AddNode(outputIndexedLabelmapVolumeNode);

  outputIndexedLabelmapVolumeNode->SetAndObserveTransformNodeID( outputIndexedLabelmapVolumeNode->GetTransformNodeID() );
  outputIndexedLabelmapVolumeNode->SetName( outputIndexedLabelmapVolumeNodeName.c_str() );
  outputIndexedLabelmapVolumeNode->SetAndObserveImageData( tempImageData );
  outputIndexedLabelmapVolumeNode->LabelMapOn();
  outputIndexedLabelmapVolumeNode->HideFromEditorsOff();

  // Create display node
  vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode> labelmapDisplayNode = vtkSmartPointer<vtkMRMLLabelMapVolumeDisplayNode>::New();
  labelmapDisplayNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(labelmapDisplayNode));
  outputIndexedLabelmapVolumeNodeName.append("Display");
  labelmapDisplayNode->SetName(outputIndexedLabelmapVolumeNodeName.c_str());

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