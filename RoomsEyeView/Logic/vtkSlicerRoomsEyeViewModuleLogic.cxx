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

// RoomsEyeView includes
#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"

// SlicerRT includes
//#include "vtkMRMLRTPlanNode.h"
//#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include "vtkMRMLModelNode.h"

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkSlicerModelsLogic.h>
#include <vtkTransform.h>
#include <SlicerRtCommon.h>
#include <vtkCollisionDetectionFilter.h>
//----------------------------------------------------------------------------
static const char* GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "GantryToFixedReferenceTransform";
static const char* COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME = "CollimatorToGantryTransform";
static const char* LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELORIGIN_TRANSFORM_NODE_NAME = "LeftImagingPanelToLeftImagingPanelOriginTransform";
static const char* LEFTIMAGINGPANELORIGIN_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME = "LeftImagingPanelOriginToLeftImagingPanelRotatedTransform";
static const char* LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME = "LeftImagingPanelTranslationTransform";
static const char* LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME = "LeftImagingPanelRotatedToGantryTransform";
static const char* RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELORIGIN_TRANSFORM_NODE_NAME = "RightImagingPanelToRightImagingPanelOriginTransform";
static const char* RIGHTIMAGINGPANELORIGIN_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME = "RightImagingPanelOriginToRightImagingPanelRotatedTransform";
static const char* RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME = "RightImagingPanelTranslationTransform";
static const char* RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME = "RightImagingPanelRotatedToGantryTransform";
static const char* PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "PatientSupportToFixedReferenceTransform";
static const char* PATIENTSUPPORTSCALED_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME = "PatientSupportScaledToPatientSupportTransform";
static const char* PATIENTSUPPORTCALEDMOVED_TO_PATIENTSUPPORTCOMPRESS_TRANSFORM_NODE_NAME = "PatientSupportScaledMovedToPatientSupportCompressTransform";
static const char* TABLETOPMOVED_TO_PATIENTSUPPORTMOVED_TRANSFORM_NODE_NAME = "TableTopMovedToPatientSupportMovedTransform";
static const char* TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME = "TableTopEccentricRotationToPatientSupportTransform";
static const char* TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME = "TableTopToTableTopEccentricRotationTransform";
vtkMatrix4x4* collimatorToWorld = vtkMatrix4x4::New();
vtkMatrix4x4* tableTopToWorld = vtkMatrix4x4::New();
vtkCollisionDetectionFilter* gantryPatientCollisionDetection = vtkCollisionDetectionFilter::New();
vtkCollisionDetectionFilter* gantryTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
vtkCollisionDetectionFilter* gantryPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();

