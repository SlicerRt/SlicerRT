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
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLLinearTransformNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkGeneralTransform.h>
#include <vtkTransform.h>

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

  this->IecTransforms.clear();
  this->IecTransforms.push_back(std::make_pair(FixedReference, RAS));
  this->IecTransforms.push_back(std::make_pair(Gantry, FixedReference));
  this->IecTransforms.push_back(std::make_pair(Collimator, Gantry));
  this->IecTransforms.push_back(std::make_pair(LeftImagingPanel, Gantry));
  this->IecTransforms.push_back(std::make_pair(RightImagingPanel, Gantry));
  this->IecTransforms.push_back(std::make_pair(PatientSupportRotation, FixedReference)); // Rotation component of patient support transform
  this->IecTransforms.push_back(std::make_pair(PatientSupport, PatientSupportRotation)); // Scaling component of patient support transform
  this->IecTransforms.push_back(std::make_pair(TableTopEccentricRotation, PatientSupportRotation)); // NOTE: Currently not supported by REV
  this->IecTransforms.push_back(std::make_pair(TableTop, TableTopEccentricRotation));
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

  // Transforms
  os << indent << "Transforms:" << std::endl;
  std::vector< std::pair<CoordinateSystemIdentifier, CoordinateSystemIdentifier> >::iterator transformIt;
  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
  for (transformIt=this->IecTransforms.begin(); transformIt!=this->IecTransforms.end(); ++transformIt)
  {
    std::string transformNodeName = this->GetTransformNodeNameBetween(transformIt->first, transformIt->second);
    vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(
      this->GetMRMLScene()->GetFirstNodeByName(transformNodeName.c_str()) );

    os << indent.GetNextIndent() << transformNodeName << std::endl;
    transformNode->GetMatrixTransformToParent(matrix);
    for (int i = 0; i < 4; i++)
    {
      os << indent.GetNextIndent() << indent.GetNextIndent();
      for (int j = 0; j < 4; j++)
      {
        os << matrix->GetElement(i,j) << " ";
      }
      os << std::endl;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::BuildIECTransformHierarchy()
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("BuildIECTransformHierarchy: Invalid MRML scene");
    return;
  }

  // Create transform nodes if they do not exist
  std::vector< std::pair<CoordinateSystemIdentifier, CoordinateSystemIdentifier> >::iterator transformIt;
  for (transformIt=this->IecTransforms.begin(); transformIt!=this->IecTransforms.end(); ++transformIt)
  {
    std::string transformNodeName = this->GetTransformNodeNameBetween(transformIt->first, transformIt->second);
    if (!this->GetMRMLScene()->GetFirstNodeByName(transformNodeName.c_str()))
    {
      vtkSmartPointer<vtkMRMLLinearTransformNode> transformNode = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
      transformNode->SetName(transformNodeName.c_str());
      transformNode->SetHideFromEditors(1);
      std::string singletonTag = std::string("IEC_") + transformNodeName;
      transformNode->SetSingletonTag(singletonTag.c_str());
      this->GetMRMLScene()->AddNode(transformNode);
    }
  }

  // Organize transforms into hierarchy based on IEC Standard 61217
  this->GetTransformNodeBetween(Gantry, FixedReference)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(FixedReference, RAS)->GetID() );
  this->GetTransformNodeBetween(Collimator, Gantry)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(Gantry, FixedReference)->GetID() );

  this->GetTransformNodeBetween(LeftImagingPanel, Gantry)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(Gantry, FixedReference)->GetID() );
  this->GetTransformNodeBetween(RightImagingPanel, Gantry)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(Gantry, FixedReference)->GetID() );

  this->GetTransformNodeBetween(PatientSupportRotation, FixedReference)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(FixedReference, RAS)->GetID() );
  this->GetTransformNodeBetween(PatientSupport, PatientSupportRotation)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(PatientSupportRotation, FixedReference)->GetID() );
  this->GetTransformNodeBetween(TableTopEccentricRotation, PatientSupportRotation)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(PatientSupportRotation, FixedReference)->GetID() );
  this->GetTransformNodeBetween(TableTop, TableTopEccentricRotation)->SetAndObserveTransformNodeID(
    this->GetTransformNodeBetween(TableTopEccentricRotation, PatientSupportRotation)->GetID() );
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateBeamTransform(vtkMRMLRTBeamNode* beamNode)
{
  //TODO: Observe beam node's geometry modified event (vtkMRMLRTBeamNode::BeamGeometryModified)
  // and its parent plan's POI markups fiducial's point modified event (vtkMRMLMarkupsNode::PointModifiedEvent)
  // so that UpdateTransformsFromBeamGeometry is called. It may be needed to change the signature of the
  // update function. It may be also needed to store a reference to the beam node (see defined nodes in SlicerRT)

  if (!beamNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Invalid beam node");
    return;
  }

  // Make sure transform node exists
  beamNode->CreateDefaultTransformNode();

  // Update transform for beam
  vtkMRMLLinearTransformNode* beamTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(
    beamNode->GetParentTransformNode() );
  if (!beamTransformNode)
  {
    vtkErrorMacro("UpdateBeamTransform: Failed to access transform node of beam " << beamNode->GetName());
    return;
  }

  // Update transforms in IEC logic from beam node parameters
  this->UpdateIECTransformsFromBeam(beamNode);

  vtkSmartPointer<vtkGeneralTransform> beamGeneralTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  if (this->GetTransformBetween(Collimator, RAS, beamGeneralTransform))
  {
    // Convert general transform to linear
    // This call also makes hard copy of the transform so that it doesn't change when other beam transforms change
    vtkSmartPointer<vtkTransform> beamLinearTransform = vtkSmartPointer<vtkTransform>::New();
    if (!vtkMRMLTransformNode::IsGeneralTransformLinear(beamGeneralTransform, beamLinearTransform))
    {
      vtkErrorMacro("UpdateBeamTransform: Unable to set transform with non-linear components to beam " << beamNode->GetName());
      return;
    }

    // Set transform to beam node
    beamTransformNode->SetAndObserveTransformToParent(beamLinearTransform);

    // Update the name of the transform node too
    // (the user may have renamed the beam, but it's very expensive to update the transform name on every beam modified event)
    std::string transformName = std::string(beamNode->GetName()) + vtkMRMLRTBeamNode::BEAM_TRANSFORM_NODE_NAME_POSTFIX;
  }  
}

