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

// Contours includes
#include "vtkSlicerContoursModuleLogic.h"
#include "vtkMRMLContourNode.h"
#include "vtkSlicerContoursPatientHierarchyPlugin.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerPatientHierarchyModuleLogic.h"
#include "vtkSlicerPatientHierarchyPluginHandler.h"

// VTK includes
#include <vtkNew.h>
#include <vtkPolyData.h>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include "vtkMRMLDisplayableHierarchyNode.h"

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerContoursModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic::vtkSlicerContoursModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerContoursModuleLogic::~vtkSlicerContoursModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLContourNode>::New());

  // Register Patient Hierarchy plugin
  vtkSlicerPatientHierarchyPluginHandler::GetInstance()->RegisterPlugin(vtkSmartPointer<vtkSlicerContoursPatientHierarchyPlugin>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);

  this->CreateDefaultStructureSetNode();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  if (node->IsA("vtkMRMLContourNode"))
  {
    // Create empty ribbon model
    this->CreateEmptyRibbonModelForContour(node);

    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    return;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (contourNode)
  {
    // Delete ribbon model representation if it is the default empty model
    if (contourNode->RibbonModelContainsEmptyPolydata())
    {
      this->GetMRMLScene()->RemoveNode(contourNode->GetRibbonModelNode());
    }

    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::OnMRMLSceneEndClose()
{
  assert(this->GetMRMLScene() != 0);

  this->CreateDefaultStructureSetNode();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::CreateDefaultStructureSetNode()
{
  assert(this->GetMRMLScene() != 0);

  vtkSmartPointer<vtkCollection> defaultStructureSetNodes = vtkSmartPointer<vtkCollection>::Take(
    this->GetMRMLScene()->GetNodesByClassByName("vtkMRMLHierarchyNode", SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME) );
  if (defaultStructureSetNodes->GetNumberOfItems() > 0)
  {
    vtkWarningMacro("CreateDefaultStructureSetNode: Default structure set node already exists");
    return;
  }

  vtkSmartPointer<vtkMRMLDisplayableHierarchyNode> contourHierarchyNode = vtkSmartPointer<vtkMRMLDisplayableHierarchyNode>::New();
  contourHierarchyNode->SetName(SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME);
  contourHierarchyNode->AllowMultipleChildrenOn();
  contourHierarchyNode->HideFromEditorsOff();
  contourHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_NAME, SlicerRtCommon::PATIENTHIERARCHY_NODE_TYPE_ATTRIBUTE_VALUE);
  contourHierarchyNode->SetAttribute(SlicerRtCommon::PATIENTHIERARCHY_DICOMLEVEL_ATTRIBUTE_NAME, vtkSlicerPatientHierarchyModuleLogic::PATIENTHIERARCHY_LEVEL_SERIES);
  contourHierarchyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_CONTOUR_HIERARCHY_ATTRIBUTE_NAME.c_str(), "1");
  this->GetMRMLScene()->AddNode(contourHierarchyNode);

  // A hierarchy node needs a display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> contourHierarchyDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  std::string contourHierarchyDisplayNodeName = std::string(SlicerRtCommon::PATIENTHIERARCHY_DEFAULT_STRUCTURE_SET_NODE_NAME) + "Display";
  contourHierarchyDisplayNode->SetName(contourHierarchyDisplayNodeName.c_str());
  contourHierarchyDisplayNode->SetVisibility(1);

  this->GetMRMLScene()->AddNode(contourHierarchyDisplayNode);
  contourHierarchyNode->SetAndObserveDisplayNodeID(contourHierarchyDisplayNode->GetID());
}

//---------------------------------------------------------------------------
void vtkSlicerContoursModuleLogic::CreateEmptyRibbonModelForContour(vtkMRMLNode* node)
{
  vtkMRMLScene* mrmlScene = this->GetMRMLScene();
  if (!mrmlScene)
  {
    vtkErrorMacro("CreateEmptyRibbonModelForContour: Invalid MRML scene!");
    return;
  }

  vtkMRMLContourNode* contourNode = vtkMRMLContourNode::SafeDownCast(node);
  if (!contourNode)
  {
    vtkErrorMacro("CreateEmptyRibbonModelForContour: Argument node is not a contour node!");
    return;
  }

  vtkSmartPointer<vtkPolyData> emptyPolyData = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkMRMLModelDisplayNode> emptyRibbonModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  mrmlScene->AddNode(emptyRibbonModelDisplayNode);

  vtkSmartPointer<vtkMRMLModelNode> emptyRibbonModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  mrmlScene->AddNode(emptyRibbonModelNode);
  emptyRibbonModelNode->SetAndObserveDisplayNodeID(emptyRibbonModelDisplayNode->GetID());
  emptyRibbonModelNode->SetAndObservePolyData(emptyPolyData);

  std::string emptyRibbonModelName(contourNode->GetName());
  emptyRibbonModelName.append(SlicerRtCommon::CONTOUR_RIBBON_MODEL_NODE_NAME_POSTFIX);
  emptyRibbonModelNode->SetName(emptyRibbonModelName.c_str());

  contourNode->SetAndObserveRibbonModelNodeId(emptyRibbonModelNode->GetID());
}