vtkCollisionDetectionFilter* collimatorPatientCollisionDetection = vtkCollisionDetectionFilter::New();
vtkCollisionDetectionFilter* collimatorTableTopCollisionDetection = vtkCollisionDetectionFilter::New();

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRoomsEyeViewModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkSlicerRoomsEyeViewModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::~vtkSlicerRoomsEyeViewModuleLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  //vtkNew<vtkIntArray> events;
  //events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  //events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  //events->InsertNextValue(vtkMRMLScene::EndCloseEvent);
  //events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  //this->SetAndObserveMRMLSceneEvents(newScene, events.GetPointer());

  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryToFixedReferenceTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode;
  
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelToLeftImagingPanelOriginTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelRotatedToGantryTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelTranslationTransformNode;
 
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelToRightImagingPanelOriginTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelOriginToRightImagingPanelRotatedTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelRotatedToGantryTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelTranslationTransformNode;

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledToPatientSupportTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledMovedToPatientSupportCompressTransformNode;

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopMovedToPatientSupportMovedTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopToTableTopEccentricRotationTransformNode;

  

  // Create transform nodes
  if (newScene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) == NULL)
  {
    gantryToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    gantryToFixedReferenceTransformNode->SetName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    //gantryToFixedReferenceTransformNode->SetSingletonOn();
    newScene->AddNode(gantryToFixedReferenceTransformNode);

    collimatorToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToGantryTransformNode->SetName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME);
    //collimatorToGantryTransformNode->SetSingletonOn();
    newScene->AddNode(collimatorToGantryTransformNode);

    leftImagingPanelToLeftImagingPanelOriginTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelToLeftImagingPanelOriginTransformNode->SetName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELORIGIN_TRANSFORM_NODE_NAME);
    //leftImagingPanelToLeftImagingPanelOriginTransformNode->SetSingletonOn();
    newScene->AddNode(leftImagingPanelToLeftImagingPanelOriginTransformNode);

    leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode->SetName(LEFTIMAGINGPANELORIGIN_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode);

    leftImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelRotatedToGantryTransformNode->SetName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelRotatedToGantryTransformNode);

    leftImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelTranslationTransformNode->SetName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelTranslationTransformNode);

    rightImagingPanelToRightImagingPanelOriginTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelToRightImagingPanelOriginTransformNode->SetName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELORIGIN_TRANSFORM_NODE_NAME);
    //leftImagingPanelToLeftImagingPanelOriginTransformNode->SetSingletonOn();
    newScene->AddNode(rightImagingPanelToRightImagingPanelOriginTransformNode);

    rightImagingPanelOriginToRightImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelOriginToRightImagingPanelRotatedTransformNode->SetName(RIGHTIMAGINGPANELORIGIN_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelOriginToRightImagingPanelRotatedTransformNode);

    rightImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelRotatedToGantryTransformNode->SetName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelRotatedToGantryTransformNode);

    rightImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelTranslationTransformNode->SetName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelTranslationTransformNode);

    patientSupportToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportToFixedReferenceTransformNode->SetName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportToFixedReferenceTransformNode);

    patientSupportScaledToPatientSupportTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledToPatientSupportTransformNode->SetName(PATIENTSUPPORTSCALED_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportScaledToPatientSupportTransformNode);

    patientSupportScaledMovedToPatientSupportCompressTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledMovedToPatientSupportCompressTransformNode->SetName(PATIENTSUPPORTCALEDMOVED_TO_PATIENTSUPPORTCOMPRESS_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportScaledMovedToPatientSupportCompressTransformNode);

    tableTopMovedToPatientSupportMovedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopMovedToPatientSupportMovedTransformNode->SetName(TABLETOPMOVED_TO_PATIENTSUPPORTMOVED_TRANSFORM_NODE_NAME);
    newScene->AddNode(tableTopMovedToPatientSupportMovedTransformNode);

    tableTopEccentricRotationToPatientSupportTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopEccentricRotationToPatientSupportTransformNode->SetName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME);
    newScene->AddNode(tableTopEccentricRotationToPatientSupportTransformNode);

    tableTopToTableTopEccentricRotationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopToTableTopEccentricRotationTransformNode->SetName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(tableTopToTableTopEccentricRotationTransformNode);
  }
  
  else
  {
      gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) );

      collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));

      leftImagingPanelToLeftImagingPanelOriginTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELORIGIN_TRANSFORM_NODE_NAME));

      leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(LEFTIMAGINGPANELORIGIN_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

      leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME) );

      leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));

      rightImagingPanelToRightImagingPanelOriginTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELORIGIN_TRANSFORM_NODE_NAME));

      rightImagingPanelOriginToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANELORIGIN_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

      rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));

      rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));
      
      patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));

      patientSupportScaledToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORTSCALED_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));

      patientSupportScaledMovedToPatientSupportCompressTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORTCALEDMOVED_TO_PATIENTSUPPORTCOMPRESS_TRANSFORM_NODE_NAME));

      tableTopToTableTopEccentricRotationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME));

      tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));

      tableTopMovedToPatientSupportMovedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(TABLETOPMOVED_TO_PATIENTSUPPORTMOVED_TRANSFORM_NODE_NAME));

  }

  collimatorToGantryTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());
  leftImagingPanelToLeftImagingPanelOriginTransformNode->SetAndObserveTransformNodeID(leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode->GetID());
  leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode->SetAndObserveTransformNodeID(leftImagingPanelRotatedToGantryTransformNode->GetID());
  leftImagingPanelRotatedToGantryTransformNode->SetAndObserveTransformNodeID(leftImagingPanelTranslationTransformNode->GetID());
  leftImagingPanelTranslationTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());

  rightImagingPanelToRightImagingPanelOriginTransformNode->SetAndObserveTransformNodeID(rightImagingPanelOriginToRightImagingPanelRotatedTransformNode->GetID());
  rightImagingPanelOriginToRightImagingPanelRotatedTransformNode->SetAndObserveTransformNodeID(rightImagingPanelRotatedToGantryTransformNode->GetID());
  rightImagingPanelRotatedToGantryTransformNode->SetAndObserveTransformNodeID(rightImagingPanelTranslationTransformNode->GetID());
  rightImagingPanelTranslationTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());

  patientSupportScaledMovedToPatientSupportCompressTransformNode->SetAndObserveTransformNodeID(patientSupportScaledToPatientSupportTransformNode->GetID());
  patientSupportScaledToPatientSupportTransformNode->SetAndObserveTransformNodeID(tableTopMovedToPatientSupportMovedTransformNode->GetID());
  tableTopMovedToPatientSupportMovedTransformNode->SetAndObserveTransformNodeID(patientSupportToFixedReferenceTransformNode->GetID());

  tableTopEccentricRotationToPatientSupportTransformNode->SetAndObserveTransformNodeID(patientSupportToFixedReferenceTransformNode->GetID());
  tableTopToTableTopEccentricRotationTransformNode->SetAndObserveTransformNodeID(tableTopEccentricRotationToPatientSupportTransformNode->GetID());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(tableTopToWorld);

  // Set transform nodes to parameter set nodes
  std::vector<vtkMRMLNode*> roomsEyeViewNodes;
  unsigned int numberOfNodes = newScene->GetNodesByClass("vtkMRMLRoomsEyeViewNode", roomsEyeViewNodes);
  for (unsigned int nodeIndex = 0; nodeIndex < numberOfNodes; nodeIndex++)
  {
    vtkMRMLRoomsEyeViewNode* node = vtkMRMLRoomsEyeViewNode::SafeDownCast(roomsEyeViewNodes[nodeIndex]);
    node->SetAndObserveGantryToFixedReferenceTransformNode(gantryToFixedReferenceTransformNode);
    node->SetAndObserveCollimatorToGantryTransformNode(collimatorToGantryTransformNode);
    
    node->SetAndObserveLeftImagingPanelToLeftImagingPanelOriginTransformNode(leftImagingPanelToLeftImagingPanelOriginTransformNode);
    node->SetAndObserveLeftImagingPanelOriginToLeftImagingPanelRotatedTransformNode(leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode);
    node->SetAndObserveLeftImagingPanelRotatedToGantryTransformNode(leftImagingPanelRotatedToGantryTransformNode);
    node->SetAndObserveLeftImagingPanelTranslationTransformNode(leftImagingPanelTranslationTransformNode);

    node->SetAndObserveRightImagingPanelToRightImagingPanelOriginTransformNode(rightImagingPanelToRightImagingPanelOriginTransformNode);
    node->SetAndObserveRightImagingPanelOriginToRightImagingPanelRotatedTransformNode(rightImagingPanelOriginToRightImagingPanelRotatedTransformNode);
    node->SetAndObserveRightImagingPanelRotatedToGantryTransformNode(rightImagingPanelRotatedToGantryTransformNode);
    node->SetAndObserveRightImagingPanelTranslationTransformNode(rightImagingPanelTranslationTransformNode);
  
    node->SetAndObservePatientSupportToFixedReferenceTransformNode(patientSupportToFixedReferenceTransformNode);
    node->SetAndObservePatientSupportScaledToPatientSupportTransformNode(patientSupportScaledToPatientSupportTransformNode);
    node->SetAndObservePatientSupportScaledMovedToPatientSupportCompressTransformNode(patientSupportScaledMovedToPatientSupportCompressTransformNode);
    
    node->SetAndObserveTableTopToTableTopEccentricRotationTransformNode(tableTopToTableTopEccentricRotationTransformNode);
    node->SetAndObserveTableTopEccentricRotationToPatientSupportTransformNode(tableTopEccentricRotationToPatientSupportTransformNode);
    node->SetAndObserveTableMovedToPatientSupportMovedTransformNode(tableTopMovedToPatientSupportMovedTransformNode);
  }
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LoadLinacModels(vtkMRMLScene* scene, const char* path)
{
  vtkSlicerModelsLogic* modelsLogic = vtkSlicerModelsLogic::New();
  modelsLogic->SetMRMLScene(scene);
  modelsLogic->AddModel(path);

}

