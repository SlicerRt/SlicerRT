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
#include <algorithm>


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

  this->IECTransforms.clear();
  this->IECTransforms.push_back(std::make_pair(FixedReference, RAS));
  this->IECTransforms.push_back(std::make_pair(Gantry, FixedReference));
  this->IECTransforms.push_back(std::make_pair(Collimator, Gantry));
  this->IECTransforms.push_back(std::make_pair(WedgeFilter, Collimator));
  this->IECTransforms.push_back(std::make_pair(LeftImagingPanel, Gantry));
  this->IECTransforms.push_back(std::make_pair(RightImagingPanel, Gantry));
  this->IECTransforms.push_back(std::make_pair(PatientSupportRotation, FixedReference)); // Rotation component of patient support transform
  this->IECTransforms.push_back(std::make_pair(PatientSupport, PatientSupportRotation)); // Scaling component of patient support transform
  this->IECTransforms.push_back(std::make_pair(TableTopEccentricRotation, PatientSupportRotation)); // NOTE: Currently not supported by REV
  this->IECTransforms.push_back(std::make_pair(TableTop, TableTopEccentricRotation));
  this->IECTransforms.push_back(std::make_pair(Patient, TableTop));
  this->IECTransforms.push_back(std::make_pair(RAS, Patient));
  this->IECTransforms.push_back(std::make_pair(FlatPanel, Gantry));

  // Set names for elementary transforms for discovery
  this->FixedReferenceToRasTransform->SetObjectName(this->GetTransformNameBetween(FixedReference, RAS).c_str());
  this->GantryToFixedReferenceTransform->SetObjectName(this->GetTransformNameBetween(Gantry, FixedReference).c_str());
  this->CollimatorToGantryTransform->SetObjectName(this->GetTransformNameBetween(Collimator, Gantry).c_str());
  this->WedgeFilterToCollimatorTransform->SetObjectName(this->GetTransformNameBetween(WedgeFilter, Collimator).c_str());
  this->LeftImagingPanelToGantryTransform->SetObjectName(this->GetTransformNameBetween(LeftImagingPanel, Gantry).c_str());
  this->RightImagingPanelToGantryTransform->SetObjectName(this->GetTransformNameBetween(RightImagingPanel, Gantry).c_str());
  this->PatientSupportRotationToFixedReferenceTransform->SetObjectName(this->GetTransformNameBetween(PatientSupportRotation, FixedReference).c_str());
  this->PatientSupportToPatientSupportRotationTransform->SetObjectName(this->GetTransformNameBetween(PatientSupport, PatientSupportRotation).c_str());
  this->TableTopEccentricRotationToPatientSupportRotationTransform->SetObjectName(this->GetTransformNameBetween(TableTopEccentricRotation, PatientSupportRotation).c_str());
  this->TableTopToTableTopEccentricRotationTransform->SetObjectName(this->GetTransformNameBetween(TableTop, TableTopEccentricRotation).c_str());
  this->PatientToTableTopTransform->SetObjectName(this->GetTransformNameBetween(Patient, TableTop).c_str());
  this->RasToPatientTransform->SetObjectName(this->GetTransformNameBetween(RAS, Patient).c_str());
  this->FlatPanelToGantryTransform->SetObjectName(this->GetTransformNameBetween(FlatPanel, Gantry).c_str());

  // Build list of elementary transforms for discovery by name
  this->ElementaryTransforms.push_back(this->FixedReferenceToRasTransform);
  this->ElementaryTransforms.push_back(this->GantryToFixedReferenceTransform);
  this->ElementaryTransforms.push_back(this->CollimatorToGantryTransform);
  this->ElementaryTransforms.push_back(this->WedgeFilterToCollimatorTransform);
  this->ElementaryTransforms.push_back(this->LeftImagingPanelToGantryTransform);
  this->ElementaryTransforms.push_back(this->RightImagingPanelToGantryTransform);
  this->ElementaryTransforms.push_back(this->PatientSupportRotationToFixedReferenceTransform);
  this->ElementaryTransforms.push_back(this->PatientSupportToPatientSupportRotationTransform);
  this->ElementaryTransforms.push_back(this->TableTopEccentricRotationToPatientSupportRotationTransform);
  this->ElementaryTransforms.push_back(this->TableTopToTableTopEccentricRotationTransform);
  this->ElementaryTransforms.push_back(this->PatientToTableTopTransform);
  this->ElementaryTransforms.push_back(this->RasToPatientTransform);
  this->ElementaryTransforms.push_back(this->FlatPanelToGantryTransform);

  // Define the transform hierarchy
  this->CoordinateSystemsHierarchy.clear();
  // key - parent, value - children
  this->CoordinateSystemsHierarchy[FixedReference] = { Gantry, PatientSupportRotation };
  this->CoordinateSystemsHierarchy[Gantry] = { Collimator, LeftImagingPanel, RightImagingPanel, FlatPanel };
  this->CoordinateSystemsHierarchy[Collimator] = { WedgeFilter };
  this->CoordinateSystemsHierarchy[PatientSupportRotation] = { PatientSupport, TableTopEccentricRotation };
  this->CoordinateSystemsHierarchy[TableTopEccentricRotation] = { TableTop };
  this->CoordinateSystemsHierarchy[TableTop] = { Patient };
  this->CoordinateSystemsHierarchy[Patient] = { RAS };

  //
  // Build concatenated transform hierarchy
  // 
  this->GantryToFixedReferenceConcatenatedTransform->Concatenate(this->FixedReferenceToRasTransform);
  this->GantryToFixedReferenceConcatenatedTransform->Concatenate(this->GantryToFixedReferenceTransform);

  this->CollimatorToGantryConcatenatedTransform->Concatenate(this->GantryToFixedReferenceConcatenatedTransform);
  this->CollimatorToGantryConcatenatedTransform->Concatenate(this->CollimatorToGantryTransform);

  this->WedgeFilterToCollimatorConcatenatedTransform->Concatenate(this->CollimatorToGantryConcatenatedTransform);
  this->WedgeFilterToCollimatorConcatenatedTransform->Concatenate(this->WedgeFilterToCollimatorTransform);
    
  this->LeftImagingPanelToGantryConcatenatedTransform->Concatenate(this->CollimatorToGantryConcatenatedTransform);
  this->LeftImagingPanelToGantryConcatenatedTransform->Concatenate(this->LeftImagingPanelToGantryTransform);

  this->RightImagingPanelToGantryConcatenatedTransform->Concatenate(this->CollimatorToGantryConcatenatedTransform);
  this->RightImagingPanelToGantryConcatenatedTransform->Concatenate(this->RightImagingPanelToGantryTransform);

  this->FlatPanelToGantryConcatenatedTransform->Concatenate(this->CollimatorToGantryConcatenatedTransform);
  this->FlatPanelToGantryConcatenatedTransform->Concatenate(this->FlatPanelToGantryTransform);

  this->PatientSupportRotationToFixedReferenceConcatenatedTransform->Concatenate(this->FixedReferenceToRasTransform);
  this->PatientSupportRotationToFixedReferenceConcatenatedTransform->Concatenate(this->PatientSupportRotationToFixedReferenceTransform);

  this->PatientSupportToPatientSupportRotationConcatenatedTransform->Concatenate(this->PatientSupportRotationToFixedReferenceConcatenatedTransform);
  this->PatientSupportToPatientSupportRotationConcatenatedTransform->Concatenate(this->PatientSupportToPatientSupportRotationTransform);

  this->TableTopEccentricRotationToPatientSupportRotationConcatenatedTransform->Concatenate(this->PatientSupportRotationToFixedReferenceConcatenatedTransform);
  this->TableTopEccentricRotationToPatientSupportRotationConcatenatedTransform->Concatenate(this->TableTopEccentricRotationToPatientSupportRotationTransform);

  this->TableTopToTableEccentricRotationConcatenatedTransform->Concatenate(this->TableTopEccentricRotationToPatientSupportRotationConcatenatedTransform);
  this->TableTopToTableEccentricRotationConcatenatedTransform->Concatenate(this->TableTopToTableTopEccentricRotationTransform);

  this->PatientToTableTopConcatenatedTransform->Concatenate(this->TableTopToTableEccentricRotationConcatenatedTransform);
  this->PatientToTableTopConcatenatedTransform->Concatenate(this->PatientToTableTopTransform);

  this->RasToPatientConcatenatedTransform->Concatenate(this->PatientToTableTopConcatenatedTransform);
  this->RasToPatientConcatenatedTransform->Concatenate(this->RasToPatientTransform);
}

