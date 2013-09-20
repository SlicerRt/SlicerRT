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
    if (event == vtkMRMLDisplayableNode::DisplayModifiedEvent)
    {
      // If the volume has this type of reference, then there is a texture to update
      vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(
        volumeNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE) );
      if (modelNode)
      {
        vtkMRMLScalarVolumeNode* textureNode = vtkMRMLScalarVolumeNode::SafeDownCast(
          modelNode->GetNodeReference(SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE) );
        if (textureNode)
        {
          // Update texture for the planar image model
          this->SetTextureForPlanarImage(volumeNode, modelNode, textureNode);
        }
      }
    }
    else if (event == vtkMRMLTransformableNode::TransformModifiedEvent)
    {
      //TODO: update model geometry
    }
  }
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

  vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode> textureVolumeDisplayNode = vtkSmartPointer<vtkMRMLScalarVolumeDisplayNode>::New();
  mrmlScene->AddNode(textureVolumeDisplayNode);
  std::string planarImageTextureDisplayNodeName = std::string(textureVolumeNode->GetName()) + "_Display";
  textureVolumeDisplayNode->SetName(planarImageTextureDisplayNodeName.c_str());
  textureVolumeDisplayNode->SetAutoWindowLevel(0);
  textureVolumeDisplayNode->SetWindow(256);
  textureVolumeDisplayNode->SetLevel(128);
  textureVolumeDisplayNode->SetDefaultColorMap();

  textureVolumeNode->AddAndObserveDisplayNodeID( textureVolumeDisplayNode->GetID() );   

  // Add reference from displayed model node to texture volume node
  displayedModelNode->AddNodeReferenceID(SlicerRtCommon::PLANARIMAGE_TEXTURE_REFERENCE_ROLE, textureVolumeNode->GetID());

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
    mapToWindowLevelColors->SetInput(textureImageData);
    mapToWindowLevelColors->SetOutputFormatToLuminance();
    mapToWindowLevelColors->SetWindow(planarImageVolumeNode->GetScalarVolumeDisplayNode()->GetWindow());
    mapToWindowLevelColors->SetLevel(planarImageVolumeNode->GetScalarVolumeDisplayNode()->GetLevel());
    mapToWindowLevelColors->Update();

    textureImageData->DeepCopy(mapToWindowLevelColors->GetOutput());
  }

  // Set texture image data to its volume node and to the planar image model node as texture
  textureVolumeNode->SetAndObserveImageData(textureImageData);
  displayedModelNode->GetModelDisplayNode()->SetAndObserveTextureImageData(textureImageData);
}