void vtkSlicerRoomsEyeViewModuleLogic::ModelToParentTransforms(vtkMRMLScene* scene)
{
  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("gantryModel"));
  vtkMRMLLinearTransformNode* gantryModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("GantryToFixedReferenceTransform"));
  gantryModel->SetAndObserveTransformNodeID(gantryModelTransforms->GetID());
  gantryModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* gantryModelDisplayNode = gantryModel->GetDisplayNode();
  gantryModelDisplayNode->SetColor(1, 1, 0);

  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("collimatorModel"));
  vtkMRMLLinearTransformNode* collimatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  collimatorModel->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());
  collimatorModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* collimatorModelDisplayNode = collimatorModel->GetDisplayNode();
  collimatorModelDisplayNode->SetColor(1, 0, 0);

  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("leftImagingPanelModel"));
  vtkMRMLLinearTransformNode* leftImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelToLeftImagingPanelOriginTransform"));
  leftImagingPanelModel->SetAndObserveTransformNodeID(leftImagingPanelModelTransforms->GetID());
  leftImagingPanelModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* leftImagingPanelModelDisplayNode = leftImagingPanelModel->GetDisplayNode();
  leftImagingPanelModelDisplayNode->SetColor(1, 0, 1);

  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("rightImagingPanelModel"));
  vtkMRMLLinearTransformNode* rightImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelToRightImagingPanelOriginTransform"));
  rightImagingPanelModel->SetAndObserveTransformNodeID(rightImagingPanelModelTransforms->GetID());
  rightImagingPanelModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* rightImagingPanelModelDisplayNode = rightImagingPanelModel->GetDisplayNode();
  rightImagingPanelModelDisplayNode->SetColor(1, 0, 1);

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("patientSupportModel"));
  vtkMRMLLinearTransformNode* patientSupportModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportScaledMovedToPatientSupportCompressTransform"));
  patientSupportModel->SetAndObserveTransformNodeID(patientSupportModelTransforms->GetID());
  patientSupportModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* patientSupportModelDisplayNode = patientSupportModel->GetDisplayNode();
  patientSupportModelDisplayNode->SetColor(0, 1, 0);

  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("tableTopModel"));
  vtkMRMLLinearTransformNode* tableTopModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  tableTopModel->SetAndObserveTransformNodeID(tableTopModelTransforms->GetID());
  tableTopModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* tableTopModelDisplayNode = tableTopModel->GetDisplayNode();
  tableTopModelDisplayNode->SetColor(0, 0, 1);

  vtkMRMLModelNode* patientModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("patientModel"));
  vtkMRMLLinearTransformNode* patientModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  patientModel->SetAndObserveTransformNodeID(patientModelTransforms->GetID());
  patientModel->CreateDefaultDisplayNodes();
  vtkMRMLDisplayNode* patientModelDisplayNode = patientModel->GetDisplayNode();
  patientModelDisplayNode->SetColor(0, 1, 1);


}

