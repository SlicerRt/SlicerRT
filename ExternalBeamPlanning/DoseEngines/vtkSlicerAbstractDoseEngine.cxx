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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Dose engines includes
#include "vtkSlicerAbstractDoseEngine.h"

// Beams includes
#include "vtkMRMLRTBeamNode.h"
#include "vtkMRMLRTPlanNode.h"

// SlicerRT includes
#include "SlicerRtCommon.h"
#include "vtkSlicerIsodoseModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLSubjectHierarchyConstants.h>
#include <vtkMRMLColorTableNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

//----------------------------------------------------------------------------
double vtkSlicerAbstractDoseEngine::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM = 16.0;

//----------------------------------------------------------------------------
static const char* INTERMEDIATE_RESULT_REFERENCE_ROLE = "IntermediateResultRef";
static const char* RESULT_DOSE_REFERENCE_ROLE = "ResultDoseRef";

//----------------------------------------------------------------------------
vtkSlicerAbstractDoseEngine::vtkSlicerAbstractDoseEngine()
{
  this->Name = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerAbstractDoseEngine::~vtkSlicerAbstractDoseEngine()
{
  this->SetName(NULL);
}

//----------------------------------------------------------------------------
void vtkSlicerAbstractDoseEngine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Name: " << (this->Name ? this->Name : "NULL");
}

//----------------------------------------------------------------------------
vtkMRMLRTBeamNode* vtkSlicerAbstractDoseEngine::CreateBeamForEngine()
{
  return vtkMRMLRTBeamNode::New();
}

//----------------------------------------------------------------------------
std::string vtkSlicerAbstractDoseEngine::CalculateDose(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    std::string errorMessage("Invalid beam node");
    vtkErrorMacro("CalculateDose: " << errorMessage);
    return errorMessage;
  }
  vtkMRMLRTPlanNode* parentPlanNode = beamNode->GetParentPlanNode();
  if (!parentPlanNode)
  {
    std::string errorMessage = std::string("Unable to access parent node for beam ") + beamNode->GetName();
    vtkErrorMacro("CalculateDose: " << errorMessage);
    return errorMessage;
  }

  // Add RT plan to the same branch where the reference volume is
  vtkMRMLScalarVolumeNode* referenceVolumeNode = parentPlanNode->GetReferenceVolumeNode();
  if (!referenceVolumeNode)
  {
    std::string errorMessage("Unable to access reference volume");
    vtkErrorMacro("CalculateDose: " << errorMessage);
    return errorMessage;
  }
  vtkMRMLSubjectHierarchyNode* referenceVolumeShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(referenceVolumeNode);
  if (referenceVolumeShNode)
  {
    vtkMRMLSubjectHierarchyNode* planShNode = parentPlanNode->GetPlanSubjectHierarchyNode();
    if (planShNode)
    {
      planShNode->SetParentNodeID(referenceVolumeShNode->GetParentNodeID());
    }
    else
    {
      vtkErrorMacro("CalculateDose: Failed to access RT plan subject hierarchy node, although it should always be available!");
    }
  }
  else
  {
    vtkErrorMacro("CalculateDose: Failed to access reference volume subject hierarchy node!");
  }

  // Remove past intermediate results for beam before calculating dose again
  this->RemoveIntermediateResults(beamNode);

  // Create output dose volume for beam
  vtkSmartPointer<vtkMRMLScalarVolumeNode> resultDoseVolumeNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  beamNode->GetScene()->AddNode(resultDoseVolumeNode);
  // Give default name for result node (engine can give it a more meaningful name)
  std::string resultDoseNodeName = std::string(beamNode->GetName()) + "_Dose";
  resultDoseVolumeNode->SetName(resultDoseNodeName.c_str());
  
  // Calculate dose
  std::string errorMessage = this->CalculateDoseUsingEngine(beamNode, resultDoseVolumeNode);
  if (errorMessage.empty())
  {
    // Add result dose volume to beam
    this->AddResultDose(resultDoseVolumeNode, beamNode);
  }

  return errorMessage;
}

//---------------------------------------------------------------------------
void vtkSlicerAbstractDoseEngine::AddIntermediateResult(vtkMRMLNode* result, vtkMRMLRTBeamNode* beamNode)
{
  if (!result)
  {
    vtkErrorMacro("AddIntermediateResult: Invalid intermediate result");
    return;
  }
  if (!beamNode)
  {
    vtkErrorMacro("AddIntermediateResult: Invalid beam node");
    return;
  }

  // Add reference in beam to result for later access
  beamNode->AddNodeReferenceID(INTERMEDIATE_RESULT_REFERENCE_ROLE, result->GetID());

  // Add result under beam in subject hierarchy
  vtkMRMLSubjectHierarchyNode* beamShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(beamNode);
  if (beamShNode)
  {
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      beamNode->GetScene(), beamShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), 
      result->GetName(), result );
  }
}

