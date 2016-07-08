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
/*TODO: Add collision detection between collimator and patient support, imaging panels and patient/patient support
        Get rid of all hard coded numbers in transforms -- calculate center of objects


*/
// RoomsEyeView includes
#include "vtkSlicerRoomsEyeViewModuleLogic.h"
#include "vtkMRMLRoomsEyeViewNode.h"



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

// SlicerRT includes
//#include "C:\d\SlicerRT\Beams\MRML\vtkMRMLRTPlanNode.h"
//#include "C:\d\SlicerRT\Beams\MRML\vtkMRMLRTBeamNode.h"
//#include "C:\d\SlicerRT\Beams\MRML\vtkMRMLRTProtonBeamNode.h"

//----------------------------------------------------------------------------
static const char* GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "GantryToFixedReferenceTransform";
static const char* COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME = "CollimatorToGantryTransform";
static const char* LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform";
static const char* LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME = "LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform";
static const char* LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME = "LeftImagingPanelTranslationTransform";
static const char* LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME = "LeftImagingPanelRotatedToGantryTransform";
static const char* RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform";
static const char* RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME = "RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform";
static const char* RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME = "RightImagingPanelTranslationTransform";
static const char* RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME = "RightImagingPanelRotatedToGantryTransform";
static const char* PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "PatientSupportToFixedReferenceTransform";
static const char* PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME = "PatientSupportScaledByTableTopVerticalMovementTransform";
static const char* PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME = "PatientSupportPositiveVerticalTranslationTransform";
static const char* PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME = "PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform";
static const char* TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME = "TableTopEccentricRotationToPatientSupportTransform";
static const char* TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME = "TableTopToTableTopEccentricRotationTransform";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerRoomsEyeViewModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::vtkSlicerRoomsEyeViewModuleLogic()
  : CollimatorToWorldTransformMatrix(NULL)
  , TableTopToWorldTransformMatrix(NULL)
  , GantryPatientCollisionDetection(NULL)
  , GantryTableTopCollisionDetection(NULL)
  , GantryPatientSupportCollisionDetection(NULL)
  , CollimatorPatientCollisionDetection(NULL)
  , CollimatorTableTopCollisionDetection(NULL)
{
  this->CollimatorToWorldTransformMatrix = vtkMatrix4x4::New();
  this->TableTopToWorldTransformMatrix = vtkMatrix4x4::New();

  this->GantryPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
}

