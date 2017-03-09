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
static const char* GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME = "GantryToFixedReferenceTransform";
static const char* COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME = "CollimatorToFixedReferenceIsocenterTransform";
static const char* FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME = "FixedReferenceIsocenterToCollimatorRotatedTransform";
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
void vtkSlicerIECTransformLogic::SetAndObserveBeamNode(vtkMRMLRTBeamNode* beamNode, unsigned long event)
{
  //TODO: Observe beam node's geometry modified event (vtkMRMLRTBeamNode::BeamGeometryModified)
  // and its parent plan's POI markups fiducial's point modified event (vtkMRMLMarkupsNode::PointModifiedEvent)
  // so that UpdateTransformsFromBeamGeometry is called. It may be needed to change the signature of the
  // update function. It may be also needed to store a reference to the beam node (see defined nodes in SlicerRT)

  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  if (!parentPlanNode)
  {
    vtkErrorMacro("Failed to access parent plan node");
  }

  vtkGeneralTransform* beamGeneralTransform = vtkGeneralTransform::New();

    // Set transform to transform node
  vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetNodeByID(beamNode->GetTransformNodeID()));
  if (transformNode){
    if (GetTransformBetween(FixedReferenceIsocenterToCollimatorRotated, FixedReference, beamNode, beamGeneralTransform)){
      transformNode->SetAndObserveTransformToParent(beamGeneralTransform);
      // Update the name of the transform node too
      // (the user may have renamed the beam, but it's very expensive to update the transform name on every beam modified event)
      std::string transformName = std::string(beamNode->GetName()) + "_Transform";
      transformNode->SetName(transformName.c_str());
    }  
  }

  this->UpdateTransformsFromBeamGeometry(beamNode);

  //TODO: Implement observe beam geometry modified event and parent plan's POI markups fiducial's point modified event , unsure of how to get second modified event.
  //if (event == vtkMRMLRTBeamNode::BeamGeometryModified){
   
  //}
  //else{
    //vtkErrorMacro("Not yet implemented!");
  //}
   //TODO:
}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame, vtkMRMLRTBeamNode* beamNode, vtkGeneralTransform* outputTransform)
{
  if (!outputTransform)
  {
    vtkErrorMacro("GetTransformBetween: Invalid output transform node!");
    return false;
  }

  // TODO: Quite expensive to get all of these nodes independently from the input arguments
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* collimatorToFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* leftImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* leftImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* rightImagingPanelRotatedToGantryTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* rightImagingPanelTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* patientSupportScaledByTableTopVerticalMovementTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* patientSupportPositiveVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* tableTopToTableTopEccentricRotationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(TABLETOP_TO_TABLETOPECCENENTRICROTATION_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* tableTopEccentricRotationToPatientSupportTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_NAME));
  vtkMRMLLinearTransformNode* patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_NAME));

  //TODO: Explore the possibility of simply replacing the frame enums with the node pointers and not use this huge list of if's
  if (fromFrame == GantryToFixedReference && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReferenceIsocenterToCollimatorRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReferenceIsocenterToCollimatorRotated && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  // Left Imaging Panel
  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == GantryToFixedReference )
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, 
      leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == LeftImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == LeftImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == LeftImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelTranslation && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelTranslation && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  // Right Imaging Panel:
  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == GantryToFixedReference )
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, 
      rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == RightImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == RightImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == RightImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelTranslation && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelTranslation && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  // Patient Support
  else if (fromFrame == PatientSupportToFixedReference && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == PatientSupportToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, 
      patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode,
      NULL, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == PatientSupportToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode,
      patientSupportToFixedReferenceTransformNode, outputTransform);
     return true;
  }
  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode,
      patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode,NULL, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == PatientSupportToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  // Table Top
  else if (fromFrame == TableTopToTableEccentricRotation && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == TableTopEccentricRotationToPatientSupport && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, NULL, outputTransform);
    return true;
  }
  else if (fromFrame == TableTopToTableEccentricRotation && toFrame == PatientSupportToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == TableTopEccentricRotationToPatientSupport && toFrame == PatientSupportToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == TableTopToTableEccentricRotation && toFrame == TableTopEccentricRotationToPatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
    return true;
  }

  // Inverse
  if (fromFrame == FixedReference && toFrame == GantryToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference  && toFrame == FixedReferenceIsocenterToCollimatorRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, fixedReferenceIsocenterToCollimatorRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference  && toFrame == FixedReferenceIsocenterToCollimatorRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, fixedReferenceIsocenterToCollimatorRotatedTransformNode, outputTransform);
    return true;
  }
  // Left Imaging Panel
  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelTranslation  && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelRotatedToGantry  && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, 
      leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference  && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelTranslation && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelRotatedToGantry  && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference  && toFrame == LeftImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == LeftImagingPanelTranslation && toFrame == LeftImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, leftImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  // Right Imaging Panel:
  else if (fromFrame == FixedReference && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame ==  RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelTranslation && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, 
      rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, 
      rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelTranslation && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == RightImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == RightImagingPanelTranslation && toFrame == RightImagingPanelRotatedToGantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == RightImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, rightImagingPanelTranslationTransformNode, outputTransform);
    return true;
  }
  // Patient Support
  else if (fromFrame == FixedReference && toFrame == PatientSupportToFixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportToFixedReference && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledByTableTopVerticalMovementTransformNode, 
      patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportToFixedReference  && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode,
      patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode,
      patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportToFixedReference && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
    return true;
  }
  // Table Top
  else if (fromFrame == FixedReference && toFrame == TableTopToTableEccentricRotation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, tableTopToTableTopEccentricRotationTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == FixedReference && toFrame == TableTopEccentricRotationToPatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportToFixedReference && toFrame == TableTopToTableEccentricRotation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == PatientSupportToFixedReference && toFrame == TableTopEccentricRotationToPatientSupport)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
    return true;
  }
  else if (fromFrame == TableTopEccentricRotationToPatientSupport && toFrame == TableTopToTableEccentricRotation)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, tableTopToTableTopEccentricRotationTransformNode, outputTransform);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateTransformsFromBeamGeometry(vtkMRMLRTBeamNode* beamNode)
{
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
   GantryToFixedReferenceTransform = vtkTransform::SafeDownCast(
    gantryToFixedReferenceTransformNode->GetTransformToParent());

  vtkMRMLLinearTransformNode* fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));
   FixedReferenceIsocenterToCollimatorRotatedTransform = vtkTransform::SafeDownCast(
    fixedReferenceIsocenterToCollimatorRotatedTransformNode->GetTransformToParent());

   vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
     beamNode->GetScene()->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
   PatientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(
     patientSupportToFixedReferenceTransformNode->GetTransformToParent());
  
   GantryToFixedReferenceTransform->Identity();
   GantryToFixedReferenceTransform->RotateY(beamNode->GetGantryAngle());
   GantryToFixedReferenceTransform->Modified();

   FixedReferenceIsocenterToCollimatorRotatedTransform->Identity();
   FixedReferenceIsocenterToCollimatorRotatedTransform->RotateZ(beamNode->GetCollimatorAngle());
   FixedReferenceIsocenterToCollimatorRotatedTransform->Modified();

   PatientSupportToFixedReferenceTransform->Identity();
   PatientSupportToFixedReferenceTransform->RotateZ(beamNode->GetCouchAngle());
   PatientSupportToFixedReferenceTransform->Modified();
}
