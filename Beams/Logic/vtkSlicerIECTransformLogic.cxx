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

// Beams includes
#include "vtkSlicerIECTransformLogic.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkGeneralTransform.h>
#include <vtkTransform.h>

// STD includes
#include <array>



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerIECTransformLogic);

//-----------------------------------------------------------------------------
vtkSlicerIECTransformLogic::vtkSlicerIECTransformLogic()
{
  // Setup coordinate system ID to name map
  this->CoordinateSystemsMap.clear();
  this->CoordinateSystemsMap[RAS] = "Ras";
  this->CoordinateSystemsMap[FixedReference] = "FixedReference";
  this->CoordinateSystemsMap[Gantry] = "Gantry";
  this->CoordinateSystemsMap[Collimator] = "Collimator";
  this->CoordinateSystemsMap[LeftImagingPanel] = "LeftImagingPanel";
  this->CoordinateSystemsMap[RightImagingPanel] = "RightImagingPanel";
  this->CoordinateSystemsMap[PatientSupportRotation] = "PatientSupportRotation";
  this->CoordinateSystemsMap[PatientSupport] = "PatientSupport";
  this->CoordinateSystemsMap[TableTopEccentricRotation] = "TableTopEccentricRotation";
  this->CoordinateSystemsMap[TableTop] = "TableTop";
  this->CoordinateSystemsMap[FlatPanel] = "FlatPanel";
  this->CoordinateSystemsMap[WedgeFilter] = "WedgeFilter";
  this->CoordinateSystemsMap[Patient] = "Patient";

  this->IecTransforms.clear();
  this->IecTransforms.push_back(std::make_pair(FixedReference, RAS));
  this->IecTransforms.push_back(std::make_pair(Gantry, FixedReference));
  this->IecTransforms.push_back(std::make_pair(Collimator, Gantry));
  this->IecTransforms.push_back(std::make_pair(WedgeFilter, Collimator));
  this->IecTransforms.push_back(std::make_pair(LeftImagingPanel, Gantry));
  this->IecTransforms.push_back(std::make_pair(RightImagingPanel, Gantry));
  this->IecTransforms.push_back(std::make_pair(PatientSupportRotation, FixedReference)); // Rotation component of patient support transform
  this->IecTransforms.push_back(std::make_pair(PatientSupport, PatientSupportRotation)); // Scaling component of patient support transform
  this->IecTransforms.push_back(std::make_pair(TableTopEccentricRotation, PatientSupportRotation)); // NOTE: Currently not supported by REV
  this->IecTransforms.push_back(std::make_pair(TableTop, TableTopEccentricRotation));
  this->IecTransforms.push_back(std::make_pair(Patient, TableTop));
  this->IecTransforms.push_back(std::make_pair(RAS, Patient));
  this->IecTransforms.push_back(std::make_pair(FlatPanel, Gantry));

  this->CoordinateSystemsHierarchy.clear();
  // key - parent, value - children
  this->CoordinateSystemsHierarchy[FixedReference] = { Gantry, PatientSupportRotation };
  this->CoordinateSystemsHierarchy[Gantry] = { Collimator, LeftImagingPanel, RightImagingPanel, FlatPanel };
  this->CoordinateSystemsHierarchy[Collimator] = { WedgeFilter };
  this->CoordinateSystemsHierarchy[PatientSupportRotation] = { PatientSupport, TableTopEccentricRotation };
  this->CoordinateSystemsHierarchy[TableTopEccentricRotation] = { TableTop };
  this->CoordinateSystemsHierarchy[TableTop] = { Patient };
  this->CoordinateSystemsHierarchy[Patient] = { RAS };
}