//---------------------------------------------------------------------------
void vtkSlicerAbstractDoseEngine::AddResultDose(vtkMRMLScalarVolumeNode* resultDose, vtkMRMLRTBeamNode* beamNode, bool replace/*=true*/)
{
  if (!resultDose)
  {
    vtkErrorMacro("AddResultDose: Invalid result dose");
    return;
  }
  if (!beamNode)
  {
    vtkErrorMacro("AddResultDose: Invalid beam node");
    return;
  }

  // Remove already existing referenced dose volume if any
  if (replace)
  {
    vtkMRMLScene* scene = beamNode->GetScene();
    std::vector<const char*> referencedDoseNodeIds;
    beamNode->GetNodeReferenceIDs(RESULT_DOSE_REFERENCE_ROLE, referencedDoseNodeIds);
    for (std::vector<const char*>::iterator refIt=referencedDoseNodeIds.begin(); refIt != referencedDoseNodeIds.end(); ++refIt)
    {
      vtkMRMLNode* node = scene->GetNodeByID(*refIt);
      scene->RemoveNode(node);
    }
  }

  // Add reference in beam to result dose for later access
  beamNode->AddNodeReferenceID(RESULT_DOSE_REFERENCE_ROLE, resultDose->GetID());

  // Set dose volume attribute so that it is identified as dose
  resultDose->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_VOLUME_IDENTIFIER_ATTRIBUTE_NAME.c_str(), "1");

  // Subject hierarchy related operations
  vtkMRMLSubjectHierarchyNode* beamShNode = vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(beamNode);
  if (beamShNode)
  {
    // Add result under beam in subject hierarchy
    vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyNode(
      beamNode->GetScene(), beamShNode, vtkMRMLSubjectHierarchyConstants::GetDICOMLevelSubseries(), 
      resultDose->GetName(), resultDose );

    // Set dose unit value to Gy if dose engine did not set it already (potentially to other unit)
    vtkMRMLSubjectHierarchyNode* studyNode = beamShNode->GetAncestorAtLevel(vtkMRMLSubjectHierarchyConstants::GetDICOMLevelStudy());
    if (!studyNode)
    {
      vtkWarningMacro("AddResultDose: Unable to find study node that contains the plan! Creating a study node and adding the reference dose and the plan under it is necessary in order for dose evaluation steps to work properly");
    }
    else if (!studyNode->GetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str()))
    {
      studyNode->SetAttribute(SlicerRtCommon::DICOMRTIMPORT_DOSE_UNIT_NAME_ATTRIBUTE_NAME.c_str(), "Gy");
    }
  }

  // Set up display for dose volume
  resultDose->CreateDefaultDisplayNodes(); // Make sure display node is present
  if (resultDose->GetDisplayNode())
  {
    vtkMRMLScalarVolumeDisplayNode* doseScalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(resultDose->GetVolumeDisplayNode());

    // Set colormap to dose
    vtkMRMLColorTableNode* defaultDoseColorTable = vtkSlicerIsodoseModuleLogic::CreateDefaultDoseColorTable(resultDose->GetScene());
    if (defaultDoseColorTable)
    {
      doseScalarVolumeDisplayNode->SetAndObserveColorNodeID(defaultDoseColorTable->GetID());
    }
    else
    {
      doseScalarVolumeDisplayNode->SetAndObserveColorNodeID("vtkMRMLColorTableNodeRainbow");
      vtkErrorMacro("AddResultDose: Failed to get default dose color table!");
    }

    vtkMRMLRTPlanNode* planNode = beamNode->GetParentPlanNode();
    if (planNode)
    {
      // Set window level based on prescription dose
      double rxDose = planNode->GetRxDose();
      doseScalarVolumeDisplayNode->AutoWindowLevelOff();
      doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, rxDose*1.1);

      // Set threshold to hide very low dose values
      doseScalarVolumeDisplayNode->SetLowerThreshold(0.05 * rxDose);
      doseScalarVolumeDisplayNode->ApplyThresholdOn();
    }
    else
    {
      doseScalarVolumeDisplayNode->AutoWindowLevelOff();
      doseScalarVolumeDisplayNode->SetWindowLevelMinMax(0.0, vtkSlicerAbstractDoseEngine::DEFAULT_DOSE_VOLUME_WINDOW_LEVEL_MAXIMUM);
    }
  }
  else
  {
    vtkWarningMacro("AddResultDose: Display node is not available for dose volume node. The default color table will be used.");
  }
}

//---------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* vtkSlicerAbstractDoseEngine::GetResultDoseForBeam(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("AddResultDose: Invalid beam node");
    return NULL;
  }

  // Get last node reference if there are more than one, so that the dose calculated last is returned
  return vtkMRMLScalarVolumeNode::SafeDownCast(
    beamNode->GetNthNodeReference(RESULT_DOSE_REFERENCE_ROLE, beamNode->GetNumberOfNodeReferences(RESULT_DOSE_REFERENCE_ROLE)-1) );
}

//---------------------------------------------------------------------------
void vtkSlicerAbstractDoseEngine::RemoveIntermediateResults(vtkMRMLRTBeamNode* beamNode)
{
  if (!beamNode)
  {
    vtkErrorMacro("RemoveIntermediateResults: Invalid beam node");
    return;
  }

  vtkMRMLScene* scene = beamNode->GetScene();
  std::vector<const char*> referencedNodeIds;
  beamNode->GetNodeReferenceIDs(INTERMEDIATE_RESULT_REFERENCE_ROLE, referencedNodeIds);
  for (std::vector<const char*>::iterator refIt=referencedNodeIds.begin(); refIt != referencedNodeIds.end(); ++refIt)
  {
    vtkMRMLNode* node = scene->GetNodeByID(*refIt);
    scene->RemoveNode(node);
  }
}
