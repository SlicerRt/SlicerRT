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

  This file was originally developed by Vinith Suriyakumar and Csaba Pinter,
  PerkLab, Queen's University and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Beams logic includes
#include "vtkSlicerIECTransformLogic.h"

#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

#include <vtkMRMLScene.h>
#include <vtkMRMLAbstractLogic.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkGeneralTransform.h>

//----------------------------------------------------------------------------
const char* vtkSlicerIECTransformLogic::FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME = "FixedReferenceToRasTransform";
const char* vtkSlicerIECTransformLogic::GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "GantryToFixedReferenceTransform";
const char* vtkSlicerIECTransformLogic::COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME = "CollimatorToGantryTransform";

const char* vtkSlicerIECTransformLogic::COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "CollimatorToFixedReferenceIsocenterTransform";
//TODO: 1. Rename as there is no CollimatorRotated coordinate system? (But there is a Collimator)
//TODO: 2. If CollimatorRotated and Collimator are the same then why are there two transforms that are the inverse of each other under each other?
//TODO: 3. The reason CollimatorRotated is very suspiciously identical to Collimator is that the transform chain is the following:
//         CollimatorToGantryTransform -> FixedReferenceIsocenterToCollimatorRotatedTransform -> CollimatorToFixedReferenceIsocenterTransform,
//         and here the coordinate frame names do not match. The first coordinate system of the first transform should be the same as the second of the second in order for this to be valid!
const char* vtkSlicerIECTransformLogic::FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME = "FixedReferenceIsocenterToCollimatorRotatedTransform";

const char* vtkSlicerIECTransformLogic::LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform";
const char* vtkSlicerIECTransformLogic::LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME = "LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform";
const char* vtkSlicerIECTransformLogic::LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME = "LeftImagingPanelTranslationTransform";
const char* vtkSlicerIECTransformLogic::LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME = "LeftImagingPanelRotatedToGantryTransform";

const char* vtkSlicerIECTransformLogic::RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform";
const char* vtkSlicerIECTransformLogic::RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME = "RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform";
const char* vtkSlicerIECTransformLogic::RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME = "RightImagingPanelTranslationTransform";
const char* vtkSlicerIECTransformLogic::RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME = "RightImagingPanelRotatedToGantryTransform";

const char* vtkSlicerIECTransformLogic::PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "PatientSupportToFixedReferenceTransform";
const char* vtkSlicerIECTransformLogic::PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME = "PatientSupportScaledByTableTopVerticalMovementTransform";
const char* vtkSlicerIECTransformLogic::PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME = "PatientSupportPositiveVerticalTranslationTransform";
const char* vtkSlicerIECTransformLogic::PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME = "PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform";
const char* vtkSlicerIECTransformLogic::TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME = "TableTopEccentricRotationToPatientSupportTransform";
const char* vtkSlicerIECTransformLogic::TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME = "TableTopToTableTopEccentricRotationTransform";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerIECTransformLogic);

//-----------------------------------------------------------------------------
vtkSlicerIECTransformLogic::vtkSlicerIECTransformLogic()
{
}