//-----------------------------------------------------------------------------
vtkSlicerIECTransformLogic::~vtkSlicerIECTransformLogic()
{
  this->CoordinateSystemsMap.clear();
  this->IecTransforms.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Transforms:" << std::endl;
  os << indent << "FixedReferenceToRasTransform: " << FixedReferenceToRasTransform << std::endl;
  os << indent << "GantryToFixedReferenceTransform: " << GantryToFixedReferenceTransform << std::endl;
  os << indent << "Concatenated GantryToFixedReferenceTransform: " << transformGantryToFixedReferenceConcatenated << std::endl;
  os << indent << "CollimatorToGantryTransform: " << CollimatorToGantryTransform << std::endl;
  os << indent << "Concatenated CollimatorToGantryTransform: " << transformCollimatorToGantryConcatenated << std::endl;
  os << indent << "WedgeFilterToCollimatorTransform: " << WedgeFilterToCollimatorTransform << std::endl;
  os << indent << "Concatenated WedgeFilterToCollimatorTransform: " << transformWedgeFilterToCollimatorConcatenated << std::endl;
  os << indent << "AdditionalCollimatorDevicesToCollimatorTransform: " << AdditionalCollimatorDevicesToCollimatorTransform << std::endl;
  os << indent << "Concatenated AdditionalCollimatorDevicesToCollimatorTransform: " << transformAdditionalCollimatorDevicesToCollimatorConcatenated << std::endl;
  os << indent << "LeftImagingPanelToGantryTransform: " << LeftImagingPanelToGantryTransform << std::endl;
  os << indent << "Concatenated LeftImagingPanelToGantryTransform: " << transformLeftImagingPanelToGantryConcatenated << std::endl;
  os << indent << "RightImagingPanelToGantryTransform: " << RightImagingPanelToGantryTransform << std::endl;
  os << indent << "Concatenated RightImagingPanelToGantryTransform: " << transformRightImagingPanelToGantryConcatenated << std::endl;
  os << indent << "FlatPanelToGantryTransform: " << FlatPanelToGantryTransform << std::endl;
  os << indent << "Concatenated FlatPanelToGantryTransform: " << transformFlatPanelToGantryConcatenated << std::endl;
  os << indent << "PatientSupportRotationToFixedReferenceTransform: " << PatientSupportRotationToFixedReferenceTransform << std::endl;
  os << indent << "Concatenated PatientSupportRotationToFixedReferenceTransform: " << transformPatientSupportRotationToFixedReferenceConcatenated << std::endl;
  os << indent << "PatientSupportToPatientSupportRotationTransform: " << PatientSupportToPatientSupportRotationTransform << std::endl;
  os << indent << "Concatenated PatientSupportToPatientSupportRotationTransform: " << transformPatientSupportToPatientSupportRotationConcatenated << std::endl;
  os << indent << "TableTopEccentricRotationToPatientSupportRotationTransform: " << TableTopEccentricRotationToPatientSupportRotationTransform << std::endl;
  os << indent << "Concatenated TableTopEccentricRotationToPatientSupportRotationTransform: " << transformTableTopEccentricRotationToPatientSupportRotationConcatenated << std::endl;
  os << indent << "TableTopToTableTopEccentricRotationTransform: " << TableTopToTableTopEccentricRotationTransform << std::endl;
  os << indent << "Concatenated TableTopToTableTopEccentricRotationTransform: " << transformTableTopToTableEccentricRotationConcatenated << std::endl;
  os << indent << "PatientToTableTopTransform: " << PatientToTableTopTransform << std::endl;
  os << indent << "Concatenated PatientToTableTopTransform: " << transformPatientToTableTopConcatenated << std::endl;
  os << indent << "RasToPatientTransform: " << RasToPatientTransform << std::endl;
  os << indent << "Concatenated RasToPatientTransform: " << transformRasToPatientConcatenated << std::endl;
}

