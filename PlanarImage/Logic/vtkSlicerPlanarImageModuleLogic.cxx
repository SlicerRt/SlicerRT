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
}

//----------------------------------------------------------------------------
vtkSlicerPlanarImageModuleLogic::~vtkSlicerPlanarImageModuleLogic()
{
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
      volumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
    if (modelNode)
    {
      // Handle window/level changes
      if (event == vtkMRMLDisplayableNode::DisplayModifiedEvent)
      {
        vtkMRMLScalarVolumeNode* textureNode = vtkMRMLScalarVolumeNode::SafeDownCast(
          modelNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_TEXTURE_VOLUME_REFERENCE_ROLE.c_str()) );
        if (textureNode)
        {
          // Update texture for the planar image model
          this->SetTextureForPlanarImage(volumeNode, modelNode, textureNode);
        }
      }
      // Handle transform changes
      else if (event == vtkMRMLTransformableNode::TransformModifiedEvent)
      {
        // Update model geometry
        this->ComputeImagePlaneCorners(volumeNode, modelNode->GetPolyData()->GetPoints());
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::OnMRMLSceneEndImport()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndImport: Invalid MRML scene!");
    return;
  }

  vtkObject* nextObject = NULL;
  vtkSmartPointer<vtkCollection> modelNodes = vtkSmartPointer<vtkCollection>::Take( this->GetMRMLScene()->GetNodesByClass("vtkMRMLModelNode") );
  for (modelNodes->InitTraversal(); (nextObject = modelNodes->GetNextItemAsObject()); )
  {
    vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(nextObject);
    if (modelNode)
    {
      // Apply the texture if it has a reference to it
      vtkMRMLScalarVolumeNode* textureNode = vtkMRMLScalarVolumeNode::SafeDownCast(
        modelNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_TEXTURE_VOLUME_REFERENCE_ROLE.c_str()) );
#if (VTK_MAJOR_VERSION <= 5)
      if (textureNode)
      {
        modelNode->GetModelDisplayNode()->SetAndObserveTextureImageData(textureNode->GetImageData());
      }
#else
      if (textureNode && modelNode->GetModelDisplayNode())
      {
        modelNode->GetModelDisplayNode()->SetTextureImageDataConnection(textureNode->GetImageDataConnection());
      }
#endif
    }
  }
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::ComputeImagePlaneCorners(vtkMRMLScalarVolumeNode* planarImageVolumeNode, vtkPoints* sliceCornerPoints)
{
  if (!planarImageVolumeNode || !sliceCornerPoints)
  {
    vtkErrorMacro("GetTextureForPlanarImage: Invalid input nodes!");
    return;
  }
  vtkMRMLScene* mrmlScene = planarImageVolumeNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("GetTextureForPlanarImage: Invalid MRML scene!");
    return;
  }

  // Check if the volume to display is really a planar image
  int dims[3] = {0, 0, 0};
  planarImageVolumeNode->GetImageData()->GetDimensions(dims);
  if (dims[2] > 1)
  {
    vtkErrorMacro("GetTextureForPlanarImage: Image to display ('" << planarImageVolumeNode->GetName() << "') is not single-slice!");
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
  double point1Image[4] = { 0.0,     0.0,     0.0 };
  double point2Image[4] = { dims[0], 0.0,     0.0 };
  double point3Image[4] = { 0.0,     dims[1], 0.0 };
  double point4Image[4] = { dims[0], dims[1], 0.0 };

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
void vtkSlicerPlanarImageModuleLogic::SetTextureForPlanarImage(vtkMRMLScalarVolumeNode* planarImageVolumeNode, vtkMRMLModelNode* displayedModelNode, vtkMRMLScalarVolumeNode* textureVolumeNode)
{
  if (!planarImageVolumeNode || !displayedModelNode || !textureVolumeNode)
  {
    vtkErrorMacro("GetTextureForPlanarImage: Invalid input nodes!");
    return;
  }
  vtkMRMLScene* mrmlScene = planarImageVolumeNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("GetTextureForPlanarImage: Invalid MRML scene!");
    return;
  }

  // Check if the volume to display is really a planar image
  int dims[3] = {0, 0, 0};
  planarImageVolumeNode->GetImageData()->GetDimensions(dims);
  if (dims[2] > 1)
  {
    vtkErrorMacro("GetTextureForPlanarImage: Image to display ('" << planarImageVolumeNode->GetName() << "') is not single-slice!");
    return;
  }

  // Set up texture volume and create a display node for it
  // These are needed so that the model can be loaded back with a scene
  textureVolumeNode->CopyOrientation(planarImageVolumeNode);

  vtkMRMLScalarVolumeDisplayNode* textureVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(textureVolumeNode->GetDisplayNode());
  if (!textureVolumeDisplayNode)
  {
    textureVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::New();
    mrmlScene->AddNode(textureVolumeDisplayNode);
    textureVolumeDisplayNode->Delete(); // Return the ownership to the scene only
    textureVolumeNode->AddAndObserveDisplayNodeID( textureVolumeDisplayNode->GetID() );   
  }
  std::string planarImageTextureDisplayNodeName = std::string(textureVolumeNode->GetName()) + "_Display";
  textureVolumeDisplayNode->SetName(planarImageTextureDisplayNodeName.c_str());
  textureVolumeDisplayNode->SetAutoWindowLevel(0);
  textureVolumeDisplayNode->SetWindow(256);
  textureVolumeDisplayNode->SetLevel(128);
  textureVolumeDisplayNode->SetDefaultColorMap();

  // Add reference from displayed model node to texture volume node
  displayedModelNode->SetNodeReferenceID(SlicerRtCommon::PLANARIMAGE_TEXTURE_VOLUME_REFERENCE_ROLE.c_str(), textureVolumeNode->GetID());

  // Observe the planar image volume node so that the texture can be updated
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLDisplayableNode::DisplayModifiedEvent);
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  vtkObserveMRMLNodeEventsMacro(planarImageVolumeNode, events);

  vtkSmartPointer<vtkImageData> textureImageData = vtkSmartPointer<vtkImageData>::New();
  textureImageData->DeepCopy(planarImageVolumeNode->GetImageData());

  // Apply window level to texture image data if requested
  if (planarImageVolumeNode->GetScalarVolumeDisplayNode())
  {
    vtkSmartPointer<vtkImageMapToWindowLevelColors> mapToWindowLevelColors = vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
#if (VTK_MAJOR_VERSION <= 5)
    mapToWindowLevelColors->SetInput(textureImageData);
#else
    mapToWindowLevelColors->SetInputData(textureImageData);
#endif
    mapToWindowLevelColors->SetOutputFormatToLuminance();
    mapToWindowLevelColors->SetWindow(planarImageVolumeNode->GetScalarVolumeDisplayNode()->GetWindow());
    mapToWindowLevelColors->SetLevel(planarImageVolumeNode->GetScalarVolumeDisplayNode()->GetLevel());
    mapToWindowLevelColors->Update();

    textureImageData->DeepCopy(mapToWindowLevelColors->GetOutput());
  }

  // Set texture image data to its volume node and to the planar image model node as texture
  textureVolumeNode->SetAndObserveImageData(textureImageData);
