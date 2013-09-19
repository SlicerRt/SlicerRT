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

// PatientHierarchy includes
#include "vtkPlanarImageDisplay.h"

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
const char* vtkPlanarImageDisplay::PLANAR_IMAGE_MODEL_NODE_NAME = "PlanarImageModel";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlanarImageDisplay);

//----------------------------------------------------------------------------
vtkPlanarImageDisplay::vtkPlanarImageDisplay()
{
}

//----------------------------------------------------------------------------
vtkPlanarImageDisplay::~vtkPlanarImageDisplay()
{
}

//----------------------------------------------------------------------------
void vtkPlanarImageDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPlanarImageDisplay::DisplayPlanarImage(vtkMRMLScalarVolumeNode* volumeToDisplay, bool preserveWindowLevel/*=true*/)
{
  if (!volumeToDisplay)
  {
    std::cerr << "vtkPlanarImageDisplay::DisplayPlanarImage: Invalid input volume node!" << std::endl;
    return;
  }

  vtkMRMLScene* mrmlScene = volumeToDisplay->GetScene();
  if (!mrmlScene)
  {
    vtkErrorWithObjectMacro(volumeToDisplay, "vtkPlanarImageDisplay::DisplayPlanarImage: Invalid MRML scene!");
    return;
  }

  // Get planar image model node from the scene if it exists
  vtkSmartPointer<vtkMRMLModelNode> planarImageModelNode;
  vtkSmartPointer<vtkCollection> planarImageModelNodeCollection = vtkSmartPointer<vtkCollection>::Take(
    mrmlScene->GetNodesByClassByName("vtkMRMLModelNode", PLANAR_IMAGE_MODEL_NODE_NAME) );
  if (planarImageModelNodeCollection->GetNumberOfItems() > 1)
  {
    vtkErrorWithObjectMacro(volumeToDisplay, "vtkPlanarImageDisplay::DisplayPlanarImage: Multiple model nodes found with name! First one is used for planar image display.");
  }
  else if (planarImageModelNodeCollection->GetNumberOfItems() == 1)
  {
    planarImageModelNode = vtkMRMLModelNode::SafeDownCast(planarImageModelNodeCollection->GetItemAsObject(0));
  }
  else
  {
    // Create model node for displaying the planar image
    planarImageModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
    mrmlScene->AddNode(planarImageModelNode);
    planarImageModelNode->SetName(PLANAR_IMAGE_MODEL_NODE_NAME);
    planarImageModelNode->SetDescription("Model displaying a planar image");
    planarImageModelNode->SetHideFromEditors(0);
    planarImageModelNode->SetSaveWithScene(0);

    // Create display node for the model
    vtkSmartPointer<vtkMRMLModelDisplayNode> planarImageModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
    mrmlScene->AddNode(planarImageModelDisplayNode);
    planarImageModelDisplayNode->SetOpacity(1.0);
    planarImageModelDisplayNode->SetColor(1.0, 1.0, 1.0);
    planarImageModelDisplayNode->SetAmbient(1.0);
    planarImageModelDisplayNode->SetBackfaceCulling(0);
    planarImageModelDisplayNode->SetDiffuse(0.0);
    planarImageModelDisplayNode->SetSaveWithScene(0);

    planarImageModelNode->SetAndObserveDisplayNodeID(planarImageModelDisplayNode->GetID());
  }

  // Get dimensions of the volume to display
  int dims[3] = {0, 0, 0};
  volumeToDisplay->GetImageData()->GetDimensions(dims);
  if (dims[0] == 0 && dims[1] == 0 && dims[2] == 0)
  {
    vtkErrorWithObjectMacro(volumeToDisplay, "vtkPlanarImageDisplay::DisplayPlanarImage: Image to display is empty!");
    return;
  }

  // Create plane
  vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
  planarImageModelNode->SetAndObservePolyData(plane->GetOutput());

  // Assemble image plane to world transform
  vtkSmartPointer<vtkTransform> volumeToDisplayParentTransform = vtkSmartPointer<vtkTransform>::New();
  volumeToDisplayParentTransform->Identity();
  if (volumeToDisplay->GetParentTransformNode())
  {
    vtkSmartPointer<vtkMatrix4x4> volumeToDisplayParentTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    volumeToDisplay->GetParentTransformNode()->GetMatrixTransformToWorld(volumeToDisplayParentTransformMatrix);
    volumeToDisplayParentTransform->SetMatrix(volumeToDisplayParentTransformMatrix);
  }

  vtkSmartPointer<vtkTransform> volumeToDisplayIjkToRasTransform = vtkSmartPointer<vtkTransform>::New();
  volumeToDisplayIjkToRasTransform->Identity();
  volumeToDisplay->GetIJKToRASMatrix( volumeToDisplayIjkToRasTransform->GetMatrix() );

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
  vtkPoints* slicePoints = planarImageModelNode->GetPolyData()->GetPoints();
  slicePoints->SetPoint(0, point1RAS);
  slicePoints->SetPoint(1, point2RAS);
  slicePoints->SetPoint(2, point3RAS);
  slicePoints->SetPoint(3, point4RAS);

  // Add image texture
  vtkSmartPointer<vtkImageData> textureImageData = vtkSmartPointer<vtkImageData>::New();
  textureImageData->DeepCopy(volumeToDisplay->GetImageData());

  // Apply window level to texture image data if requested
  if (preserveWindowLevel && volumeToDisplay->GetScalarVolumeDisplayNode())
  {
    vtkSmartPointer<vtkImageMapToWindowLevelColors> mapToWindowLevelColors = vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
    mapToWindowLevelColors->SetInput(textureImageData);
    mapToWindowLevelColors->SetOutputFormatToLuminance();
    mapToWindowLevelColors->SetWindow(volumeToDisplay->GetScalarVolumeDisplayNode()->GetWindow());
    mapToWindowLevelColors->SetLevel(volumeToDisplay->GetScalarVolumeDisplayNode()->GetLevel());
    mapToWindowLevelColors->Update();
    
    textureImageData->DeepCopy(mapToWindowLevelColors->GetOutput());
  }

  planarImageModelNode->GetModelDisplayNode()->SetAndObserveTextureImageData(textureImageData);
}
