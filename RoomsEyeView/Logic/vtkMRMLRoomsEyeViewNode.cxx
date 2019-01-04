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

// MRMLDoseAccumulation includes
#include "vtkMRMLRoomsEyeViewNode.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLSegmentationNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
static const char* GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE = "gantryToFixedReferenceTransformRef";

static const char* COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_NODE_REFERENCE_ROLE = "collimatorToFixedReferenceIsocenterTransformRef";
static const char* FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_NODE_REFERENCE_ROLE = "fixedReferenceIsocenterToCollimatorTransformRef";
static const char* COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE = "collimatorToGantryTransformRef";
static const char* ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_REFERENCE_ROLE = "additionalCollimatorDevicesToCollimatorTransformRef";

static const char* LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE = "leftImagingPanelToLeftImagingPanelTransformRef";
static const char* LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_REFERENCE_ROLE = "leftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotated";
static const char* LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE = "leftImagingPanelRotatedToGantryTransformRef";
static const char* LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE = "leftImagingPanelTranslationTransformRef";

static const char* RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE = "rightImagingPanelToRightImagingPanelTransformRef";
static const char* RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_REFERENCE_ROLE = "rightImagingPanelOriginToRightImagingPanelRotated";
static const char* RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE = "rightImagingPanelRotatedToGantryTransformRef";
static const char* RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE = "rightImagingPanelTranslationTransformRef";


static const char* PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE = "patientSupportToFixedReferenceTransformRef";
static const char* PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_REFERENCE_ROLE = "patientSupportScaledByTableTopVerticalMovementTransformRef";
static const char* PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE = "patientSupportPositiveVerticalTranslationTransformRef";
static const char* PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE = "patientSupportScaledTranslatedToTableTopVerticalTranslationTransformRef";
static const char* TABLETOP_TO_TABLETOPECCENTRICROTATION_TRANSFORM_NODE_REFERENCE_ROLE = "tableTopToTableTopEccentricRotationTransformRef";
static const char* TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_REFERENCE_ROLE = "tableTopEccentricToPatientSupportTransformRef";

static const char* BEAM_REFERENCE_ROLE = "beamRef";
static const char* PATIENT_BODY_SEGMENTATION_REFERENCE_ROLE = "patientBodySegmentationRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRoomsEyeViewNode);

//----------------------------------------------------------------------------
vtkMRMLRoomsEyeViewNode::vtkMRMLRoomsEyeViewNode()
  : CollisionDetectionEnabled(true)
  , GantryRotationAngle(0.0)
  , CollimatorRotationAngle(0.0)
  , ImagingPanelMovement(-68.50)
  , PatientSupportRotationAngle(0.0)
  , VerticalTableTopDisplacement(0.0)
  , LongitudinalTableTopDisplacement(0.0)
  , LateralTableTopDisplacement(0.0)
  , AdditionalModelVerticalDisplacement(0.0)
  , AdditionalModelLateralDisplacement(0.0)
  , AdditionalModelLongitudinalDisplacement(0.0)
  , PatientBodySegmentID(NULL)
  , TreatmentMachineType(NULL)
{
  this->SetSingletonTag("IEC");
}