#if (VTK_MAJOR_VERSION <= 5)
  displayedModelNode->GetModelDisplayNode()->SetAndObserveTextureImageData(textureImageData);
#else
  if (displayedModelNode->GetModelDisplayNode())
  {
    displayedModelNode->GetModelDisplayNode()->SetTextureImageDataConnection(textureVolumeNode->GetImageDataConnection());
  }
#endif

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
    planarImageNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str()) );
  if (!displayedModelNode)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Missing displayed model reference in parameter set node for planar image '" << planarImageVolume->GetName() << "'!");
    return;
  }
  displayedModelNode->SetDescription("Model displaying a planar image");

  // Get texture volume for the displayed model
  vtkMRMLScalarVolumeNode* textureVolume = vtkMRMLScalarVolumeNode::SafeDownCast(
    planarImageNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_TEXTURE_VOLUME_REFERENCE_ROLE.c_str()) );
  if (!textureVolume)
  {
    vtkErrorMacro("CreateModelForPlanarImage: Missing texture volume reference in parameter set node for planar image '" << planarImageVolume->GetName() << "'!");
    return;
  }

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
  planarImageVolume->SetNodeReferenceID(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE.c_str(), displayedModelNode->GetID());

  // Create plane
  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
  displayedModelNode->SetAndObservePolyData(plane->GetOutput());

  // Compute the image plane corners in world coordinate system
  this->ComputeImagePlaneCorners(planarImageVolume, displayedModelNode->GetPolyData()->GetPoints());

  // Create and set image texture
  this->SetTextureForPlanarImage(planarImageVolume, displayedModelNode, textureVolume);
}
