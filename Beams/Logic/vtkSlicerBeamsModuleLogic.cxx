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

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkSlicerSubjectHierarchyModuleLogic.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLDoubleArrayNode.h>
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
#include <vtkDoubleArray.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerBeamsModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::vtkSlicerBeamsModuleLogic()
{
  this->RTPlanNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerBeamsModuleLogic::~vtkSlicerBeamsModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->RTPlanNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::SetAndObserveRTPlanNode(vtkMRMLRTPlanNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->RTPlanNode, node);
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
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
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

  if (node->IsA("vtkMRMLMarkupsFiducialNode") || node->IsA("vtkMRMLRTPlanNode") || node->IsA("vtkMRMLRTBeamNode"))
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

  if (node->IsA("vtkMRMLMarkupsFiducialNode") || node->IsA("vtkMRMLRTPlanNode") || node->IsA("vtkMRMLRTBeamNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::OnMRMLSceneEndImport()
{
  //TODO: Why is this needed?
  vtkMRMLNode* node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLRTPlanNode");
  if (node)
  {
    vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->RTPlanNode, planNode);
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
void vtkSlicerBeamsModuleLogic::UpdateBeamTransform(vtkMRMLScene* scene, vtkMRMLRTBeamNode* beamNode)
{
  if (!scene || !beamNode)
  {
    std::cerr << "UpdateBeamTransform: Invalid MRML scene or RT Beam node!";
    return;
  }

  beamNode->UpdateBeamTransform();
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateBeamTransformByID(const char* nodeID)
{
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    vtkErrorMacro("UpdateBeamTransformByID: Invalid MRML scene or RT Plan node!");
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  this->RTPlanNode->GetRTBeamNodes(beams);
  // Fill the table
  if (!beams) return;
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode && std::string(beamNode->GetID()) == std::string(nodeID))
    {
      break;
    }
  }

  if (!beamNode) {
    vtkErrorMacro("UpdateBeamTransformByID: Beam with specified ID not found.");
    return;
  }

  UpdateBeamTransform (this->GetMRMLScene(), beamNode);
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::UpdateBeamGeometryModelByID(const char* nodeID)
{
  if ( !this->GetMRMLScene() || !this->RTPlanNode )
  {
    vtkErrorMacro("UpdateBeamGeometryModelByID: Invalid MRML scene or RT Plan node!");
    return;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  this->RTPlanNode->GetRTBeamNodes(beams);
  if (!beams) 
  {
    return;
  }
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode && std::string(beamNode->GetID()) == std::string(nodeID))
    {
      break;
    }
  }

  // Make sure inputs are initialized
  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamGeometryModelByID: beamNode are not initialized!");
    return;
  }

  vtkSmartPointer<vtkPolyData> beamModelPolyData = NULL;
  vtkMRMLDoubleArrayNode* MLCPositionDoubleArrayNode = beamNode->GetMLCPositionDoubleArrayNode();
  if (MLCPositionDoubleArrayNode)
  {
    beamModelPolyData = CreateBeamPolyData(
        beamNode->GetX1Jaw(),
        beamNode->GetX2Jaw(), 
        beamNode->GetY1Jaw(),
        beamNode->GetY2Jaw(),
        beamNode->GetSAD(),
        beamNode->GetMLCPositionDoubleArrayNode()->GetArray());
  }
  else
  {
    beamModelPolyData = CreateBeamPolyData(
        beamNode->GetX1Jaw(),
        beamNode->GetX2Jaw(), 
        beamNode->GetY1Jaw(),
        beamNode->GetY2Jaw(),
        beamNode->GetSAD());
  }

  beamNode->SetAndObservePolyData(beamModelPolyData);
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

//---------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkSlicerBeamsModuleLogic::CreateBeamPolyData(
  double X1, double X2, double Y1, double Y2, double SAD, vtkDoubleArray* leafPositionsArray)
{
  // First we extract the shape of the mlc
  int X2count = X2/10;
  int X1count = X1/10;
  int numLeavesVisible = X2count - (-X1count); // Calculate the number of leaves visible
  int numPointsEachSide = numLeavesVisible *2;

  double Y2LeavePosition[40];
  double Y1LeavePosition[40];

  // Calculate Y2 first
  for (int i = X2count; i >= -X1count; i--)
  {
    double leafPosition = leafPositionsArray->GetComponent(-(i-20), 1);
    if (-leafPosition>-Y2)
    {
      Y2LeavePosition[-(i-20)] = leafPosition;
    }
    else
    {
      Y2LeavePosition[-(i-20)] = Y2;
    }
  }
  // Calculate Y1 next
  for (int i = X2count; i >= -X1count; i--)
  {
    double leafPosition = leafPositionsArray->GetComponent(-(i-20), 0);
    if (leafPosition<Y1)
    {
      Y1LeavePosition[-(i-20)] = leafPosition;
    }
    else
    {
      Y1LeavePosition[-(i-20)] = Y1;
    }
  }

  // Create beam model
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertPoint(0,0,0,SAD);

  int count = 1;
  for (int i = X2count; i > -X1count; i--)
  {
    points->InsertPoint(count,-Y2LeavePosition[-(i-20)]*2, i*10*2, -SAD );
    count ++;
    points->InsertPoint(count,-Y2LeavePosition[-(i-20)]*2, (i-1)*10*2, -SAD );
    count ++;
  }

  for (int i = -X1count; i < X2count; i++)
  {
    points->InsertPoint(count,Y1LeavePosition[-(i-20)]*2, i*10*2, -SAD );
    count ++;
    points->InsertPoint(count,Y1LeavePosition[-(i-20)]*2, (i+1)*10*2, -SAD );
    count ++;
  }

  vtkSmartPointer<vtkCellArray> cellArray = vtkSmartPointer<vtkCellArray>::New();
  for (int i = 1; i <numPointsEachSide; i++)
  {
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(i);
    cellArray->InsertCellPoint(i+1);
  }
  // Add connection between Y2 and Y1
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(numPointsEachSide);
  cellArray->InsertCellPoint(numPointsEachSide+1);
  for (int i = numPointsEachSide+1; i <2*numPointsEachSide; i++)
  {
    cellArray->InsertNextCell(3);
    cellArray->InsertCellPoint(0);
    cellArray->InsertCellPoint(i);
    cellArray->InsertCellPoint(i+1);
  }

  // Add connection between Y2 and Y1
  cellArray->InsertNextCell(3);
  cellArray->InsertCellPoint(0);
  cellArray->InsertCellPoint(2*numPointsEachSide);
  cellArray->InsertCellPoint(1);

  // Add the cap to the bottom
  cellArray->InsertNextCell(2*numPointsEachSide);
  for (int i = 1; i <= 2*numPointsEachSide; i++)
  {
    cellArray->InsertCellPoint(i);
  }

  vtkSmartPointer<vtkPolyData> beamModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  beamModelPolyData->SetPoints(points);
  beamModelPolyData->SetPolys(cellArray);

  return beamModelPolyData;
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode* vtkSlicerBeamsModuleLogic::CreateDefaultRTPlanNode(const char* nodeName)
{
  vtkMRMLScene *scene = this->GetMRMLScene();
  if ( !scene )
  {
    vtkErrorMacro("CreateDefaultRTPlanNode: Invalid MRML scene!");
    return 0;
  }
  // Create RTPlan node
  vtkMRMLRTPlanNode* planNode = vtkMRMLRTPlanNode::New();
  std::string planNodeName = nodeName ;
  planNode->SetName(planNodeName.c_str());
  scene->AddNode(planNode);
  planNode->Delete(); // Return ownership to the scene only

  return planNode;
}

//---------------------------------------------------------------------------
void vtkSlicerBeamsModuleLogic::AddDefaultModelToRTBeamNode(vtkMRMLScene* scene, vtkMRMLRTBeamNode* beamNode)
{
  scene->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkPolyData> beamModelPolyData;
  beamModelPolyData = CreateBeamPolyData(
      100,
      100, 
      100,
      100,
      1000);

  // Transform for visualization
  //TODO: Awful names for transforms. They should be barToFooTransform
  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(0);
  transform->RotateY(0);
  transform->RotateX(-90);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  transform2->Translate(0.0, 0.0, 0.0);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  // Create transform node for beam
  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->AddNode(transformNode));
  transformNode->SetMatrixTransformToParent(transform->GetMatrix());
  transformNode->SetAttribute(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyExcludeFromTreeAttributeName().c_str(), "1");

  // Create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  beamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(scene->AddNode(beamModelDisplayNode));
  beamModelDisplayNode->SetColor(0.0, 1.0, 0.2);
  beamModelDisplayNode->SetOpacity(0.3);
  beamModelDisplayNode->SetBackfaceCulling(0); // Disable backface culling to make the back side of the contour visible as well
  beamModelDisplayNode->VisibilityOn(); 
  beamModelDisplayNode->SliceIntersectionVisibilityOn();

  // Setup beam model node
  beamNode->SetAndObservePolyData(beamModelPolyData);
  beamNode->SetAndObserveTransformNodeID(transformNode->GetID());
  beamNode->SetAndObserveDisplayNodeID(beamModelDisplayNode->GetID());

  scene->EndState(vtkMRMLScene::BatchProcessState); 
}

//---------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerBeamsModuleLogic::CreateDefaultRTBeamNode(const char* beamName)
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("CreateDefaultRTBeamNode: Invalid MRML scene!");
    return 0;
  }

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTBeamNode> beamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  beamNode = vtkMRMLRTBeamNode::SafeDownCast(this->GetMRMLScene()->AddNode(beamNode));
  beamNode->SetName(beamName);
  beamNode->HideFromEditorsOff();

  this->AddDefaultModelToRTBeamNode(this->GetMRMLScene(), beamNode);

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 
  this->Modified();

  return beamNode;
}