//----------------------------------------------------------------------------
vtkSlicerRoomsEyeViewModuleLogic::~vtkSlicerRoomsEyeViewModuleLogic()
{
 
  if (this->CollimatorToWorldTransformMatrix)
  {
    this->CollimatorToWorldTransformMatrix->Delete();
    this->CollimatorToWorldTransformMatrix = NULL;
  }
  if (this->TableTopToWorldTransformMatrix)
  {
    this->TableTopToWorldTransformMatrix->Delete();
    this->TableTopToWorldTransformMatrix = NULL;
  }

  if (this->GantryPatientCollisionDetection)
  {
    this->GantryPatientCollisionDetection->Delete();
    this->GantryPatientCollisionDetection = NULL;
  }
  if (this->GantryTableTopCollisionDetection)
  {
    this->GantryTableTopCollisionDetection->Delete();
    this->GantryTableTopCollisionDetection = NULL;
  }
  if (this->GantryPatientSupportCollisionDetection)
  {
    this->GantryPatientSupportCollisionDetection->Delete();
    this->GantryPatientSupportCollisionDetection = NULL;
  }
  if (this->CollimatorPatientCollisionDetection)
  {
    this->CollimatorPatientCollisionDetection->Delete();
    this->CollimatorPatientCollisionDetection = NULL;
  }
  if (this->CollimatorTableTopCollisionDetection)
  {
    this->CollimatorTableTopCollisionDetection->Delete();
    this->CollimatorTableTopCollisionDetection = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryToFixedReferenceTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode;
  
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelRotatedToGantryTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelTranslationTransformNode;
 
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelRotatedToGantryTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelTranslationTransformNode;

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledByTableTopVerticalMovementTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportPositiveVerticalTranslationTransformNode;

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopToTableTopEccentricRotationTransformNode;

  // Create transform nodes
  if (newScene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) == NULL)
  {
    gantryToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    gantryToFixedReferenceTransformNode->SetName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    newScene->AddNode(gantryToFixedReferenceTransformNode);

    collimatorToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToGantryTransformNode->SetName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME);
    newScene->AddNode(collimatorToGantryTransformNode);

    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->SetName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode);

    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->SetName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode);

    leftImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelRotatedToGantryTransformNode->SetName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelRotatedToGantryTransformNode);

    leftImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelTranslationTransformNode->SetName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(leftImagingPanelTranslationTransformNode);

    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->SetName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode);

    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->SetName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode);

    rightImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelRotatedToGantryTransformNode->SetName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelRotatedToGantryTransformNode);

    rightImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelTranslationTransformNode->SetName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(rightImagingPanelTranslationTransformNode);

    patientSupportToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportToFixedReferenceTransformNode->SetName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportToFixedReferenceTransformNode);

    patientSupportScaledByTableTopVerticalMovementTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledByTableTopVerticalMovementTransformNode->SetName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportScaledByTableTopVerticalMovementTransformNode);

    patientSupportPositiveVerticalTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportPositiveVerticalTranslationTransformNode->SetName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportPositiveVerticalTranslationTransformNode);

    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->SetName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME);
    newScene->AddNode(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode);

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

      leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

      leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

      leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME) );

      leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));

      rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

      rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

      rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));

      rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));
      
      patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));

      patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME));

      patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME));

      tableTopToTableTopEccentricRotationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME));

      tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));

      patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
        newScene->GetFirstNodeByName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME));

  }
  // Organize transforms into hierarchy based on IEC Standard 61217
  collimatorToGantryTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->SetAndObserveTransformNodeID(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->GetID());
  leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->SetAndObserveTransformNodeID(leftImagingPanelRotatedToGantryTransformNode->GetID());
  leftImagingPanelRotatedToGantryTransformNode->SetAndObserveTransformNodeID(leftImagingPanelTranslationTransformNode->GetID());
  leftImagingPanelTranslationTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());

  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->SetAndObserveTransformNodeID(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->GetID());
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->SetAndObserveTransformNodeID(rightImagingPanelRotatedToGantryTransformNode->GetID());
  rightImagingPanelRotatedToGantryTransformNode->SetAndObserveTransformNodeID(rightImagingPanelTranslationTransformNode->GetID());
  rightImagingPanelTranslationTransformNode->SetAndObserveTransformNodeID(gantryToFixedReferenceTransformNode->GetID());

  patientSupportPositiveVerticalTranslationTransformNode->SetAndObserveTransformNodeID(patientSupportScaledByTableTopVerticalMovementTransformNode->GetID());
  patientSupportScaledByTableTopVerticalMovementTransformNode->SetAndObserveTransformNodeID(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->GetID());
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->SetAndObserveTransformNodeID(patientSupportToFixedReferenceTransformNode->GetID());

  tableTopEccentricRotationToPatientSupportTransformNode->SetAndObserveTransformNodeID(patientSupportToFixedReferenceTransformNode->GetID());
  tableTopToTableTopEccentricRotationTransformNode->SetAndObserveTransformNodeID(tableTopEccentricRotationToPatientSupportTransformNode->GetID());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);

  // Set transform nodes to parameter set nodes
  std::vector<vtkMRMLNode*> roomsEyeViewNodes;
  unsigned int numberOfNodes = newScene->GetNodesByClass("vtkMRMLRoomsEyeViewNode", roomsEyeViewNodes);
  for (unsigned int nodeIndex = 0; nodeIndex < numberOfNodes; nodeIndex++)
  {
    vtkSmartPointer<vtkMRMLRoomsEyeViewNode> node = vtkMRMLRoomsEyeViewNode::SafeDownCast(roomsEyeViewNodes[nodeIndex]);
    node->SetAndObserveGantryToFixedReferenceTransformNode(gantryToFixedReferenceTransformNode);
    node->SetAndObserveCollimatorToGantryTransformNode(collimatorToGantryTransformNode);
    
    node->SetAndObserveLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode);
    node->SetAndObserveLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode);
    node->SetAndObserveLeftImagingPanelRotatedToGantryTransformNode(leftImagingPanelRotatedToGantryTransformNode);
    node->SetAndObserveLeftImagingPanelTranslationTransformNode(leftImagingPanelTranslationTransformNode);

    node->SetAndObserveRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode);
    node->SetAndObserveRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode);
    node->SetAndObserveRightImagingPanelRotatedToGantryTransformNode(rightImagingPanelRotatedToGantryTransformNode);
    node->SetAndObserveRightImagingPanelTranslationTransformNode(rightImagingPanelTranslationTransformNode);
  
    node->SetAndObservePatientSupportToFixedReferenceTransformNode(patientSupportToFixedReferenceTransformNode);
    node->SetAndObservePatientSupportScaledByTableTopVerticalMovementTransformNode(patientSupportScaledByTableTopVerticalMovementTransformNode);
    node->SetAndObservePatientSupportPositiveVerticalTranslationTransformNode(patientSupportPositiveVerticalTranslationTransformNode);
    
    node->SetAndObserveTableTopToTableTopEccentricRotationTransformNode(tableTopToTableTopEccentricRotationTransformNode);
    node->SetAndObserveTableTopEccentricRotationToPatientSupportTransformNode(tableTopEccentricRotationToPatientSupportTransformNode);
    node->SetAndObservePatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode);
  }
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LoadLinacModels(vtkMRMLScene* scene, const char* path)
{
  // Adds piece of treamtent room to MRMLScene based on file path provided
  vtkSmartPointer <vtkSlicerModelsLogic> modelsLogic = vtkSmartPointer<vtkSlicerModelsLogic>::New();
  modelsLogic->SetMRMLScene(scene);
  modelsLogic->AddModel(path);
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::ModelToParentTransforms(vtkMRMLScene* scene)
{
  // Displays all pieces of the treatment room and sets each piece a color to provide realistic representation
  vtkSmartPointer<vtkMRMLModelNode> gantryModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("gantryModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("GantryToFixedReferenceTransform"));
  gantryModel->SetAndObserveTransformNodeID(gantryModelTransforms->GetID());
  gantryModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> gantryModelDisplayNode = gantryModel->GetDisplayNode();
  gantryModelDisplayNode->SetColor(0.95, 0.95, 0.95);

  vtkSmartPointer<vtkMRMLModelNode> collimatorModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("collimatorModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  collimatorModel->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());
  collimatorModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> collimatorModelDisplayNode = collimatorModel->GetDisplayNode();
  collimatorModelDisplayNode->SetColor(0.7, 0.7, 0.95);

  vtkSmartPointer<vtkMRMLModelNode> leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("leftImagingPanelModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform"));
  leftImagingPanelModel->SetAndObserveTransformNodeID(leftImagingPanelModelTransforms->GetID());
  leftImagingPanelModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> leftImagingPanelModelDisplayNode = leftImagingPanelModel->GetDisplayNode();
  leftImagingPanelModelDisplayNode->SetColor(0.95, 0.95, 0.95);

  vtkSmartPointer<vtkMRMLModelNode> rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("rightImagingPanelModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform"));
  rightImagingPanelModel->SetAndObserveTransformNodeID(rightImagingPanelModelTransforms->GetID());
  rightImagingPanelModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> rightImagingPanelModelDisplayNode = rightImagingPanelModel->GetDisplayNode();
  rightImagingPanelModelDisplayNode->SetColor(0.95, 0.95, 0.95);

  vtkSmartPointer<vtkMRMLModelNode> patientSupportModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("patientSupportModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportPositiveVerticalTranslationTransform"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransform = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportToFixedReferenceTransform"));
  patientSupportModel->SetAndObserveTransformNodeID(patientSupportModelTransforms->GetID());
  patientSupportModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> patientSupportModelDisplayNode = patientSupportModel->GetDisplayNode();
  patientSupportModelDisplayNode->SetColor(0.85, 0.85, 0.85);

  vtkSmartPointer<vtkMRMLModelNode> tableTopModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("tableTopModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  tableTopModel->SetAndObserveTransformNodeID(tableTopModelTransforms->GetID());
  tableTopModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> tableTopModelDisplayNode = tableTopModel->GetDisplayNode();
  tableTopModelDisplayNode->SetColor(0, 0, 0);

  vtkSmartPointer<vtkMRMLModelNode> patientModel = vtkMRMLModelNode::SafeDownCast(scene->GetFirstNodeByName("patientModel"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  patientModel->SetAndObserveTransformNodeID(patientModelTransforms->GetID());
  patientModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> patientModelDisplayNode = patientModel->GetDisplayNode();
  patientModelDisplayNode->SetColor(0, 1, 1);

  // Sets up collision detection between pieces of the treatment room
  this->GantryPatientCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientCollisionDetection->SetInput(1, patientModel->GetPolyData());
  this->GantryPatientCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()));
  this->GantryPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryPatientCollisionDetection->Update();

  this->GantryTableTopCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()));
  this->GantryTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryTableTopCollisionDetection->Update();

  this->GantryPatientSupportCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetTransform(0, vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()));
  this->GantryPatientSupportCollisionDetection->SetTransform(1, vtkLinearTransform::SafeDownCast(patientSupportToFixedReferenceTransform->GetTransformToParent()));
  this->GantryPatientSupportCollisionDetection->Update();

  this->CollimatorPatientCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorPatientCollisionDetection->SetInput(1, patientModel->GetPolyData());
  this->CollimatorPatientCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->Update();

  this->CollimatorTableTopCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorTableTopCollisionDetection->Update();
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::CollimatorRotationValueChanged(vtkMRMLScene* scene, double rotationAngle)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  // Translates collimator to actual center of rotation and then rotates based on rotationAngle
  // TODO: Look into why translating the object twice prevents wobble
  collimatorToGantryTransform->Identity();
  double implicitCenterOfRotation[3] = { 3.652191162109375, 8.4510498046875, 528.0714111328125 };
  double actualCenterOfRotation[3] = { -0.041, 0.094, 528.0714111328125 };
  double implicitCenterToActualCenterDisplacement[3] = { actualCenterOfRotation[0] - implicitCenterOfRotation[0], actualCenterOfRotation[1] - implicitCenterOfRotation[1], actualCenterOfRotation[2] - implicitCenterOfRotation[2] };
  collimatorToGantryTransform->Translate(implicitCenterToActualCenterDisplacement);
  collimatorToGantryTransform->RotateZ(rotationAngle);
  collimatorToGantryTransform->Translate(implicitCenterToActualCenterDisplacement);
  collimatorToGantryTransform->Modified();
  CheckForCollisions();
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::GantryRotationValueChanged(vtkMRMLScene* scene, double rotationAngle)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("GantryToFixedReferenceTransform"));
  vtkSmartPointer<vtkTransform> gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent());

  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(rotationAngle);
  gantryToFixedReferenceTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform"));
  vtkSmartPointer<vtkTransform> leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->GetTransformToParent());
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Identity();
  double translationArray[3] = { 656.606, -1518.434, -345.164 };
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Translate(translationArray);
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform"));
  vtkSmartPointer<vtkTransform> LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform= vtkTransform::SafeDownCast(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->GetTransformToParent());
  LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->Identity();
  LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->RotateZ(panelMovement+68.5);
  LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelRotatedToGantry(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelRotatedToGantryTransform"));
  vtkSmartPointer<vtkTransform> leftImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(leftImagingPanelRotatedToGantryTransformNode->GetTransformToParent());
  leftImagingPanelRotatedToGantryTransform->Identity();
  double translationArray[3] = { -656.606, 1518.434, 345.164 };
  leftImagingPanelRotatedToGantryTransform->Translate(translationArray);
  leftImagingPanelRotatedToGantryTransform ->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LeftImagingPanelTranslation(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("LeftImagingPanelTranslationTransform"));
  vtkSmartPointer<vtkTransform> leftImagingPanelTranslationTransform = vtkTransform::SafeDownCast(leftImagingPanelTranslationTransformNode->GetTransformToParent());
  leftImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  leftImagingPanelTranslationTransform->Translate(translationArray);
  leftImagingPanelTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelToRightImagingPanelFixedReferenceIsocenter(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->GetTransformToParent());
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Identity();
  double translationArray[3] = { -649.763, -1504.412, -342.200 };
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Translate(translationArray);
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform = vtkTransform::SafeDownCast(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->GetTransformToParent());
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->Identity();
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->RotateZ(-(68.5+panelMovement));
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelRotatedToGantry(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelRotatedToGantryTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(rightImagingPanelRotatedToGantryTransformNode->GetTransformToParent());
  rightImagingPanelRotatedToGantryTransform->Identity();
  double translationArray[3] = { 649.763, 1504.412, 342.200 };
  rightImagingPanelRotatedToGantryTransform->Translate(translationArray);
  rightImagingPanelRotatedToGantryTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::RightImagingPanelTranslation(vtkMRMLScene* scene, double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("RightImagingPanelTranslationTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelTranslationTransform = vtkTransform::SafeDownCast(rightImagingPanelTranslationTransformNode->GetTransformToParent());
  rightImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  rightImagingPanelTranslationTransform->Translate(translationArray);
  rightImagingPanelTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::ImagingPanelMovementValueChanged(vtkMRMLScene* scene, double panelMovement)
{
  if (panelMovement < 0)
  {
    LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter(scene, panelMovement);
    LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated(scene, panelMovement);
    LeftImagingPanelRotatedToGantry(scene, panelMovement);
    RightImagingPanelToRightImagingPanelFixedReferenceIsocenter(scene, panelMovement);
    RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated(scene, panelMovement);
    RightImagingPanelRotatedToGantry(scene, panelMovement);
  }
  else
  {
    LeftImagingPanelTranslation(scene, panelMovement);
    RightImagingPanelTranslation(scene, panelMovement);
  }
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PatientSupportRotationValueChanged(vtkMRMLScene* scene, double rotationAngle)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportToFixedReferenceTransform"));
  vtkSmartPointer<vtkTransform> patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(patientSupportToFixedReferenceTransformNode->GetTransformToParent());

  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(rotationAngle);
  patientSupportToFixedReferenceTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkSmartPointer<vtkTransform> tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::TableTopDisplacementValueChanged(vtkMRMLScene* scene, double lattableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkSmartPointer<vtkTransform> tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransform->Identity();
  double translationArray[3] = { lattableTopDisplacement, longTableTopDisplacement, vertTableTopDisplacement };
  tableTopEccentricRotationToPatientSupportTransform->Translate(translationArray);
  tableTopEccentricRotationToPatientSupportTransform->Modified();

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PatientSupportScaledTranslatedToTableTopVerticalTranslation(vtkMRMLScene* scene, double tableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform"));
  vtkSmartPointer<vtkTransform> patientSupportScaledTranslatedToTableTopVerticalTranslationTransform = vtkTransform::SafeDownCast(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->GetTransformToParent());

  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, -1359.469848632281225 };
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Translate(translationArray);
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkSmartPointer<vtkTransform> tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::TableTopDisplacementPatientSupportChanged(vtkMRMLScene* scene, double tableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportScaledByTableTopVerticalMovementTransform"));
  vtkTransform* patientSupportScaledByTableTopVerticalMovementTransform = vtkTransform::SafeDownCast(patientSupportScaledByTableTopVerticalMovementTransformNode->GetTransformToParent());

  patientSupportScaledByTableTopVerticalMovementTransform->Identity();
  patientSupportScaledByTableTopVerticalMovementTransform->Scale(1, 1, ((906 + tableTopDisplacement*1.01)) / 900);
  patientSupportScaledByTableTopVerticalMovementTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::PatientSupportPositiveVerticalTranslation(vtkMRMLScene* scene, double tableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("PatientSupportPositiveVerticalTranslationTransform"));
  vtkTransform* patientSupportPositiveVerticalTranslationTransform = vtkTransform::SafeDownCast(patientSupportPositiveVerticalTranslationTransformNode->GetTransformToParent());

  patientSupportPositiveVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, 1359.469848632281225 };
  patientSupportPositiveVerticalTranslationTransform->Translate(translationArray);
  patientSupportPositiveVerticalTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::VerticalDisplacementValueChanged(vtkMRMLScene* scene, double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  TableTopDisplacementValueChanged(scene, latDisplacementValue, longDisplacementValue, vertDisplacementValue);
  PatientSupportScaledTranslatedToTableTopVerticalTranslation(scene, vertDisplacementValue);
  TableTopDisplacementPatientSupportChanged(scene, vertDisplacementValue);
  PatientSupportPositiveVerticalTranslation(scene, vertDisplacementValue);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LongitudinalDisplacementValueChanged(vtkMRMLScene* scene, double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  TableTopDisplacementValueChanged(scene, latDisplacementValue, longDisplacementValue, vertDisplacementValue);
  CheckForCollisions();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LateralDisplacementValueChanged(vtkMRMLScene* scene, double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  TableTopDisplacementValueChanged(scene, latDisplacementValue, longDisplacementValue, vertDisplacementValue);
  CheckForCollisions();
}
//-----------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::CheckForCollisions()
{
  // Updates all collision detection filters before checking if collisions are present
  this->GantryPatientCollisionDetection->Update();
  this->GantryTableTopCollisionDetection->Update();
  this->GantryPatientSupportCollisionDetection->Update();
  this->CollimatorPatientCollisionDetection->Update();
  this->CollimatorTableTopCollisionDetection->Update();

  std::string text = "";

  // If number of contacts between pieces of treatment room is greater than 0, the collision between which pieces
  // will be set to the output string and returned by the function.
  if (this->GantryPatientCollisionDetection->GetNumberOfContacts() > 0)
  {
    text = text + "Collision between gantry and patient\n";
  }
  if (this->GantryTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    text = text + "Collision between gantry and table top\n";
  }
  if (this->GantryPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
  {
    text = text + "Collision between gantry and patient support\n";
  }
  if (this->CollimatorPatientCollisionDetection->GetNumberOfContacts() > 0)
  {
    text = text + "Collision between collimator and patient\n";
  }
  if (this->CollimatorTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    text = text + "Collision between collimator and table top\n";
  }
  return text;
}



