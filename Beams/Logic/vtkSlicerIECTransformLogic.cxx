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

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

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

  vtkMRMLRTPlanNode* parentPlan = beamNode->GetParentPlanNode();
  

}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame, vtkTransform* outputTransform)
{
  if (!outputTransform)
  {
    vtkErrorMacro("GetTransformBetween: Invalid output transform node!");
    return false;
  }

  outputTransform->PreMultiply();
  //TODO:

  if (fromFrame == GantryToFixedReference && toFrame == FixedReference)
  {
    outputTransform = this->GantryToFixedReferenceTransform;
  }

  else if (fromFrame == CollimatorToGantry && toFrame == FixedReference)
  {
    outputTransform = this->CollimatorToGantryTransform;
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == CollimatorToGantry && toFrame == GantryToFixedReference )
  {
    outputTransform = this->CollimatorToGantryTransform;
  }


 //Left Imaging Panel:
  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == FixedReference)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == GantryToFixedReference )
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
  }

  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelTranslation)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
  }

  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelRotatedToGantry)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
  }

  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
  }

  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == FixedReference)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == GantryToFixedReference)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
  }

  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == LeftImagingPanelTranslation)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
  }

  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == LeftImagingPanelRotatedToGantry)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
  }

  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == FixedReference)
  {
    outputTransform = this->LeftImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == GantryToFixedReference)
  {
    outputTransform = this->LeftImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
  }

  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == LeftImagingPanelTranslation)
  {
    outputTransform = this->LeftImagingPanelRotatedToGantryTransform;
  }

  else if (fromFrame == LeftImagingPanelTranslation && toFrame == FixedReference)
  {
    outputTransform = this->LeftImagingPanelTranslationTransform;
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == LeftImagingPanelTranslation && toFrame == GantryToFixedReference)
  {
    outputTransform = this->LeftImagingPanelTranslationTransform;
  }

  //Right Imaging Panel:
  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == FixedReference)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == GantryToFixedReference)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
  }

  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelTranslation)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
  }

  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelRotatedToGantry)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
  }

  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
  }

  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == FixedReference)
  {
    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == GantryToFixedReference)
  {
    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
  }

  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == RightImagingPanelTranslation)
  {
    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
  }

  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == RightImagingPanelRotatedToGantry)
  {

    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
  }

  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == FixedReference)
  {
    outputTransform = this->RightImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == GantryToFixedReference)
  {
    outputTransform = this->RightImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
  }

  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == RightImagingPanelTranslation)
  {
    outputTransform = this->RightImagingPanelRotatedToGantryTransform;
  }

  else if (fromFrame == RightImagingPanelTranslation && toFrame == FixedReference)
  {
    outputTransform = this->RightImagingPanelTranslationTransform;
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
  }

  else if (fromFrame == RightImagingPanelTranslation && toFrame == GantryToFixedReference)
  {
    outputTransform = this->RightImagingPanelTranslationTransform;
  }

  //Patient Support
  else if (fromFrame == PatientSupportToFixedReference && toFrame == FixedReference)
  {
    outputTransform = this->PatientSupportToFixedReferenceTransform;
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == FixedReference)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportScaledByTableTopVerticalMovementTransform);
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
    outputTransform->Concatenate(this->PatientSupportToFixedReferenceTransform);
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == PatientSupportToFixedReference)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportScaledByTableTopVerticalMovementTransform);
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportScaledByTableTopVerticalMovementTransform);
   
  }

  else if (fromFrame == PatientSupportPositiveVerticalTranslation && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
  }

  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == FixedReference)
  {
    outputTransform = this->PatientSupportScaledByTableTopVerticalMovementTransform;
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
    outputTransform->Concatenate(this->PatientSupportToFixedReferenceTransform);
  }

  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == PatientSupportToFixedReference)
  {
    outputTransform = this->PatientSupportScaledByTableTopVerticalMovementTransform;
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
  }

  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    outputTransform = this->PatientSupportScaledByTableTopVerticalMovementTransform;
  }

  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == FixedReference)
  {
    outputTransform = this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportToFixedReferenceTransform);
  }

  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == PatientSupportToFixedReference)
  {
    outputTransform = this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform;
  }

  // Table Top
  else if (fromFrame == TableTopToTableEccentricRotation && toFrame == FixedReference)
  {
    outputTransform = this->TableTopToTableTopEccentricRotationTransform;
    outputTransform->Concatenate(TableTopEccentricRotationToPatientSupportTransform);
    outputTransform->Concatenate(PatientSupportToFixedReferenceTransform);
  }

  else if (fromFrame == TableTopEccentricRotationToPatientSupport && toFrame == FixedReference)
  {
    outputTransform = this->TableTopEccentricRotationToPatientSupportTransform;
    outputTransform->Concatenate(PatientSupportToFixedReferenceTransform);
  }

  else if (fromFrame == TableTopToTableEccentricRotation && toFrame == PatientSupportToFixedReference)
  {
    outputTransform = this->TableTopToTableTopEccentricRotationTransform;
    outputTransform->Concatenate(TableTopEccentricRotationToPatientSupportTransform);
  }

  else if (fromFrame == TableTopEccentricRotationToPatientSupport && toFrame == PatientSupportToFixedReference)
  {
    outputTransform = this->TableTopEccentricRotationToPatientSupportTransform;
  }

  else if (fromFrame == TableTopToTableEccentricRotation && toFrame == TableTopEccentricRotationToPatientSupport)
  {
    outputTransform = this->TableTopToTableTopEccentricRotationTransform;
  }

  //Inverse:

  if (fromFrame == FixedReference && toFrame == GantryToFixedReference)
  {
    
    outputTransform = this->GantryToFixedReferenceTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == CollimatorToGantry )
  {
    outputTransform = this->CollimatorToGantryTransform;
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == CollimatorToGantry)
  {
    outputTransform = this->CollimatorToGantryTransform;
    outputTransform->Inverse();
  }
  
  // Left Imaging Panel
  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == LeftImagingPanelTranslation && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == LeftImagingPanelRotatedToGantry && toFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenter && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    outputTransform = this->LeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == LeftImagingPanelTranslation && toFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated)
  {
    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->LeftImagingPanelRotatedToGantryTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated && toFrame == LeftImagingPanelRotatedToGantry)
  {

    outputTransform = this->LeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelRotatedToGantry)
  {
    outputTransform = this->LeftImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelRotatedToGantry)
  {
    outputTransform = this->LeftImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->LeftImagingPanelTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == LeftImagingPanelTranslation && toFrame == LeftImagingPanelRotatedToGantry)
  {
    outputTransform = this->LeftImagingPanelRotatedToGantryTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == LeftImagingPanelTranslation)
  {
    outputTransform = this->LeftImagingPanelTranslationTransform;
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == LeftImagingPanelTranslation)
  {
    outputTransform = this->LeftImagingPanelTranslationTransform;
    outputTransform->Inverse();
  }

  //Right Imaging Panel:
  else if (fromFrame == FixedReference && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == RightImagingPanelTranslation && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == RightImagingPanelRotatedToGantry && toFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Concatenate(this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == RightImagingPanelToRightImagingPanelFixedReferenceIsocenter && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    outputTransform = this->RightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == RightImagingPanelTranslation && toFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated)
  {
    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Concatenate(this->RightImagingPanelRotatedToGantryTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotated && toFrame == RightImagingPanelRotatedToGantry)
  {

    outputTransform = this->RightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == RightImagingPanelRotatedToGantry)
  {
    outputTransform = this->RightImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelRotatedToGantry)
  {
    outputTransform = this->RightImagingPanelRotatedToGantryTransform;
    outputTransform->Concatenate(this->RightImagingPanelTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == RightImagingPanelTranslation && toFrame == RightImagingPanelRotatedToGantry)
  {
    outputTransform = this->RightImagingPanelRotatedToGantryTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == RightImagingPanelTranslation)
  {
    outputTransform = this->RightImagingPanelTranslationTransform;
    outputTransform->Concatenate(this->GantryToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == GantryToFixedReference && toFrame == RightImagingPanelTranslation)
  {
    outputTransform = this->RightImagingPanelTranslationTransform;
    outputTransform->Inverse();
  }

  //Patient Support
  else if (fromFrame == FixedReference && toFrame == PatientSupportToFixedReference)
  {
    outputTransform = this->PatientSupportToFixedReferenceTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportScaledByTableTopVerticalMovementTransform);
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
    outputTransform->Concatenate(this->PatientSupportToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportToFixedReference  && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportScaledByTableTopVerticalMovementTransform);
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation  && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportScaledByTableTopVerticalMovementTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportScaledByTableTopVerticalMovement  && toFrame == PatientSupportPositiveVerticalTranslation)
  {
    outputTransform = this->PatientSupportPositiveVerticalTranslationTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference  && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    outputTransform = this->PatientSupportScaledByTableTopVerticalMovementTransform;
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
    outputTransform->Concatenate(this->PatientSupportToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportToFixedReference  && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    outputTransform = this->PatientSupportScaledByTableTopVerticalMovementTransform;
    outputTransform->Concatenate(this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation && toFrame == PatientSupportScaledByTableTopVerticalMovement)
  {
    outputTransform = this->PatientSupportScaledByTableTopVerticalMovementTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference  && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    outputTransform = this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform;
    outputTransform->Concatenate(this->PatientSupportToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportToFixedReference  && toFrame == PatientSupportScaledTranslatedToTableTopVerticalTranslation)
  {
    outputTransform = this->PatientSupportScaledTranslatedToTableTopVerticalTranslationTransform;
    outputTransform->Inverse();
  }

  //Table Top:
  else if (fromFrame == FixedReference && toFrame == TableTopToTableEccentricRotation)
  {
    outputTransform = this->TableTopToTableTopEccentricRotationTransform;
    outputTransform->Concatenate(TableTopEccentricRotationToPatientSupportTransform);
    outputTransform->Concatenate(PatientSupportToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == FixedReference && toFrame == TableTopEccentricRotationToPatientSupport)
  {
    outputTransform = this->TableTopEccentricRotationToPatientSupportTransform;
    outputTransform->Concatenate(PatientSupportToFixedReferenceTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportToFixedReference && toFrame == TableTopToTableEccentricRotation)
  {
    outputTransform = this->TableTopToTableTopEccentricRotationTransform;
    outputTransform->Concatenate(TableTopEccentricRotationToPatientSupportTransform);
    outputTransform->Inverse();
  }

  else if (fromFrame == PatientSupportToFixedReference  && toFrame == TableTopEccentricRotationToPatientSupport)
  {
    outputTransform = this->TableTopEccentricRotationToPatientSupportTransform;
    outputTransform->Inverse();
  }

  else if (fromFrame == TableTopEccentricRotationToPatientSupport && toFrame == TableTopToTableEccentricRotation)
  {
    outputTransform = this->TableTopToTableTopEccentricRotationTransform;
    outputTransform->Inverse();
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateTransformsFromBeamGeometry(vtkMRMLRTBeamNode* beamNode)
{
}