//-----------------------------------------------------------------------------
vtkSlicerIECTransformLogic::~vtkSlicerIECTransformLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateTransformForBeam(vtkMRMLRTBeamNode* beamNode)
{
  //TODO: Observe beam node's geometry modified event (vtkMRMLRTBeamNode::BeamGeometryModified)
  // and its parent plan's POI markups fiducial's point modified event (vtkMRMLMarkupsNode::PointModifiedEvent)
  // so that UpdateTransformsFromBeamGeometry is called. It may be needed to change the signature of the
  // update function. It may be also needed to store a reference to the beam node (see defined nodes in SlicerRT)

  vtkSmartPointer<vtkGeneralTransform> beamGeneralTransform = vtkSmartPointer<vtkGeneralTransform>::New();

  // Update transform for beam
  //TODO: Get transform directly without getting the ID first (GetParentTransformNode)
  vtkMRMLLinearTransformNode* beamTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetNodeByID(beamNode->GetTransformNodeID()));
  if (beamTransformNode)
  {
    //TODO: Get transform between Collimator and PatientSupport, once the GetTransformBetween has been generalized
    if (this->GetTransformBetween(Collimator, RAS, beamNode->GetScene(), beamGeneralTransform))
    {
      // Make hard copy of the transform so that it doesn't change when other beam transforms change
      vtkSmartPointer<vtkGeneralTransform> beamGeneralTransformCopy = vtkSmartPointer<vtkGeneralTransform>::New();
      beamGeneralTransformCopy->DeepCopy(beamGeneralTransform);

      // Set transform to beam node
      beamTransformNode->SetAndObserveTransformToParent(beamGeneralTransformCopy);

      // Update the name of the transform node too
      // (the user may have renamed the beam, but it's very expensive to update the transform name on every beam modified event)
      std::string transformName = std::string(beamNode->GetName()) + "_Transform";
    }  
  }

  this->UpdateTransformsFromBeam(beamNode);
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateTransformsFromBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateTransformsFromBeam: Invalid beam node");
    return;
  }
  vtkMRMLScene* scene = beamNode->GetScene();
  if (!scene)
  {
    vtkErrorMacro("UpdateTransformsFromBeam: Invalid MRML scene");
    return;
  }

  // Make sure the transform hierarchy is set up
  this->BuildIECTransformHierarchy(scene);

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  vtkTransform* gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent());
  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(beamNode->GetGantryAngle() * (-1.0));
  gantryToFixedReferenceTransform->Modified();

  //TODO: This should be CollimatorToGantry!
  vtkMRMLLinearTransformNode* fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));
  vtkTransform* fixedReferenceIsocenterToCollimatorRotatedTransform = vtkTransform::SafeDownCast(fixedReferenceIsocenterToCollimatorRotatedTransformNode->GetTransformToParent());
  fixedReferenceIsocenterToCollimatorRotatedTransform->Identity();
  fixedReferenceIsocenterToCollimatorRotatedTransform->RotateZ(beamNode->GetCollimatorAngle());
  fixedReferenceIsocenterToCollimatorRotatedTransform->Modified();

  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  vtkTransform* patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(patientSupportToFixedReferenceTransformNode->GetTransformToParent());
  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(beamNode->GetCouchAngle());
  patientSupportToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME));
  vtkTransform* fixedReferenceToRasTransform = vtkTransform::SafeDownCast(fixedReferenceToRasTransformNode->GetTransformToParent());
  fixedReferenceToRasTransform->Identity();
  // Apply isocenter translation
  double isocenterPosition[3] = { 0.0, 0.0, 0.0 };
  if (beamNode->GetPlanIsocenterPosition(isocenterPosition))
  {
    fixedReferenceToRasTransform->Translate(isocenterPosition[0], isocenterPosition[1], isocenterPosition[2]);
  }
  else
  {
    vtkErrorMacro("UpdateTransformsFromBeam: Failed to get isocenter position for beam " << beamNode->GetName());
  }
  // The "S" direction in RAS is the "A" direction in FixedReference 
  fixedReferenceToRasTransform->RotateX(-90);
  fixedReferenceToRasTransform->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::BuildIECTransformHierarchy(vtkMRMLScene* scene)
{
  if (!scene)
  {
    vtkErrorMacro("BuildIECTransformHierarchy: Invalid MRML scene");
    return;
  }

  // Create transform nodes if they do not exist
  vtkSmartPointer<vtkMRMLLinearTransformNode> fixedReferenceToRasTransformNode;
  if (!scene->GetFirstNodeByName(FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME))
  {
    fixedReferenceToRasTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    fixedReferenceToRasTransformNode->SetName(FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME);
    fixedReferenceToRasTransformNode->SetHideFromEditors(1);
    scene->AddNode(fixedReferenceToRasTransformNode);
  }
  else
  {
    fixedReferenceToRasTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> gantryToFixedReferenceTransformNode;
  if (!scene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME))
  {
    gantryToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    gantryToFixedReferenceTransformNode->SetName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    gantryToFixedReferenceTransformNode->SetHideFromEditors(1);
    scene->AddNode(gantryToFixedReferenceTransformNode);
  }
  else
  {
    gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToGantryTransformNode;
  if (!scene->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME))
  {
    collimatorToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToGantryTransformNode->SetName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME);
    collimatorToGantryTransformNode->SetHideFromEditors(1);
    scene->AddNode(collimatorToGantryTransformNode);
  }
  else
  {
    collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> collimatorToFixedReferenceIsocenterTransformNode;
  if (!scene->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME))
  {
    collimatorToFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    collimatorToFixedReferenceIsocenterTransformNode->SetName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    collimatorToFixedReferenceIsocenterTransformNode->SetHideFromEditors(1);
    scene->AddNode(collimatorToFixedReferenceIsocenterTransformNode);
  }
  else
  {
    collimatorToFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> fixedReferenceIsocenterToCollimatorRotatedTransformNode;
  if (!scene->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME))
  {
    fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    fixedReferenceIsocenterToCollimatorRotatedTransformNode->SetName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME);
    fixedReferenceIsocenterToCollimatorRotatedTransformNode->SetHideFromEditors(1);
    scene->AddNode(fixedReferenceIsocenterToCollimatorRotatedTransformNode);
  }
  else
  {
    fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode;
  if (!scene->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME))
  {
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->SetName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode->SetHideFromEditors(1);
    scene->AddNode(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode);
  }
  else
  {
    leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode;
  if (!scene->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME))
  {
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->SetName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode->SetHideFromEditors(1);
    scene->AddNode(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode);
  }
  else
  {
    leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelRotatedToGantryTransformNode;
  if (!scene->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME))
  {
    leftImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelRotatedToGantryTransformNode->SetName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    leftImagingPanelRotatedToGantryTransformNode->SetHideFromEditors(1);
    scene->AddNode(leftImagingPanelRotatedToGantryTransformNode);
  }
  else
  {
    leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> leftImagingPanelTranslationTransformNode;
  if (!scene->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME))
  {
    leftImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    leftImagingPanelTranslationTransformNode->SetName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    leftImagingPanelTranslationTransformNode->SetHideFromEditors(1);
    scene->AddNode(leftImagingPanelTranslationTransformNode);
  }
  else
  {
    leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode;
  if (!scene->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME))
  {
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->SetName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME);
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode->SetHideFromEditors(1);
    scene->AddNode(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode);
  }
  else
  {
    rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode;
  if (!scene->GetFirstNodeByName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME))
  {
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->SetName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME);
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode->SetHideFromEditors(1);
    scene->AddNode(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode);
  }
  else
  {
    rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelRotatedToGantryTransformNode;
  if (!scene->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME))
  {
    rightImagingPanelRotatedToGantryTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelRotatedToGantryTransformNode->SetName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME);
    rightImagingPanelRotatedToGantryTransformNode->SetHideFromEditors(1);
    scene->AddNode(rightImagingPanelRotatedToGantryTransformNode);
  }
  else
  {
    rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> rightImagingPanelTranslationTransformNode;
  if (!scene->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME))
  {
    rightImagingPanelTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    rightImagingPanelTranslationTransformNode->SetName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME);
    rightImagingPanelTranslationTransformNode->SetHideFromEditors(1);
    scene->AddNode(rightImagingPanelTranslationTransformNode);
  }
  else
  {
    rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportToFixedReferenceTransformNode;
  if (!scene->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME))
  {
    patientSupportToFixedReferenceTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportToFixedReferenceTransformNode->SetName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME);
    patientSupportToFixedReferenceTransformNode->SetHideFromEditors(1);
    scene->AddNode(patientSupportToFixedReferenceTransformNode);
  }
  else
  {
    patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledByTableTopVerticalMovementTransformNode;
  if (!scene->GetFirstNodeByName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME))
  {
    patientSupportScaledByTableTopVerticalMovementTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledByTableTopVerticalMovementTransformNode->SetName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME);
    patientSupportScaledByTableTopVerticalMovementTransformNode->SetHideFromEditors(1);
    scene->AddNode(patientSupportScaledByTableTopVerticalMovementTransformNode);
  }
  else
  {
    patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportPositiveVerticalTranslationTransformNode;
  if (!scene->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME))
  {
    patientSupportPositiveVerticalTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportPositiveVerticalTranslationTransformNode->SetName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME);
    patientSupportPositiveVerticalTranslationTransformNode->SetHideFromEditors(1);
    scene->AddNode(patientSupportPositiveVerticalTranslationTransformNode);
  }
  else
  {
    patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode;
  if (!scene->GetFirstNodeByName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME))
  {
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->SetName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME);
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode->SetHideFromEditors(1);
    scene->AddNode(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode);
  }
  else
  {
    patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopEccentricRotationToPatientSupportTransformNode;
  if (!scene->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME))
  {
    tableTopEccentricRotationToPatientSupportTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopEccentricRotationToPatientSupportTransformNode->SetName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME);
    tableTopEccentricRotationToPatientSupportTransformNode->SetHideFromEditors(1);
    scene->AddNode(tableTopEccentricRotationToPatientSupportTransformNode);
  }
  else
  {
    tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));
  }

  vtkSmartPointer<vtkMRMLLinearTransformNode> tableTopToTableTopEccentricRotationTransformNode;
  if (!scene->GetFirstNodeByName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME))
  {
    tableTopToTableTopEccentricRotationTransformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
    tableTopToTableTopEccentricRotationTransformNode->SetName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME);
    tableTopToTableTopEccentricRotationTransformNode->SetHideFromEditors(1);
    scene->AddNode(tableTopToTableTopEccentricRotationTransformNode);
  }
  else
  {
    tableTopToTableTopEccentricRotationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      scene->GetFirstNodeByName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME));
  }

  // Organize transforms into hierarchy based on IEC Standard 61217
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

  gantryToFixedReferenceTransformNode->SetAndObserveTransformNodeID(fixedReferenceToRasTransformNode->GetID());
  patientSupportToFixedReferenceTransformNode->SetAndObserveTransformNodeID(fixedReferenceToRasTransformNode->GetID());
}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame, vtkMRMLScene* scene, vtkGeneralTransform* outputTransform)
{
  if (!outputTransform)
  {
    vtkErrorMacro("GetTransformBetween: Invalid output transform node!");
    return false;
  }
  if (!scene)
  {
    vtkErrorMacro("UpdateTransformsFromBeam: Invalid MRML scene");
    return false;
  }

  vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(FIXEDREFERENCE_TO_RAS_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* collimatorToFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));

  vtkMRMLLinearTransformNode* patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    scene->GetFirstNodeByName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME));

  //TODO: Replace this huge list of static if/else code with dynamic code
  if (fromFrame == Gantry && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Collimator && toFrame == RAS)
  {
    //TODO: This is where it becomes clear that there is a problem with this transform.
    //      This query should go through Collimator -> Gantry -> FixedReference using CollimatorToGantryTransform and GantryToFixedReferenceTransform
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, NULL, outputTransform);
    return true;
  }

  else if (fromFrame == Collimator && toFrame == FixedReference)
  {
    //TODO: This is where it becomes clear that there is a problem with this transform.
    //      This query should go through Collimator -> Gantry -> FixedReference using CollimatorToGantryTransform and GantryToFixedReferenceTransform
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Collimator && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  
  // Left Imaging Panel
  else if (fromFrame == LeftImagingPanel && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanel && toFrame == Gantry )
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanel && toFrame == LeftImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanel && toFrame == LeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanel && toFrame == LeftImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, 
      leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelIsocenter && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelIsocenter && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelIsocenter && toFrame == LeftImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelIsocenter && toFrame == LeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelRotated && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelRotated && toFrame == LeftImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelTranslated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelTranslated && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  // Right Imaging Panel:
  else if (fromFrame == RightImagingPanel && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanel && toFrame == Gantry )
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanel && toFrame == RightImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanel && toFrame == RightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanel && toFrame == RightImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, 
      rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelIsocenter && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelIsocenter && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelIsocenter && toFrame == RightImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelIsocenter && toFrame == RightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelRotated && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelRotated && toFrame == RightImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelTranslated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelTranslated && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  // Patient Support
  else if (fromFrame == PatientSupport && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslated && toFrame == PatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslated && toFrame == PatientSupportScaledTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
   
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslated && toFrame == PatientSupportScaled)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, 
      patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportScaled && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode,
      fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportScaled && toFrame == PatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode,
      patientSupportToFixedReferenceTransformNode, outputTransform);
     return true;
  }

  else if (fromFrame == PatientSupportScaled && toFrame == PatientSupportScaledTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode,
      patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportScaledTranslated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportScaledTranslated && toFrame == PatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  // Table Top
  else if (fromFrame == TableTop && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == TableTopEccentricRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, fixedReferenceToRasTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == TableTop && toFrame == PatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == TableTopEccentricRotated && toFrame == PatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == TableTop && toFrame == TableTopEccentricRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
    return true;
  }

  // Inverse
  if (fromFrame == FixedReference && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference  && toFrame == Collimator)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, fixedReferenceIsocenterToCollimatorRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry  && toFrame == Collimator)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, fixedReferenceIsocenterToCollimatorRotatedTransformNode, outputTransform);
    return true;
  }

  // Left Imaging Panel
  else if (fromFrame == FixedReference && toFrame == LeftImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame == LeftImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelTranslated  && toFrame == LeftImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelRotated  && toFrame == LeftImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelIsocenter && toFrame == LeftImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, 
      leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry  && toFrame == LeftImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelTranslated && toFrame == LeftImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelRotated  && toFrame == LeftImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference  && toFrame == LeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame == LeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == LeftImagingPanelTranslated && toFrame == LeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame == LeftImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  // Right Imaging Panel:
  else if (fromFrame == FixedReference && toFrame == RightImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame ==  RightImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelTranslated && toFrame == RightImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelRotated && toFrame == RightImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, 
      rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelIsocenter && toFrame == RightImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, 
      rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == RightImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame == RightImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelTranslated && toFrame == RightImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelRotated && toFrame == RightImagingPanelIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == RightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame == RightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == RightImagingPanelTranslated && toFrame == RightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == RightImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry && toFrame == RightImagingPanelTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }

  // Patient Support
  else if (fromFrame == FixedReference && toFrame == PatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == PatientSupportPositiveVerticalTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupport && toFrame == PatientSupportPositiveVerticalTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportScaledTranslated && toFrame == PatientSupportPositiveVerticalTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;

  }

  else if (fromFrame == PatientSupportScaled && toFrame == PatientSupportPositiveVerticalTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode, 
      patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == PatientSupportScaled)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupport  && toFrame == PatientSupportScaled)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode,
      patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportScaledTranslated && toFrame == PatientSupportScaled)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode,
      patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == PatientSupportScaledTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupport && toFrame == PatientSupportScaledTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }

  // Table Top
  else if (fromFrame == FixedReference && toFrame == TableTop)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, tableTopToTableTopEccentricRotationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == TableTopEccentricRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceToRasTransformNode, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupport && toFrame == TableTop)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupport && toFrame == TableTopEccentricRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == TableTopEccentricRotated && toFrame == TableTop)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, tableTopToTableTopEccentricRotationTransformNode, outputTransform);
    return true;
  }

  return false;
}