//----------------------------------------------------------------------------
vtkMRMLRoomsEyeViewNode::~vtkMRMLRoomsEyeViewNode()
{
  this->SetPatientBodySegmentID(NULL);
  this->SetTreatmentMachineType(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLBooleanMacro(CollisionDetectionEnabled, CollisionDetectionEnabled);
  vtkMRMLWriteXMLFloatMacro(GantryRotationAngle, GantryRotationAngle);
  vtkMRMLWriteXMLFloatMacro(CollimatorRotationAngle, CollimatorRotationAngle);
  vtkMRMLWriteXMLFloatMacro(ImagingPanelMovement, ImagingPanelMovement);
  vtkMRMLWriteXMLFloatMacro(PatientSupportRotationAngle, PatientSupportRotationAngle);
  vtkMRMLWriteXMLFloatMacro(VerticalTableTopDisplacement, VerticalTableTopDisplacement);
  vtkMRMLWriteXMLFloatMacro(LongitudinalTableTopDisplacement, LongitudinalTableTopDisplacement);
  vtkMRMLWriteXMLFloatMacro(LateralTableTopDisplacement, LateralTableTopDisplacement);
  vtkMRMLWriteXMLFloatMacro(AdditionalModelVerticalDisplacement, AdditionalModelVerticalDisplacement);
  vtkMRMLWriteXMLFloatMacro(AdditionalModelLongitudinalDisplacement, AdditionalModelLongitudinalDisplacement);
  vtkMRMLWriteXMLFloatMacro(AdditionalModelLateralDisplacement, AdditionalModelLateralDisplacement);
  vtkMRMLWriteXMLStringMacro(PatientBodySegmentID, PatientBodySegmentID);
  vtkMRMLWriteXMLStringMacro(TreatmentMachineType, TreatmentMachineType);
  vtkMRMLWriteXMLEndMacro(); 
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  vtkMRMLNode::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLBooleanMacro(CollisionDetectionEnabled, CollisionDetectionEnabled);
  vtkMRMLReadXMLFloatMacro(GantryRotationAngle, GantryRotationAngle);
  vtkMRMLReadXMLFloatMacro(CollimatorRotationAngle, CollimatorRotationAngle);
  vtkMRMLReadXMLFloatMacro(ImagingPanelMovement, ImagingPanelMovement);
  vtkMRMLReadXMLFloatMacro(PatientSupportRotationAngle, PatientSupportRotationAngle);
  vtkMRMLReadXMLFloatMacro(VerticalTableTopDisplacement, VerticalTableTopDisplacement);
  vtkMRMLReadXMLFloatMacro(LongitudinalTableTopDisplacement, LongitudinalTableTopDisplacement);
  vtkMRMLReadXMLFloatMacro(LateralTableTopDisplacement, LateralTableTopDisplacement);
  vtkMRMLReadXMLFloatMacro(AdditionalModelVerticalDisplacement, AdditionalModelVerticalDisplacement);
  vtkMRMLReadXMLFloatMacro(AdditionalModelLongitudinalDisplacement, AdditionalModelLongitudinalDisplacement);
  vtkMRMLReadXMLFloatMacro(AdditionalModelLateralDisplacement, AdditionalModelLateralDisplacement);
  vtkMRMLReadXMLStringMacro(PatientBodySegmentID, PatientBodySegmentID);
  vtkMRMLReadXMLStringMacro(TreatmentMachineType, TreatmentMachineType);
  vtkMRMLReadXMLEndMacro(); 

  this->EndModify(disabledModify);

  // Note: ReportString is not read from XML, it is a strictly temporary value
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRoomsEyeViewNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyBooleanMacro(CollisionDetectionEnabled, CollisionDetectionEnabled);
  vtkMRMLCopyFloatMacro(GantryRotationAngle, GantryRotationAngle);
  vtkMRMLCopyFloatMacro(CollimatorRotationAngle, CollimatorRotationAngle);
  vtkMRMLCopyFloatMacro(ImagingPanelMovement, ImagingPanelMovement);
  vtkMRMLCopyFloatMacro(PatientSupportRotationAngle, PatientSupportRotationAngle);
  vtkMRMLCopyFloatMacro(VerticalTableTopDisplacement, VerticalTableTopDisplacement);
  vtkMRMLCopyFloatMacro(LongitudinalTableTopDisplacement, LongitudinalTableTopDisplacement);
  vtkMRMLCopyFloatMacro(LateralTableTopDisplacement, LateralTableTopDisplacement);
  vtkMRMLCopyFloatMacro(AdditionalModelVerticalDisplacement, AdditionalModelVerticalDisplacement);
  vtkMRMLCopyFloatMacro(AdditionalModelLongitudinalDisplacement, AdditionalModelLongitudinalDisplacement);
  vtkMRMLCopyFloatMacro(AdditionalModelLateralDisplacement, AdditionalModelLateralDisplacement);
  vtkMRMLCopyStringMacro(PatientBodySegmentID, PatientBodySegmentID);
  vtkMRMLCopyStringMacro(TreatmentMachineType, TreatmentMachineType);
  vtkMRMLCopyEndMacro(); 

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintBooleanMacro(CollisionDetectionEnabled, CollisionDetectionEnabled);
  vtkMRMLPrintFloatMacro(GantryRotationAngle, GantryRotationAngle);
  vtkMRMLPrintFloatMacro(CollimatorRotationAngle, CollimatorRotationAngle);
  vtkMRMLPrintFloatMacro(ImagingPanelMovement, ImagingPanelMovement);
  vtkMRMLPrintFloatMacro(PatientSupportRotationAngle, PatientSupportRotationAngle);
  vtkMRMLPrintFloatMacro(VerticalTableTopDisplacement, VerticalTableTopDisplacement);
  vtkMRMLPrintFloatMacro(LongitudinalTableTopDisplacement, LongitudinalTableTopDisplacement);
  vtkMRMLPrintFloatMacro(LateralTableTopDisplacement, LateralTableTopDisplacement);
  vtkMRMLPrintFloatMacro(AdditionalModelVerticalDisplacement, AdditionalModelVerticalDisplacement);
  vtkMRMLPrintFloatMacro(AdditionalModelLongitudinalDisplacement, AdditionalModelLongitudinalDisplacement);
  vtkMRMLPrintFloatMacro(AdditionalModelLateralDisplacement, AdditionalModelLateralDisplacement);
  vtkMRMLPrintStringMacro(PatientBodySegmentID, PatientBodySegmentID);
  vtkMRMLPrintStringMacro(TreatmentMachineType, TreatmentMachineType);
  vtkMRMLPrintEndMacro(); 
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetGantryToFixedReferenceTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveGantryToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetCollimatorToFixedReferenceIsocenterTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveCollimatorToFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(COLLIMATOR_TO_FIXEDREFERENCEISOCENTER_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetFixedReferenceIsocenterToCollimatorRotatedTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveFixedReferenceIsocenterToCollimatorRotatedTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(FIXEDREFERENCEISOCENTER_TO_COLLIMATORROTATED_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetCollimatorToGantryTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveCollimatorToGantryTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetAdditionalCollimatorDevicesToCollimatorTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_REFERENCE_ROLE));
}
//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveAdditionalCollimatorDevicesToCollimatorTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(ADDITIONALCOLLIMATORDEVICES_TO_COLLIMATOR_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveLeftImagingPanelFixedReferenceIsocenterToLeftImagingPanelRotatedTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_LEFTIMAGINGPANELROTATED_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetLeftImagingPanelRotatedToGantryTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveLeftImagingPanelRotatedToGantryTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetLeftImagingPanelTranslationTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE));
}