void vtkSlicerRoomsEyeViewModuleLogic::CollimatorRotationValueChanged(vtkMRMLScene* scene, double rotationAngle)
{
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransform->Identity();
  double implicitCenterOfRotation[3] = { 3.652191162109375, 8.4510498046875, 528.0714111328125 };
  double actualCenterOfRotation[3] = { -0.041, 0.094, 528.0714111328125 };
  double implicitCenterToActualCenterDisplacement[3] = { actualCenterOfRotation[0] - implicitCenterOfRotation[0], actualCenterOfRotation[1] - implicitCenterOfRotation[1], actualCenterOfRotation[2] - implicitCenterOfRotation[2] };
  collimatorToGantryTransform->Translate(implicitCenterToActualCenterDisplacement);
  collimatorToGantryTransform->RotateZ(rotationAngle);
  collimatorToGantryTransform->Translate(implicitCenterToActualCenterDisplacement);
  collimatorToGantryTransform->Modified();

}

void vtkSlicerRoomsEyeViewModuleLogic::GantryRotationValueChanged(vtkMRMLScene* scene, double rotationAngle)
{
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("GantryToFixedReferenceTransform"));
  vtkTransform* gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent());

  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(rotationAngle);
  gantryToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);

}

void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelToOrigin(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* leftImagingPanelToLeftImagingPanelOriginTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelToLeftImagingPanelOriginTransform"));
  vtkTransform* leftImagingPanelToLeftImagingPanelOriginTransform = vtkTransform::SafeDownCast(leftImagingPanelToLeftImagingPanelOriginTransformNode->GetTransformToParent());
  leftImagingPanelToLeftImagingPanelOriginTransform->Identity();
  double translationArray[3] = { 656.606, -1518.434, -345.164 };
  leftImagingPanelToLeftImagingPanelOriginTransform->Translate(translationArray);
  leftImagingPanelToLeftImagingPanelOriginTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelOriginToLeftImagingPanelRotated(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelOriginToLeftImagingPanelRotatedTransform"));
  vtkTransform* leftImagingPanelOriginToLeftImagingPanelRotatedTransform= vtkTransform::SafeDownCast(leftImagingPanelOriginToLeftImagingPanelRotatedTransformNode->GetTransformToParent());
  leftImagingPanelOriginToLeftImagingPanelRotatedTransform->Identity();
  leftImagingPanelOriginToLeftImagingPanelRotatedTransform->RotateZ(panelMovement+68.5);
  leftImagingPanelOriginToLeftImagingPanelRotatedTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelRotatedToGantry(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelRotatedToGantryTransform"));
  vtkTransform* leftImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(leftImagingPanelRotatedToGantryTransformNode->GetTransformToParent());
  leftImagingPanelRotatedToGantryTransform->Identity();
  double translationArray[3] = { -656.606, 1518.434, 345.164 };
  leftImagingPanelRotatedToGantryTransform->Translate(translationArray);
  leftImagingPanelRotatedToGantryTransform ->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelTranslation(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelTranslationTransform"));
  vtkTransform* leftImagingPanelTranslationTransform = vtkTransform::SafeDownCast(leftImagingPanelTranslationTransformNode->GetTransformToParent());
  leftImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  leftImagingPanelTranslationTransform->Translate(translationArray);
  leftImagingPanelTranslationTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelToOrigin(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* rightImagingPanelToRightImagingPanelOriginTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelToRightImagingPanelOriginTransform"));
  vtkTransform* rightImagingPanelToRightImagingPanelOriginTransform = vtkTransform::SafeDownCast(rightImagingPanelToRightImagingPanelOriginTransformNode->GetTransformToParent());
  rightImagingPanelToRightImagingPanelOriginTransform->Identity();
  double translationArray[3] = { -649.763, -1504.412, -342.200 };
  rightImagingPanelToRightImagingPanelOriginTransform->Translate(translationArray);
  rightImagingPanelToRightImagingPanelOriginTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelOriginToRightImagingPanelRotated(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* rightImagingPanelOriginToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelOriginToRightImagingPanelRotatedTransform"));
  vtkTransform* rightImagingPanelOriginToRightImagingPanelRotatedTransform = vtkTransform::SafeDownCast(rightImagingPanelOriginToRightImagingPanelRotatedTransformNode->GetTransformToParent());
  rightImagingPanelOriginToRightImagingPanelRotatedTransform->Identity();
  rightImagingPanelOriginToRightImagingPanelRotatedTransform->RotateZ(-(68.5+panelMovement));
  rightImagingPanelOriginToRightImagingPanelRotatedTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelRotatedToGantry(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelRotatedToGantryTransform"));
  vtkTransform* rightImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(rightImagingPanelRotatedToGantryTransformNode->GetTransformToParent());
  rightImagingPanelRotatedToGantryTransform->Identity();
  double translationArray[3] = { 649.763, 1504.412, 342.200 };
  rightImagingPanelRotatedToGantryTransform->Translate(translationArray);
  rightImagingPanelRotatedToGantryTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelTranslation(vtkMRMLScene* scene, double panelMovement)
{
  vtkMRMLLinearTransformNode* rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelTranslationTransform"));
  vtkTransform* rightImagingPanelTranslationTransform = vtkTransform::SafeDownCast(rightImagingPanelTranslationTransformNode->GetTransformToParent());
  rightImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  rightImagingPanelTranslationTransform->Translate(translationArray);
  rightImagingPanelTranslationTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(collimatorToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::ImagingPanelMovementValueChanged(vtkMRMLScene* scene, double panelMovement)
{
  if (panelMovement < 0)
  {
    LeftImagingPanelToOrigin(scene, panelMovement);
    LeftImagingPanelOriginToLeftImagingPanelRotated(scene, panelMovement);
    LeftImagingPanelRotatedToGantry(scene, panelMovement);
    RightImagingPanelToOrigin(scene, panelMovement);
    RightImagingPanelOriginToRightImagingPanelRotated(scene, panelMovement);
    RightImagingPanelRotatedToGantry(scene, panelMovement);
  }
  else
  {
    LeftImagingPanelTranslation(scene, panelMovement);
    RightImagingPanelTranslation(scene, panelMovement);
  }
}

void vtkSlicerRoomsEyeViewModuleLogic::PatientSupportRotationValueChanged(vtkMRMLScene* scene, double rotationAngle)
{
  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportToFixedReferenceTransform"));
  vtkTransform* patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(patientSupportToFixedReferenceTransformNode->GetTransformToParent());

  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(rotationAngle);
  patientSupportToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(tableTopToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::TableTopDisplacementValueChanged(vtkMRMLScene* scene, double lattableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement)
{
  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransform->Identity();
  double translationArray[3] = { lattableTopDisplacement, longTableTopDisplacement, vertTableTopDisplacement };
  tableTopEccentricRotationToPatientSupportTransform->Translate(translationArray);
  tableTopEccentricRotationToPatientSupportTransform->Modified();

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(tableTopToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::TableTopMovedPatientSupportMoved(vtkMRMLScene* scene, double tableTopDisplacement)
{
  vtkMRMLLinearTransformNode* tableTopMovedToPatientSupportMovedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopMovedToPatientSupportMovedTransform"));
  vtkTransform* tableTopMovedToPatientSupportMovedTransform = vtkTransform::SafeDownCast(tableTopMovedToPatientSupportMovedTransformNode->GetTransformToParent());

  tableTopMovedToPatientSupportMovedTransform->Identity();
  double translationArray[3] = { 0, 0, -1359.469848632281225 };
  tableTopMovedToPatientSupportMovedTransform->Translate(translationArray);
  tableTopMovedToPatientSupportMovedTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(tableTopToWorld);


}

void vtkSlicerRoomsEyeViewModuleLogic::TableTopDisplacementPatientSupportChanged(vtkMRMLScene* scene, double tableTopDisplacement)
{
  vtkMRMLLinearTransformNode* patientSupportScaledToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportScaledToPatientSupportTransform"));
  vtkTransform* patientSupportScaledToPatientSupportTransform = vtkTransform::SafeDownCast(patientSupportScaledToPatientSupportTransformNode->GetTransformToParent());

  patientSupportScaledToPatientSupportTransform->Identity();
  patientSupportScaledToPatientSupportTransform->Scale(1, 1, ((906 + tableTopDisplacement*1.01)) / 900);
  patientSupportScaledToPatientSupportTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(tableTopToWorld);
}

void vtkSlicerRoomsEyeViewModuleLogic::PatientSupportScaledMovedToPatientSupportCompress(vtkMRMLScene* scene, double tableTopDisplacement)
{
  vtkMRMLLinearTransformNode* patientSupportScaledMovedToPatientSupportCompressTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportScaledMovedToPatientSupportCompressTransform"));
  vtkTransform* patientSupportScaledMovedToPatientSupportCompressTransform = vtkTransform::SafeDownCast(patientSupportScaledMovedToPatientSupportCompressTransformNode->GetTransformToParent());

  patientSupportScaledMovedToPatientSupportCompressTransform->Identity();
  double translationArray[3] = { 0, 0, 1359.469848632281225 };
  patientSupportScaledMovedToPatientSupportCompressTransform->Translate(translationArray);
  patientSupportScaledMovedToPatientSupportCompressTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(tableTopToWorld);

}

void vtkSlicerRoomsEyeViewModuleLogic::VerticalDisplacementValueChanged(vtkMRMLScene* scene, double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  TableTopDisplacementValueChanged(scene, latDisplacementValue, longDisplacementValue, vertDisplacementValue);
  TableTopMovedPatientSupportMoved(scene, vertDisplacementValue);
  TableTopDisplacementPatientSupportChanged(scene, vertDisplacementValue);
  PatientSupportScaledMovedToPatientSupportCompress(scene, vertDisplacementValue);
}

void vtkSlicerRoomsEyeViewModuleLogic::LongitudinalDisplacementValueChanged(vtkMRMLScene* scene, double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  TableTopDisplacementValueChanged(scene, latDisplacementValue, longDisplacementValue, vertDisplacementValue);
}


void vtkSlicerRoomsEyeViewModuleLogic::LateralDisplacementValueChanged(vtkMRMLScene* scene, double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  TableTopDisplacementValueChanged(scene, latDisplacementValue, longDisplacementValue, vertDisplacementValue);
}
