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
#include <vtkMRMLRTBeamNode.h>
#include <vtkMRMLViewNode.h>
//#include <vtkSlicerIECTransformLogic.h>

// Slicer includes
#include <vtkSlicerSegmentationsModuleLogic.h>
#include <vtkSegmentationConverter.h>
#include <qSlicerCoreIOManager.h>
#include <qSlicerCoreApplication.h>
#include <qSlicerApplication.h>
#include <qSlicerLayoutManager.h>
#include <qMRMLSliceWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDView.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkSlicerModelsLogic.h>
#include <vtkSlicerTransformLogic.h>
#include <vtkTransform.h>
#include <vtkCenterOfMass.h>
#include <vtkAlgorithmOutput.h>
#include <vtkAppendPolyData.h>
#include <vtkDebugLeaks.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkVersion.h>
#include <vtksys/SystemTools.hxx>
#include <vtkSlicerTransformLogic.h>
#include <qSlicerFileDialog.h>
#include <qSlicerDataDialog.h>
#include <qSlicerIOManager.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkGeneralTransform.h>
#include <vtkTransformFilter.h>

//----------------------------------------------------------------------------
// Treatment machine component names
static const char* COLLIMATOR_MODEL_NAME = "CollimatorModel";
static const char* GANTRY_MODEL_NAME = "GantryModel";
static const char* IMAGINGPANELLEFT_MODEL_NAME = "ImagingPanelLeftModel";
static const char* IMAGINGPANELRIGHT_MODEL_NAME = "ImagingPanelRightModel";
static const char* LINACBODY_MODEL_NAME = "LinacBodyModel";
static const char* PATIENTSUPPORT_MODEL_NAME = "PatientSupportModel";
static const char* TABLETOP_MODEL_NAME = "TableTopModel";
static const char* APPLICATORHOLDER_MODEL_NAME = "ApplicatorHolderModel";
static const char* ELECTRONAPPLICATOR_MODEL_NAME = "ElectronApplicatorModel";

