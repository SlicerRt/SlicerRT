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

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLDisplayNode.h>
#include "vtkMRMLModelNode.h"

// Slicer includes
#include <vtkSlicerSegmentationsModuleLogic.h>
#include <vtkSegmentationConverter.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkSlicerModelsLogic.h>
#include <vtkTransform.h>

//----------------------------------------------------------------------------
// Treatment machine component names
static const char* COLLIMATOR_MODEL_NAME = "CollimatorModel";
static const char* GANTRY_MODEL_NAME = "GantryModel";
static const char* IMAGINGPANELLEFT_MODEL_NAME = "ImagingPanelLeftModel";
static const char* IMAGINGPANELRIGHT_MODEL_NAME = "ImagingPanelRightModel";
static const char* LINACBODY_MODEL_NAME = "LinacBodyModel";
static const char* PATIENTSUPPORT_MODEL_NAME = "PatientSupportModel";
static const char* TABLETOP_MODEL_NAME = "TableTopModel";

//----------------------------------------------------------------------------
// Transform names
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
  this->Superclass::SetMRMLSceneInternal(newScene);

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
    gantryToFixedReferenceTransformNode->SetHideFromEditors(1);
    newScene->AddNode(gantryToFixedReferenceTransformNode);

    collimatorToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToGantryTransformNode->SetName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME);
    collimatorToGantryTransformNode->SetHideFromEditors(1);
    newScene->AddNode(collimatorToGantryTransformNode);

    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->SetName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->SetHideFromEditors(1);
    newScene->AddNode(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode);

    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->SetName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->SetHideFromEditors(1);
    newScene->AddNode(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode);

    leftImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelRotatedToGantryTransformNode->SetName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    leftImagingPanelRotatedToGantryTransformNode->SetHideFromEditors(1);
    newScene->AddNode(leftImagingPanelRotatedToGantryTransformNode);

    leftImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelTranslationTransformNode->SetName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    leftImagingPanelTranslationTransformNode->SetHideFromEditors(1);
    newScene->AddNode(leftImagingPanelTranslationTransformNode);

    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->SetName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->SetHideFromEditors(1);
    newScene->AddNode(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode);

    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->SetName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->SetHideFromEditors(1);
    newScene->AddNode(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode);

    rightImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelRotatedToGantryTransformNode->SetName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    rightImagingPanelRotatedToGantryTransformNode->SetHideFromEditors(1);
    newScene->AddNode(rightImagingPanelRotatedToGantryTransformNode);

    rightImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelTranslationTransformNode->SetName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    rightImagingPanelTranslationTransformNode->SetHideFromEditors(1);
    newScene->AddNode(rightImagingPanelTranslationTransformNode);

    patientSupportToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportToFixedReferenceTransformNode->SetName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    patientSupportToFixedReferenceTransformNode->SetHideFromEditors(1);
    newScene->AddNode(patientSupportToFixedReferenceTransformNode);

    patientSupportScaledByTableTopVerticalMovementTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledByTableTopVerticalMovementTransformNode->SetName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME);
    patientSupportScaledByTableTopVerticalMovementTransformNode->SetHideFromEditors(1);
    newScene->AddNode(patientSupportScaledByTableTopVerticalMovementTransformNode);

    patientSupportPositiveVerticalTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportPositiveVerticalTranslationTransformNode->SetName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME);
    patientSupportPositiveVerticalTranslationTransformNode->SetHideFromEditors(1);
    newScene->AddNode(patientSupportPositiveVerticalTranslationTransformNode);

    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->SetName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME);
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->SetHideFromEditors(1);
    newScene->AddNode(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode);

    tableTopEccentricRotationToPatientSupportTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopEccentricRotationToPatientSupportTransformNode->SetName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME);
    tableTopEccentricRotationToPatientSupportTransformNode->SetHideFromEditors(1);
    newScene->AddNode(tableTopEccentricRotationToPatientSupportTransformNode);

    tableTopToTableTopEccentricRotationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopToTableTopEccentricRotationTransformNode->SetName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME);
    tableTopToTableTopEccentricRotationTransformNode->SetHideFromEditors(1);
    newScene->AddNode(tableTopToTableTopEccentricRotationTransformNode);
  }
  else
  {
    gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));

    collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));

    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

    leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));

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
void vtkSlicerRoomsEyeViewModuleLogic::LoadLinacModels()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("LoadLinacModels: Invalid scene!");
    return;
  }

  std::string moduleShareDirectory = this->GetModuleShareDirectory();

  //TODO: Only the Varian TrueBeam STx models are supported right now.
  //      Allow loading multiple types of machines
  std::string treatmentMachineModelsDirectory = moduleShareDirectory + "/" + "VarianTrueBeamSTx";

  // Load supported treatment machine models
  vtkSmartPointer<vtkSlicerModelsLogic> modelsLogic = vtkSmartPointer<vtkSlicerModelsLogic>::New();
  modelsLogic->SetMRMLScene(this->GetMRMLScene());

  std::string collimatorModelFilePath = treatmentMachineModelsDirectory + "/" + COLLIMATOR_MODEL_NAME + ".stl";
  modelsLogic->AddModel(collimatorModelFilePath.c_str());
  std::string gantryModelFilePath = treatmentMachineModelsDirectory + "/" + GANTRY_MODEL_NAME + ".stl";
  modelsLogic->AddModel(gantryModelFilePath.c_str());
  std::string imagingPanelLeftModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELLEFT_MODEL_NAME + ".stl";
  modelsLogic->AddModel(imagingPanelLeftModelFilePath.c_str());
  std::string imagingPanelRightModelFilePath = treatmentMachineModelsDirectory + "/" + IMAGINGPANELRIGHT_MODEL_NAME + ".stl";
  modelsLogic->AddModel(imagingPanelRightModelFilePath.c_str());
  std::string linacBodyModelFilePath = treatmentMachineModelsDirectory + "/" + LINACBODY_MODEL_NAME + ".stl";
  modelsLogic->AddModel(linacBodyModelFilePath.c_str());
  std::string patientSupportModelFilePath = treatmentMachineModelsDirectory + "/" + PATIENTSUPPORT_MODEL_NAME + ".stl";
  modelsLogic->AddModel(patientSupportModelFilePath.c_str());
  std::string tableTopModelFilePath = treatmentMachineModelsDirectory + "/" + TABLETOP_MODEL_NAME + ".stl";
  modelsLogic->AddModel(tableTopModelFilePath.c_str());
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::InitializeIEC()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("InitializeIEC: Invalid scene!");
    return;
  }

  // Display all pieces of the treatment room and sets each piece a color to provide realistic representation
  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME));
  if (!gantryModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access gantry model!");
    return;
  }
  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("GantryToFixedReferenceTransform"));
  gantryModel->SetAndObserveTransformNodeID(gantryModelTransforms->GetID());
  gantryModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> gantryModelDisplayNode = gantryModel->GetDisplayNode();
  gantryModelDisplayNode->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME));
  if (!collimatorModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access collimator model!");
    return;
  }
  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  collimatorModel->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());
  collimatorModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> collimatorModelDisplayNode = collimatorModel->GetDisplayNode();
  collimatorModelDisplayNode->SetColor(0.7, 0.7, 0.95);

  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  if (!leftImagingPanelModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access left imaging panel model!");
    return;
  }
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform"));
  leftImagingPanelModel->SetAndObserveTransformNodeID(leftImagingPanelModelTransforms->GetID());
  leftImagingPanelModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> leftImagingPanelModelDisplayNode = leftImagingPanelModel->GetDisplayNode();
  leftImagingPanelModelDisplayNode->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  if (!rightImagingPanelModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access right imaging panel model!");
    return;
  }
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform"));
  rightImagingPanelModel->SetAndObserveTransformNodeID(rightImagingPanelModelTransforms->GetID());
  rightImagingPanelModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> rightImagingPanelModelDisplayNode = rightImagingPanelModel->GetDisplayNode();
  rightImagingPanelModelDisplayNode->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access patient support model!");
    return;
  }
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("PatientSupportPositiveVerticalTranslationTransform"));
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransform = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("PatientSupportToFixedReferenceTransform"));
  patientSupportModel->SetAndObserveTransformNodeID(patientSupportModelTransforms->GetID());
  patientSupportModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> patientSupportModelDisplayNode = patientSupportModel->GetDisplayNode();
  patientSupportModelDisplayNode->SetColor(0.85, 0.85, 0.85);

  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME));
  if (!tableTopModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access table top model!");
    return;
  }
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  tableTopModel->SetAndObserveTransformNodeID(tableTopModelTransforms->GetID());
  tableTopModel->CreateDefaultDisplayNodes();
  vtkSmartPointer<vtkMRMLDisplayNode> tableTopModelDisplayNode = tableTopModel->GetDisplayNode();
  tableTopModelDisplayNode->SetColor(0, 0, 0);

  // Set up collision detection between components
  this->GantryTableTopCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()));
  this->GantryTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryTableTopCollisionDetection->Update();

  this->GantryPatientSupportCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()));
  this->GantryPatientSupportCollisionDetection->SetTransform(1,
    vtkLinearTransform::SafeDownCast(patientSupportToFixedReferenceTransform->GetTransformToParent()));
  this->GantryPatientSupportCollisionDetection->Update();

  this->CollimatorTableTopCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->CollimatorTableTopCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorTableTopCollisionDetection->Update();

  //TODO: Whole patient (segmentation, CT) will need to be transformed when the table top is transformed
  //vtkMRMLLinearTransformNode* patientModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
  //  this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  //patientModel->SetAndObserveTransformNodeID(patientModelTransforms->GetID());

  this->GantryPatientCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()));
  this->GantryPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryPatientCollisionDetection->Update();

  this->CollimatorPatientCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorPatientCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->Update();
}

