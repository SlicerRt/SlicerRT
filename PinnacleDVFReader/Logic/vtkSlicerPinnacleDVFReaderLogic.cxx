/*==========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==========================================================================*/

// PinnacleDVFReader includes
#include "vtkSlicerPinnacleDVFReaderLogic.h"
#include "vtkSlicerPinnacleDVFReader.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLGridTransformNode.h>
#include <vtkOrientedGridTransform.h>

// Slicer logic includes
#include <vtkSlicerApplicationLogic.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPinnacleDVFReaderLogic);

//----------------------------------------------------------------------------
vtkSlicerPinnacleDVFReaderLogic::vtkSlicerPinnacleDVFReaderLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerPinnacleDVFReaderLogic::~vtkSlicerPinnacleDVFReaderLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDVFReaderLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDVFReaderLogic::LoadPinnacleDVF(char *filename, double gridOriginX, double gridOriginY, double gridOriginZ)
{
  ifstream readFileStream;
  readFileStream.open(filename, std::ios::binary);
  if (readFileStream.fail())
  {
    vtkErrorMacro("LoadPinnacleDVF: The specified file could not be opened.");
  }
  vtkSmartPointer<vtkSlicerPinnacleDVFReader> pinnacleDVFReader = vtkSmartPointer<vtkSlicerPinnacleDVFReader>::New();
  pinnacleDVFReader->SetFileName(filename);
  pinnacleDVFReader->SetGridOrigin(gridOriginX, gridOriginY, gridOriginZ);
  pinnacleDVFReader->Update();

  // Post deformation node
  vtkMatrix4x4* postDeformationMatrix = NULL;
  postDeformationMatrix = pinnacleDVFReader->GetPostDeformationRegistrationMatrix();

  // Add post deformation transform node
  vtkSmartPointer<vtkMRMLLinearTransformNode> spatialPostTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  spatialPostTransformNode->SetScene(this->GetMRMLScene());
  spatialPostTransformNode->SetDisableModifiedEvent(1);
  std::string spatialPostTransformNodeName;
  spatialPostTransformNodeName = std::string(filename);
  spatialPostTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(spatialPostTransformNodeName+"_PostDeformationMatrix");
  spatialPostTransformNode->SetName(spatialPostTransformNodeName.c_str());
  spatialPostTransformNode->HideFromEditorsOff();
  spatialPostTransformNode->SetAndObserveMatrixTransformToParent(postDeformationMatrix);
  spatialPostTransformNode->SetDisableModifiedEvent(0);
  this->GetMRMLScene()->AddNode(spatialPostTransformNode);

  // Deformable grid vector image
  vtkImageData* deformableRegistrationGrid = NULL;
  deformableRegistrationGrid = pinnacleDVFReader->GetDeformableRegistrationGrid();

  // vtkOrientedGridTransform
  vtkSmartPointer<vtkOrientedGridTransform> gridTransform = vtkSmartPointer<vtkOrientedGridTransform>::New();
  gridTransform->SetDisplacementGridData(pinnacleDVFReader->GetDeformableRegistrationGrid());
  gridTransform->SetDisplacementScale(1);
  gridTransform->SetDisplacementShift(0);
  gridTransform->SetInterpolationModeToLinear();

  // Post deformation node
  vtkMatrix4x4* gridOrientationMatrix = NULL;
  gridOrientationMatrix = pinnacleDVFReader->GetDeformableRegistrationGridOrientationMatrix();

  gridTransform->SetGridDirectionMatrix(gridOrientationMatrix);

  // Add grid transform node
  vtkSmartPointer<vtkMRMLGridTransformNode> deformableRegistrationGridTransformNode = vtkSmartPointer<vtkMRMLGridTransformNode>::New();
  deformableRegistrationGridTransformNode->SetScene(this->GetMRMLScene());
  deformableRegistrationGridTransformNode->SetDisableModifiedEvent(1);
  std::string deformableRegistrationGridTransformNodeName;
  deformableRegistrationGridTransformNodeName = std::string(filename);
  deformableRegistrationGridTransformNodeName = this->GetMRMLScene()->GenerateUniqueName(deformableRegistrationGridTransformNodeName+"_DeformableRegistrationGrid");
  deformableRegistrationGridTransformNode->SetName(deformableRegistrationGridTransformNodeName.c_str());
  deformableRegistrationGridTransformNode->HideFromEditorsOff();
  deformableRegistrationGridTransformNode->SetAndObserveTransformToParent(gridTransform); 
  deformableRegistrationGridTransformNode->SetDisableModifiedEvent(0);
  this->GetMRMLScene()->AddNode(deformableRegistrationGridTransformNode);
}