//----------------------------------------------------------------------------
// Transform names
static const char* GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "GantryToFixedReferenceTransform";
static const char* COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "CollimatorToFixedReferenceIsocenterTransform";
static const char* FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME = "FixedReferenceIsocenterToCollimatorRotatedTransform";
static const char* COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME = "CollimatorToGantryTransform";
static const char* ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME = "AdditionalCollimatorDevicesToCollimatorTransform";
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
  , AdditionalModelsTableTopCollisionDetection(NULL)
  , AdditionalModelsPatientSupportCollisionDetection(NULL)
{
  this->CollimatorToWorldTransformMatrix = vtkMatrix4x4::New();
  this->TableTopToWorldTransformMatrix = vtkMatrix4x4::New();

  this->GantryPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->GantryPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorPatientCollisionDetection = vtkCollisionDetectionFilter::New();
  this->CollimatorTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->AdditionalModelsTableTopCollisionDetection = vtkCollisionDetectionFilter::New();
  this->AdditionalModelsPatientSupportCollisionDetection = vtkCollisionDetectionFilter::New();
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
  if (this->AdditionalModelsTableTopCollisionDetection)
  {
    this->AdditionalModelsTableTopCollisionDetection->Delete();
    this->AdditionalModelsTableTopCollisionDetection = NULL;
  }
  if (this->AdditionalModelsPatientSupportCollisionDetection)
  {
    this->AdditionalModelsPatientSupportCollisionDetection->Delete();
    this->AdditionalModelsPatientSupportCollisionDetection = NULL;
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

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToFixedReferenceIsocenterTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> fixedReferenceIsocenterToCollimatorRotatedTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode;
  vtkSmartPointer<vtkMRMLLinearTransformNode> additionalCollimatorDevicesToCollimatorTransformNode;

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

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientImmobilizationBoardToTableTopTransformNode;

  // Create transform nodes
  if (newScene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) == NULL)
  {
    gantryToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    gantryToFixedReferenceTransformNode->SetName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    gantryToFixedReferenceTransformNode->SetHideFromEditors(1);
    newScene->AddNode(gantryToFixedReferenceTransformNode);

    collimatorToFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToFixedReferenceIsocenterTransformNode->SetName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    collimatorToFixedReferenceIsocenterTransformNode->SetHideFromEditors(1);
    newScene->AddNode(collimatorToFixedReferenceIsocenterTransformNode);

    fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    fixedReferenceIsocenterToCollimatorRotatedTransformNode->SetName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME);
    fixedReferenceIsocenterToCollimatorRotatedTransformNode->SetHideFromEditors(1);
    newScene->AddNode(fixedReferenceIsocenterToCollimatorRotatedTransformNode);

    collimatorToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToGantryTransformNode->SetName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME);
    collimatorToGantryTransformNode->SetHideFromEditors(1);
    newScene->AddNode(collimatorToGantryTransformNode);

    additionalCollimatorDevicesToCollimatorTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    additionalCollimatorDevicesToCollimatorTransformNode->SetName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME);
    additionalCollimatorDevicesToCollimatorTransformNode->SetHideFromEditors(1);
    newScene->AddNode(additionalCollimatorDevicesToCollimatorTransformNode);

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

    collimatorToFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

    fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));

    collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));

    additionalCollimatorDevicesToCollimatorTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      newScene->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));

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
  additionalCollimatorDevicesToCollimatorTransformNode->SetAndObserveTransformNodeID(collimatorToFixedReferenceIsocenterTransformNode->GetID());
  collimatorToFixedReferenceIsocenterTransformNode->SetAndObserveTransformNodeID(fixedReferenceIsocenterToCollimatorRotatedTransformNode->GetID());
  fixedReferenceIsocenterToCollimatorRotatedTransformNode->SetAndObserveTransformNodeID(collimatorToGantryTransformNode->GetID());
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
  std::string additionalDevicesDirectory = moduleShareDirectory + "/" + "AdditionalTreatmentDevices";
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

  // TODO: Uncomment once Additional Devices STL models are created
  std::string applicatorHolderModelFilePath = treatmentMachineModelsDirectory + "/" + APPLICATORHOLDER_MODEL_NAME + ".stl";
  modelsLogic->AddModel(applicatorHolderModelFilePath.c_str());
  std::string electronApplicatorModelFilePath = treatmentMachineModelsDirectory + "/" + ELECTRONAPPLICATOR_MODEL_NAME + ".stl";
  modelsLogic->AddModel(electronApplicatorModelFilePath.c_str());

}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::LoadAdditionalDevices()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("LoadLinacModels: Invalid scene!");
    return;
  }

  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  vtkCollection* loadedModelsCollection = vtkCollection::New();
  ioManager->openDialog("ModelFile", qSlicerDataDialog::Read, qSlicerIO::IOProperties(), loadedModelsCollection);
  
  vtkMRMLModelNode* loadedModelNode = vtkMRMLModelNode::New();
  loadedModelNode = vtkMRMLModelNode::SafeDownCast(loadedModelsCollection->GetNextItemAsObject());
  vtkMRMLLinearTransformNode* collimatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  loadedModelNode->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());



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
    this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME) );
  if (!gantryModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access gantry model!");
    return;
  }
  vtkMRMLLinearTransformNode* gantryModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) );
  gantryModel->SetAndObserveTransformNodeID(gantryModelTransforms->GetID());
  gantryModel->CreateDefaultDisplayNodes();
  gantryModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME) );
  if (!collimatorModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access collimator model!");
    return;
  }
  vtkMRMLLinearTransformNode* collimatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME) );
  collimatorModel->SetAndObserveTransformNodeID(collimatorModelTransforms->GetID());
  collimatorModel->CreateDefaultDisplayNodes();
  collimatorModel->GetDisplayNode()->SetColor(0.7, 0.7, 0.95);

  // TODO: Uncomment once Additional Devices STL models are created
  vtkMRMLModelNode* applicatorHolderModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(APPLICATORHOLDER_MODEL_NAME));
  if (!applicatorHolderModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access applicator holder model!");
    return;
  }
  vtkMRMLLinearTransformNode* applicatorHolderModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
  applicatorHolderModel->SetAndObserveTransformNodeID(applicatorHolderModelTransforms->GetID());
  applicatorHolderModel->CreateDefaultDisplayNodes();
  applicatorHolderModel->GetDisplayNode()->VisibilityOff();

  vtkMRMLModelNode* electronApplicatorModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ELECTRONAPPLICATOR_MODEL_NAME));
  if (!electronApplicatorModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access electron applicator model!");
    return;
  }
  vtkMRMLLinearTransformNode* electronApplicatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME));
  electronApplicatorModel->SetAndObserveTransformNodeID(electronApplicatorModelTransforms->GetID());
  electronApplicatorModel->CreateDefaultDisplayNodes();
  electronApplicatorModel->GetDisplayNode()->VisibilityOff();

  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME) );
  if (!leftImagingPanelModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access left imaging panel model!");
    return;
  }
  vtkMRMLLinearTransformNode* leftImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  leftImagingPanelModel->SetAndObserveTransformNodeID(leftImagingPanelModelTransforms->GetID());
  leftImagingPanelModel->CreateDefaultDisplayNodes();
  leftImagingPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME) );
  if (!rightImagingPanelModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access right imaging panel model!");
    return;
  }
  vtkMRMLLinearTransformNode* rightImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME) );
  rightImagingPanelModel->SetAndObserveTransformNodeID(rightImagingPanelModelTransforms->GetID());
  rightImagingPanelModel->CreateDefaultDisplayNodes();
  rightImagingPanelModel->GetDisplayNode()->SetColor(0.95, 0.95, 0.95);

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME) );
  if (!patientSupportModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access patient support model!");
    return;
  }
  vtkMRMLLinearTransformNode* patientSupportModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME) );
  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransform = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) );
  patientSupportModel->SetAndObserveTransformNodeID(patientSupportModelTransforms->GetID());
  patientSupportModel->CreateDefaultDisplayNodes();
  patientSupportModel->GetDisplayNode()->SetColor(0.85, 0.85, 0.85);

  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME) );
  if (!tableTopModel)
  {
    vtkErrorMacro("InitializeIEC: Unable to access table top model!");
    return;
  }
  vtkMRMLLinearTransformNode* tableTopModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME) );
  tableTopModel->SetAndObserveTransformNodeID(tableTopModelTransforms->GetID());
  tableTopModel->CreateDefaultDisplayNodes();
  tableTopModel->GetDisplayNode()->SetColor(0, 0, 0);

  // Set up collision detection between components
  this->GantryTableTopCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->GantryTableTopCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()) );
  this->GantryTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryTableTopCollisionDetection->Update();

  this->GantryPatientSupportCollisionDetection->SetInput(0, gantryModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());
  this->GantryPatientSupportCollisionDetection->SetTransform(0,
    vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()) );
  this->GantryPatientSupportCollisionDetection->SetTransform(1,
    vtkLinearTransform::SafeDownCast(patientSupportToFixedReferenceTransform->GetTransformToParent()) );
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
    vtkLinearTransform::SafeDownCast(gantryModelTransforms->GetTransformToParent()) );
  this->GantryPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->GantryPatientCollisionDetection->Update();

  this->CollimatorPatientCollisionDetection->SetInput(0, collimatorModel->GetPolyData());
  this->CollimatorPatientCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->CollimatorPatientCollisionDetection->Update();

  vtkSmartPointer<vtkAppendPolyData> additionalDeviceAppending = vtkSmartPointer<vtkAppendPolyData>::New();
  vtkPolyData* inputs[] = { applicatorHolderModel->GetPolyData(), electronApplicatorModel->GetPolyData() };
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkMRMLModelNode> outputModel = vtkSmartPointer<vtkMRMLModelNode>::New();
  additionalDeviceAppending->ExecuteAppend(output, inputs, 2);
  this->GetMRMLScene()->AddNode(outputModel);
  outputModel->SetAndObservePolyData(output);

 
  this->AdditionalModelsTableTopCollisionDetection->SetInput(0, outputModel->GetPolyData());
  this->AdditionalModelsTableTopCollisionDetection->SetInput(1, tableTopModel->GetPolyData());
  this->AdditionalModelsTableTopCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->AdditionalModelsTableTopCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->AdditionalModelsTableTopCollisionDetection->Update();

  this->AdditionalModelsPatientSupportCollisionDetection->SetInput(0, outputModel->GetPolyData());
  this->AdditionalModelsPatientSupportCollisionDetection->SetInput(1, patientSupportModel->GetPolyData());
  this->AdditionalModelsPatientSupportCollisionDetection->SetMatrix(0, this->CollimatorToWorldTransformMatrix);
  this->AdditionalModelsPatientSupportCollisionDetection->SetMatrix(1, this->TableTopToWorldTransformMatrix);
  this->AdditionalModelsPatientSupportCollisionDetection->Update();


}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTreatmentOrientationMarker()
{

  vtkSmartPointer<vtkAppendPolyData> deviceAppending = vtkSmartPointer<vtkAppendPolyData>::New();
  //vtkSmartPointer<vtkSlicerTransformLogic> transformLogic = vtkSmartPointer<vtkSlicerTransformLogic>::New();
  //vtkSmartPointer<vtkTransformPolyDataFilter> transformPolyData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();

  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME));
  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME));
  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  vtkMRMLModelNode* tableTopModel = vtkMRMLModelNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOP_MODEL_NAME));

  vtkMRMLLinearTransformNode* gantryModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* collimatorModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* leftImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* rightImagingPanelModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* patientSupportModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* tableTopModelTransforms = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));

  vtkSmartPointer<vtkPolyData> gantryModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  gantryModelPolyData->DeepCopy(gantryModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> collimatorModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  collimatorModelPolyData->DeepCopy(collimatorModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> leftImagingPanelModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  leftImagingPanelModelPolyData->DeepCopy(leftImagingPanelModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> rightImagingPanelModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  rightImagingPanelModelPolyData->DeepCopy(rightImagingPanelModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> patientSupportModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  patientSupportModelPolyData->DeepCopy(patientSupportModel->GetPolyData());

  vtkSmartPointer<vtkPolyData> tableTopModelPolyData = vtkSmartPointer<vtkPolyData>::New();
  tableTopModelPolyData->DeepCopy(tableTopModel->GetPolyData());

  vtkTransformFilter* gantryTransformFilter = vtkTransformFilter::New();
  gantryTransformFilter->SetInputData(gantryModelPolyData);
  vtkGeneralTransform* gantryModelTransform = vtkGeneralTransform::New();
  gantryModelTransforms->GetTransformFromWorld(gantryModelTransform);
  gantryModelTransform->Inverse();
  gantryTransformFilter->SetTransform(gantryModelTransform);
  gantryTransformFilter->Update();
  gantryModelPolyData = vtkPolyData::SafeDownCast(gantryTransformFilter->GetOutput());

  vtkTransformFilter* collimatorTransformFilter = vtkTransformFilter::New();
  collimatorTransformFilter->SetInputData(collimatorModelPolyData);
  vtkGeneralTransform* collimatorModelTransform = vtkGeneralTransform::New();
  collimatorModelTransforms->GetTransformFromWorld(collimatorModelTransform);
  collimatorModelTransform->Inverse();
  collimatorTransformFilter->SetTransform(collimatorModelTransform);
  collimatorTransformFilter->Update();
  collimatorModelPolyData = vtkPolyData::SafeDownCast(collimatorTransformFilter->GetOutput());

  vtkTransformFilter* leftImagingPanelTransformFilter = vtkTransformFilter::New();
  leftImagingPanelTransformFilter->SetInputData(leftImagingPanelModelPolyData);
  vtkGeneralTransform* leftImagingPanelModelTransform = vtkGeneralTransform::New();
  leftImagingPanelModelTransforms->GetTransformFromWorld(leftImagingPanelModelTransform);
  leftImagingPanelModelTransform->Inverse();
  leftImagingPanelTransformFilter->SetTransform(leftImagingPanelModelTransform);
  leftImagingPanelTransformFilter->Update();
  leftImagingPanelModelPolyData = vtkPolyData::SafeDownCast(leftImagingPanelTransformFilter->GetOutput());

  vtkTransformFilter* rightImagingPanelTransformFilter = vtkTransformFilter::New();
  rightImagingPanelTransformFilter->SetInputData(rightImagingPanelModelPolyData);
  vtkGeneralTransform* rightImagingPanelModelTransform = vtkGeneralTransform::New();
  rightImagingPanelModelTransforms->GetTransformFromWorld(rightImagingPanelModelTransform);
  rightImagingPanelModelTransform->Inverse();
  rightImagingPanelTransformFilter->SetTransform(rightImagingPanelModelTransform);
  rightImagingPanelTransformFilter->Update();
  rightImagingPanelModelPolyData = vtkPolyData::SafeDownCast(rightImagingPanelTransformFilter->GetOutput());

  vtkTransformFilter* patientSupportTransformFilter = vtkTransformFilter::New();
  patientSupportTransformFilter->SetInputData(patientSupportModelPolyData);
  vtkGeneralTransform* patientSupportModelTransform = vtkGeneralTransform::New();
  patientSupportModelTransforms->GetTransformFromWorld(patientSupportModelTransform);
  patientSupportModelTransform->Inverse();
  patientSupportTransformFilter->SetTransform(patientSupportModelTransform);
  patientSupportTransformFilter->Update();
  patientSupportModelPolyData = vtkPolyData::SafeDownCast(patientSupportTransformFilter->GetOutput());

  vtkTransformFilter* tableTopTransformFilter = vtkTransformFilter::New();
  tableTopTransformFilter->SetInputData(tableTopModelPolyData);
  vtkGeneralTransform* tableTopModelTransform = vtkGeneralTransform::New();
  tableTopModelTransforms->GetTransformFromWorld(tableTopModelTransform);
  tableTopModelTransform->Inverse();
  tableTopTransformFilter->SetTransform(tableTopModelTransform);
  tableTopTransformFilter->Update();
  tableTopModelPolyData = vtkPolyData::SafeDownCast(tableTopTransformFilter->GetOutput());

  vtkPolyData* inputs[] = { gantryModelPolyData, collimatorModelPolyData, leftImagingPanelModelPolyData, rightImagingPanelModelPolyData, patientSupportModelPolyData, tableTopModelPolyData};
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkMRMLModelNode> outputModel = vtkSmartPointer<vtkMRMLModelNode>::New();
  deviceAppending->ExecuteAppend(output,inputs,6);
  this->GetMRMLScene()->AddNode(outputModel);
  outputModel->SetAndObservePolyData(output);

  qSlicerApplication* slicerApplication = qSlicerApplication::application();
  qSlicerLayoutManager* layoutManager = slicerApplication->layoutManager();
  qMRMLThreeDView* threeDView = layoutManager->threeDWidget(0)->threeDView();
  vtkMRMLViewNode* viewNode = threeDView->mrmlViewNode();
  viewNode->SetOrientationMarkerHumanModelNodeID(outputModel->GetID());
  
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
    patientBodyPolyData );
}
//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateCollimatorToFixedReferenceIsocenterTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateCollimatorToFixedReferenceIsocenterTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME));
  if (!collimatorModel)
  {
    vtkErrorMacro("collimatorModel: Invalid model node!");
    return;
  }

  vtkPolyData* collimatorModelPolyData = collimatorModel->GetPolyData();

  vtkMRMLLinearTransformNode* collimatorToFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkTransform* collimatorToFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(
    collimatorToFixedReferenceIsocenterTransformNode->GetTransformToParent());

  double collimatorCenterOfRotation[3];
  double collimatorModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  collimatorModelPolyData->GetBounds(collimatorModelBounds);
  collimatorCenterOfRotation[0] = (collimatorModelBounds[0] + collimatorModelBounds[1]) / 2;
  collimatorCenterOfRotation[1] = (collimatorModelBounds[2] + collimatorModelBounds[3]) / 2;
  collimatorCenterOfRotation[2] = (collimatorModelBounds[4] + collimatorModelBounds[5]) / 2;

  collimatorToFixedReferenceIsocenterTransform->Identity();
  for (int i = 0; i < 3; i++){
    collimatorCenterOfRotation[i] *= -1;
  }
 
  collimatorToFixedReferenceIsocenterTransform->Identity();
  collimatorToFixedReferenceIsocenterTransform->Translate(collimatorCenterOfRotation);
  collimatorToFixedReferenceIsocenterTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}
//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateFixedReferenceIsocenterToCollimatorRotatedTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateFixedReferenceIsocenterToCollimatorRotatedTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLLinearTransformNode* fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));
  vtkTransform* fixedReferenceIsocenterToCollimatorRotatedTransform = vtkTransform::SafeDownCast(
    fixedReferenceIsocenterToCollimatorRotatedTransformNode->GetTransformToParent());

  fixedReferenceIsocenterToCollimatorRotatedTransform->Identity();
  fixedReferenceIsocenterToCollimatorRotatedTransform->RotateZ(parameterNode->GetCollimatorRotationAngle());
  fixedReferenceIsocenterToCollimatorRotatedTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}
