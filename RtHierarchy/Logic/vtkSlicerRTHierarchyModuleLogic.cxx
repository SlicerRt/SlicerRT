/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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

// RTHierarchy Logic includes
#include "vtkSlicerRTHierarchyModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRTHierarchyModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerRTHierarchyModuleLogic::vtkSlicerRTHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerRTHierarchyModuleLogic::~vtkSlicerRTHierarchyModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerRTHierarchyModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerRTHierarchyModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerRTHierarchyModuleLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RegisterNodes: Invalid MRML Scene!");
    return;
    }
}

//---------------------------------------------------------------------------
void vtkSlicerRTHierarchyModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
    {
    return;
    }
}