//----------------------------------------------------------------------------
bool vtkSlicerRoomsEyeViewModuleLogic::GetPatientBodyPolyData(vtkMRMLRoomsEyeViewNode* parameterNode, vtkPolyData* patientBodyPolyData)
{
  if (!parameterNode)
  {
    vtkErrorMacro("GetPatientBodyPolyData: Invalid parameter set node!");
    return false;
  }
  if (!patientBodyPolyData)
  {
    vtkErrorMacro("GetPatientBodyPolyData: Invalid output poly data!");
    return false;
  }

  // Get patient body segmentation
  vtkMRMLSegmentationNode* segmentationNode = parameterNode->GetPatientBodySegmentationNode();
  if (!segmentationNode || !parameterNode->GetPatientBodySegmentID())
  {
    return false;
  }

  // Get closed surface representation for patient body
  return vtkSlicerSegmentationsModuleLogic::GetSegmentRepresentation(
    segmentationNode, parameterNode->GetPatientBodySegmentID(),
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName(),
    patientBodyPolyData);
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateCollimatorToGantryTransform: Invalid parameter set node!");
    return;
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  //TODO: This is specific to the Varian TrueBeam STx model, move this somewhere else when generalizing for any treatment machine
  // Translates collimator to actual center of rotation and then rotates based on rotationAngle
  // TODO: Look into why translating the object twice prevents wobble
  collimatorToGantryTransform->Identity();
  double implicitCenterOfRotation[3] = { 3.652191162109375, 8.4510498046875, 528.0714111328125 };
  double actualCenterOfRotation[3] = { -0.041, 0.094, 528.0714111328125 };
  double implicitCenterToActualCenterDisplacement[3] = { actualCenterOfRotation[0] - implicitCenterOfRotation[0], actualCenterOfRotation[1] - implicitCenterOfRotation[1], actualCenterOfRotation[2] - implicitCenterOfRotation[2] };
  collimatorToGantryTransform->Translate(implicitCenterToActualCenterDisplacement);
  collimatorToGantryTransform->RotateZ(parameterNode->GetCollimatorRotationAngle());
  collimatorToGantryTransform->Translate(implicitCenterToActualCenterDisplacement);
  collimatorToGantryTransform->Modified();
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateGantryToFixedReferenceTransform(double rotationAngle)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("GantryToFixedReferenceTransform"));
  vtkSmartPointer<vtkTransform> gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent());

  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(rotationAngle);
  gantryToFixedReferenceTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform"));
  vtkSmartPointer<vtkTransform> leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->GetTransformToParent());
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Identity();
  double translationArray[3] = { 656.606, -1518.434, -345.164 };
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Translate(translationArray);
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform"));
  vtkSmartPointer<vtkTransform> LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform = vtkTransform::SafeDownCast(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->GetTransformToParent());
  LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->Identity();
  if (panelMovement > 0){
    LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->RotateZ(68.5);
  }
  else{
    LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->RotateZ(panelMovement + 68.5);
  }

  LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelRotatedToGantryTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("LeftImagingPanelRotatedToGantryTransform"));
  vtkSmartPointer<vtkTransform> leftImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(leftImagingPanelRotatedToGantryTransformNode->GetTransformToParent());
  leftImagingPanelRotatedToGantryTransform->Identity();
  double translationArray[3] = { -656.606, 1518.434, 345.164 };
  leftImagingPanelRotatedToGantryTransform->Translate(translationArray);
  leftImagingPanelRotatedToGantryTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelTranslationTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("LeftImagingPanelTranslationTransform"));
  vtkSmartPointer<vtkTransform> leftImagingPanelTranslationTransform = vtkTransform::SafeDownCast(leftImagingPanelTranslationTransformNode->GetTransformToParent());
  leftImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  leftImagingPanelTranslationTransform->Translate(translationArray);
  leftImagingPanelTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->GetTransformToParent());
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Identity();
  double translationArray[3] = { -649.763, -1504.412, -342.200 };
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Translate(translationArray);
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform = vtkTransform::SafeDownCast(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->GetTransformToParent());
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->Identity();
  if (panelMovement > 0){
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->RotateZ(-(68.5));
  }
  else{
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->RotateZ(-(68.5 + panelMovement));
  }
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelRotatedToGantryTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("RightImagingPanelRotatedToGantryTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(rightImagingPanelRotatedToGantryTransformNode->GetTransformToParent());
  rightImagingPanelRotatedToGantryTransform->Identity();
  double translationArray[3] = { 649.763, 1504.412, 342.200 };
  rightImagingPanelRotatedToGantryTransform->Translate(translationArray);
  rightImagingPanelRotatedToGantryTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelTranslationTransform(double panelMovement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("RightImagingPanelTranslationTransform"));
  vtkSmartPointer<vtkTransform> rightImagingPanelTranslationTransform = vtkTransform::SafeDownCast(rightImagingPanelTranslationTransformNode->GetTransformToParent());
  rightImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  rightImagingPanelTranslationTransform->Translate(translationArray);
  rightImagingPanelTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("CollimatorToGantryTransform"));
  vtkSmartPointer<vtkTransform> collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateImagingPanelMovementTransforms(double panelMovement)
{
  this->UpdateLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform(panelMovement);
  this->UpdateLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform(panelMovement);
  this->UpdateLeftImagingPanelRotatedToGantryTransform(panelMovement);
  this->UpdateRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform(panelMovement);
  this->UpdateRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform(panelMovement);
  this->UpdateRightImagingPanelRotatedToGantryTransform(panelMovement);

  if (panelMovement > 0)
  {
    this->UpdateLeftImagingPanelTranslationTransform(panelMovement);
    this->UpdateRightImagingPanelTranslationTransform(panelMovement);
  }
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportToFixedReferenceTransform(double rotationAngle)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("PatientSupportToFixedReferenceTransform"));
  vtkSmartPointer<vtkTransform> patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(patientSupportToFixedReferenceTransformNode->GetTransformToParent());

  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(rotationAngle);
  patientSupportToFixedReferenceTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkSmartPointer<vtkTransform> tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTableTopEccentricRotationToPatientSupportTransform(double lattableTopDisplacement, double longTableTopDisplacement, double vertTableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkSmartPointer<vtkTransform> tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransform->Identity();
  double translationArray[3] = { lattableTopDisplacement, longTableTopDisplacement, vertTableTopDisplacement };
  tableTopEccentricRotationToPatientSupportTransform->Translate(translationArray);
  tableTopEccentricRotationToPatientSupportTransform->Modified();

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(double tableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform"));
  vtkSmartPointer<vtkTransform> patientSupportScaledTranslatedToTableTopVerticalTranslationTransform = vtkTransform::SafeDownCast(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->GetTransformToParent());

  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, -1359.469848632281225 };
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Translate(translationArray);
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkSmartPointer<vtkTransform> tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportScaledByTableTopVerticalMovementTransform(double tableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("PatientSupportScaledByTableTopVerticalMovementTransform"));
  vtkTransform* patientSupportScaledByTableTopVerticalMovementTransform = vtkTransform::SafeDownCast(patientSupportScaledByTableTopVerticalMovementTransformNode->GetTransformToParent());

  patientSupportScaledByTableTopVerticalMovementTransform->Identity();
  patientSupportScaledByTableTopVerticalMovementTransform->Scale(1, 1, ((906 + tableTopDisplacement*1.01)) / 900);
  patientSupportScaledByTableTopVerticalMovementTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportPositiveVerticalTranslationTransform(double tableTopDisplacement)
{
  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("PatientSupportPositiveVerticalTranslationTransform"));
  vtkTransform* patientSupportPositiveVerticalTranslationTransform = vtkTransform::SafeDownCast(patientSupportPositiveVerticalTranslationTransformNode->GetTransformToParent());

  patientSupportPositiveVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, 1359.469848632281225 };
  patientSupportPositiveVerticalTranslationTransform->Translate(translationArray);
  patientSupportPositiveVerticalTranslationTransform->Modified();

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName("TableTopEccentricRotationToPatientSupportTransform"));
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent());

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateVerticalDisplacementTransforms(double latDisplacementValue, double longDisplacementValue, double vertDisplacementValue)
{
  this->UpdateTableTopEccentricRotationToPatientSupportTransform(latDisplacementValue, longDisplacementValue, vertDisplacementValue);
  this->UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(vertDisplacementValue);
  this->UpdatePatientSupportScaledByTableTopVerticalMovementTransform(vertDisplacementValue);
  this->UpdatePatientSupportPositiveVerticalTranslationTransform(vertDisplacementValue);
}

