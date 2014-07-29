/*==========================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

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

// PinnacleDvfReader includes
#include "vtkSlicerPinnacleDvfReaderLogic.h"
#include "vtkSlicerPinnacleDvfReader.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLGridTransformNode.h>
#include <vtkOrientedGridTransform.h>

// Slicer logic includes
#include <vtkSlicerApplicationLogic.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPinnacleDvfReaderLogic);

//----------------------------------------------------------------------------
vtkSlicerPinnacleDvfReaderLogic::vtkSlicerPinnacleDvfReaderLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerPinnacleDvfReaderLogic::~vtkSlicerPinnacleDvfReaderLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDvfReaderLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerPinnacleDvfReaderLogic::LoadPinnacleDvf(char *filename, double gridOriginX, double gridOriginY, double gridOriginZ)
{
  ifstream readFileStream;
  readFileStream.open(filename, std::ios::binary);
  if (readFileStream.fail())
  {
    vtkErrorMacro("LoadPinnacleDvf: The specified file could not be opened.");
  }
  vtkSmartPointer<vtkSlicerPinnacleDvfReader> pinnacleDvfReader = vtkSmartPointer<vtkSlicerPinnacleDvfReader>::New();
  pinnacleDvfReader->SetFileName(filename);
  pinnacleDvfReader->SetGridOrigin(gridOriginX, gridOriginY, gridOriginZ);
  pinnacleDvfReader->Update();

  // Post deformation node
  vtkMatrix4x4* postDeformationMatrix = NULL;
  postDeformationMatrix = pinnacleDvfReader->GetPostDeformationRegistrationMatrix();

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
  deformableRegistrationGrid = pinnacleDvfReader->GetDeformableRegistrationGrid();

  // vtkOrientedGridTransform
  vtkSmartPointer<vtkOrientedGridTransform> gridTransform = vtkSmartPointer<vtkOrientedGridTransform>::New();
#if (VTK_MAJOR_VERSION <= 5)
  gridTransform->SetDisplacementGrid(pinnacleDvfReader->GetDeformableRegistrationGrid());
#else
  gridTransform->SetDisplacementGridData(pinnacleDvfReader->GetDeformableRegistrationGrid());
#endif
  gridTransform->SetDisplacementScale(1);
  gridTransform->SetDisplacementShift(0);
  gridTransform->SetInterpolationModeToLinear();

  // Post deformation node
  vtkMatrix4x4* gridOrientationMatrix = NULL;
  gridOrientationMatrix = pinnacleDvfReader->GetDeformableRegistrationGridOrientationMatrix();

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