//----------------------------------------------------------------------------
void vtkSlicerPlanarImageModuleLogic::CreateModelForPlanarImage(vtkMRMLPlanarImageNode* planarImageNode)
{

  if (!planarImageNode)
  {
    vtkErrorMacro("DisplayPlanarImage: Invalid input node!");
    return;
  }
  vtkMRMLScene* mrmlScene = planarImageNode->GetScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("DisplayPlanarImage: Invalid MRML scene!");
    return;
  }

  // Get planar image volume
  vtkMRMLScalarVolumeNode* planarImageVolume = vtkMRMLScalarVolumeNode::SafeDownCast(
    planarImageNode->GetNodeReference(vtkMRMLPlanarImageNode::PlanarImageVolumeNodeReferenceRole) );
  if (!planarImageVolume)
  {
    vtkErrorMacro("DisplayPlanarImage: Invalid input planar image volume node!");
    return;
  }
  // Sanity checks for the planar image volume
  int dims[3] = {0, 0, 0};
  planarImageVolume->GetImageData()->GetDimensions(dims);
  if (dims[0] == 0 && dims[1] == 0 && dims[2] == 0)
  {
    vtkErrorMacro("DisplayPlanarImage: Image to display is empty!");
    return;
  }
  else if (dims[2] > 1)
  {
    vtkErrorMacro("DisplayPlanarImage: Image to display ('" << planarImageVolume->GetName() << "') is not single-slice!");
    return;
  }

  // Get and set up model node for displaying the planar image
  vtkMRMLModelNode* displayedModelNode = vtkMRMLModelNode::SafeDownCast(
    planarImageNode->GetNodeReference(vtkMRMLPlanarImageNode::DisplayedModelNodeReferenceRole) );
  if (!displayedModelNode)
  {
    vtkErrorMacro("DisplayPlanarImage: Missing displayed model reference in parameter set node for planar image '" << planarImageVolume->GetName() << "'!");
    return;
  }
  displayedModelNode->SetDescription("Model displaying a planar image");

  // Get texture volume for the displayed model
  vtkMRMLScalarVolumeNode* textureVolume = vtkMRMLScalarVolumeNode::SafeDownCast(
    planarImageNode->GetNodeReference(vtkMRMLPlanarImageNode::TextureVolumeNodeReferenceRole) );
  if (!textureVolume)
  {
    vtkErrorMacro("DisplayPlanarImage: Missing texture volume reference in parameter set node for planar image '" << planarImageVolume->GetName() << "'!");
    return;
  }

  // Create display node for the model
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayedModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  mrmlScene->AddNode(displayedModelDisplayNode);
  std::string displayedModelDisplayNodeName = std::string(displayedModelNode->GetName()) + "_Display";
  displayedModelDisplayNode->SetName(displayedModelDisplayNodeName.c_str());
  displayedModelDisplayNode->SetOpacity(1.0);
  displayedModelDisplayNode->SetColor(1.0, 1.0, 1.0);
  displayedModelDisplayNode->SetAmbient(1.0);
  displayedModelDisplayNode->SetBackfaceCulling(0);
  displayedModelDisplayNode->SetDiffuse(0.0);
  displayedModelDisplayNode->SetSaveWithScene(0);

  displayedModelNode->SetAndObserveDisplayNodeID(displayedModelDisplayNode->GetID());

  // Add reference from the planar image to the model
  planarImageVolume->AddNodeReferenceID(SlicerRtCommon::PLANARIMAGE_DISPLAYED_MODEL_REFERENCE_ROLE, displayedModelNode->GetID());

  // Create plane
  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
  displayedModelNode->SetAndObservePolyData(plane->GetOutput());

  // Assemble image plane to world transform
  vtkSmartPointer<vtkTransform> volumeToDisplayParentTransform = vtkSmartPointer<vtkTransform>::New();
  volumeToDisplayParentTransform->Identity();
  if (planarImageVolume->GetParentTransformNode())
  {
    vtkSmartPointer<vtkMatrix4x4> volumeToDisplayParentTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    planarImageVolume->GetParentTransformNode()->GetMatrixTransformToWorld(volumeToDisplayParentTransformMatrix);
    volumeToDisplayParentTransform->SetMatrix(volumeToDisplayParentTransformMatrix);
  }

  vtkSmartPointer<vtkTransform> volumeToDisplayIjkToRasTransform = vtkSmartPointer<vtkTransform>::New();
  volumeToDisplayIjkToRasTransform->Identity();
  planarImageVolume->GetIJKToRASMatrix( volumeToDisplayIjkToRasTransform->GetMatrix() );

  vtkSmartPointer<vtkTransform> volumeToDisplayIjkToWorldTransform = vtkSmartPointer<vtkTransform>::New();
  volumeToDisplayIjkToWorldTransform->Identity();
  volumeToDisplayIjkToWorldTransform->PostMultiply();
  volumeToDisplayIjkToWorldTransform->Concatenate(volumeToDisplayParentTransform);
  volumeToDisplayIjkToWorldTransform->Concatenate(volumeToDisplayIjkToRasTransform);
  volumeToDisplayIjkToWorldTransform->Update();

  // Four corners of the image in volume IJK coordinate system.
  double point1Image[4] = { 0.0,     0.0,     0.0, 1.0 };
  double point2Image[4] = { dims[0], 0.0,     0.0, 1.0 };
  double point3Image[4] = { 0.0,     dims[1], 0.0, 1.0 };
  double point4Image[4] = { dims[0], dims[1], 0.0, 1.0 };

  // Compute the four corners of the image in world coordinate system.
  double point1RAS[4] = { 0, 0, 0, 0 };
  double point2RAS[4] = { 0, 0, 0, 0 }; 
  double point3RAS[4] = { 0, 0, 0, 0 }; 
  double point4RAS[4] = { 0, 0, 0, 0 };
  volumeToDisplayIjkToWorldTransform->MultiplyPoint(point1Image, point1RAS);
  volumeToDisplayIjkToWorldTransform->MultiplyPoint(point2Image, point2RAS);
  volumeToDisplayIjkToWorldTransform->MultiplyPoint(point3Image, point3RAS);
  volumeToDisplayIjkToWorldTransform->MultiplyPoint(point4Image, point4RAS);

  // Set position of the model
  vtkPoints* slicePoints = displayedModelNode->GetPolyData()->GetPoints();
  slicePoints->SetPoint(0, point1RAS);
  slicePoints->SetPoint(1, point2RAS);
  slicePoints->SetPoint(2, point3RAS);
  slicePoints->SetPoint(3, point4RAS);

  // Create and set image texture
  this->SetTextureForPlanarImage(planarImageVolume, displayedModelNode, textureVolume);
}