//-----------------------------------------------------------------------------
std::string vtkSlicerRoomsEyeViewModuleLogic::CheckForCollisions(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("CheckForCollisions: Invalid parameter set node!");
    return "Invalid parameters!";
  }

  std::string statusString = "";

  // If number of contacts between pieces of treatment room is greater than 0, the collision between which pieces
  // will be set to the output string and returned by the function.
  this->GantryTableTopCollisionDetection->Update();
  if (this->GantryTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between gantry and table top\n";
  }

  this->GantryPatientSupportCollisionDetection->Update();
  if (this->GantryPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between gantry and patient support\n";
  }

  this->CollimatorTableTopCollisionDetection->Update();
  if (this->CollimatorTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between collimator and table top\n";
  }

  // Get patient body poly data
  vtkSmartPointer<vtkPolyData> patientBodyPolyData = vtkSmartPointer<vtkPolyData>::New();
  if (this->GetPatientBodyPolyData(parameterNode, patientBodyPolyData))
  {
    this->GantryPatientCollisionDetection->SetInput(1, patientBodyPolyData);
    this->GantryPatientCollisionDetection->Update();
    if (this->GantryPatientCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between gantry and patient\n";
    }

    this->CollimatorPatientCollisionDetection->SetInput(1, patientBodyPolyData);
    this->CollimatorPatientCollisionDetection->Update();
    if (this->CollimatorPatientCollisionDetection->GetNumberOfContacts() > 0)
    {
      statusString = statusString + "Collision between collimator and patient\n";
    }
  }

  return statusString;
}