//-----------------------------------------------------------------------------
vtkSlicerIECTransformLogic::~vtkSlicerIECTransformLogic()
{
  this->CoordinateSystemsMap.clear();
  this->IECTransforms.clear();
  this->ElementaryTransforms.clear();
}

//----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << std::endl << "Elementary tansforms:" << std::endl;

  os << indent.GetNextIndent() << "FixedReferenceToRasTransform:" << std::endl;
  this->FixedReferenceToRasTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "GantryToFixedReferenceTransform:" << std::endl;
  this->GantryToFixedReferenceTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "CollimatorToGantryTransform:" << std::endl;
  this->CollimatorToGantryTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "WedgeFilterToCollimatorTransform:" << std::endl;
  this->WedgeFilterToCollimatorTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "LeftImagingPanelToGantryTransform:" << std::endl;
  this->LeftImagingPanelToGantryTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "RightImagingPanelToGantryTransform:" << std::endl;
  this->RightImagingPanelToGantryTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "FlatPanelToGantryTransform:" << std::endl;
  this->FlatPanelToGantryTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "PatientSupportRotationToFixedReferenceTransform:" << std::endl;
  this->PatientSupportRotationToFixedReferenceTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "PatientSupportToPatientSupportRotationTransform:" << std::endl;
  this->PatientSupportToPatientSupportRotationTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "TableTopEccentricRotationToPatientSupportRotationTransform:" << std::endl;
  this->TableTopEccentricRotationToPatientSupportRotationTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "TableTopToTableTopEccentricRotationTransform:" << std::endl;
  this->TableTopToTableTopEccentricRotationTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "PatientToTableTopTransform:" << std::endl;
  this->PatientToTableTopTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
  os << indent.GetNextIndent() << "RasToPatientTransform:" << std::endl;
  this->RasToPatientTransform->GetMatrix()->PrintSelf(os, indent.GetNextIndent().GetNextIndent());
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
vtkTransform* vtkSlicerIECTransformLogic::GetElementaryTransformBetween(
  CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame)
{
  std::string requestedTransformName = this->GetTransformNameBetween(fromFrame, toFrame);
  for (auto& transform : this->ElementaryTransforms)
  {
    std::string currentTransformName(transform->GetObjectName());
    if (currentTransformName == requestedTransformName)
    {
      return transform;
    }
  }

  vtkErrorMacro("GetElementaryTransformBetween: Elementary transform not found: " << requestedTransformName);
  return nullptr;
}