//-----------------------------------------------------------------------------
void vtkSlicerIECTransformLogic::UpdateIECTransformsFromBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("UpdateIECTransformsFromBeam: Invalid beam node");
    return;
  }
  vtkMRMLScene* scene = beamNode->GetScene();
  if (!scene || this->GetMRMLScene() != scene)
  {
    vtkErrorMacro("UpdateIECTransformsFromBeam: Invalid MRML scene");
    return;
  }

  // Make sure the transform hierarchy is set up
  this->BuildIECTransformHierarchy();

  //TODO: Code duplication (RevLogic::Update...)
  vtkMRMLLinearTransformNode* gantryToFixedReferenceTransformNode =
    this->GetTransformNodeBetween(Gantry, FixedReference);
  vtkTransform* gantryToFixedReferenceTransform = vtkTransform::SafeDownCast(gantryToFixedReferenceTransformNode->GetTransformToParent());
  gantryToFixedReferenceTransform->Identity();
  gantryToFixedReferenceTransform->RotateY(beamNode->GetGantryAngle() * (-1.0));
  gantryToFixedReferenceTransform->Modified();

  vtkMRMLLinearTransformNode* collimatorToGantryTransformNode =
    this->GetTransformNodeBetween(Collimator, Gantry);
  vtkTransform* collimatorToGantryTransform = vtkTransform::SafeDownCast(collimatorToGantryTransformNode->GetTransformToParent());
  collimatorToGantryTransform->Identity();
  collimatorToGantryTransform->RotateZ(beamNode->GetCollimatorAngle());
  collimatorToGantryTransform->Modified();

  vtkMRMLLinearTransformNode* patientSupportRotationToFixedReferenceTransformNode =
    this->GetTransformNodeBetween(PatientSupportRotation, FixedReference);
  vtkTransform* patientSupportToFixedReferenceTransform = vtkTransform::SafeDownCast(patientSupportRotationToFixedReferenceTransformNode->GetTransformToParent());
  patientSupportToFixedReferenceTransform->Identity();
  patientSupportToFixedReferenceTransform->RotateZ(beamNode->GetCouchAngle());
  patientSupportToFixedReferenceTransform->Modified();

  // Update IEC FixedReference to RAS transform based on the isocenter defined in the beam's parent plan
  vtkMRMLLinearTransformNode* fixedReferenceToRasTransformNode =
    this->GetTransformNodeBetween(FixedReference, RAS);
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
    vtkErrorMacro("UpdateIECTransformsFromBeam: Failed to get isocenter position for beam " << beamNode->GetName());
  }
  // The "S" direction in RAS is the "A" direction in FixedReference 
  fixedReferenceToRasTransform->RotateX(-90.0);
  // The "S" direction to be toward the gantry (head first position) by default
  fixedReferenceToRasTransform->RotateZ(180.0);
  fixedReferenceToRasTransform->Modified();
}

//-----------------------------------------------------------------------------
std::string vtkSlicerIECTransformLogic::GetTransformNodeNameBetween(
  CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame)
{
  return this->CoordinateSystemsMap[fromFrame] + "To" + this->CoordinateSystemsMap[toFrame] + "Transform";
}

//-----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* vtkSlicerIECTransformLogic::GetTransformNodeBetween(
  CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame )
{
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateIECTransformsFromBeam: Invalid MRML scene");
    return NULL;
  }

  return vtkMRMLLinearTransformNode::SafeDownCast(
    this->GetMRMLScene()->GetFirstNodeByName( this->GetTransformNodeNameBetween(fromFrame, toFrame).c_str() ) );
}

//-----------------------------------------------------------------------------
bool vtkSlicerIECTransformLogic::GetTransformBetween(CoordinateSystemIdentifier fromFrame, CoordinateSystemIdentifier toFrame, vtkGeneralTransform* outputTransform)
{
  if (!outputTransform)
  {
    vtkErrorMacro("GetTransformBetween: Invalid output transform node");
    return false;
  }
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("GetTransformBetween: Invalid MRML scene");
    return false;
  }

  //TODO: !!! IMPLEMENT DYNAMICALLY !!!

  if (fromFrame == Collimator && toFrame == RAS)
  {
    vtkMRMLTransformNode::GetTransformBetweenNodes(
      this->GetTransformNodeBetween(Collimator, Gantry), NULL, outputTransform );
    return true;
  }

  vtkErrorMacro("GetTransformBetween: Failed to get transform " << this->GetTransformNodeNameBetween(fromFrame, toFrame));
  return false;
}