//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateCollimatorToGantryTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateCollimatorToGantryTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLModelNode* collimatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_MODEL_NAME));
  if (!collimatorModel)
  {
    vtkErrorMacro("collimatorModel: Invalid model node!");
    return;
  }

  vtkPolyData* collimatorModelPolyData = collimatorModel->GetPolyData();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME) );
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent() );

  double collimatorCenterOfRotation[3];
  double collimatorModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  collimatorModelPolyData->GetBounds(collimatorModelBounds);
  collimatorCenterOfRotation[0] = (collimatorModelBounds[0] + collimatorModelBounds[1]) / 2;
  collimatorCenterOfRotation[1] = (collimatorModelBounds[2] + collimatorModelBounds[3]) / 2;
  collimatorCenterOfRotation[2] = (collimatorModelBounds[4] + collimatorModelBounds[5]) / 2;

  // Translates collimator to actual center of rotation and then rotates based on rotationAngle
  collimatorToGantryTransform->Identity();
  collimatorToGantryTransform->Translate(collimatorCenterOfRotation);
  collimatorToGantryTransform->Modified();

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateGantryToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateGantryToFixedReferenceTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLModelNode* gantryModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(GANTRY_MODEL_NAME));
  if (!gantryModel)
  {
    vtkErrorMacro("collimatorModel: Invalid model node!");
    return;
  }

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) );
  vtkTransform* gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(
    gantryToFixedReferenceTransformNode->GetTransformToParent() );
  
  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(parameterNode->GetGantryRotationAngle());
  gantryToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME) );
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent() );

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform()
{

  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  if (!leftImagingPanelModel)
  {
    vtkErrorMacro("leftImagingPanelModel: Invalid MRML model node!");
  }

  vtkPolyData* leftImagingPanelModelPolyData = leftImagingPanelModel->GetPolyData();

  double leftImagingPanelCenterOfMass[3];
  double leftImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  leftImagingPanelModelPolyData->GetBounds(leftImagingPanelModelBounds);
  leftImagingPanelCenterOfMass[0] = leftImagingPanelModelBounds[1];
  leftImagingPanelCenterOfMass[1] = (leftImagingPanelModelBounds[2] + leftImagingPanelModelBounds[3]) / 2;
  leftImagingPanelCenterOfMass[2] = (leftImagingPanelModelBounds[4] + leftImagingPanelModelBounds[5]) / 2;

  vtkMRMLLinearTransformNode* leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME) );
  vtkTransform* leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->GetTransformToParent() );

  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Identity();
  for (int i = 0; i < 3; i++){
    leftImagingPanelCenterOfMass[i] *= -1;
  }
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Translate(leftImagingPanelCenterOfMass);
  leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform->Modified();

}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform: Invalid parameter set node!");
    return;
  }

  double panelMovement = parameterNode->GetImagingPanelMovement();

  vtkMRMLLinearTransformNode* leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME) );
  vtkTransform* leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform = vtkTransform::SafeDownCast(
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->GetTransformToParent() );

  leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->Identity();
  if (panelMovement > 0)
  {
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->RotateZ(68.5);
  }
  else
  {
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->RotateZ(panelMovement + 68.5);
  }

  leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelRotatedToGantryTransform()
{

  vtkMRMLModelNode* leftImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELLEFT_MODEL_NAME));
  if (!leftImagingPanelModel)
  {
    vtkErrorMacro("leftImagingPanelModel: Invalid MRML model node!");
  }

  vtkPolyData* leftImagingPanelModelPolyData = leftImagingPanelModel->GetPolyData();

  double leftImagingPanelCenterOfMass[3];
  double leftImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  leftImagingPanelModelPolyData->GetBounds(leftImagingPanelModelBounds);
  leftImagingPanelCenterOfMass[0] = leftImagingPanelModelBounds[1];
  leftImagingPanelCenterOfMass[1] = (leftImagingPanelModelBounds[2] + leftImagingPanelModelBounds[3]) / 2;
  leftImagingPanelCenterOfMass[2] = (leftImagingPanelModelBounds[4] + leftImagingPanelModelBounds[5]) / 2;

  
  vtkMRMLLinearTransformNode* leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME) );
  vtkTransform* leftImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(
    leftImagingPanelRotatedToGantryTransformNode->GetTransformToParent() );

  leftImagingPanelRotatedToGantryTransform->Identity();
  leftImagingPanelRotatedToGantryTransform->Translate(leftImagingPanelCenterOfMass);
  leftImagingPanelRotatedToGantryTransform->Modified();

}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateLeftImagingPanelTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateLeftImagingPanelTranslationTransform: Invalid parameter set node!");
    return;
  }

  double panelMovement = parameterNode->GetImagingPanelMovement();

  vtkMRMLLinearTransformNode* leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME) );
  vtkTransform* leftImagingPanelTranslationTransform = vtkTransform::SafeDownCast(
    leftImagingPanelTranslationTransformNode->GetTransformToParent() );

  leftImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  leftImagingPanelTranslationTransform->Translate(translationArray);
  leftImagingPanelTranslationTransform->Modified();

}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform()
{
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  if (!rightImagingPanelModel)
  {
    vtkErrorMacro("rightImagingPanelModel: Invalid MRML model node!");
  }

  vtkPolyData* rightImagingPanelModelPolyData = rightImagingPanelModel->GetPolyData();

  double rightImagingPanelCenterOfMass[3];
  double rightImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  rightImagingPanelModelPolyData->GetBounds(rightImagingPanelModelBounds);
  rightImagingPanelCenterOfMass[0] = rightImagingPanelModelBounds[0];
  rightImagingPanelCenterOfMass[1] = (rightImagingPanelModelBounds[2] + rightImagingPanelModelBounds[3]) / 2;
  rightImagingPanelCenterOfMass[2] = (rightImagingPanelModelBounds[4] + rightImagingPanelModelBounds[5]) / 2;

  vtkMRMLLinearTransformNode* rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME) );
  vtkTransform* rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform = vtkTransform::SafeDownCast(
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->GetTransformToParent() );

  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Identity();
  for (int i = 0; i < 3; i++){
    rightImagingPanelCenterOfMass[i] *= -1;
  }
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Translate(rightImagingPanelCenterOfMass);
  rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform: Invalid parameter set node!");
    return;
  }

  double panelMovement = parameterNode->GetImagingPanelMovement();

  vtkMRMLLinearTransformNode* rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME) );
  vtkTransform* rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform = vtkTransform::SafeDownCast(
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->GetTransformToParent() );

  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->Identity();
  if (panelMovement > 0)
  {
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->RotateZ(-(68.5));
  }
  else
  {
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->RotateZ(-(68.5 + panelMovement));
  }
  rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelRotatedToGantryTransform()
{
  vtkMRMLModelNode* rightImagingPanelModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(IMAGINGPANELRIGHT_MODEL_NAME));
  if (!rightImagingPanelModel)
  {
    vtkErrorMacro("rightImagingPanelModel: Invalid MRML model node!");
  }


  vtkPolyData* rightImagingPanelModelPolyData = rightImagingPanelModel->GetPolyData();

  double rightImagingPanelCenterOfMass[3];
  double rightImagingPanelModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  rightImagingPanelModelPolyData->GetBounds(rightImagingPanelModelBounds);
  rightImagingPanelCenterOfMass[0] = rightImagingPanelModelBounds[0];
  rightImagingPanelCenterOfMass[1] = (rightImagingPanelModelBounds[2] + rightImagingPanelModelBounds[3]) / 2;
  rightImagingPanelCenterOfMass[2] = (rightImagingPanelModelBounds[4] + rightImagingPanelModelBounds[5]) / 2;


  
  vtkMRMLLinearTransformNode* rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME) );
  vtkTransform* rightImagingPanelRotatedToGantryTransform = vtkTransform::SafeDownCast(
    rightImagingPanelRotatedToGantryTransformNode->GetTransformToParent() );

  rightImagingPanelRotatedToGantryTransform->Identity();
  
  rightImagingPanelRotatedToGantryTransform->Translate(rightImagingPanelCenterOfMass);
  rightImagingPanelRotatedToGantryTransform->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateRightImagingPanelTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateRightImagingPanelTranslationTransform: Invalid parameter set node!");
    return;
  }

  double panelMovement = parameterNode->GetImagingPanelMovement();

  vtkMRMLLinearTransformNode* rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME) );
  vtkTransform* rightImagingPanelTranslationTransform = vtkTransform::SafeDownCast(
    rightImagingPanelTranslationTransformNode->GetTransformToParent() );

  rightImagingPanelTranslationTransform->Identity();
  double translationArray[3] = { 0, -(panelMovement), 0 };
  rightImagingPanelTranslationTransform->Translate(translationArray);
  rightImagingPanelTranslationTransform->Modified();

}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateImagingPanelMovementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateImagingPanelMovementTransforms: Invalid parameter set node!");
    return;
  }

  this->UpdateLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform();
  this->UpdateLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform(parameterNode);
  this->UpdateLeftImagingPanelRotatedToGantryTransform();
  this->UpdateRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform();
  this->UpdateRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform(parameterNode);
  this->UpdateRightImagingPanelRotatedToGantryTransform();

  double panelMovement = parameterNode->GetImagingPanelMovement();
  if (panelMovement > 0) //TODO: This check should be in the two functions in the if branch?
  {
    this->UpdateLeftImagingPanelTranslationTransform(parameterNode);
    this->UpdateRightImagingPanelTranslationTransform(parameterNode);
  }
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportToFixedReferenceTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportToFixedReferenceTransform: Invalid parameter set node!");
    return;
  }

  double rotationAngle = parameterNode->GetPatientSupportRotationAngle();

  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME) );
  vtkTransform* patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(
    patientSupportToFixedReferenceTransformNode->GetTransformToParent() );

  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(rotationAngle);
  patientSupportToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME) );
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateTableTopEccentricRotationToPatientSupportTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateTableTopEccentricRotationToPatientSupportTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME) );
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransform->Identity();
  double translationArray[3] = {
    parameterNode->GetLateralTableTopDisplacement(), parameterNode->GetLongitudinalTableTopDisplacement(), parameterNode->GetVerticalTableTopDisplacement() };
  tableTopEccentricRotationToPatientSupportTransform->Translate(translationArray);
  tableTopEccentricRotationToPatientSupportTransform->Modified();

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("patientSupportModel: Invalid MRML model node!");
  }

  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();

  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);

  //TODO: This method does not use any input
  vtkMRMLLinearTransformNode* patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME) );
  vtkTransform* patientSupportScaledTranslatedToTableTopVerticalTranslationTransform = vtkTransform::SafeDownCast(
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->GetTransformToParent() );

  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, patientSupportModelBounds[4]};
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Translate(translationArray);
  patientSupportScaledTranslatedToTableTopVerticalTranslationTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME) );
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportScaledByTableTopVerticalMovementTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportScaledByTableTopVerticalMovementTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("patientSupportModel: Invalid MRML model node!");
  }

  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();

  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);
  double scaleFactor = abs((patientSupportModelBounds[4] + patientSupportModelBounds[5]) / 2);

  double tableTopDisplacement = parameterNode->GetVerticalTableTopDisplacement();

  vtkMRMLLinearTransformNode* patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME) );
  vtkTransform* patientSupportScaledByTableTopVerticalMovementTransform = vtkTransform::SafeDownCast(
    patientSupportScaledByTableTopVerticalMovementTransformNode->GetTransformToParent() );

  patientSupportScaledByTableTopVerticalMovementTransform->Identity();
  patientSupportScaledByTableTopVerticalMovementTransform->Scale(1, 1, ((abs(patientSupportModelBounds[5]) + tableTopDisplacement*0.525) / abs(patientSupportModelBounds[5])));
  patientSupportScaledByTableTopVerticalMovementTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME) );
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdatePatientSupportPositiveVerticalTranslationTransform(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdatePatientSupportScaledByTableTopVerticalMovementTransform: Invalid parameter set node!");
    return;
  }

  vtkMRMLModelNode* patientSupportModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORT_MODEL_NAME));
  if (!patientSupportModel)
  {
    vtkErrorMacro("patientSupportModel: Invalid MRML model node!");
  }

  vtkPolyData* patientSupportModelPolyData = patientSupportModel->GetPolyData();

  double patientSupportModelBounds[6] = { 0, 0, 0, 0, 0, 0 };

  patientSupportModelPolyData->GetBounds(patientSupportModelBounds);

  //TODO: This method does not use any input
  vtkMRMLLinearTransformNode* patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME) );
  vtkTransform* patientSupportPositiveVerticalTranslationTransform = vtkTransform::SafeDownCast(
    patientSupportPositiveVerticalTranslationTransformNode->GetTransformToParent() );

  patientSupportPositiveVerticalTranslationTransform->Identity();
  double translationArray[3] = { 0, 0, patientSupportModelBounds[4]*-1};
  patientSupportPositiveVerticalTranslationTransform->Translate(translationArray);
  patientSupportPositiveVerticalTranslationTransform->Modified();

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME) );
  vtkTransform* tableTopEccentricRotationToPatientSupportTransform = vtkTransform::SafeDownCast(
    tableTopEccentricRotationToPatientSupportTransformNode->GetTransformToParent() );

  tableTopEccentricRotationToPatientSupportTransformNode->GetMatrixTransformToWorld(this->TableTopToWorldTransformMatrix);
}

