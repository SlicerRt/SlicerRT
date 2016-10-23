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

static const char* PATIENT_BODY_SEGMENTATION_REFERENCE_ROLE = "patientBodySegmentationRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLRoomsEyeViewNode);

//----------------------------------------------------------------------------
vtkMRMLRoomsEyeViewNode::vtkMRMLRoomsEyeViewNode()
{
  this->GantryRotationAngle = 0.0; //TODO:
  this->CollimatorRotationAngle = 0.0;
  this->ImagingPanelMovement = -68.50;
  this->PatientSupportRotationAngle = 0.0;
  this->VerticalTableTopDisplacement = 0.0;
  this->LongitudinalTableTopDisplacement = 0.0;
  this->LateralTableTopDisplacement = 0.0;
  this->PatientBodySegmentID = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLRoomsEyeViewNode::~vtkMRMLRoomsEyeViewNode()
{
  this->SetPatientBodySegmentID(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  of << indent << " GantryRotationAngle=\"" << this->GantryRotationAngle << "\"";
  of << indent << " CollimatorRotationAngle=\"" << this->CollimatorRotationAngle << "\"";
  of << indent << " ImagingPanelMovement=\"" << this->ImagingPanelMovement << "\"";
  of << indent << " PatientSupportRotationAngle=\"" << this->PatientSupportRotationAngle << "\"";
  of << indent << " VerticalTableTopDisplacement=\"" << this->VerticalTableTopDisplacement << "\"";
  of << indent << " LongitudinalTableTopDisplacement=\"" << this->LongitudinalTableTopDisplacement << "\"";
  of << indent << " LateralTableTopDisplacement=\"" << this->LateralTableTopDisplacement << "\"";
  of << indent << " PatientBodySegmentID=\"" << (this->PatientBodySegmentID ? this->PatientBodySegmentID : "NULL") << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName = NULL;
  const char* attValue = NULL;

  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "GantryRotationAngle")) 
    {
      this->GantryRotationAngle = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "CollimatorRotationAngle"))
    {
      this->CollimatorRotationAngle = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "ImagingPanelMovement"))
    {
      this->ImagingPanelMovement = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "PatientSupportRotationAngle"))
    {
      this->PatientSupportRotationAngle = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "VerticalTableTopDisplacement"))
    {
      this->VerticalTableTopDisplacement = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "LongitudinalTableTopDisplacement"))
    {
      this->LongitudinalTableTopDisplacement = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "LateralTableTopDisplacement"))
    {
      this->LateralTableTopDisplacement = vtkVariant(attValue).ToDouble();
    }
    else if (!strcmp(attName, "PatientBodySegmentID")) 
    {
      this->SetPatientBodySegmentID(vtkVariant(attValue).ToString());
    }
}

  // Note: ReportString is not read from XML, it is a strictly temporary value
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLRoomsEyeViewNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLRoomsEyeViewNode *node = (vtkMRMLRoomsEyeViewNode *) anode;

  this->GantryRotationAngle = node->GantryRotationAngle;
  this->CollimatorRotationAngle = node->CollimatorRotationAngle;
  this->ImagingPanelMovement = node->ImagingPanelMovement;
  this->PatientSupportRotationAngle = node->PatientSupportRotationAngle;
  this->VerticalTableTopDisplacement = node->VerticalTableTopDisplacement;
  this->LongitudinalTableTopDisplacement = node->LongitudinalTableTopDisplacement;
  this->LateralTableTopDisplacement = node->LateralTableTopDisplacement;
  this->SetPatientBodySegmentID(node->PatientBodySegmentID);

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "GantryRotationAngle:   " << this->GantryRotationAngle << "\n";
  os << indent << "CollimatorRotationAngle:   " << this->CollimatorRotationAngle << "\n";
  os << indent << "ImagingPanelMovement:    " << this->ImagingPanelMovement << "\n";
  os << indent << "PatientSupportRotationAngle:   " << this->PatientSupportRotationAngle << "\n";
  os << indent << "VerticalTableTopDisplacement:    " << this->VerticalTableTopDisplacement << "\n";
  os << indent << "LongitudinalTableTopDisplacement:    " << this->LongitudinalTableTopDisplacement << "\n";
  os << indent << "LateralTableTopDisplacement:    " << this->LateralTableTopDisplacement << "\n";
  os << indent << "PatientBodySegmentID:   " << (this->PatientBodySegmentID ? this->PatientBodySegmentID : "NULL") << "\n";
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetGantryToFixedReferenceTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(GANTRY_TO_FIXEDREFERENCE_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveGantryToFixedReferenceTransformNode(vtkMRMLLinearTransformNode* node)
{
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
  this->SetNodeReferenceID(COLLIMATOR_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(LEFTIMAGINGPANEL_TO_LEFTIMAGINGPANELFIXEDREFERENCEISOCENTER_TRANSFORM_NODE_REFERENCE_ROLE));
}

//----------------------------------------------------------------------------
void vtkMRMLRoomsEyeViewNode::SetAndObserveLeftImagingPanelToLeftImagingPanelFixedReferenceIsocenterTransformNode(vtkMRMLLinearTransformNode* node)
{
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
  this->SetNodeReferenceID(LEFTIMAGINGPANELROTATED_TO_GANTRY_TRANSFORM_NODE_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkMRMLRoomsEyeViewNode::GetLeftImagingPanelTranslationTransformNode()
{
  return vtkMRMLLinearTransformNode::SafeDownCast(this->GetNodeReference(LEFTIMAGINGPANELTRANSLATION_TRANSFORM_NODE_REFERENCE_ROLE));
}

void vtkMRMLRoomsEyeViewNode::SetAndObserveLeftImagingPanelTranslationTransformNode(vtkMRMLLinearTransformNode* node)
{
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
  this->SetNodeReferenceID(PATIENT_BODY_SEGMENTATION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

