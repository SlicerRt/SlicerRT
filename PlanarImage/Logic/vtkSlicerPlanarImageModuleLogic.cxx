/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

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

// PlanarImage includes
#include "vtkSlicerPlanarImageModuleLogic.h"
#include "vtkMRMLPlanarImageNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>
#include <vtkTransform.h>
#include <vtkPlaneSource.h>
#include <vtkImageMapToWindowLevelColors.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlanarImageModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerPlanarImageModuleLogic::vtkSlicerPlanarImageModuleLogic()
{
  this->TextureWindowLevelMappers.clear();
}

//----------------------------------------------------------------------------
vtkSlicerPlanarImageModuleLogic::~vtkSlicerPlanarImageModuleLogic()
{
  std::map<vtkMRMLScalarVolumeNode*, vtkImageMapToWindowLevelColors*>::iterator mapperIt;
  for (mapperIt=this->TextureWindowLevelMappers.begin(); mapperIt!=this->TextureWindowLevelMappers.end(); ++mapperIt)
  {
    mapperIt->second->Delete();
  }
  this->TextureWindowLevelMappers.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::NodeAboutToBeRemovedEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLPlanarImageNode>::New());
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessMRMLNodesEvents(caller, event, callData);

  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("ProcessMRMLNodesEvents: Invalid MRML scene!");
    return;
  }
  if (mrmlScene->IsBatchProcessing())
  {
    return;
  }

  // Handle volume nodes only
  if (caller->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(caller);
    // If the volume has this type of reference, then there is a texture to update
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
      volumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
    // Handle window/level changes
    if (event == vtkMRMLDisplayableNode::DisplayModifiedEvent && modelNode)
    {
      std::map<vtkMRMLScalarVolumeNode*, vtkImageMapToWindowLevelColors*>::iterator mapperIt =
        this->TextureWindowLevelMappers.find(volumeNode);
      if (mapperIt != this->TextureWindowLevelMappers.end())
      {
        volumeNode->CreateDefaultDisplayNodes();
        mapperIt->second->SetWindow(volumeNode->GetScalarVolumeDisplayNode()->GetWindow());
        mapperIt->second->SetLevel(volumeNode->GetScalarVolumeDisplayNode()->GetLevel());
        modelNode->CreateDefaultDisplayNodes();
        modelNode->GetDisplayNode()->Modified();
      }
      else
      {
        vtkErrorMacro("ProcessMRMLNodesEvents: Failed to find texture pipeline for RT image " << volumeNode->GetName());
        return;
      }
    }
    // Handle transform changes
    else if (event == vtkMRMLTransformableNode::TransformModifiedEvent && modelNode)
    {
      // Update model geometry
      this->ComputeImagePlaneCorners(volumeNode, modelNode->GetPolyData()->GetPoints());
    }
  } // If ScalarVolume
}

