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
void vtkSlicerIECTransformLogic::SetAndObserveBeamNode(vtkMRMLRTBeamNode* beamNode)
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
    if (GetTransformBetween(CollimatorRotated, FixedReference, beamNode, beamGeneralTransform)){
      double isocenterPosition[3] = { 0.0, 0.0, 0.0 };
      if (!beamNode->GetPlanIsocenterPosition(isocenterPosition))
      {
        vtkErrorMacro("UpdateTransform: Failed to get isocenter position");
        return;
      }
      vtkSmartPointer<vtkTransform> transform2 = vtkSmartPointer<vtkTransform>::New();
      transform2->Identity();
      transform2->Translate(isocenterPosition[0], isocenterPosition[1], isocenterPosition[2]);

      beamGeneralTransform->PostMultiply();
      beamGeneralTransform->Concatenate(transform2->GetMatrix());
      transformNode->SetAndObserveTransformToParent(beamGeneralTransform);
      // Update the name of the transform node too
      // (the user may have renamed the beam, but it's very expensive to update the transform name on every beam modified event)
      std::string transformName = std::string(beamNode->GetName()) + "_Transform";
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

  //TODO: Replace this huge list of static if/else code with dynamic code
  if (fromFrame == Gantry && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, NULL, outputTransform);
    return true;
  }

  else if (fromFrame == CollimatorRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, NULL, outputTransform);
    return true;
  }

  else if (fromFrame == CollimatorRotated && toFrame == Gantry)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(fixedReferenceIsocenterToCollimatorRotatedTransformNode, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }
  
  // Left Imaging Panel
  else if (fromFrame == LeftImagingPanel && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelRotatedToGantryTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(leftImagingPanelTranslationTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelRotatedToGantryTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(rightImagingPanelTranslationTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportToFixedReferenceTransformNode, NULL, outputTransform);
    return true;
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportPositiveVerticalTranslationTransformNode, NULL, outputTransform);
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
      NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode,NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopToTableTopEccentricRotationTransformNode, NULL, outputTransform);
    return true;
  }

  else if (fromFrame == TableTopEccentricRotated && toFrame == FixedReference)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(tableTopEccentricRotationToPatientSupportTransformNode, NULL, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, gantryToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference  && toFrame == CollimatorRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, fixedReferenceIsocenterToCollimatorRotatedTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == Gantry  && toFrame == CollimatorRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(gantryToFixedReferenceTransformNode, fixedReferenceIsocenterToCollimatorRotatedTransformNode, outputTransform);
    return true;
  }

  // Left Imaging Panel
  else if (fromFrame == FixedReference && toFrame == LeftImagingPanel)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelRotatedToGantryTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, leftImagingPanelTranslationTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelRotatedToGantryTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, rightImagingPanelTranslationTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportToFixedReferenceTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == PatientSupportPositiveVerticalTranslated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportPositiveVerticalTranslationTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportScaledByTableTopVerticalMovementTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, patientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode, outputTransform);
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
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, tableTopToTableTopEccentricRotationTransformNode, outputTransform);
    return true;
  }

  else if (fromFrame == FixedReference && toFrame == TableTopEccentricRotated)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(NULL, tableTopEccentricRotationToPatientSupportTransformNode, outputTransform);
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

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateTransformsFromBeamGeometry(vtkMRMLRTBeamNode* beamNode)
{
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
  this->GantryToFixedReferenceTransform = vtkTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent());

  vtkMRMLLinearTransformNode* fixedReferenceIsocenterToCollimatorRotatedTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetScene()->GetFirstNodeByName(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_TRANSFORM_NODE_NAME));
  this->FixedReferenceIsocenterToCollimatorRotatedTransform = vtkTransform::SafeDownCast(fixedReferenceIsocenterToCollimatorRotatedTransformNode->GetTransformToParent());

   vtkMRMLLinearTransformNode* patientSupportToFixedReferenceTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
     beamNode->GetScene()->GetFirstNodeByName(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_NAME));
   this->PatientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(patientSupportToFixedReferenceTransformNode->GetTransformToParent());
  
   this->GantryToFixedReferenceTransform->Identity();
   this->GantryToFixedReferenceTransform->RotateY(beamNode->GetGantryAngle());
   this->GantryToFixedReferenceTransform->Modified();

   this->FixedReferenceIsocenterToCollimatorRotatedTransform->Identity();
   this->FixedReferenceIsocenterToCollimatorRotatedTransform->RotateZ(beamNode->GetCollimatorAngle());
   this->FixedReferenceIsocenterToCollimatorRotatedTransform->Modified();

   this->PatientSupportToFixedReferenceTransform->Identity();
   this->PatientSupportToFixedReferenceTransform->RotateZ(beamNode->GetCouchAngle());
   this->PatientSupportToFixedReferenceTransform->Modified();
}