void vtkMRMLRoomsEyeViewNode::SetAndObserveLeftImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveRightImagingPanelToRightImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(RIGHTIMAGINGPANEL_TO_RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveRightImagingPanelFixedReferenceIsocenterToRightImagingPanelRotatedTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(RIGHTIMAGINGPANELFIXEDREFERENCEISOCENTER_TO_RIGHTIMAGINGPANELROTATED_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetRightImagingPanelRotatedToGantryTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveRightImagingPanelRotatedToGantryTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(RIGHTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetRightImagingPanelTranslationTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveRightImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(RIGHTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//-------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetPatientSupportToFixedReferenceTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObservePatientSupportToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(PATIENTSUPPORT_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetPatientSupportScaledByTableTopVerticalMovementTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObservePatientSupportScaledByTableTopVerticalMovementTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(PATIENTSUPPORTSCALEDBYTABLETOPVERTICALMOVEMENT_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetPatientSupportPositiveVerticalTranslationTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObservePatientSupportPositiveVerticalTranslationTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(PATIENTSUPPORTPOSITIVEVERTICALTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetPatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObservePatientSupportScaledTranslatedToTableTopVerticalTranslationTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(PATIENTSUPPORTSCALEDTRANSLATED_TO_TABLETOPVERTICALTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//-----------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetTableTopToTableTopEccentricRotationTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(TABLETOP_TO_TABLETOPECCENTRICROTATION_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveTableTopToTableTopEccentricRotationTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(TABLETOP_TO_TABLETOPECCENTRICROTATION_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetTableTopEccentricRotationToPatientSupportTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveTableTopEccentricRotationToPatientSupportTransformNode(vtkMRMLLinearTransformNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(TABLETOPECCENTRICROTATION_TO_PATIENTSUPPORT_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLRoomsEyeViewNode::GetPatientBodySegmentationNode()
{
  return vtkMRMLSegmentationNode::SafeDownCast( this->GetNodeReference(PATIENT_BODY_SEGMENTATION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObservePatientBodySegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(PATIENT_BODY_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkMRMLRoomsEyeViewNode::GetBeamNode()
{
  return vtkMRMLRTBeamNode::SafeDownCast( this->GetNodeReference(BEAM_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveBeamNode(vtkMRMLRTBeamNode* node)
{
  if (node && this->Scene != node->GetScene())
    {
    vtkErrorMacro("Cannot set reference: the referenced and referencing node are not in the same scene");
    return;
    }

  this->SetNodeReferenceID(BEAM_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}