//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateVerticalDisplacementTransforms(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateVerticalDisplacementTransforms: Invalid parameter set node!");
    return;
  }

  this->UpdateTableTopEccentricRotationToPatientSupportTransform(parameterNode);
  this->UpdatePatientSupportScaledTranslatedToTableTopVerticalTranslationTransform(parameterNode);
  this->UpdatePatientSupportScaledByTableTopVerticalMovementTransform(parameterNode);
  this->UpdatePatientSupportPositiveVerticalTranslationTransform(parameterNode);
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateAdditionalCollimatorDevicesToCollimatorTransforms(vtkMRMLRoomsEyeViewNode* parameterNode)
{
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateAdditionalCollimatorDeviceToCollimatorTransforms: Invalid parameter set node!");
    return;
  }

  vtkMRMLLinearTransformNode* additionalCollimatorDeviceToCollimatorTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_NAME) );
  vtkTransform* additionalCollimatorDeviceToCollimatorTransform = vtkTransform::SafeDownCast(
    additionalCollimatorDeviceToCollimatorTransformNode->GetTransformToParent());
  
  double translationArray[3] = { parameterNode->GetAdditionalModelLateralDisplacement(), parameterNode->GetAdditionalModelLongitudinalDisplacement(),
    parameterNode->GetAdditionalModelVerticalDisplacement() };

  additionalCollimatorDeviceToCollimatorTransform->Identity();
  additionalCollimatorDeviceToCollimatorTransform->Translate(translationArray);
  additionalCollimatorDeviceToCollimatorTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(
    collimatorToGantryTransformNode->GetTransformToParent());

  collimatorToGantryTransformNode->GetMatrixTransformToWorld(this->CollimatorToWorldTransformMatrix);
}
//-----------------------------------------------------------------------------
void vtkSlicerRoomsEyeViewModuleLogic::UpdateAdditionalDevicesVisibility(vtkMRMLRoomsEyeViewNode* parameterNode){
  if (!parameterNode)
  {
    vtkErrorMacro("UpdateAdditionalDevicesVisibility: Invalid parameter set node!");
  }
  vtkMRMLModelNode* applicatorHolderModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(APPLICATORHOLDER_MODEL_NAME));
  if (!applicatorHolderModel)
  {
    vtkErrorMacro("applicatorHolderModel: Invalid model node!");
    return;
  }
  vtkMRMLModelNode* electronApplicatorModel = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName(ELECTRONAPPLICATOR_MODEL_NAME));
  if (!electronApplicatorModel)
  {
    vtkErrorMacro("electronApplicatorModel: Invalid model node!");
    return;
  }

  if (parameterNode->GetElectronApplicatorVisibility()){
   
    electronApplicatorModel->GetDisplayNode()->VisibilityOn();
  }
  else{
    electronApplicatorModel->GetDisplayNode()->VisibilityOff();
  }

  if (parameterNode->GetApplicatorHolderVisibility()){

    applicatorHolderModel->GetDisplayNode()->VisibilityOn();
  }
  else{
    applicatorHolderModel->GetDisplayNode()->VisibilityOff();
  }
  return;
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

  this->AdditionalModelsTableTopCollisionDetection->Update();
  if (this->AdditionalModelsTableTopCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between additional devices and table top\n";
  }

  this->AdditionalModelsPatientSupportCollisionDetection->Update();
  if (this->AdditionalModelsPatientSupportCollisionDetection->GetNumberOfContacts() > 0)
  {
    statusString = statusString + "Collision between additional devices and patient support\n";
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

/** This function was added to help with the issue of the vtkMRMLRTBeamNode::CalculateSourcePosition function not accounting for beam transformations, but has been unsuccessful thus far
bool vtkSlicerRoomsEyeViewModuleLogic::CalculateNewSourcePosition(vtkMRMLRTBeamNode* beamNode, double oldSourcePosition[3], double newSourcePosition[3]){
  vtkMatrix4x4* sourcePositionTransformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkTransform* sourcePositionTransform = vtkSmartPointer<vtkTransform>::New();
  vtkMRMLLinearTransformNode* sourcePositionTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(this->GetMRMLScene()->GetFirstNodeByName("NewBeam_7_Transform"));
  sourcePositionTransform = vtkTransform::SafeDownCast(sourcePositionTransformNode->GetTransformFromParent());
  sourcePositionTransform->TransformPoint(oldSourcePosition, newSourcePosition);

  return true;
}**/