//-----------------------------------------------------------------------------
std::string vtkSlicerIECTransformLogic::GetTransformNameBetween(
  CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame)
{
  return this->CoordinateSystemsMap[fromFrame] + "To" + this->CoordinateSystemsMap[toFrame] + "Transform";
}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetTransformBetween(vtkSlicerIECTransformLogic::CoordinateSystemIdentifier fromFrame, vtkSlicerIECTransformLogic::CoordinateSystemIdentifier toFrame,
  vtkGeneralTransform* outputTransform, bool transformForBeam/*=false*/)
{
  if (!outputTransform)
  {
    vtkErrorMacro("GetTransformBetween: Invalid output transform node");
    return false;
  }

  vtkSlicerIECTransformLogic::CoordinateSystemsList fromFramePath, toFramePath;
  if (this->GetPathToRoot(fromFrame, fromFramePath) && this->GetPathFromRoot(toFrame, toFramePath))
  {
    std::vector< vtkSlicerIECTransformLogic::CoordinateSystemIdentifier > toFrameVector(toFramePath.size());
    std::vector< vtkSlicerIECTransformLogic::CoordinateSystemIdentifier > fromFrameVector(fromFramePath.size());

    std::copy(toFramePath.begin(), toFramePath.end(), toFrameVector.begin());
    std::copy(fromFramePath.begin(), fromFramePath.end(), fromFrameVector.begin());

    outputTransform->Identity();
    outputTransform->PostMultiply();
    for (size_t i = 0; i < fromFrameVector.size() - 1; ++i)
    {
      vtkSlicerIECTransformLogic::CoordinateSystemIdentifier parent, child;
      child = fromFrameVector[i];
      parent = fromFrameVector[i + 1];

      if (child == parent)
      {
        continue;
      }

      vtkTransform* fromTransform = this->GetElementaryTransformBetween(child, parent);
      if (fromTransform)
      {
        outputTransform->Concatenate(fromTransform->GetMatrix());
      }
      else
      {
        vtkErrorMacro("GetTransformBetween: Transform node is invalid");
        return false;
      }
    }

    for (size_t i = 0; i < toFrameVector.size() - 1; ++i)
    {
      vtkSlicerIECTransformLogic::CoordinateSystemIdentifier parent, child;
      parent = toFrameVector[i];
      child = toFrameVector[i + 1];

      if (child == parent)
      {
        continue;
      }

      vtkTransform* toTransform = this->GetElementaryTransformBetween(child, parent);
      if (toTransform)
      {
        vtkNew<vtkMatrix4x4> mat;
        toTransform->GetMatrix(mat);
        if (!transformForBeam) // Do not invert for beam transformation
        {
          mat->Invert();
        }
        outputTransform->Concatenate(mat);
      }
      else
      {
        vtkErrorMacro("GetTransformBetween: Transform node is invalid");
        return false;
      }
    }

    outputTransform->Modified();
    return true;
  }

  vtkErrorMacro("GetTransformBetween: Failed to get transform " << this->GetTransformNameBetween(fromFrame, toFrame));
  return false;
}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetPathToRoot(vtkSlicerIECTransformLogic::CoordinateSystemIdentifier frame, vtkSlicerIECTransformLogic::CoordinateSystemsList& path)
{
  if (frame == vtkSlicerIECTransformLogic::CoordinateSystemIdentifier::FixedReference)
  {
    path.push_back(vtkSlicerIECTransformLogic::FixedReference);
    return true;
  }

  bool found = false;
  do
  {
    for (auto& pair : this->CoordinateSystemsHierarchy)
    {
      vtkSlicerIECTransformLogic::CoordinateSystemIdentifier parent = pair.first;

      auto& children = pair.second;
      auto iter = std::find(children.begin(), children.end(), frame);
      if (iter != children.end())
      {
        vtkSlicerIECTransformLogic::CoordinateSystemIdentifier id = *iter;

        vtkDebugMacro("GetPathToRoot: Checking affine transformation "
          << "\"" << this->CoordinateSystemsMap[id] << "\" -> "
          << "\"" << this->CoordinateSystemsMap[parent] << "\"");

        frame = parent;
        path.push_back(id);
        if (frame != vtkSlicerIECTransformLogic::FixedReference)
        {
          found = true;
          break;
        }
        else
        {
          path.push_back(vtkSlicerIECTransformLogic::FixedReference);
        }
      }
      else
      {
        found = false;
      }
    }
  } while (found);

  return (path.size() > 0);
}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetPathFromRoot(vtkSlicerIECTransformLogic::CoordinateSystemIdentifier frame, vtkSlicerIECTransformLogic::CoordinateSystemsList& path)
{
  if (this->GetPathToRoot(frame, path))
  {
    std::reverse(path.begin(), path.end());
    return true;
  }
  else
  {
    return false;
  }
}