//---------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::ProcessMRMLSceneEvents(vtkObject * vtkNotUsed(caller), unsigned long event, void *callData)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("ProcessMRMLSceneEvents: Invalid MRML scene or input node!");
    return;
  }
  vtkMRMLNode* node = reinterpret_cast<vtkMRMLNode*>(callData);

  // Remove texture pipeline and displayed model if node is about to be removed
  if ( node && node->IsA("vtkMRMLScalarVolumeNode")
    && event == vtkMRMLScene::NodeAboutToBeRemovedEvent )
  {
    vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
    // Remove displayed model node
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
      volumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
    if (modelNode)
    {
      this->GetMRMLScene()->RemoveNode(modelNode);

      // Remove texture display pipeline
      std::map<vtkMRMLScalarVolumeNode*, vtkImageMapToWindowLevelColors*>::iterator mapperIt =
        this->TextureWindowLevelMappers.find(volumeNode);
      if (mapperIt != this->TextureWindowLevelMappers.end())
      {
        vtkImageMapToWindowLevelColors* mapper = mapperIt->second;
        this->TextureWindowLevelMappers.erase(mapperIt);
        mapper->Delete();
      }
      else
      {
        vtkErrorMacro("ProcessMRMLSceneEvents: Failed to find texture pipeline for RT image " << volumeNode->GetName());
      }
    }
  }
  // Create texture pipeline after scene is imported
  else if (event == vtkMRMLScene::EndImportEvent)
  {
    std::vector<vtkMRMLNode*> nodes;
    this->GetMRMLScene()->GetNodesByClass("vtkMRMLScalarVolumeNode", nodes);
    for (std::vector<vtkMRMLNode*>::iterator nodeIt=nodes.begin(); nodeIt!=nodes.end(); ++nodeIt)
    {
      vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(*nodeIt);
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        volumeNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
      std::map<vtkMRMLScalarVolumeNode*, vtkImageMapToWindowLevelColors*>::iterator mapperIt =
        this->TextureWindowLevelMappers.find(volumeNode);
      if (modelNode && mapperIt == this->TextureWindowLevelMappers.end())
      {
        this->SetTextureForPlanarImage(volumeNode, modelNode);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::ComputeImagePlaneCorners(vtkMRMLScalarVolumeNode* planarImageVolumeNode, vtkPoints* sliceCornerPoints)
{
  if (!planarImageVolumeNode || !sliceCornerPoints)
  {
    vtkErrorMacro("ComputeImagePlaneCorners: Invalid input nodes!");
    return;
  }
  vtkMRMLScene* mrmlScene = planarImageVolumeNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("ComputeImagePlaneCorners: Invalid MRML scene!");
    return;
  }

  // Check if the volume to display is really a planar image
  int dims[3] = {0, 0, 0};
  planarImageVolumeNode->GetImageData()->GetDimensions(dims);
  if (dims[2] > 1)
  {
    vtkErrorMacro("ComputeImagePlaneCorners: Image to display ('" << planarImageVolumeNode->GetName() << "') is not single-slice!");
    return;
  }

  // Assemble image plane to world transform
  vtkSmartPointer<vtkTransform> planarImageParentTransform = vtkSmartPointer<vtkTransform>::New();
  planarImageParentTransform->Identity();
  if (planarImageVolumeNode->GetParentTransformNode())
  {
    vtkSmartPointer<vtkMatrix4x4> planarImageParentTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    planarImageVolumeNode->GetParentTransformNode()->GetMatrixTransformToWorld(planarImageParentTransformMatrix);
    planarImageParentTransform->SetMatrix(planarImageParentTransformMatrix);
  }

  vtkSmartPointer<vtkTransform> planarImageIjkToRasTransform = vtkSmartPointer<vtkTransform>::New();
  planarImageIjkToRasTransform->Identity();
  planarImageVolumeNode->GetIJKToRASMatrix( planarImageIjkToRasTransform->GetMatrix() );

  vtkSmartPointer<vtkTransform> planarImageIjkToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  planarImageIjkToWorldTransform->Identity();
  planarImageIjkToWorldTransform->PostMultiply();
  planarImageIjkToWorldTransform->Concatenate(planarImageParentTransform);
  planarImageIjkToWorldTransform->Concatenate(planarImageIjkToRasTransform);

  // Four corners of the image in volume IJK coordinate system.
  double point1Image[4] = { 0.0,             0.0,             0.0 };
  double point2Image[4] = { (double)dims[0], 0.0,             0.0 };
  double point3Image[4] = { 0.0,             (double)dims[1], 0.0 };
  double point4Image[4] = { (double)dims[0], (double)dims[1], 0.0 };

  // Compute the four corners of the image in world coordinate system.
  double point1RAS[4] = { 0.0, 0.0, 0.0 };
  double point2RAS[4] = { 0.0, 0.0, 0.0 }; 
  double point3RAS[4] = { 0.0, 0.0, 0.0 }; 
  double point4RAS[4] = { 0.0, 0.0, 0.0 };
  planarImageIjkToWorldTransform->TransformPoint(point1Image, point1RAS);
  planarImageIjkToWorldTransform->TransformPoint(point2Image, point2RAS);
  planarImageIjkToWorldTransform->TransformPoint(point3Image, point3RAS);
  planarImageIjkToWorldTransform->TransformPoint(point4Image, point4RAS);

  // Set position of the model
  sliceCornerPoints->SetPoint(0, point1RAS);
  sliceCornerPoints->SetPoint(1, point2RAS);
  sliceCornerPoints->SetPoint(2, point3RAS);
  sliceCornerPoints->SetPoint(3, point4RAS);
  sliceCornerPoints->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::SetTextureForPlanarImage(vtkMRMLScalarVolumeNode* planarImageVolumeNode, vtkMRMLModelNode* displayedModelNode)
{
  if (!planarImageVolumeNode || !displayedModelNode)
  {
    vtkErrorMacro("SetTextureForPlanarImage: Invalid input nodes!");
    return;
  }
  vtkMRMLScene* mrmlScene = planarImageVolumeNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("SetTextureForPlanarImage: Invalid MRML scene!");
    return;
  }

  // Check if the volume to display is really a planar image
  int dims[3] = {0, 0, 0};
  planarImageVolumeNode->GetImageData()->GetDimensions(dims);
  if (dims[2] > 1)
  {
    vtkErrorMacro("SetTextureForPlanarImage: Image to display ('" << planarImageVolumeNode->GetName() << "') is not single-slice!");
    return;
  }

  // Observe the planar image volume node so that the texture can be updated
  if ( !vtkIsObservedMRMLNodeEventMacro(planarImageVolumeNode, vtkMRMLDisplayableNode::DisplayModifiedEvent)
    && !vtkIsObservedMRMLNodeEventMacro(planarImageVolumeNode, vtkMRMLTransformableNode::TransformModifiedEvent) )
  {
    vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
    events->InsertNextValue(vtkMRMLDisplayableNode::DisplayModifiedEvent);
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(planarImageVolumeNode, events);
  }

  // Set window/level pipeline for planar image
  planarImageVolumeNode->CreateDefaultDisplayNodes();
  vtkImageMapToWindowLevelColors* textureWindowLevelMapper = vtkImageMapToWindowLevelColors::New();
  textureWindowLevelMapper->SetInputConnection(planarImageVolumeNode->GetImageDataConnection());
  textureWindowLevelMapper->SetOutputFormatToLuminance();
  textureWindowLevelMapper->SetWindow(planarImageVolumeNode->GetScalarVolumeDisplayNode()->GetWindow());
  textureWindowLevelMapper->SetLevel(planarImageVolumeNode->GetScalarVolumeDisplayNode()->GetLevel());
  this->TextureWindowLevelMappers[planarImageVolumeNode] = textureWindowLevelMapper;

  displayedModelNode->CreateDefaultDisplayNodes();
  displayedModelNode->GetModelDisplayNode()->SetTextureImageDataConnection(
    textureWindowLevelMapper->GetOutputPort());
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::CreateModelForPlanarImage(vtkMRMLPlanarImageNode* planarImageNode)
{
  if (!planarImageNode)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Invalid input node!");
    return;
  }
  vtkMRMLScene* mrmlScene = planarImageNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Invalid MRML scene!");
    return;
  }

  // Get planar image volume
  vtkMRMLScalarVolumeNode* planarImageVolume = planarImageNode->GetRtImageVolumeNode();
  if (!planarImageVolume)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Invalid input planar image volume node!");
    return;
  }
  // Sanity checks for the planar image volume
  int dims[3] = {0, 0, 0};
  planarImageVolume->GetImageData()->GetDimensions(dims);
  if (dims[0] == 0 && dims[1] == 0 && dims[2] == 0)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Image to display is empty!");
    return;
  }
  else if (dims[2] > 1)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Image to display ('" << planarImageVolume->GetName() << "') is not single-slice!");
    return;
  }

  // Get and set up model node for displaying the planar image
  vtkMRMLModelNode* displayedModelNode = vtkMRMLModelNode::SafeDownCast(
    planarImageNode->GetNodeReference(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
  if (!displayedModelNode)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Missing displayed model reference in parameter set node for planar image '" << planarImageVolume->GetName() << "'!");
    return;
  }
  displayedModelNode->SetDescription("Model displaying a planar image");

  // Create display node for the model
  vtkMRMLModelDisplayNode* displayedModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(displayedModelNode->GetDisplayNode());
  if (!displayedModelDisplayNode)
  {
    displayedModelDisplayNode = vtkMRMLModelDisplayNode::New();
    mrmlScene->AddNode(displayedModelDisplayNode);
    displayedModelDisplayNode->Delete(); // Return the ownership to the scene only
    displayedModelNode->SetAndObserveDisplayNodeID(displayedModelDisplayNode->GetID());
  }
  std::string displayedModelDisplayNodeName = std::string(displayedModelNode->GetName()) + "_Display";
  displayedModelDisplayNode->SetName(displayedModelDisplayNodeName.c_str());
  displayedModelDisplayNode->SetOpacity(1.0);
  displayedModelDisplayNode->SetColor(1.0, 1.0, 1.0);
  displayedModelDisplayNode->SetAmbient(1.0);
  displayedModelDisplayNode->SetBackfaceCulling(0);
  displayedModelDisplayNode->SetDiffuse(0.0);

  // Add reference from the planar image to the model
  planarImageVolume->SetNodeReferenceID(vtkMRMLPlanarImageNode::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str(), displayedModelNode->GetID());

  // Create plane
  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
  displayedModelNode->SetPolyDataConnection(plane->GetOutputPort());

  // Compute the image plane corners in world coordinate system
  this->ComputeImagePlaneCorners(planarImageVolume, displayedModelNode->GetPolyData()->GetPoints());

  // Create and set image texture
  this->SetTextureForPlanarImage(planarImageVolume, displayedModelNode);
}
