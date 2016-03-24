/*==============================================================================

  Copyright (c) Radiation Medicine Program, University Health Network,
  Princess Margaret Hospital, Toronto, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, Princess Margaret Cancer Centre 
  and was supported by Cancer Care Ontario (CCO)'s ACRU program 
  with funds provided by the Ontario Ministry of Health and Long-Term Care
  and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).

==============================================================================*/

// MRMLDoseAccumulation includes
#include "vtkMRMLExternalBeamPlanningNode.h"

// SlicerRT includes
#include "vtkMRMLRTPlanNode.h"
#include "vtkMRMLSegmentationNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLDoubleArrayNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
const char* vtkMRMLExternalBeamPlanningNode::NEW_BEAM_NODE_NAME_PREFIX = "NewBeam_";
const char* vtkMRMLExternalBeamPlanningNode::TOTAL_PROTON_DOSE_VOLUME_REFERENCE_ROLE = "totalProtonDoseVolumeRef";

static const char* RT_PLAN_REFERENCE_ROLE = "rtPlanRef";
static const char* MLCPOSITION_REFERENCE_ROLE = "mlcPositionRef";
static const char* APERTURE_VOLUME_REFERENCE_ROLE_PREFIX = "apertureVolumeRef_";
static const char* RANGE_COMPENSATOR_VOLUME_REFERENCE_ROLE_PREFIX = "rangeCompensatorVolumeRef_";
static const char* PROTON_DOSE_VOLUME_REFERENCE_ROLE_PREFIX = "protonDoseVolumeRef_";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLExternalBeamPlanningNode);

//----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode::vtkMRMLExternalBeamPlanningNode()
{
  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLExternalBeamPlanningNode::~vtkMRMLExternalBeamPlanningNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLExternalBeamPlanningNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLExternalBeamPlanningNode *node = (vtkMRMLExternalBeamPlanningNode *)anode;

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkMRMLRTPlanNode* vtkMRMLExternalBeamPlanningNode::GetRTPlanNode()
{
  return vtkMRMLRTPlanNode::SafeDownCast( this->GetNodeReference(RT_PLAN_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveRTPlanNode(vtkMRMLRTPlanNode* node)
{
  this->SetNodeReferenceID(RT_PLAN_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
vtkMRMLDoubleArrayNode* vtkMRMLExternalBeamPlanningNode::GetMLCPositionDoubleArrayNode()
{
  return vtkMRMLDoubleArrayNode::SafeDownCast( this->GetNodeReference(MLCPOSITION_REFERENCE_ROLE) );
}

//----------------------------------------------------------------------------
void vtkMRMLExternalBeamPlanningNode::SetAndObserveMLCPositionDoubleArrayNode(vtkMRMLDoubleArrayNode* node)
{
  this->SetNodeReferenceID(MLCPOSITION_REFERENCE_ROLE, (node ? node->GetID() : NULL));
}

//----------------------------------------------------------------------------
std::string vtkMRMLExternalBeamPlanningNode::AssembleApertureVolumeReference(vtkMRMLNode* beamNode)
{
  if (!beamNode)
  {
    std::cerr << "vtkMRMLExternalBeamPlanningNode::AssembleApertureVolumeReference: Invalid beam node!";
    return "";
  }
  
  std::string referenceRole(APERTURE_VOLUME_REFERENCE_ROLE_PREFIX);
  referenceRole.append(beamNode->GetID());
  return referenceRole;
}

//----------------------------------------------------------------------------
std::string vtkMRMLExternalBeamPlanningNode::AssembleRangeCompensatorVolumeReference(vtkMRMLNode* beamNode)
{
  if (!beamNode)
  {
    std::cerr << "vtkMRMLExternalBeamPlanningNode::AssembleRangeCompensatorVolumeReference: Invalid beam node!";
    return "";
  }
  
  std::string referenceRole(RANGE_COMPENSATOR_VOLUME_REFERENCE_ROLE_PREFIX);
  referenceRole.append(beamNode->GetID());
  return referenceRole;
}

//----------------------------------------------------------------------------
std::string vtkMRMLExternalBeamPlanningNode::AssembleProtonDoseVolumeReference(vtkMRMLNode* beamNode)
{
  if (!beamNode)
  {
    std::cerr << "vtkMRMLExternalBeamPlanningNode::AssembleProtonDoseVolumeReference: Invalid beam node!";
    return "";
  }
  
  std::string referenceRole(PROTON_DOSE_VOLUME_REFERENCE_ROLE_PREFIX);
  referenceRole.append(beamNode->GetID());
  return referenceRole;
}
