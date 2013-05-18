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

  This file was originally developed by Kevin Wang, Radiation Medicine Program, 
  University Health Network and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// ExternalBeamPlanning includes
#include "vtkSlicerExternalBeamPlanningModuleLogic.h"
#include "vtkMRMLContourNode.h"
#include "vtkMRMLExternalBeamPlanningNode.h"
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanHierarchyNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLScalarVolumeNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkConeSource.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerExternalBeamPlanningModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::vtkSlicerExternalBeamPlanningModuleLogic()
{
  this->ExternalBeamPlanningNode = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerExternalBeamPlanningModuleLogic::~vtkSlicerExternalBeamPlanningModuleLogic()
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetAndObserveExternalBeamPlanningNode(vtkMRMLExternalBeamPlanningNode *node)
{
  vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, node);
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene(); 
  if (!scene)
  {
    return;
  }
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLExternalBeamPlanningNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTBeamNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLRTPlanHierarchyNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->ExternalBeamPlanningNode)
  {
    return;
  }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene() || !this->ExternalBeamPlanningNode)
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
  {
    return;
  }

  if (node->IsA("vtkMRMLScalarVolumeNode") || node->IsA("vtkMRMLExternalBeamPlanningNode"))
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndImport()
{
  // If we have a parameter node select it
  vtkMRMLExternalBeamPlanningNode *paramNode = NULL;
  vtkMRMLNode *node = this->GetMRMLScene()->GetNthNodeByClass(0, "vtkMRMLExternalBeamPlanningNode");
  if (node)
    {
    paramNode = vtkMRMLExternalBeamPlanningNode::SafeDownCast(node);
    vtkSetAndObserveMRMLNodeMacro(this->ExternalBeamPlanningNode, paramNode);
    }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::OnMRMLSceneEndClose()
{
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::CreateBeamPolyData()
{
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::UpdateBeam(char *beamname, double gantryAngle)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }
  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  vtkMRMLAnnotationFiducialNode* IsocenterFiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetIsocenterNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode || !IsocenterFiducialNode )
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);
  // Fill the table
  if (!beams) return;
  vtkMRMLRTBeamNode* beamNode = NULL;
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        //RTPlanNode->RemoveRTBeamNode(beamNode);
        break;
      }
    }
  }

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(gantryAngle);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  IsocenterFiducialNode->GetFiducialCoordinates(isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();

  vtkMRMLLinearTransformNode *transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
             this->GetMRMLScene()->GetNodeByID(beamModelNode->GetTransformNodeID()));
  if (transformNode)
  {
    transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::AddBeam()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  vtkMRMLAnnotationFiducialNode* IsocenterFiducialNode = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetIsocenterNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode || !IsocenterFiducialNode )
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  // Get rtplan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState); 

  // Create beam model
  vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
  double baseRadius = 2;
  coneSource->SetRadius(baseRadius*2.0*sqrt(2.0));
  coneSource->SetHeight(100*2.0);
  coneSource->SetResolution(4);
  coneSource->SetDirection(0,1,0);
  coneSource->Update();

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->RotateZ(0);

  vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
  transform2->Identity();
  double isoCenterPosition[3] = {0.0,0.0,0.0};
  IsocenterFiducialNode->GetFiducialCoordinates(isoCenterPosition);
  transform2->Translate(isoCenterPosition[0], isoCenterPosition[1], isoCenterPosition[2]);

  transform->PostMultiply();
  transform->Concatenate(transform2->GetMatrix());

  vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
  transformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->AddNode(transformNode));
  transformNode->SetAndObserveMatrixTransformToParent(transform->GetMatrix());
  
  // Create rtbeam model node
  vtkSmartPointer<vtkMRMLModelNode> RTBeamModelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  RTBeamModelNode = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelNode));
  //RTBeamNode->SetBeamName("BeamName");
  RTBeamModelNode->SetAndObservePolyData(coneSource->GetOutput());
  RTBeamModelNode->SetAndObserveTransformNodeID(transformNode->GetID());
  RTBeamModelNode->HideFromEditorsOff();

  // create model node for beam
  vtkSmartPointer<vtkMRMLModelDisplayNode> RTBeamModelDisplayNode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  RTBeamModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamModelDisplayNode));
  RTBeamModelDisplayNode->SetColor(0.0, 1.0, 0.0);
  RTBeamModelDisplayNode->SetOpacity(0.3);
  // Disable backface culling to make the back side of the contour visible as well
  RTBeamModelDisplayNode->SetBackfaceCulling(0);
  RTBeamModelDisplayNode->HideFromEditorsOff();
  RTBeamModelDisplayNode->VisibilityOn(); 
  RTBeamModelNode->SetAndObserveDisplayNodeID(RTBeamModelDisplayNode->GetID());
  RTBeamModelDisplayNode->SliceIntersectionVisibilityOn();  

  // Create rtbeam node
  vtkSmartPointer<vtkMRMLRTBeamNode> RTBeamNode = vtkSmartPointer<vtkMRMLRTBeamNode>::New();
  RTBeamNode = vtkMRMLRTBeamNode::SafeDownCast(this->GetMRMLScene()->AddNode(RTBeamNode));
  RTBeamNode->SetBeamName("BeamName");
  RTBeamNode->SetAndObserveBeamModelNodeId(RTBeamModelNode->GetID());
  RTBeamNode->HideFromEditorsOff();

  // Put the RTBeam node in the hierarchy
  RTPlanNode->AddRTBeamNode(RTBeamNode);

  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState); 

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::RemoveBeam(char *beamname)
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  vtkMRMLRTPlanNode* RTPlanNode = vtkMRMLRTPlanNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetRTPlanNodeID()));
  vtkMRMLScalarVolumeNode* referenceVolumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  // Make sure inputs are initialized
  if (!RTPlanNode)
  {
    vtkErrorMacro("RTPlan: inputs are not initialized!")
    return ;
  }

  // Get rtplan hierarchy node
  vtkSmartPointer<vtkMRMLRTPlanHierarchyNode> RTPlanHierarchyRootNode;

  // Find rtbeam node in rtplan hierarchy node
  vtkSmartPointer<vtkCollection> beams = vtkSmartPointer<vtkCollection>::New();
  RTPlanNode->GetRTBeamNodes(beams);

  beams->InitTraversal();
  if (beams->GetNumberOfItems() < 1)
  {
    std::cerr << "Warning: Selected RTPlan node has no children contour nodes!" << std::endl;
    return;
  }

  // Fill the table
  for (int i=0; i<beams->GetNumberOfItems(); ++i)
  {
    vtkMRMLRTBeamNode* beamNode = vtkMRMLRTBeamNode::SafeDownCast(beams->GetItemAsObject(i));
    if (beamNode)
    {
      if ( strcmp(beamNode->GetBeamName(), beamname) == 0)
      {
        vtkSmartPointer<vtkMRMLModelNode> beamModelNode = beamNode->GetBeamModelNode();
        vtkSmartPointer<vtkMRMLModelDisplayNode> beamModelDisplayNode = beamModelNode->GetModelDisplayNode();
        RTPlanNode->RemoveRTBeamNode(beamNode);
        this->GetMRMLScene()->RemoveNode(beamModelNode);
        this->GetMRMLScene()->RemoveNode(beamModelDisplayNode);
        break;
      }
    }
  }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerExternalBeamPlanningModuleLogic::ComputeDose()
{
  if ( !this->GetMRMLScene() || !this->ExternalBeamPlanningNode )
  {
    return;
  }

  // Convert input images to ITK format for Plastimatch
  vtkMRMLVolumeNode* referenceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetReferenceVolumeNodeID()));
  itk::Image<float, 3>::Pointer referenceVolumeItk = itk::Image<float, 3>::New();

  vtkMRMLContourNode* targetContourNode = vtkMRMLContourNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->ExternalBeamPlanningNode->GetProtonTargetVolumeNodeID()));
  vtkMRMLScalarVolumeNode* targetVolumeNode = targetContourNode->GetIndexedLabelmapVolumeNode();
  itk::Image<unsigned char, 3>::Pointer targetVolumeItk = itk::Image<unsigned char, 3>::New();

  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(referenceVolumeNode, referenceVolumeItk);
  SlicerRtCommon::ConvertVolumeNodeToItkImage<unsigned char>(targetVolumeNode, targetVolumeItk);

}