//---------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::BuildIECTransformHierarchy()
{
  transformGantryToFixedReferenceConcatenated->Concatenate(FixedReferenceToRasTransform);
  transformGantryToFixedReferenceConcatenated->Concatenate(GantryToFixedReferenceTransform);

  transformCollimatorToGantryConcatenated->Concatenate(transformGantryToFixedReferenceConcatenated);
  transformCollimatorToGantryConcatenated->Concatenate(CollimatorToGantryTransform);

  transformWedgeFilterToCollimatorConcatenated->Concatenate(transformCollimatorToGantryConcatenated);
  transformWedgeFilterToCollimatorConcatenated->Concatenate(WedgeFilterToCollimatorTransform);
  
  transformAdditionalCollimatorDevicesToCollimatorConcatenated->Concatenate(transformWedgeFilterToCollimatorConcatenated);
  transformAdditionalCollimatorDevicesToCollimatorConcatenated->Concatenate(AdditionalCollimatorDevicesToCollimatorTransform);
  
  transformLeftImagingPanelToGantryConcatenated->Concatenate(transformCollimatorToGantryConcatenated);
  transformLeftImagingPanelToGantryConcatenated->Concatenate(LeftImagingPanelToGantryTransform);

  transformRightImagingPanelToGantryConcatenated->Concatenate(transformCollimatorToGantryConcatenated);
  transformRightImagingPanelToGantryConcatenated->Concatenate(RightImagingPanelToGantryTransform);

  transformFlatPanelToGantryConcatenated->Concatenate(transformCollimatorToGantryConcatenated);
  transformFlatPanelToGantryConcatenated->Concatenate(FlatPanelToGantryTransform);

  transformPatientSupportRotationToFixedReferenceConcatenated->Concatenate(FixedReferenceToRasTransform);
  transformPatientSupportRotationToFixedReferenceConcatenated->Concatenate(PatientSupportRotationToFixedReferenceTransform);

  transformPatientSupportToPatientSupportRotationConcatenated->Concatenate(transformPatientSupportRotationToFixedReferenceConcatenated);
  transformPatientSupportToPatientSupportRotationConcatenated->Concatenate(PatientSupportToPatientSupportRotationTransform);

  transformTableTopEccentricRotationToPatientSupportRotationConcatenated->Concatenate(transformPatientSupportRotationToFixedReferenceConcatenated);
  transformTableTopEccentricRotationToPatientSupportRotationConcatenated->Concatenate(TableTopEccentricRotationToPatientSupportRotationTransform);

  transformTableTopToTableEccentricRotationConcatenated->Concatenate(transformTableTopEccentricRotationToPatientSupportRotationConcatenated);
  transformTableTopToTableEccentricRotationConcatenated->Concatenate(TableTopToTableTopEccentricRotationTransform);

  transformPatientToTableTopConcatenated->Concatenate(transformTableTopToTableEccentricRotationConcatenated);
  transformPatientToTableTopConcatenated->Concatenate(PatientToTableTopTransform);

  transformRasToPatientConcatenated->Concatenate(transformPatientToTableTopConcatenated);
  transformRasToPatientConcatenated->Concatenate(RasToPatientTransform);
}

//----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateGantryToFixedReferenceTransform(double gantryRotationAngleDeg)
{
  this->GantryToFixedReferenceTransform->Identity();
  this->GantryToFixedReferenceTransform->RotateY(gantryRotationAngleDeg);
}

//----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateCollimatorToGantryTransform(double collimatorRotationAngleDeg)
{
  this->CollimatorToGantryTransform->Identity();
  this->CollimatorToGantryTransform->RotateZ(collimatorRotationAngleDeg);
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdatePatientSupportRotationToFixedReferenceTransform(double patientSupportRotationAngleDeg)
{
  this->PatientSupportRotationToFixedReferenceTransform->Identity();
  this->PatientSupportRotationToFixedReferenceTransform->RotateZ(patientSupportRotationAngleDeg);
}

//-----------------------------------------------------------------------------
std::string vtkSlicerIECTransformLogic::GetTransformNameBetween(
  CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame)
{
  return this->GetCoordinateSystemsMap()[fromFrame] + "To" + this->GetCoordinateSystemsMap()[toFrame] + "Transform";
}