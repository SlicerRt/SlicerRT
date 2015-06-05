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

// Beams includes
#include "vtkSlicerBeamsModuleLogic.h"
#include "vtkMRMLBeamsNode.h"

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkConeSource.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::vtkSlicerBeamsModuleLogic()
{
  this->BeamsNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::~vtkSlicerBeamsModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamsNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::SetAndObserveBeamsNode(vtkMRMLBeamsNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->BeamsNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    vtkErrorMacro("RegisterNodes: Invalid MRML scene!");
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLBeamsNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateFromMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateFromMRMLScene: Invalid MRML scene!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLMarkupsFiducialNode") || node->IsA("vtkMRMLBeamsNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or input node!");
    return;
  }

  if (node->IsA("vtkMRMLMarkupsFiducialNode") || node->IsA("vtkMRMLBeamsNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLBeamsNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLBeamsNode");
  if (node)
  {
    paramNode = vtkMRMLBeamsNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->BeamsNode, paramNode);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneEndClose()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("OnMRMLSceneEndClose: Invalid MRML scene or input node!");
    return;
  }

  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSlicerBeamsModuleLogic::CreateBeamModel()
{
  if (!this->BeamsNode || !this->GetMRMLScene())
  {
    std::string errorMessage("Invalid MRML scene or parameter set node");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }

  if (!this->BeamsNode->GetIsocenterMarkupsNode())
  {
    std::string errorMessage("Isocenter markup list is empty");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }

  // Get isocenter and source markups node
  vtkMRMLMarkupsFiducialNode* isocenterNode = this->BeamsNode->GetIsocenterMarkupsNode();
  if (!isocenterNode)
  {
    std::string errorMessage("Unable to retrieve isocenter markups node according its ID");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }

  // Get subject hierarchy node for the isocenter
  vtkMRMLSubjectHierarchyNode* isocenterSubjectHierarchyNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(isocenterNode);
  if (!isocenterSubjectHierarchyNode)
  {
    std::string errorMessage("Unable to retrieve isocenter subject hierarchy node");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }

  // Extract beam-related parameters needed to compute source position
  double collimatorAngle = 0.0;
  const char* collimatorAngleChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_COLLIMATOR_ANGLE_ATTRIBUTE_NAME.c_str());
  if (collimatorAngleChars != NULL)
  {
    std::stringstream ss;
    ss << collimatorAngleChars;
    ss >> collimatorAngle;
  }

  double gantryAngle = 0.0;
  const char* gantryAngleChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_GANTRY_ANGLE_ATTRIBUTE_NAME.c_str());
  if (gantryAngleChars != NULL)
  {
    std::stringstream ss;
    ss << gantryAngleChars;
    ss >> gantryAngle;
  }

  double couchAngle = 0.0;
  const char* couchAngleChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_COUCH_ANGLE_ATTRIBUTE_NAME.c_str());
  if (couchAngleChars != NULL)
  {
    std::stringstream ss;
    ss << couchAngleChars;
    ss >> couchAngle;
  }

  double jawPosition[2][2] = {{0.0, 0.0},{0.0, 0.0}};
  const char* jawPositionChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_BEAM_JAW_POSITIONS_ATTRIBUTE_NAME.c_str());
  if (jawPositionChars != NULL)
  {
    std::stringstream ss;
    ss << jawPositionChars;
    ss >> jawPosition[0][0] >> jawPosition[0][1] >> jawPosition[1][0] >> jawPosition[1][1];
  }

  double sourceAxisDistance = 0.0;
  const char* sourceAxisDistanceChars = isocenterSubjectHierarchyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_SOURCE_AXIS_DISTANCE_ATTRIBUTE_NAME.c_str());
  if (sourceAxisDistanceChars != NULL)
  {
    std::stringstream ss;
    ss << sourceAxisDistanceChars;
    ss >> sourceAxisDistance;
  }

  // Get beam model node
  if (!this->BeamsNode->GetBeamModelNode())
  {
    std::string errorMessage("Invalid beam model node");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }
  vtkMRMLModelNode* beamModelNode = this->BeamsNode->GetBeamModelNode();
  if (!beamModelNode)
  {
    std::string errorMessage("Unable to retrieve beam model node");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }

  // Get isocenter position
  if (isocenterNode->GetNumberOfFiducials() != 2)
  {
    std::string errorMessage("Invalid isocenter markups fiducial count! It is supposed to be 2");
    vtkErrorMacro("CreateBeamModel: " << errorMessage);
    return errorMessage;
  }
  double isocenterCoordinates[4] = {0.0, 0.0, 0.0};
  isocenterNode->GetNthFiducialPosition(0, isocenterCoordinates);

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkPolyData> beamModelPolyData = NULL;
    beamModelPolyData = this->CreateBeamPolyData(
        -jawPosition[0][0],
         jawPosition[0][1],
        -jawPosition[1][0],
         jawPosition[1][1],
        sourceAxisDistance);

  // Assemble transform that places the beam in the proper position and orientation
  vtkSmartPointer<vtkTransform> beamModelTransform = vtkSmartPointer<vtkTransform>::New();
  beamModelTransform->Identity();
  beamModelTransform->RotateZ(gantryAngle);
  beamModelTransform->RotateY(collimatorAngle);
  beamModelTransform->RotateX(-90);

  vtkSmartPointer<vtkTransform> beamModelTranslationTransform = vtkSmartPointer<vtkTransform>::New();
  beamModelTranslationTransform->Identity();
  beamModelTranslationTransform->Translate(isocenterCoordinates[0], isocenterCoordinates[1], isocenterCoordinates[2]);

  beamModelTransform->PostMultiply();
  beamModelTransform->Concatenate(beamModelTranslationTransform->GetMatrix());

  vtkSmartPointer<vtkTransformPolyDataFilter> beamModelTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  beamModelTransformFilter->SetInputData(beamModelPolyData);
  beamModelTransformFilter->SetTransform(beamModelTransform);
  beamModelTransformFilter->Update();

  // Create display node
  vtkSmartPointer<vtkMRMLModelDisplayNode> displayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  displayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(displayNode));
  displayNode->SliceIntersectionVisibilityOn();  
  displayNode->VisibilityOn(); 
  displayNode->SetColor(0.0, 1.0, 0.0);
  displayNode->SetOpacity(this->BeamsNode->GetBeamModelOpacity());
  // Disable backface culling to make the back side of the model visible as well
  displayNode->SetBackfaceCulling(0);

  beamModelNode->SetAndObservePolyData( beamModelTransformFilter->GetOutput() );
  beamModelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
  beamModelNode->SetHideFromEditors(0);
  beamModelNode->SetSelectable(1);
  beamModelNode->Modified();

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);

  return "";
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerBeamsModuleLogic::CreateBeamPolyData(
    double X1, double X2, double Y1, double Y2, double SAD)
{
  // Create beam model
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertPoint(0,0,0,SAD);
  points->InsertPoint(1,-2*Y2, -2*X2, -SAD );
  points->InsertPoint(2,-2*Y2,  2*X1, -SAD );
  points->InsertPoint(3, 2*Y1,  2*X1, -SAD );
  points->InsertPoint(4, 2*Y1, -2*X2, -SAD );

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(1);
  cellArray->InsertCellPoint(2);

  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(2);
  cellArray->InsertCellPoint(3);

  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(3);
  cellArray->InsertCellPoint(4);

  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(4);
  cellArray->InsertCellPoint(1);

  // Add the cap to the bottom
  cellArray->InsertNextCell(4);
  cellArray->InsertCellPoint(1);
  cellArray->InsertCellPoint(2);
  cellArray->InsertCellPoint(3);
  cellArray->InsertCellPoint(4);

  vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);

  return beamModelPolyData;
}
